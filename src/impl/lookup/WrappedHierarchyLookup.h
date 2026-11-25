// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "mc/world/actor/Actor.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"

#include "api/Accessor.h"
#include "impl/lookup/HierarchyLookup.h"

namespace jade::impl::lookup {

template <class P>
class WrappedHierarchyLookup {
public:
    void register_(api::BlockClass clazz, std::shared_ptr<P> provider) {
        blocks.register_(std::move(clazz), std::move(provider));
        mEmpty = false;
    }
    void register_(api::BlockActorClass clazz, std::shared_ptr<P> provider) {
        blockActors.register_(std::move(clazz), std::move(provider));
        mEmpty = false;
    }
    void register_(api::EntityClass clazz, std::shared_ptr<P> provider) {
        entities.register_(std::move(clazz), std::move(provider));
        mEmpty = false;
    }
    void registerAnyTarget(std::shared_ptr<P> provider) {
        register_(api::BlockActorClass::any(), provider);
        register_(api::EntityClass::any(), std::move(provider));
    }

    std::vector<std::shared_ptr<P>> wrappedGet(api::Accessor& accessor) {
        std::vector<std::shared_ptr<P>> set;
        auto addAll = [&set](std::vector<std::shared_ptr<P>> const& list) {
            for (auto const& p : list)
                if (std::find(set.begin(), set.end(), p) == set.end()) set.push_back(p);
        };
        if (accessor.getAccessorType() == api::AccessorType::BLOCK) {
            auto& a = static_cast<api::BlockAccessor&>(accessor);
            addAll(blocks.get(a.getBlock(), a.getBlock().getTypeName()));
            if (BlockActor* be = a.getBlockEntity()) addAll(blockActors.get(*be, std::to_string(static_cast<int>(be->getType()))));
        } else if (accessor.getAccessorType() == api::AccessorType::ENTITY) {
            if (Actor* e = static_cast<api::EntityAccessor&>(accessor).getEntity()) addAll(entities.get(*e, e->getTypeName()));
        }
        if (mPriorities) {
            auto* pr = mPriorities;
            std::stable_sort(set.begin(), set.end(), [pr](auto const& a, auto const& b) { return pr->byValue(*a) < pr->byValue(*b); });
        }
        return set;
    }

    bool hitsAny(api::Accessor& accessor, std::function<bool(P&, api::Accessor&)> const& predicate) {
        if (accessor.isServersideContent()) return true;
        for (auto const& p : wrappedGet(accessor))
            if (predicate(*p, accessor)) return true;
        return false;
    }

    void invalidate() {
        blocks.invalidate();
        blockActors.invalidate();
        entities.invalidate();
    }
    void loadComplete(PriorityStore& priorities) {
        blocks.loadComplete(priorities);
        blockActors.loadComplete(priorities);
        entities.loadComplete(priorities);
        mPriorities = &priorities;
    }
    [[nodiscard]] bool isEmpty() const { return mEmpty; }

    HierarchyLookup<Block, api::BlockClass, P>           blocks;
    HierarchyLookup<BlockActor, api::BlockActorClass, P> blockActors;
    HierarchyLookup<Actor, api::EntityClass, P>          entities;

private:
    PriorityStore* mPriorities = nullptr;
    bool           mEmpty      = true;
};

}
