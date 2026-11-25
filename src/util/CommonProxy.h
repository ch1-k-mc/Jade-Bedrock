// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>
#include <utility>

#include "api/Accessor.h"
#include "api/view/IExtensionProvider.h"
#include "api/view/FluidView.h"
#include "api/view/ItemView.h"
#include "impl/lookup/WrappedHierarchyLookup.h"
#include "util/WailaExceptionHandler.h"

class Container;
class Block;
class Player;

namespace jade::addon::universal {
class IItemCollector;
}

namespace jade::util {

class CommonProxy {
public:
    static Container* findContainer(api::Accessor& accessor);
    static Container* findEnderChest(api::Accessor& accessor);
    static bool       isEnderChest(api::Accessor& accessor);
    static bool       isFurnace(api::Accessor& accessor);
    static bool       hasPendingLoot(api::Accessor& accessor);
    static bool       hasDefaultItemStorage(api::Accessor& accessor);

    static std::shared_ptr<addon::universal::IItemCollector> createItemCollector(api::Accessor& accessor);

    static std::optional<api::view::ViewGroups<api::view::FluidView::Data>> wrapFluidStorage(api::Accessor& accessor);
    static bool                                                             hasDefaultFluidStorage(api::Accessor& accessor);
    static bool                                                             isCauldron(api::Accessor& accessor);

    static bool isCorrectToolForDrops(Block const& block, Player const& player);

    template <class T>
    static std::optional<std::pair<Identifier, api::view::ViewGroups<T>>>
    getServerExtensionData(api::Accessor& accessor, impl::lookup::WrappedHierarchyLookup<api::view::IServerExtensionProvider<T>>& lookup) {
        for (auto const& provider : lookup.wrappedGet(accessor)) {
            std::optional<api::view::ViewGroups<T>> groups;
            try {
                groups = provider->getGroups(accessor);
            } catch (std::exception const& e) {
                WailaExceptionHandler::handleErr(e, provider.get());
                continue;
            }
            if (groups) return std::make_pair(provider->getUid(), std::move(*groups));
        }
        return std::nullopt;
    }
};

}
