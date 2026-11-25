// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "api/IJadeProvider.h"
#include "impl/PriorityStore.h"

namespace jade::impl::lookup {

template <class Target, class Class, class P>
class HierarchyLookup {
public:
    explicit HierarchyLookup(bool singleton = false) : mSingleton(singleton) {}

    void register_(Class clazz, std::shared_ptr<P> provider);

    std::vector<std::shared_ptr<P>> const& get(Target const& target, std::string const& key);

    [[nodiscard]] bool isEmpty() const { return mObjects.empty(); }
    void               invalidate() {
        std::lock_guard lock(mMutex);
        mCache.clear();
    }
    void loadComplete(PriorityStore& priorities);

    [[nodiscard]] bool isMapped(std::shared_ptr<P> const& provider) const {
        for (auto const& e : mObjects)
            if (e.provider == provider) return true;
        return false;
    }
    std::shared_ptr<P> byKey(Identifier const& uid) const {
        for (auto const& e : mObjects)
            if (e.provider->getUid() == uid) return e.provider;
        return nullptr;
    }

    struct Entry {
        Class              clazz;
        std::shared_ptr<P> provider;
        size_t             order;
    };
    [[nodiscard]] std::vector<Entry> const& entries() const { return mObjects; }

private:
    bool                                                   mSingleton;
    std::vector<Entry>                                     mObjects;
    std::map<std::string, std::vector<std::shared_ptr<P>>> mCache;
    PriorityStore*                                         mPriorities = nullptr;
    std::mutex                                             mMutex;
};

template <class Target, class Class, class P>
void HierarchyLookup<Target, Class, P>::register_(Class clazz, std::shared_ptr<P> provider) {
    if (!provider) throw std::invalid_argument("provider is null");
    mObjects.push_back({std::move(clazz), std::move(provider), mObjects.size()});
    invalidate();
}

template <class Target, class Class, class P>
std::vector<std::shared_ptr<P>> const& HierarchyLookup<Target, Class, P>::get(Target const& target, std::string const& key) {
    std::lock_guard lock(mMutex);
    auto            it = mCache.find(key);
    if (it != mCache.end()) return it->second;
    std::vector<Entry const*> matched;
    for (auto const& e : mObjects)
        if (e.clazz.test(target)) matched.push_back(&e);
    std::stable_sort(matched.begin(), matched.end(), [](Entry const* a, Entry const* b) {
        if (a->clazz.depth != b->clazz.depth) return a->clazz.depth < b->clazz.depth;
        return a->order < b->order;
    });
    std::vector<std::shared_ptr<P>> list;
    for (auto const* e : matched) list.push_back(e->provider);
    if (mPriorities) {
        auto* pr = mPriorities;
        std::stable_sort(list.begin(), list.end(), [pr](auto const& a, auto const& b) { return pr->byKey(a->getUid()) < pr->byKey(b->getUid()); });
    }
    if (mSingleton && !list.empty()) list.resize(1);
    return mCache.emplace(key, std::move(list)).first->second;
}

template <class Target, class Class, class P>
void HierarchyLookup<Target, Class, P>::loadComplete(PriorityStore& priorities) {
    std::map<std::string, std::set<Identifier>> seen;
    for (auto const& e : mObjects) {
        if (!seen[e.clazz.name].insert(e.provider->getUid()).second) {
            throw std::logic_error("Duplicate UID: " + e.provider->getUid() + " for " + e.clazz.name);
        }
    }
    mPriorities = &priorities;
    invalidate();
}

template <class P>
class PairHierarchyLookup {
public:
    HierarchyLookup<Block, api::BlockClass, P>           first;
    HierarchyLookup<BlockActor, api::BlockActorClass, P> second;

    void register_(api::BlockClass clazz, std::shared_ptr<P> provider) { first.register_(std::move(clazz), std::move(provider)); }
    void register_(api::BlockActorClass clazz, std::shared_ptr<P> provider) { second.register_(std::move(clazz), std::move(provider)); }

    std::vector<std::shared_ptr<P>> getMerged(Block const& block, std::string const& blockKey, BlockActor const& actor, std::string const& actorKey, PriorityStore& pr) {
        auto const& a = first.get(block, blockKey);
        auto const& b = second.get(actor, actorKey);
        if (a.empty()) return b;
        if (b.empty()) return a;
        std::vector<std::shared_ptr<P>> merged;
        for (auto const& p : a)
            if (std::find(merged.begin(), merged.end(), p) == merged.end()) merged.push_back(p);
        for (auto const& p : b)
            if (std::find(merged.begin(), merged.end(), p) == merged.end()) merged.push_back(p);
        std::stable_sort(merged.begin(), merged.end(), [&pr](auto const& x, auto const& y) { return pr.byKey(x->getUid()) < pr.byKey(y->getUid()); });
        return merged;
    }
    bool isMapped(std::shared_ptr<P> const& p) const { return first.isMapped(p) || second.isMapped(p); }
    void loadComplete(PriorityStore& pr) {
        first.loadComplete(pr);
        second.loadComplete(pr);
    }
    void invalidate() {
        first.invalidate();
        second.invalidate();
    }
};

}
