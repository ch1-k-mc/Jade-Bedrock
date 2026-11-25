// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "api/ExternalPlugins.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <vector>

#include "ll/api/mod/NativeMod.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/Block.h"

#include "api/Accessor.h"
#include "api/IJadeProvider.h"
#include "api/ITooltip.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"
#include "impl/ui/JadeUIInternal.h"
#include "jade/JadeApi.h"
#include "platform/item/ScopedItemRegistry.h"

namespace {

struct Entry {
    uint64_t                 id = 0;
    std::string              uid, name, configIcon;
    int                      target = JADE_TARGET_BLOCK, priority = 0;
    bool                     enabled = true;
    std::vector<std::string> targets;
    jade_callback_t          append = nullptr, icon = nullptr, server = nullptr;
    void*                    user = nullptr;
    std::shared_mutex        callMutex;
    bool                     alive = true;
};

std::mutex                          gMutex;
std::vector<std::shared_ptr<Entry>> gEntries;
uint64_t                            gNextId = 1;
std::atomic_bool                    gDirty{false};

}

struct jade_context {
    jade::api::Accessor*    accessor = nullptr;
    jade::api::ITooltip*    tooltip  = nullptr;
    jade::api::ui::ElementPtr* icon  = nullptr;
    CompoundTag*            out      = nullptr;
    Entry*                  entry    = nullptr;
    std::string             typeBuffer, dataBuffer;
};

namespace jade::api::external {

namespace {

template <class A>
class ExternalProvider final : public IComponentProvider<A>, public IServerDataProvider<A> {
public:
    explicit ExternalProvider(std::shared_ptr<Entry> entry) : mEntry(std::move(entry)) {}

    Identifier  getUid() const override { return mEntry->uid; }
    int         getDefaultPriority() const override { return TooltipPosition::BODY + mEntry->priority; }
    std::string getConfigIcon() const override { return mEntry->configIcon; }
    bool        enabledByDefault() const override { return mEntry->enabled; }

    void appendTooltip(ITooltip& tooltip, A& accessor, IPluginConfig&) override {
        std::shared_lock lock(mEntry->callMutex);
        if (!mEntry->alive || !mEntry->append) return;
        jade_context ctx;
        ctx.accessor = &accessor;
        ctx.tooltip  = &tooltip;
        ctx.entry    = mEntry.get();
        mEntry->append(&ctx, mEntry->user);
    }

    ui::ElementPtr getIcon(A& accessor, IPluginConfig&, ui::ElementPtr) override {
        std::shared_lock lock(mEntry->callMutex);
        if (!mEntry->alive || !mEntry->icon) return nullptr;
        ui::ElementPtr result;
        jade_context   ctx;
        ctx.accessor = &accessor;
        ctx.icon     = &result;
        ctx.entry    = mEntry.get();
        mEntry->icon(&ctx, mEntry->user);
        return result;
    }

    void appendServerData(CompoundTag& data, A& accessor) override {
        std::shared_lock lock(mEntry->callMutex);
        if (!mEntry->alive || !mEntry->server) return;
        jade_context ctx;
        ctx.accessor = &accessor;
        ctx.out      = &data;
        ctx.entry    = mEntry.get();
        mEntry->server(&ctx, mEntry->user);
    }

    bool shouldRequestData(A&) override { return mEntry->alive && mEntry->server != nullptr; }

private:
    std::shared_ptr<Entry> mEntry;
};

}

void registerAll(impl::WailaCommonRegistration& common, impl::WailaClientRegistration& client) {
    std::vector<std::shared_ptr<Entry>> entries;
    {
        std::lock_guard lock(gMutex);
        entries = gEntries;
    }
    for (auto const& e : entries) {
        if (e->target == JADE_TARGET_BLOCK) {
            auto p     = std::make_shared<ExternalProvider<BlockAccessor>>(e);
            auto clazz = [&] { return e->targets.empty() ? BlockClass::any() : BlockClass::ofTypes(e->targets); };
            client.registerBlockComponent(p, clazz());
            if (e->icon) client.registerBlockIcon(p, clazz());
            if (e->server) common.registerBlockDataProvider(p, clazz());
        } else {
            auto p     = std::make_shared<ExternalProvider<EntityAccessor>>(e);
            auto clazz = [&] { return e->targets.empty() ? EntityClass::any() : EntityClass::ofIdentifiers(e->targets, "External"); };
            client.registerEntityComponent(p, clazz());
            if (e->icon) client.registerEntityIcon(p, clazz());
            if (e->server) common.registerEntityDataProvider(p, clazz());
        }
        if (!e->server) client.markAsClientFeature(e->uid);
    }
}

bool consumeDirty() { return gDirty.exchange(false); }

uint64_t idOf(std::string const& uid) {
    std::lock_guard lock(gMutex);
    for (auto const& e : gEntries)
        if (e->uid == uid) return e->id;
    return 0;
}
bool isDirty() { return gDirty.load(); }

std::optional<std::string> displayName(std::string const& uid) {
    std::lock_guard lock(gMutex);
    for (auto const& e : gEntries) {
        if (e->uid == uid && !e->name.empty()) return e->name;
    }
    return std::nullopt;
}

}

extern "C" {

__declspec(dllexport) int jade_api_version(void) { return JADE_API_VERSION; }

__declspec(dllexport) uint64_t jade_register_provider(jade_provider_desc const* d) {
    auto warn = [](char const* why, char const* uid) { ll::mod::NativeMod::current()->getLogger().warn("Jade API: cannot register ({}): {}", why, uid ? uid : "(null)"); };
    if (!d || d->struct_size < sizeof(jade_provider_desc) || !d->uid) {
        warn("bad arguments", d ? d->uid : nullptr);
        return 0;
    }
    std::string uid = d->uid;
    auto        pos = uid.find(':');
    if (pos == std::string::npos || pos == 0 || pos + 1 == uid.size()) {
        warn("uid must be namespace:path", d->uid);
        return 0;
    }
    std::string ns = uid.substr(0, pos);
    if (ns == "minecraft" || ns == "jade" || ns == "jade_access" || ns == "debug") {
        warn("reserved namespace", d->uid);
        return 0;
    }
    if ((d->target != JADE_TARGET_BLOCK && d->target != JADE_TARGET_ENTITY) || (!d->append && !d->icon && !d->server)) {
        warn("bad target or no callback", d->uid);
        return 0;
    }
    auto e        = std::make_shared<Entry>();
    e->uid        = uid;
    e->name       = d->display_name ? d->display_name : "";
    e->configIcon = d->config_icon ? d->config_icon : "";
    e->target     = d->target;
    e->priority   = d->priority;
    e->enabled    = d->enabled_by_default != 0;
    e->append     = d->append;
    e->icon       = d->icon;
    e->server     = d->server;
    e->user       = d->user;
    if (d->targets) {
        std::stringstream ss(d->targets);
        for (std::string t; std::getline(ss, t, ',');) {
            t.erase(0, t.find_first_not_of(' '));
            t.erase(t.find_last_not_of(' ') + 1);
            if (!t.empty()) e->targets.push_back(t);
        }
    }
    std::lock_guard lock(gMutex);
    if (std::any_of(gEntries.begin(), gEntries.end(), [&](auto const& x) { return x->uid == uid; })) {
        warn("duplicate uid", d->uid);
        return 0;
    }
    e->id = gNextId++;
    gEntries.push_back(e);
    gDirty = true;
    return e->id;
}

__declspec(dllexport) int jade_unregister_provider(uint64_t id) {
    std::shared_ptr<Entry> e;
    {
        std::lock_guard lock(gMutex);
        auto it = std::find_if(gEntries.begin(), gEntries.end(), [&](auto const& x) { return x->id == id; });
        if (it == gEntries.end()) return 0;
        e = *it;
        gEntries.erase(it);
        gDirty = true;
    }
    std::unique_lock lock(e->callMutex);
    e->alive = false;
    return 1;
}

__declspec(dllexport) int jade_ctx_target(jade_context const* c) {
    return c->accessor->getAccessorType() == jade::api::AccessorType::ENTITY ? JADE_TARGET_ENTITY : JADE_TARGET_BLOCK;
}

__declspec(dllexport) char const* jade_ctx_type_id(jade_context* c) {
    if (c->accessor->getAccessorType() == jade::api::AccessorType::ENTITY) {
        Actor* a      = static_cast<jade::api::EntityAccessor*>(c->accessor)->getEntity();
        c->typeBuffer = a ? std::string(a->getTypeName()) : "";
    } else {
        c->typeBuffer = std::string(static_cast<jade::api::BlockAccessor*>(c->accessor)->getBlock().getTypeName());
    }
    return c->typeBuffer.c_str();
}

__declspec(dllexport) void jade_ctx_position(jade_context const* c, int* x, int* y, int* z) {
    BlockPos p;
    if (c->accessor->getAccessorType() == jade::api::AccessorType::ENTITY) {
        Actor* a = static_cast<jade::api::EntityAccessor*>(c->accessor)->getEntity();
        if (a) p = BlockPos(a->getPosition());
    } else {
        p = static_cast<jade::api::BlockAccessor*>(c->accessor)->getPosition();
    }
    if (x) *x = p.x;
    if (y) *y = p.y;
    if (z) *z = p.z;
}

__declspec(dllexport) int jade_ctx_show_details(jade_context const* c) { return c->accessor->showDetails() ? 1 : 0; }
__declspec(dllexport) int jade_ctx_server_connected(jade_context const* c) { return c->accessor->isServerConnected() ? 1 : 0; }

__declspec(dllexport) void jade_ctx_add_line(jade_context* c, char const* utf8) {
    if (c->tooltip && utf8) c->tooltip->add(jade::text::Component::literal(utf8));
}

__declspec(dllexport) void jade_ctx_set_icon_item(jade_context* c, char const* item) {
    if (!c->icon || !item) return;
    jade::platform::item::ScopedItemRegistry scoped;
    ItemStack                                stack;
    stack.reinit(item, 1, 0);
    if (!stack.isNull()) *c->icon = jade::impl::ui::JadeUIInternal::item(stack);
}

__declspec(dllexport) void jade_ctx_set_data(jade_context* c, char const* utf8) {
    if (c->out && utf8) (*c->out)[c->entry->uid] = std::string(utf8);
}

__declspec(dllexport) char const* jade_ctx_get_data(jade_context* c) {
    auto& data = c->accessor->getServerData();
    if (!data.contains(c->entry->uid) || !data.at(c->entry->uid).hold<StringTag>()) return nullptr;
    c->dataBuffer = std::string(data.at(c->entry->uid).get<StringTag>());
    return c->dataBuffer.c_str();
}

}
