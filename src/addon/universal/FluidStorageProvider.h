// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>

#include "api/IJadeProvider.h"
#include "api/view/FluidView.h"
#include "api/view/IExtensionProvider.h"
#include "api/view/ProgressView.h"

namespace jade::addon::universal {

using namespace jade::api;

struct FluidStorageCommon {
    static void appendServerData(Accessor& accessor);
    static bool shouldRequestData(Accessor& accessor);
    static void append(ITooltip& tooltip, Accessor& accessor, IPluginConfig& config);
};

template <class T>
class FluidStorageProvider : public virtual IServerDataProvider<T> {
public:
    static inline std::shared_ptr<FluidStorageProvider> const INSTANCE = std::make_shared<FluidStorageProvider>();

    Identifier getUid() const override { return JadeIds::UNIVERSAL_FLUID_STORAGE; }
    int        getDefaultPriority() const override { return TooltipPosition::BODY + 1000; }
    void       appendServerData(CompoundTag&, T& accessor) override { FluidStorageCommon::appendServerData(accessor); }
    bool       shouldRequestData(T& accessor) override { return FluidStorageCommon::shouldRequestData(accessor); }
};

template <class T>
class FluidStorageProviderClient : public FluidStorageProvider<T>, public IComponentProvider<T> {
public:
    static inline std::shared_ptr<FluidStorageProviderClient> const INSTANCE = std::make_shared<FluidStorageProviderClient>();

    Identifier  getUid() const override { return JadeIds::UNIVERSAL_FLUID_STORAGE; }
    int         getDefaultPriority() const override { return TooltipPosition::BODY + 1000; }
    std::string getConfigIcon() const override { return "minecraft:water_bucket"; }
    void        appendTooltip(ITooltip& tooltip, T& accessor, IPluginConfig& config) override { FluidStorageCommon::append(tooltip, accessor, config); }
};

class FluidStorageExtension : public view::IServerExtensionProvider<view::FluidView::Data>,
                              public view::IClientExtensionProvider<view::FluidView::Data, view::FluidView> {
public:
    static std::shared_ptr<FluidStorageExtension> const INSTANCE;

    Identifier getUid() const override { return JadeIds::UNIVERSAL_FLUID_STORAGE_DEFAULT; }
    int        getDefaultPriority() const override { return 9999; }
    std::optional<view::ViewGroups<view::FluidView::Data>> getGroups(Accessor& accessor) override;
    std::vector<view::ClientViewGroup<view::FluidView>>
         getClientGroups(Accessor& accessor, view::ViewGroups<view::FluidView::Data> const& groups) override;
    bool shouldRequestData(Accessor& accessor) override;
};

struct ProgressCommon {
    static void appendServerData(Accessor& accessor);
    static bool shouldRequestData(Accessor& accessor);
    static void append(ITooltip& tooltip, Accessor& accessor, IPluginConfig& config);
};

template <class T>
class ProgressProvider : public virtual IServerDataProvider<T> {
public:
    static inline std::shared_ptr<ProgressProvider> const INSTANCE = std::make_shared<ProgressProvider>();

    Identifier getUid() const override { return JadeIds::UNIVERSAL_PROGRESS; }
    int        getDefaultPriority() const override { return TooltipPosition::BODY + 1000; }
    void       appendServerData(CompoundTag&, T& accessor) override { ProgressCommon::appendServerData(accessor); }
    bool       shouldRequestData(T& accessor) override { return ProgressCommon::shouldRequestData(accessor); }
};

template <class T>
class ProgressProviderClient : public ProgressProvider<T>, public IComponentProvider<T> {
public:
    static inline std::shared_ptr<ProgressProviderClient> const INSTANCE = std::make_shared<ProgressProviderClient>();

    Identifier getUid() const override { return JadeIds::UNIVERSAL_PROGRESS; }
    int        getDefaultPriority() const override { return TooltipPosition::BODY + 1000; }
    bool       isRequired() const override { return true; }
    void       appendTooltip(ITooltip& tooltip, T& accessor, IPluginConfig& config) override { ProgressCommon::append(tooltip, accessor, config); }
};

}
