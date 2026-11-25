// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "api/AccessorClientHandler.h"
#include "api/IJadeProvider.h"
#include "api/callback/Callbacks.h"
#include "api/view/IExtensionProvider.h"
#include "api/view/ItemView.h"
#include "api/view/FluidView.h"
#include "api/view/ProgressView.h"
#include "impl/config/entry/ConfigEntry.h"
#include "impl/lookup/HierarchyLookup.h"
#include "platform/text/Component.h"

namespace jade::impl {

using api::BlockAccessor;
using api::EntityAccessor;

class WailaClientRegistration {
public:
    using BlockLookup  = lookup::HierarchyLookup<Block, api::BlockClass, api::IBlockComponentProvider>;
    using EntityLookup = lookup::HierarchyLookup<Actor, api::EntityClass, api::IEntityComponentProvider>;

    static WailaClientRegistration& instance();
    static void                     reset();

    void registerBlockIcon(std::shared_ptr<api::IBlockComponentProvider> provider, api::BlockClass blockClass);
    void registerBlockComponent(std::shared_ptr<api::IBlockComponentProvider> provider, api::BlockClass blockClass);
    void registerEntityIcon(std::shared_ptr<api::IEntityComponentProvider> provider, api::EntityClass entityClass);
    void registerEntityComponent(std::shared_ptr<api::IEntityComponentProvider> provider, api::EntityClass entityClass);

    std::vector<std::shared_ptr<api::IBlockComponentProvider>>  getBlockProviders(Block const& block);
    std::vector<std::shared_ptr<api::IBlockComponentProvider>>  getBlockIconProviders(Block const& block);
    std::vector<std::shared_ptr<api::IEntityComponentProvider>> getEntityProviders(Actor const& entity);
    std::vector<std::shared_ptr<api::IEntityComponentProvider>> getEntityIconProviders(Actor const& entity);

    void addConfig(std::unique_ptr<config::entry::ConfigEntry> entry);
    void addConfig(Identifier const& key, std::string icon, bool defaultValue);
    void addConfig(Identifier const& key, std::string icon, std::string defaultValue, std::vector<std::string> enumNames);
    template <class E>
    void addConfigEnum(Identifier const& key, std::string icon, E defaultValue) {
        std::vector<std::string> names;
        for (auto n : api::config::EnumNames<E>::values) names.emplace_back(n);
        addConfig(key, std::move(icon), std::string(api::config::enumName(defaultValue)), std::move(names));
    }
    void addConfig(Identifier const& key, std::string icon, std::string defaultValue, std::function<bool(std::string const&)> validator);
    void addConfig(Identifier const& key, std::string icon, int defaultValue, int min, int max, bool slider);
    void addConfig(Identifier const& key, std::string icon, float defaultValue, float min, float max, bool slider);
    void addConfigListener(Identifier const& key, std::function<void(Identifier const&)> listener);
    void setConfigCategoryOverride(Identifier const& key, text::Component override);
    void setConfigCategoryOverride(Identifier const& key, std::vector<text::Component> const& overrides);
    void setConfigIcon(Identifier const& key, std::string icon);

    [[nodiscard]] std::set<Identifier> getConfigKeys() const;
    [[nodiscard]] std::set<Identifier> getConfigKeys(std::string const& ns) const;
    [[nodiscard]] bool                 hasConfig(Identifier const& key) const { return configEntries.contains(key); }
    config::entry::ConfigEntry*        getConfigEntry(Identifier const& key);

    void markAsClientFeature(Identifier const& uid) { clientFeatures.insert(uid); }
    void markAsServerFeature(Identifier const& uid) { clientFeatures.erase(uid); }
    [[nodiscard]] bool isClientFeature(Identifier const& uid) const { return clientFeatures.contains(uid); }

    void addAfterRenderCallback(int priority, api::callback::JadeAfterRenderCallback cb) { afterRenderCallback.add(priority, std::move(cb)); }
    void addBeforeRenderCallback(int priority, api::callback::JadeBeforeRenderCallback cb) { beforeRenderCallback.add(priority, std::move(cb)); }
    void addRayTraceCallback(int priority, api::callback::JadeRayTraceCallback cb) { rayTraceCallback.add(priority, std::move(cb)); }
    void addTooltipCollectedCallback(int priority, api::callback::JadeTooltipCollectedCallback cb) { tooltipCollectedCallback.add(priority, std::move(cb)); }
    void addItemModNameCallback(int priority, api::callback::JadeItemModNameCallback cb) { itemModNameCallback.add(priority, std::move(cb)); }
    void addBeforeTooltipCollectCallback(int priority, api::callback::JadeBeforeTooltipCollectCallback cb) {
        beforeTooltipCollectCallback.add(priority, std::move(cb));
    }

    using ItemStorageClient = api::view::IClientExtensionProvider<api::view::ItemData, api::view::ItemView>;
    void registerItemStorageClient(std::shared_ptr<ItemStorageClient> provider) { itemStorageProviders[provider->getUid()] = std::move(provider); }
    ItemStorageClient* getItemStorageClient(Identifier const& uid) {
        auto it = itemStorageProviders.find(uid);
        return it == itemStorageProviders.end() ? nullptr : it->second.get();
    }

    using FluidStorageClient = api::view::IClientExtensionProvider<api::view::FluidView::Data, api::view::FluidView>;
    using ProgressClient     = api::view::IClientExtensionProvider<api::view::ProgressView::Data, api::view::ProgressView>;
    void registerFluidStorageClient(std::shared_ptr<FluidStorageClient> provider) { fluidStorageProviders[provider->getUid()] = std::move(provider); }
    void registerProgressClient(std::shared_ptr<ProgressClient> provider) { progressProviders[provider->getUid()] = std::move(provider); }
    FluidStorageClient* getFluidStorageClient(Identifier const& uid) {
        auto it = fluidStorageProviders.find(uid);
        return it == fluidStorageProviders.end() ? nullptr : it->second.get();
    }
    ProgressClient* getProgressClient(Identifier const& uid) {
        auto it = progressProviders.find(uid);
        return it == progressProviders.end() ? nullptr : it->second.get();
    }

    void                        registerAccessorHandler(api::AccessorType type, std::unique_ptr<api::AccessorClientHandler> handler);
    api::AccessorClientHandler& getAccessorHandler(api::AccessorType type);

    void setServerConfig(nlohmann::json const& config);

    void loadComplete();

    BlockLookup                                                                   blockIconProviders;
    BlockLookup                                                                   blockComponentProviders;
    EntityLookup                                                                  entityIconProviders;
    EntityLookup                                                                  entityComponentProviders;
    api::callback::CallbackContainer<api::callback::JadeAfterRenderCallback>          afterRenderCallback;
    api::callback::CallbackContainer<api::callback::JadeBeforeRenderCallback>         beforeRenderCallback;
    api::callback::CallbackContainer<api::callback::JadeRayTraceCallback>             rayTraceCallback;
    api::callback::CallbackContainer<api::callback::JadeTooltipCollectedCallback>     tooltipCollectedCallback;
    api::callback::CallbackContainer<api::callback::JadeItemModNameCallback>          itemModNameCallback;
    api::callback::CallbackContainer<api::callback::JadeBeforeTooltipCollectCallback> beforeTooltipCollectCallback;
    std::map<Identifier, std::unique_ptr<config::entry::ConfigEntry>>             configEntries;
    std::map<Identifier, std::vector<text::Component>>                            configCategoryOverrides;
    std::set<Identifier>                                                          clientFeatures;
    std::map<api::AccessorType, std::unique_ptr<api::AccessorClientHandler>>      accessorHandlers;
    std::map<Identifier, std::shared_ptr<ItemStorageClient>>                      itemStorageProviders;
    std::map<Identifier, std::shared_ptr<FluidStorageClient>>                     fluidStorageProviders;
    std::map<Identifier, std::shared_ptr<ProgressClient>>                         progressProviders;

private:
    void tryAddConfig(api::IToggleableProvider const& provider);
};

}
