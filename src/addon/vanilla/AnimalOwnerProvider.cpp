// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDataIDs.h"
#include "mc/world/actor/SynchedActorDataEntityWrapper.h"
#include "mc/world/level/Level.h"

#include "addon/core/CoreProviders.h"
#include "platform/text/I18n.h"

namespace jade::addon::vanilla {

using text::Component;

bool AnimalOwnerProvider::shouldRequestData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    return e && e->isTame();
}

std::optional<std::string> AnimalOwnerProvider::streamData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e || !e->isTame()) return std::nullopt;
    ActorUniqueID id = e->getOwnerId();
    if (id.rawID == -1) return std::nullopt;
    Actor* owner = accessor.getLevel().fetchEntity(id, false);
    if (!owner) return std::nullopt;
    return core::ObjectNameProvider::getEntityName(*owner, false).getString();
}

std::optional<std::string> AnimalOwnerProvider::decode(CompoundTag const& in) {
    if (!in.contains("name") || !in.at("name").hold<StringTag>()) return std::nullopt;
    return std::string(in.at("name").get<StringTag>());
}

void AnimalOwnerProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    auto name = AnimalOwnerProvider::INSTANCE->decodeFromData(accessor);
    if (!name) {
        Actor* e = accessor.getEntity();
        if (!e || !e->isTame()) return;
        if (Actor* owner = accessor.getLevel().fetchEntity(e->getOwnerId(), false)) name = core::ObjectNameProvider::getEntityName(*owner, false).getString();
    }
    if (!name) return;
    tooltip.add(Component::translatable("jade.owner", std::vector<Component>{Component::literal(*name)}));
}

void VillagerProfessionProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* e = accessor.getEntity();
    if (!e) return;
    auto const& type = e->getTypeName();
    if (type != "minecraft:villager_v2" && type != "minecraft:zombie_villager_v2" && type != "minecraft:villager" && type != "minecraft:zombie_villager") return;
    static char const* const professions[]{
        "unskilled", "farmer", "fisherman", "shepherd", "fletcher", "librarian", "cartographer", "cleric",
        "armor",     "weapon", "tool",      "butcher",  "leather",  "mason",     "nitwit"};
    int variant = e->getVariant();
    if (variant < 0 || variant >= static_cast<int>(std::size(professions))) return;
    std::string key  = std::string("entity.villager.") + professions[variant];
    Component   name = Component::literal(text::I18n::has(key) ? text::I18n::get(key) : professions[variant]);
    if (variant != 0 && variant != 14) {
        int level = e->mEntityData->getInt(static_cast<ushort>(ActorDataIDs::TradeTier)) + 1;
        if (level >= 1 && level <= 5) name.append(" - " + text::I18n::get("trade.level." + std::to_string(level)));
    }
    tooltip.add(name);
}

}
