// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>

#include "api/IJadeProvider.h"
#include "api/view/IExtensionProvider.h"
#include "api/view/ItemView.h"

namespace jade::addon::universal {

using namespace jade::api;

struct ItemStorageCommon {
    static void putData(Accessor& accessor);
    static void appendServerData(Accessor& accessor);
    static bool shouldRequestData(Accessor& accessor);
    static void append(ITooltip& tooltip, Accessor& accessor, IPluginConfig& config);
};

template <class T>
class ItemStorageProvider : public virtual IServerDataProvider<T> {
public:
    static std::shared_ptr<ItemStorageProvider> const INSTANCE;

    Identifier getUid() const override { return JadeIds::UNIVERSAL_ITEM_STORAGE; }
    int        getDefaultPriority() const override { return TooltipPosition::BODY + 1000; }
    void       appendServerData(CompoundTag&, T& accessor) override { ItemStorageCommon::appendServerData(accessor); }
    bool       shouldRequestData(T& accessor) override { return ItemStorageCommon::shouldRequestData(accessor); }
};

template <class T>
inline std::shared_ptr<ItemStorageProvider<T>> const ItemStorageProvider<T>::INSTANCE = std::make_shared<ItemStorageProvider<T>>();

template <class T>
class ItemStorageProviderClient : public ItemStorageProvider<T>, public IComponentProvider<T> {
public:
    static std::shared_ptr<ItemStorageProviderClient> const INSTANCE;

    Identifier  getUid() const override { return JadeIds::UNIVERSAL_ITEM_STORAGE; }
    int         getDefaultPriority() const override { return TooltipPosition::BODY + 1000; }
    std::string getConfigIcon() const override { return "minecraft:chest"; }
    void        appendTooltip(ITooltip& tooltip, T& accessor, IPluginConfig& config) override;
};

template <class T>
inline std::shared_ptr<ItemStorageProviderClient<T>> const ItemStorageProviderClient<T>::INSTANCE = std::make_shared<ItemStorageProviderClient<T>>();

class ItemStorageExtension : public view::IServerExtensionProvider<view::ItemData>,
                             public view::IClientExtensionProvider<view::ItemData, view::ItemView> {
public:
    static std::shared_ptr<ItemStorageExtension> const INSTANCE;

    Identifier getUid() const override { return JadeIds::UNIVERSAL_ITEM_STORAGE_DEFAULT; }
    int        getDefaultPriority() const override { return 9999; }

    std::optional<view::ViewGroups<view::ItemData>> getGroups(Accessor& accessor) override;
    std::vector<view::ClientViewGroup<view::ItemView>>
         getClientGroups(Accessor& accessor, view::ViewGroups<view::ItemData> const& groups) override;
    bool shouldRequestData(Accessor& accessor) override;
};

}
