// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/ItemFrameBlockActor.h"
#include "mc/world/level/block/actor/SignBlockActor.h"

#include "addon/access/AccessibilityPlugin.h"
#include "impl/ui/Elements.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/item/ScopedItemRegistry.h"

namespace jade::addon::vanilla {

using impl::ui::JadeUIInternal;

void ItemFrameProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be || (be->getType() != BlockActorType::ItemFrame && be->getType() != BlockActorType::GlowItemFrame)) return;
    platform::item::ScopedItemRegistry scoped;
    ItemInstance const&                item = static_cast<ItemFrameBlockActor*>(be)->mItem.get();
    if (item.isNull()) return;
    tooltip.add(text::Component::literal(text::stripFormatting(item.getHoverName())));
}

namespace {
SignBlockActor* signOf(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be || (be->getType() != BlockActorType::Sign && be->getType() != BlockActorType::HangingSign)) return nullptr;
    return static_cast<SignBlockActor*>(be);
}
}

ui::ElementPtr WaxedProvider::waxedIcon(ui::ElementPtr icon, bool waxed) {
    if (!waxed) return icon;
    platform::item::ScopedItemRegistry scoped;
    ItemStack                          honeycomb;
    honeycomb.reinit("minecraft:honeycomb", 1, 0);
    return std::make_shared<impl::ui::CompoundElement>(icon, JadeUIInternal::item(honeycomb, 0.5f));
}

ui::ElementPtr WaxedProvider::getIcon(BlockAccessor& accessor, IPluginConfig&, ui::ElementPtr currentIcon) {
    ItemStack const& picked = accessor.getPickedResult();
    if (auto* sign = signOf(accessor); sign && !picked.isNull()) return waxedIcon(JadeUIInternal::item(picked), sign->mIsWaxed);
    return currentIcon;
}

void WaxedProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) {
    auto* sign = signOf(accessor);
    if (config::IWailaConfig::get().accessibility().getEnableAccessibilityPlugin() && config.get(JadeIds::ACCESS_BLOCK_DETAILS) && sign && sign->mIsWaxed) {
        std::string objectName = tooltip.getString(JadeIds::CORE_OBJECT_NAME);
        access::AccessibilityPlugin::replaceTitle(tooltip, objectName, "waxed");
    }
}

std::optional<int> CopperGolemWaxedProvider::streamData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e) return std::nullopt;
    CompoundTag tag;
    if (!e->save(tag) || !tag.contains("properties") || !tag.at("properties").hold<CompoundTag>()) return std::nullopt;
    auto const& props = tag.at("properties").get<CompoundTag>();
    int         state = 0;
    if (props.contains("minecraft:oxidation_level") && props.at("minecraft:oxidation_level").hold<StringTag>()) {
        std::string level = props.at("minecraft:oxidation_level").get<StringTag>();
        state             = level == "exposed" ? 1 : level == "weathered" ? 2 : level == "oxidized" ? 3 : 0;
    }
    if (props.contains("minecraft:is_waxed") && static_cast<int>(props.at("minecraft:is_waxed")) != 0) state |= WAXED;
    return state;
}

ui::ElementPtr CopperGolemWaxedProvider::Client::getIcon(EntityAccessor& accessor, IPluginConfig&, ui::ElementPtr currentIcon) {
    auto state = CopperGolemWaxedProvider::INSTANCE->decodeFromData(accessor);
    if (!state) return currentIcon;
    static char const* const PREFIX[]{"", "exposed_", "weathered_", "oxidized_"};
    bool                     waxed = *state & WAXED;
    platform::item::ScopedItemRegistry scoped;
    ItemStack                          statue;
    statue.reinit(std::string("minecraft:") + (waxed ? "waxed_" : "") + PREFIX[*state & 3] + "copper_golem_statue", 1, 0);
    if (statue.isNull()) return currentIcon;
    return WaxedProvider::waxedIcon(JadeUIInternal::item(statue), waxed);
}

void CopperGolemWaxedProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) {
    if (!config::IWailaConfig::get().accessibility().getEnableAccessibilityPlugin() || !config.get(JadeIds::ACCESS_ENTITY_DETAILS)) return;
    auto state = CopperGolemWaxedProvider::INSTANCE->decodeFromData(accessor);
    if (!state) return;
    std::string objectName = tooltip.getString(JadeIds::CORE_OBJECT_NAME);
    static char const* const KEYS[]{nullptr, "entity.exposed", "entity.weathered", "entity.oxidized"};
    if (auto const* key = KEYS[*state & 3]) access::AccessibilityPlugin::replaceTitle(tooltip, objectName, key);
    if (*state & WAXED) access::AccessibilityPlugin::replaceTitle(tooltip, objectName, "waxed");
}

ui::ElementPtr ItemBERProvider::getIcon(BlockAccessor& accessor, IPluginConfig&, ui::ElementPtr) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be) return nullptr;
    ItemStack const& picked = accessor.getPickedResult();
    if (picked.isNull()) return nullptr;
    platform::item::ScopedItemRegistry scoped;
    ItemStack                          stack(picked);
    stack.addCustomUserData(*be, accessor.getRegion());
    return JadeUIInternal::item(stack);
}

}
