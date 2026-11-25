// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "addon/universal/ItemIterator.h"
#include "api/view/ItemView.h"
#include "api/view/ViewGroup.h"
#include "util/ClientProxy.h"

namespace jade::addon::universal {

using api::view::ItemData;
using api::view::ViewGroup;
using api::view::ViewGroups;

class IItemCollector {
public:
    virtual ~IItemCollector()                                                       = default;
    virtual std::optional<ViewGroups<ItemData>> update(api::Accessor& accessor)    = 0;
};

class EmptyItemCollector : public IItemCollector {
public:
    std::optional<ViewGroups<ItemData>> update(api::Accessor&) override { return std::nullopt; }
};

template <class T>
class ItemCollector : public IItemCollector {
public:
    static constexpr int MAX_SIZE = 54;

    explicit ItemCollector(std::unique_ptr<ItemIterator<T>> iterator) : mIterator(std::move(iterator)) {}

    std::optional<ViewGroups<ItemData>> update(api::Accessor& accessor) override {
        if (!mIterator) return std::nullopt;
        T* container = mIterator->find(accessor);
        if (!container) return std::nullopt;
        bool sorted = false;
        if (accessor.getServerData().contains("SortItems")) {
            try {
                sorted = static_cast<int>(accessor.getServerData().at("SortItems")) != 0;
            } catch (...) {}
        }
        long long currentVersion = mIterator->getVersion(*container);
        long long gameTime       = util::ClientProxy::millis();
        auto&     result         = sorted ? mSortedMergedResult : mMergedResult;
        if (result && mIterator->isFinished()) {
            if (mVersion == currentVersion) return result;
            if (mLastTimeFinished + 250 > gameTime) return result;
            mIterator->reset();
        }
        int count = 0;
        for (ItemStack const* stack : mIterator->populate(*container, MAX_SIZE * 2)) {
            ++count;
            if (shown(*stack)) mItems.addTo(ItemData::of(*stack, 0), static_cast<int>(stack->mCount));
        }
        mIterator->afterPopulate(count);
        if (result && !mIterator->isFinished()) {
            updateCollectingProgress(result->front());
            return result;
        }
        ViewGroups<ItemData> groups{updateCollectingProgress(ViewGroup<ItemData>(mItems.partialResult(sorted)))};
        if (mIterator->isFinished()) {
            if (sorted) {
                mMergedResult       = ViewGroups<ItemData>{updateCollectingProgress(ViewGroup<ItemData>(mItems.partialResult(false)))};
                mSortedMergedResult = groups;
            } else {
                mMergedResult       = groups;
                mSortedMergedResult = ViewGroups<ItemData>{updateCollectingProgress(ViewGroup<ItemData>(mItems.partialResult(true)))};
            }
            mLastTimeIsEmpty  = groups.front().views.empty();
            mVersion          = currentVersion;
            mLastTimeFinished = gameTime;
            mItems.clear();
        }
        return groups;
    }

private:
    static bool shown(ItemStack const& stack) { return !stack.isNull() && stack.mCount > 0; }

    ViewGroup<ItemData>& updateCollectingProgress(ViewGroup<ItemData>& group) {
        if (mLastTimeIsEmpty && group.views.empty()) return group;
        float progress = mIterator->getCollectingProgress();
        auto& data     = group.getExtraData();
        if (std::isnan(progress) || progress >= 1) data.erase("Collecting");
        else data["Collecting"] = progress;
        return group;
    }
    ViewGroup<ItemData> updateCollectingProgress(ViewGroup<ItemData>&& group) {
        updateCollectingProgress(group);
        return std::move(group);
    }

    class Items {
    public:
        void clear() {
            mSmallestCount = 0;
            mOrder.clear();
            mIndex.clear();
            mSorted.clear();
            mSortedSet.clear();
        }
        std::vector<ItemData> partialResult(bool sort) {
            std::vector<ItemData> out;
            if (sort) {
                std::stable_sort(mSorted.begin(), mSorted.end(), [this](std::string const& a, std::string const& b) { return countOf(a) > countOf(b); });
                if (mSorted.size() > MAX_SIZE) {
                    mSorted.resize(MAX_SIZE);
                    mSortedSet.clear();
                    for (auto const& k : mSorted) mSortedSet[k] = true;
                }
                if (!mSorted.empty()) mSmallestCount = countOf(mSorted.back());
                for (auto const& k : mSorted) out.push_back(stackOf(k));
            } else {
                for (auto const& k : mOrder) {
                    if (out.size() >= MAX_SIZE) break;
                    out.push_back(stackOf(k));
                }
            }
            return out;
        }
        void addTo(ItemData def, int count) {
            std::string key = def.key();
            int         old = 0;
            auto        it  = mIndex.find(key);
            if (it == mIndex.end()) {
                def.count = count;
                mIndex.emplace(key, std::move(def));
                mOrder.push_back(key);
            } else {
                old              = it->second.count;
                it->second.count = old + count;
            }
            if (!mSortedSet.contains(key)) {
                count += old;
                if (mSorted.size() < MAX_SIZE) {
                    mSortedSet[key] = true;
                    mSorted.push_back(key);
                    mSmallestCount = std::min(mSmallestCount, count);
                } else if (count > mSmallestCount) {
                    mSortedSet[key] = true;
                    mSorted.push_back(key);
                }
            }
        }

    private:
        int      countOf(std::string const& k) const { return mIndex.at(k).count; }
        ItemData stackOf(std::string const& k) const { return mIndex.at(k); }

        std::vector<std::string>                  mOrder;
        std::unordered_map<std::string, ItemData> mIndex;
        std::vector<std::string>                  mSorted;
        std::unordered_map<std::string, bool>     mSortedSet;
        int                                       mSmallestCount = 0;
    };

    Items                                mItems;
    std::unique_ptr<ItemIterator<T>>     mIterator;
    long long                            mVersion          = 0;
    long long                            mLastTimeFinished = 0;
    bool                                 mLastTimeIsEmpty  = false;
    std::optional<ViewGroups<ItemData>>  mMergedResult;
    std::optional<ViewGroups<ItemData>>  mSortedMergedResult;
};

}
