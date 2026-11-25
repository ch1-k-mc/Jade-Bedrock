// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "util/ModIdentification.h"
#include "addon/bedrock/BedrockPlugin.h"

#include <map>
#include <mutex>

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/Block.h"

#include "impl/WailaClientRegistration.h"
#include "platform/item/ScopedItemRegistry.h"
#include "platform/text/I18n.h"
#include "util/JadeFont.h"

namespace jade::util {

namespace {
std::mutex                                        gMutex;
std::map<std::string, std::optional<std::string>> gNames;
std::map<std::string, std::optional<std::string>> gCutNames;

std::string nsOf(std::string const& id) {
    auto c = id.find(':');
    return c == std::string::npos ? std::string("minecraft") : id.substr(0, c);
}
}

void ModIdentification::invalidateCache() {
    std::lock_guard lock(gMutex);
    gNames.clear();
    gCutNames.clear();
}

std::optional<std::string> ModIdentification::getModFullName(std::string const& ns) {
    {
        std::lock_guard lock(gMutex);
        auto            it = gNames.find(ns);
        if (it != gNames.end()) return it->second;
    }
    std::optional<std::string> result;
    std::string                key = "jade.modName." + ns;
    if (text::I18n::has(key)) {
        result = text::I18n::get(key);
    } else if (text::I18n::has("itemGroup." + ns)) {
        result = text::I18n::get("itemGroup." + ns);
    } else if (ns == "minecraft") {
        result = "Minecraft";
    } else {
        result = addon::bedrock::PackNames::find(ns);
    }
    std::lock_guard lock(gMutex);
    gNames[ns] = result;
    return result;
}

std::string ModIdentification::cutName(std::string fullName, int maxWidth) {
    auto b = fullName.find_first_not_of(' ');
    auto e = fullName.find_last_not_of(' ');
    fullName = b == std::string::npos ? std::string() : fullName.substr(b, e - b + 1);
    if (maxWidth <= 0 || JadeFont::width(fullName) <= maxWidth) return fullName;
    std::string const ellipsis = "\xE2\x80\xA6";
    std::string       s        = fullName;
    while (!s.empty() && JadeFont::width(s + ellipsis) > maxWidth) {
        size_t i = s.size() - 1;
        while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) --i;
        s.erase(i);
    }
    return s + ellipsis;
}

std::optional<std::string> ModIdentification::getModName(std::string const& ns, int maxWidth) {
    if (maxWidth == NAME_MAX_WIDTH) {
        std::lock_guard lock(gMutex);
        auto            it = gCutNames.find(ns);
        if (it != gCutNames.end()) return it->second;
    }
    auto full = getModFullName(ns);
    std::optional<std::string> result;
    if (full) result = cutName(*full, maxWidth);
    if (maxWidth == NAME_MAX_WIDTH) {
        std::lock_guard lock(gMutex);
        gCutNames[ns] = result;
    }
    return result;
}

std::string ModIdentification::getModNameOfId(std::string const& id) {
    auto ns = nsOf(id);
    return getModName(ns).value_or(ns);
}

std::string ModIdentification::getModName(Block const& block) { return getModNameOfId(block.getTypeName()); }

std::string ModIdentification::getModName(ItemStack const& stack) {
    std::string result;
    impl::WailaClientRegistration::instance().itemModNameCallback.call([&](auto const& cb) {
        if (result.empty()) result = cb(stack);
    });
    if (!result.empty()) return result;
    platform::item::ScopedItemRegistry scoped;
    return getModNameOfId(stack.getTypeName());
}

std::string ModIdentification::getModName(Actor const& entity) {
    if (entity.hasCategory(ActorCategory::Item)) return getModName(static_cast<ItemActor const&>(entity).mItem.get());
    return getModNameOfId(entity.getTypeName());
}

}
