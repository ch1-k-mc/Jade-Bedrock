// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include <algorithm>
#include <cstdio>

#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components/AgeableComponent.h"
#include "mc/entity/components_json_legacy/BreedableComponent.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/animal/Allay.h"
#include "mc/world/effect/EffectDuration.h"
#include "mc/world/effect/MobEffect.h"
#include "mc/world/effect/MobEffectInstance.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/Tick.h"

#include "api/theme/IThemeHelper.h"
#include "impl/WailaCommonRegistration.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/text/I18n.h"

namespace jade::addon::vanilla {

using impl::ui::JadeUIInternal;
using text::Component;

namespace {

template <class C>
C* componentOf(Actor& e) {
    auto ref = e.getEntityContext().tryGetComponent<C>();
    return ref ? ref.as_ptr() : nullptr;
}

std::string formatTickDuration(int ticks, float tickRate) {
    int  seconds = static_cast<int>(static_cast<float>(ticks) / (tickRate <= 0 ? 20.0f : tickRate));
    int  h = seconds / 3600, m = seconds / 60 % 60, s = seconds % 60;
    char buf[32];
    if (h > 0) std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    else std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

}

bool StatusEffectsProvider::shouldRequestData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    return e && e->hasCategory(ActorCategory::Mob);
}

std::optional<std::vector<EffectData>> StatusEffectsProvider::streamData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e) return std::nullopt;
    std::vector<EffectData> out;
    auto&                   hide = impl::WailaCommonRegistration::instance().mobEffectOperations();
    for (MobEffectInstance const& inst : e->getAllEffects()) {
        int ticks = inst.mDuration->mValue;
        if (ticks == 0 || !inst.mEffectVisible) continue;
        std::string name = MobEffect::getNameById(inst.mId);
        if (name.empty() || name == "empty" || hide.shouldHide("minecraft:" + name)) continue;
        out.push_back(EffectData{static_cast<int>(inst.mId), inst.mAmplifier, ticks});
    }
    if (out.empty()) return std::nullopt;
    return out;
}

void StatusEffectsProvider::encode(CompoundTag& out, std::vector<EffectData> const& value) {
    ListTag list;
    for (auto const& e : value) {
        CompoundTag t;
        t["id"]    = e.id;
        t["amp"]   = e.amplifier;
        t["ticks"] = e.ticks;
        list.add(std::make_unique<CompoundTag>(std::move(t)));
    }
    out["effects"] = std::move(list);
}

std::optional<std::vector<EffectData>> StatusEffectsProvider::decode(CompoundTag const& in) {
    std::vector<EffectData> out;
    for (auto const& p : in.at("effects").get<ListTag>()) {
        if (!p.hold<CompoundTag>()) continue;
        auto const& t = p.get<CompoundTag>();
        out.push_back(EffectData{static_cast<int>(t.at("id")), static_cast<int>(t.at("amp")), static_cast<int>(t.at("ticks"))});
    }
    return out;
}

Component StatusEffectsProvider::Client::getEffectName(EffectData const& effect) {
    std::string      name = MobEffect::getNameById(static_cast<uint>(effect.id));
    MobEffect const* mob  = MobEffect::getByName(name);
    Component        out  = Component::literal(mob ? text::I18n::get(*mob->mDescriptionId) : name);
    if (effect.amplifier >= 1) {
        std::string key   = "enchantment.level." + std::to_string(effect.amplifier + 1);
        std::string level = text::I18n::has(key) ? text::I18n::get(key) : std::to_string(effect.amplifier + 1);
        out.append(" " + level);
    }
    return out;
}

void StatusEffectsProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) {
    auto effects = StatusEffectsProvider::INSTANCE->decodeFromData(accessor);
    if (!effects || effects->empty()) return;
    auto& hide = impl::WailaCommonRegistration::instance().mobEffectOperations();
    std::erase_if(*effects, [&](EffectData const& e) { return hide.shouldHide("minecraft:" + MobEffect::getNameById(static_cast<uint>(e.id))); });
    if (static_cast<int>(effects->size()) > config.getInt(JadeIds::MC_POTION_EFFECTS_LIMIT)) effects->resize(static_cast<size_t>(std::max(0, config.getInt(JadeIds::MC_POTION_EFFECTS_LIMIT))));
    if (effects->empty()) return;
    auto  box   = JadeUIInternal::tooltip();
    auto& t     = theme::IThemeHelper::get();
    float scale = effects->size() > 2 ? 0.75f : 1.0f;
    for (auto const& data : *effects) {
        std::string duration = data.ticks < 0 ? text::I18n::get("effect.duration.infinite") : formatTickDuration(data.ticks, accessor.tickRate());
        auto        s        = Component::translatable("jade.potion", std::vector<Component>{getEffectName(data), Component::literal(duration)});
        MobEffect const* mob = MobEffect::getByName(MobEffect::getNameById(static_cast<uint>(data.id)));
        s                    = (mob && mob->mIsHarmful) ? t.danger(s) : t.success(s);
        auto text            = JadeUIInternal::text(s);
        text->scale(scale);
        box->add(text);
    }
    auto element = JadeUIInternal::box(box, ui::BoxStyle::nestedBox());
    element->flexGrow(1);
    tooltip.add(element);
}

bool MobGrowthProvider::shouldRequestData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    return e && e->isBaby();
}

std::optional<int> MobGrowthProvider::streamData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e || !e->isBaby()) return std::nullopt;
    auto* age = componentOf<AgeableComponent>(*e);
    if (!age) return std::nullopt;
    if (age->mGrowthPaused) return PAUSED;
    int time = -age->mAge;
    if (time > 0) return time;
    return std::nullopt;
}

void MobGrowthProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* e = accessor.getEntity();
    if (!e) return;
    int time = MobGrowthProvider::INSTANCE->decodeFromData(accessor).value_or(0);
    if (time == PAUSED) {
        tooltip.add(Component::translatable("jade.mobgrowth.time", std::vector<Component>{theme::IThemeHelper::get().info(Component::translatable("jade.mobgrowth.paused"))}));
        return;
    }
    if (time > 0) tooltip.add(Component::translatable("jade.mobgrowth.time", std::vector<Component>{theme::IThemeHelper::get().seconds(time, accessor.tickRate())}));
}

std::optional<int> MobBreedingProvider::streamData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e) return std::nullopt;
    int time = 0;
    if (e->getTypeName() == "minecraft:allay") {
        uint64 end = static_cast<Allay*>(e)->mDuplicationCooldownCompleteTick->tickID;
        uint64 now = accessor.getLevel().getCurrentTick().tickID;
        if (end > now && end - now < 0x7FFFFFFF) time = static_cast<int>(end - now);
    } else if (e->isInLove()) {
        return IN_LOVE;
    } else if (auto* breed = componentOf<BreedableComponent>(*e)) {
        if (breed->mLoveTimer > 0) return IN_LOVE;
        time = breed->mBreedCooldown;
    }
    if (time > 0) return time;
    return std::nullopt;
}

void MobBreedingProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    int time = MobBreedingProvider::INSTANCE->decodeFromData(accessor).value_or(0);
    if (time == IN_LOVE) {
        tooltip.add(Component::translatable("jade.mobbreeding.fed"));
    } else if (time > 0) {
        Actor*      e   = accessor.getEntity();
        std::string key = e && e->getTypeName() == "minecraft:allay" ? "jade.mobduplication.time" : "jade.mobbreeding.time";
        tooltip.add(Component::translatable(key, std::vector<Component>{theme::IThemeHelper::get().seconds(time, accessor.tickRate())}));
    }
}

}
