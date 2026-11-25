// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "api/IJadeProvider.h"
#include "impl/PriorityStore.h"
#include "impl/config/TargetOperationRepository.h"
#include "impl/lookup/HierarchyLookup.h"
#include "impl/lookup/WrappedHierarchyLookup.h"
#include "api/view/IExtensionProvider.h"
#include "api/view/ItemView.h"
#include "api/view/FluidView.h"
#include "api/view/ProgressView.h"

namespace jade::impl {

class WailaCommonRegistration {
public:
    using BlockDataProvider  = api::IServerDataProvider<api::BlockAccessor>;
    using EntityDataProvider = api::IServerDataProvider<api::EntityAccessor>;

    WailaCommonRegistration();
    static WailaCommonRegistration& instance();
    static void                     reset();

    void registerBlockDataProvider(std::shared_ptr<BlockDataProvider> provider, api::BlockClass clazz);
    void registerBlockDataProvider(std::shared_ptr<BlockDataProvider> provider, api::BlockActorClass clazz);
    void registerEntityDataProvider(std::shared_ptr<EntityDataProvider> provider, api::EntityClass clazz);

    using ItemStorageProvider = api::view::IServerExtensionProvider<api::view::ItemData>;
    template <class C>
    void registerItemStorage(std::shared_ptr<ItemStorageProvider> provider, C clazz) {
        itemStorageProviders.register_(std::move(clazz), std::move(provider));
    }
    void registerItemStorageAnyTarget(std::shared_ptr<ItemStorageProvider> provider) { itemStorageProviders.registerAnyTarget(std::move(provider)); }

    using FluidStorageProvider = api::view::IServerExtensionProvider<api::view::FluidView::Data>;
    using ProgressProvider     = api::view::IServerExtensionProvider<api::view::ProgressView::Data>;
    template <class C>
    void registerFluidStorage(std::shared_ptr<FluidStorageProvider> provider, C clazz) {
        fluidStorageProviders.register_(std::move(clazz), std::move(provider));
    }
    void registerFluidStorageAnyTarget(std::shared_ptr<FluidStorageProvider> provider) { fluidStorageProviders.registerAnyTarget(std::move(provider)); }
    template <class C>
    void registerProgress(std::shared_ptr<ProgressProvider> provider, C clazz) {
        progressProviders.register_(std::move(clazz), std::move(provider));
    }
    void registerProgressAnyTarget(std::shared_ptr<ProgressProvider> provider) { progressProviders.registerAnyTarget(std::move(provider)); }

    std::vector<std::shared_ptr<BlockDataProvider>>  blockDataProvidersOf(Block const& block, BlockActor const* blockActor, bool checkIsHidden);
    std::vector<std::shared_ptr<EntityDataProvider>> entityDataProvidersOf(Actor const& entity);

    void loadComplete();
    void reloadOperations(std::filesystem::path const& configDir);

    config::TargetOperationRepository& blockOperations() { return mBlockOperations; }
    config::TargetOperationRepository& entityTypeOperations() { return mEntityTypeOperations; }
    config::TargetOperationRepository& mobEffectOperations() { return mMobEffectOperations; }

    lookup::PairHierarchyLookup<BlockDataProvider>                         blockDataProviders;
    lookup::HierarchyLookup<Actor, api::EntityClass, EntityDataProvider>   entityDataProviders;
    lookup::WrappedHierarchyLookup<ItemStorageProvider>                    itemStorageProviders;
    lookup::WrappedHierarchyLookup<FluidStorageProvider>                   fluidStorageProviders;
    lookup::WrappedHierarchyLookup<ProgressProvider>                       progressProviders;
    PriorityStore                                                          priorities;

private:
    config::TargetOperationRepository mBlockOperations;
    config::TargetOperationRepository mEntityTypeOperations;
    config::TargetOperationRepository mMobEffectOperations;
};

}
