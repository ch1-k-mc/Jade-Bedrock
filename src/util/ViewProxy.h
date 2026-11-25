// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "mc/deps/nbt/CompoundTag.h"

#include "api/Accessor.h"
#include "api/ITooltip.h"
#include "api/view/IExtensionProvider.h"
#include "util/WailaExceptionHandler.h"

namespace jade::util {

template <class In, class Out>
std::optional<std::vector<api::view::ClientViewGroup<Out>>> mapToClientGroups(
    api::Accessor&                                                                       accessor,
    Identifier const&                                                                    key,
    std::function<api::view::IClientExtensionProvider<In, Out>*(Identifier const&)> const& mapper,
    api::ITooltip&                                                                       tooltip
) {
    CompoundTag const& data = accessor.getServerData();
    if (!data.contains(key)) return std::nullopt;
    auto const& v = data.at(key);
    if (!v.template hold<CompoundTag>()) return std::nullopt;
    auto entry = api::view::decodeGroups<In>(v.template get<CompoundTag>());
    if (!entry) return std::nullopt;
    auto* provider = mapper(entry->first);
    if (!provider) return std::nullopt;
    try {
        return provider->getClientGroups(accessor, entry->second);
    } catch (std::exception const& e) {
        WailaExceptionHandler::handleErr(e, provider, &tooltip);
        return std::nullopt;
    }
}

}
