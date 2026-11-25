// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/WailaCommonRegistration.h"

#include <algorithm>

#include "mc/world/actor/Actor.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"

#include "api/config/IWailaConfig.h"

namespace jade::impl {

namespace {
std::unique_ptr<WailaCommonRegistration> gInstance;
}

WailaCommonRegistration::WailaCommonRegistration()
: mBlockOperations("hide-blocks", {"barrier"}),
  mEntityTypeOperations("hide-entities", {"area_effect_cloud", "fireworks_rocket", "lightning_bolt"}),
  mMobEffectOperations("hide-mob-effects", {}) {
    priorities.setSortingFunction([](PriorityStore& store, std::set<Identifier> const& allKeys) {
        std::vector<Identifier> keys;
        for (auto const& k : allKeys)
            if (api::config::IPluginConfig::isPrimaryKey(k)) keys.push_back(k);
        std::stable_sort(keys.begin(), keys.end(), [&store](auto const& a, auto const& b) { return store.byKey(a) < store.byKey(b); });
        for (auto const& k : allKeys) {
            if (api::config::IPluginConfig::isPrimaryKey(k)) continue;
            auto it = std::find(keys.begin(), keys.end(), api::config::IPluginConfig::getPrimaryKey(k));
            keys.insert(it == keys.end() ? keys.begin() : it + 1, k);
        }
        return keys;
    });
}

WailaCommonRegistration& WailaCommonRegistration::instance() {
    if (!gInstance) gInstance = std::make_unique<WailaCommonRegistration>();
    return *gInstance;
}

void WailaCommonRegistration::reset() { gInstance = std::make_unique<WailaCommonRegistration>(); }

void WailaCommonRegistration::registerBlockDataProvider(std::shared_ptr<BlockDataProvider> provider, api::BlockClass clazz) {
    priorities.put(*provider);
    blockDataProviders.register_(std::move(clazz), std::move(provider));
}

void WailaCommonRegistration::registerBlockDataProvider(std::shared_ptr<BlockDataProvider> provider, api::BlockActorClass clazz) {
    priorities.put(*provider);
    blockDataProviders.register_(std::move(clazz), std::move(provider));
}

void WailaCommonRegistration::registerEntityDataProvider(std::shared_ptr<EntityDataProvider> provider, api::EntityClass clazz) {
    priorities.put(*provider);
    entityDataProviders.register_(std::move(clazz), std::move(provider));
}

std::vector<std::shared_ptr<WailaCommonRegistration::BlockDataProvider>>
WailaCommonRegistration::blockDataProvidersOf(Block const& block, BlockActor const* blockActor, bool checkIsHidden) {
    std::string const& type = block.getTypeName();
    if (checkIsHidden && mBlockOperations.shouldHide(type)) return {};
    if (!blockActor) return blockDataProviders.first.get(block, type);
    return blockDataProviders.getMerged(block, type, *blockActor, std::to_string(static_cast<int>(blockActor->getType())), priorities);
}

std::vector<std::shared_ptr<WailaCommonRegistration::EntityDataProvider>> WailaCommonRegistration::entityDataProvidersOf(Actor const& entity) {
    std::string const& type = entity.getTypeName();
    if (mEntityTypeOperations.shouldHide(type)) return {};
    return entityDataProviders.get(entity, type);
}

void WailaCommonRegistration::loadComplete() {
    blockDataProviders.loadComplete(priorities);
    entityDataProviders.loadComplete(priorities);
    itemStorageProviders.loadComplete(priorities);
    fluidStorageProviders.loadComplete(priorities);
    progressProviders.loadComplete(priorities);
}

void WailaCommonRegistration::reloadOperations(std::filesystem::path const& configDir) {
    mBlockOperations.reload(configDir);
    mEntityTypeOperations.reload(configDir);
    mMobEffectOperations.reload(configDir);
}

}
