// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
// Includes code adapted from the JustEnoughItems Bedrock port (MIT; derived from JEI, Copyright (c) 2014-2015 mezz).
#pragma once

#include <optional>

#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/world/item/registry/ItemRegistryManager.h"
#include "mc/world/item/registry/ItemRegistryRef.h"

namespace jade::platform::item {

class ScopedItemRegistry {
public:
    ScopedItemRegistry() {
        if (auto ci = ll::service::getClientInstance()) init(*ci);
    }
    explicit ScopedItemRegistry(ClientInstance& ci) { init(ci); }
    ScopedItemRegistry(ScopedItemRegistry const&)            = delete;
    ScopedItemRegistry& operator=(ScopedItemRegistry const&) = delete;

private:
    void init(ClientInstance& ci) {
        mRegistry = ci.getItemRegistry();
        mScoped.emplace(mRegistry.mWeakRegistry);
    }

    ItemRegistryRef                                        mRegistry;
    std::optional<ItemRegistryManager::ScopedItemRegistry> mScoped;
};

}
