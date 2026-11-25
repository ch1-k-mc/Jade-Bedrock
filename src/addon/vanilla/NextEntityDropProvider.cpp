// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components_json_legacy/SpawnActorComponent.h"
#include "mc/entity/components_json_legacy/SpawnActorEntry.h"
#include "mc/entity/components_json_legacy/TransformationComponent.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemStack.h"

#include "api/theme/IThemeHelper.h"
#include "impl/ui/JadeUIInternal.h"

namespace jade::addon::vanilla {

using text::Component;

namespace {
template <class C>
C* componentOf(Actor& e) {
    auto ref = e.getEntityContext().tryGetComponent<C>();
    return ref ? ref.as_ptr() : nullptr;
}

void appendSeconds(ITooltip& tooltip, Accessor& accessor, char const* tagKey, char const* translationKey) {
    auto const& data = accessor.getServerData();
    if (!data.contains(tagKey)) return;
    int ticks = static_cast<int>(data.at(tagKey));
    tooltip.add(Component::translatable(translationKey, std::vector<Component>{theme::IThemeHelper::get().seconds(ticks, accessor.tickRate())}));
}
}

bool NextEntityDropProvider::shouldRequestData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    return e && !e->isBaby();
}

void NextEntityDropProvider::appendServerData(CompoundTag& tag, EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e || e->isBaby()) return;
    constexpr int max  = 24000 * 2;
    auto const&   type = e->getTypeName();
    char const*   key  = type == "minecraft:chicken" ? "NextEggIn" : type == "minecraft:armadillo" ? "NextScuteIn" : nullptr;
    if (!key) return;
    auto* spawn = componentOf<SpawnActorComponent>(*e);
    if (!spawn || spawn->mSpawnEntries->empty()) return;
    int time = spawn->mSpawnEntries->front().mSpawnTimer;
    if (time >= 0 && time < max) tag[key] = time;
}

void NextEntityDropProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    appendSeconds(tooltip, accessor, "NextEggIn", "jade.nextEgg");
    appendSeconds(tooltip, accessor, "NextScuteIn", "jade.nextScute");
    appendSeconds(tooltip, accessor, "NextSniffIn", "jade.nextSniff");
}

std::optional<int> ZombieVillagerProvider::streamData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e) return std::nullopt;
    auto* t = componentOf<TransformationComponent>(*e);
    if (!t) return std::nullopt;
    int time = t->mDelayTicks;
    if (time > 0) return time;
    return std::nullopt;
}

void ZombieVillagerProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    int time = ZombieVillagerProvider::INSTANCE->decodeFromData(accessor).value_or(0);
    if (time > 0) tooltip.add(Component::translatable("jade.zombieConversion.time", std::vector<Component>{theme::IThemeHelper::get().seconds(time, accessor.tickRate())}));
}

bool PetArmorProvider::shouldRequestData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e) return false;
    if (!e->isTame() && e->getTypeName() != "minecraft:happy_ghast") return false;
    return config::IWailaConfig::get().plugin().getEnum<PetArmorMode>(JadeIds::MC_PET_ARMOR) != PetArmorMode::OFF;
}

std::optional<view::ItemData> PetArmorProvider::streamData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e) return std::nullopt;
    ItemStack const& armor = e->getArmor(SharedTypes::Legacy::ArmorSlot::Body);
    if (armor.isNull()) return std::nullopt;
    return view::ItemData::of(armor);
}

void PetArmorProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    if (!PetArmorProvider::INSTANCE->shouldRequestData(accessor)) return;
    auto data = PetArmorProvider::INSTANCE->decodeFromData(accessor);
    if (!data) return;
    auto view = view::ItemView::fromData(*data);
    if (!view) return;
    ItemStack const& armor = *view->item;
    auto             mode  = config::IWailaConfig::get().plugin().getEnum<PetArmorMode>(JadeIds::MC_PET_ARMOR);
    if (mode == PetArmorMode::SHOW_DAMAGEABLE && !armor.isDamageableItem()) return;
    tooltip.add(impl::ui::JadeUIInternal::smallItem(armor));
    tooltip.append(impl::ui::JadeUIInternal::text(Component::literal(text::stripFormatting(armor.getHoverName()))));
}

}
