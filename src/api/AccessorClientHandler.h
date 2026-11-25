// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "api/IJadeProvider.h"

namespace jade::api {

class AccessorClientHandler {
public:
    virtual ~AccessorClientHandler() = default;

    virtual bool shouldDisplay(Accessor& accessor) = 0;
    virtual std::vector<std::shared_ptr<IJadeProvider>> shouldRequestData(Accessor& accessor)                                                 = 0;
    virtual void                                        requestData(Accessor& accessor, std::vector<std::shared_ptr<IJadeProvider>> const& providers) = 0;
    virtual ui::ElementPtr                              getIcon(Accessor& accessor)                                                           = 0;
    virtual void gatherComponents(Accessor& accessor, std::function<ITooltip&(IJadeProvider const&)> const& tooltipProvider)                 = 0;

    static bool isEnabled(IToggleableProvider const& provider);
};

}

namespace jade::impl {

class BlockAccessorClientHandler : public api::AccessorClientHandler {
public:
    bool                                             shouldDisplay(api::Accessor& accessor) override;
    std::vector<std::shared_ptr<api::IJadeProvider>> shouldRequestData(api::Accessor& accessor) override;
    void        requestData(api::Accessor& accessor, std::vector<std::shared_ptr<api::IJadeProvider>> const& providers) override;
    api::ui::ElementPtr getIcon(api::Accessor& accessor) override;
    void gatherComponents(api::Accessor& accessor, std::function<api::ITooltip&(api::IJadeProvider const&)> const& tooltipProvider) override;
};

class EntityAccessorClientHandler : public api::AccessorClientHandler {
public:
    bool                                             shouldDisplay(api::Accessor& accessor) override;
    std::vector<std::shared_ptr<api::IJadeProvider>> shouldRequestData(api::Accessor& accessor) override;
    void        requestData(api::Accessor& accessor, std::vector<std::shared_ptr<api::IJadeProvider>> const& providers) override;
    api::ui::ElementPtr getIcon(api::Accessor& accessor) override;
    void gatherComponents(api::Accessor& accessor, std::function<api::ITooltip&(api::IJadeProvider const&)> const& tooltipProvider) override;
};

class EmptyAccessorClientHandler : public api::AccessorClientHandler {
public:
    bool                                             shouldDisplay(api::Accessor&) override { return false; }
    std::vector<std::shared_ptr<api::IJadeProvider>> shouldRequestData(api::Accessor&) override { return {}; }
    void                requestData(api::Accessor&, std::vector<std::shared_ptr<api::IJadeProvider>> const&) override {}
    api::ui::ElementPtr getIcon(api::Accessor&) override { return nullptr; }
    void                gatherComponents(api::Accessor&, std::function<api::ITooltip&(api::IJadeProvider const&)> const&) override {}
};

}
