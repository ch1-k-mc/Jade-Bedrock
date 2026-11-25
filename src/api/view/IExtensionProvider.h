// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "api/Accessor.h"
#include "api/IJadeProvider.h"
#include "api/view/ClientViewGroup.h"
#include "api/view/ViewGroup.h"

namespace jade::api::view {

template <class T>
class IServerExtensionProvider : public virtual IJadeProvider {
public:
    virtual std::optional<ViewGroups<T>> getGroups(Accessor& accessor) = 0;
    virtual bool shouldRequestData(Accessor& accessor) { return true; }
};

template <class In, class Out>
class IClientExtensionProvider : public virtual IJadeProvider {
public:
    virtual std::vector<ClientViewGroup<Out>> getClientGroups(Accessor& accessor, ViewGroups<In> const& groups) = 0;
};

template <class In, class Out>
class HideThingsExtensionProvider : public IServerExtensionProvider<In>, public IClientExtensionProvider<In, Out> {
public:
    static std::shared_ptr<HideThingsExtensionProvider> instance() {
        static auto inst = std::make_shared<HideThingsExtensionProvider>();
        return inst;
    }
    Identifier                        getUid() const override { return JadeIds::UNIVERSAL_HIDE_THINGS; }
    std::optional<ViewGroups<In>>     getGroups(Accessor&) override { return ViewGroups<In>{}; }
    std::vector<ClientViewGroup<Out>> getClientGroups(Accessor&, ViewGroups<In> const&) override { return {}; }
};

}
