// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/universal/UniversalPlugin.h"

#include "addon/universal/FluidStorageProvider.h"
#include "addon/universal/ItemStorageProvider.h"
#include "api/view/IExtensionProvider.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"

namespace jade::addon::universal {

void UniversalPlugin::register_(impl::WailaCommonRegistration& registration) {
    registration.registerBlockDataProvider(ItemStorageProvider<BlockAccessor>::INSTANCE, BlockClass::any());
    registration.registerEntityDataProvider(ItemStorageProvider<EntityAccessor>::INSTANCE, EntityClass::any());
    registration.registerItemStorageAnyTarget(ItemStorageExtension::INSTANCE);
    registration.registerItemStorage(ItemStorageExtension::INSTANCE, BlockClass::any());

    registration.registerBlockDataProvider(FluidStorageProvider<BlockAccessor>::INSTANCE, BlockClass::any());
    registration.registerEntityDataProvider(FluidStorageProvider<EntityAccessor>::INSTANCE, EntityClass::any());
    registration.registerFluidStorageAnyTarget(FluidStorageExtension::INSTANCE);
    registration.registerFluidStorage(FluidStorageExtension::INSTANCE, BlockClass::any());

    registration.registerBlockDataProvider(ProgressProvider<BlockAccessor>::INSTANCE, BlockClass::any());
    registration.registerEntityDataProvider(ProgressProvider<EntityAccessor>::INSTANCE, EntityClass::any());
}

void UniversalPlugin::registerClient(impl::WailaClientRegistration& registration) {
    registration.addConfig(JadeIds::UNIVERSAL_ITEM_STORAGE_DETAILED_AMOUNT, "", 54, 0, 54, false);
    registration.addConfig(JadeIds::UNIVERSAL_ITEM_STORAGE_NORMAL_AMOUNT, "", 9, 0, 54, false);
    registration.addConfig(JadeIds::UNIVERSAL_ITEM_STORAGE_SHOW_NAME_AMOUNT, "", 5, 0, 9, true);
    registration.addConfig(JadeIds::UNIVERSAL_ITEM_STORAGE_ITEMS_PER_LINE, "", 9, 3, 27, true);
    registration.addConfig(JadeIds::UNIVERSAL_ITEM_STORAGE_SORT, "", false);
    registration.addConfig(JadeIds::UNIVERSAL_FLUID_STORAGE_DETAILED, "", false);
    registration.addConfigEnum(JadeIds::UNIVERSAL_FLUID_STORAGE_STYLE, "", config::HandlerDisplayStyle::PROGRESS_BAR);

    registration.registerItemStorageClient(view::HideThingsExtensionProvider<view::ItemData, view::ItemView>::instance());
    registration.registerFluidStorageClient(view::HideThingsExtensionProvider<view::FluidView::Data, view::FluidView>::instance());
    registration.registerProgressClient(view::HideThingsExtensionProvider<view::ProgressView::Data, view::ProgressView>::instance());

    registration.registerBlockComponent(ItemStorageProviderClient<BlockAccessor>::INSTANCE, BlockClass::any());
    registration.registerEntityComponent(ItemStorageProviderClient<EntityAccessor>::INSTANCE, EntityClass::any());
    registration.registerItemStorageClient(ItemStorageExtension::INSTANCE);

    registration.registerBlockComponent(FluidStorageProviderClient<BlockAccessor>::INSTANCE, BlockClass::any());
    registration.registerEntityComponent(FluidStorageProviderClient<EntityAccessor>::INSTANCE, EntityClass::any());
    registration.registerFluidStorageClient(FluidStorageExtension::INSTANCE);

    registration.registerBlockComponent(ProgressProviderClient<BlockAccessor>::INSTANCE, BlockClass::any());
    registration.registerEntityComponent(ProgressProviderClient<EntityAccessor>::INSTANCE, EntityClass::any());

    auto category = text::Component::translatable("config.jade.plugin_jade");
    registration.setConfigCategoryOverride(JadeIds::UNIVERSAL_ITEM_STORAGE, category);
    registration.setConfigCategoryOverride(JadeIds::UNIVERSAL_FLUID_STORAGE, category);
}

}
