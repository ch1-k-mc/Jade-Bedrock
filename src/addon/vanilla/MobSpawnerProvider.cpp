// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/level/BaseMobSpawner.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/Tick.h"
#include "mc/world/level/TrialSpawner.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/VanillaStates.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/MobSpawnerBlockActor.h"
#include "mc/world/level/block/actor/TrialSpawnerBlockActor.h"

#include "api/theme/IThemeHelper.h"
#include "platform/text/I18n.h"
#include "util/BlockStates.h"

namespace jade::addon::vanilla {

using text::Component;

namespace {

constexpr int TRIAL_SPAWNER_COOLDOWN = 5;

ActorDefinitionIdentifier const* spawnTypeOf(BlockActor* be) {
    if (!be) return nullptr;
    if (be->getType() == BlockActorType::MobSpawner) {
        auto* spawner = static_cast<MobSpawnerBlockActor*>(be)->mSpawner.get();
        return spawner ? &spawner->mActorDefId.get() : nullptr;
    }
    if (be->getType() == BlockActorType::TrialSpawner) {
        auto* spawner = static_cast<TrialSpawnerBlockActor*>(be)->mSpawner.get();
        return spawner ? &spawner->mData->mActorDefId.get() : nullptr;
    }
    return nullptr;
}

}

void MobSpawnerProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto const* id = spawnTypeOf(accessor.getBlockEntity());
    if (!id || id->mIdentifier->empty()) return;
    std::string key        = "entity." + *id->mIdentifier + ".name";
    std::string entityName = text::I18n::has(key) ? text::I18n::get(key) : *id->mFullName;
    std::string blockName  = tooltip.getString(JadeIds::CORE_OBJECT_NAME);
    auto        name       = Component::translatable("jade.spawner", std::vector<std::string>{blockName, entityName});
    tooltip.replace(JadeIds::CORE_OBJECT_NAME, theme::IThemeHelper::get().title(name));
}

bool MobSpawnerCooldownProvider::shouldRequestData(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    return be && be->getType() == BlockActorType::TrialSpawner
        && util::BlockStates::intState(accessor.getBlock(), "trial_spawner_state").value_or(0) == TRIAL_SPAWNER_COOLDOWN;
}

std::optional<int> MobSpawnerCooldownProvider::streamData(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be || be->getType() != BlockActorType::TrialSpawner) return std::nullopt;
    if (util::BlockStates::intState(accessor.getBlock(), "trial_spawner_state").value_or(0) != TRIAL_SPAWNER_COOLDOWN) return std::nullopt;
    auto* spawner = static_cast<TrialSpawnerBlockActor*>(be)->mSpawner.get();
    if (!spawner) return std::nullopt;
    uint64 now = accessor.getLevel().getCurrentTick().tickID;
    uint64 end = spawner->mData->mCooldownEndsAt;
    if (now < end) return static_cast<int>(end - now);
    return std::nullopt;
}

void MobSpawnerCooldownProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) {
    if (!config.get(JadeIds::MC_MOB_SPAWNER)) return;
    int cooldown = MobSpawnerCooldownProvider::INSTANCE->decodeFromData(accessor).value_or(-1);
    if (cooldown >= 0) {
        tooltip.add(Component::translatable("jade.trial_spawner_cd", std::vector<Component>{theme::IThemeHelper::get().seconds(cooldown, accessor.tickRate())}));
    }
}

}
