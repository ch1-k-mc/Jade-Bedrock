// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <functional>
#include <vector>

#include "mc/world/Container.h"
#include "mc/world/item/ItemStack.h"

#include "api/Accessor.h"

namespace jade::addon::universal {

template <class T>
class ItemIterator {
public:
    using Finder = std::function<T*(api::Accessor&)>;
    static inline std::atomic<long long> version{0};

    virtual ~ItemIterator() = default;

    T*                 find(api::Accessor& accessor) { return mContainerFinder(accessor); }
    [[nodiscard]] bool isFinished() const { return mFinished; }
    virtual long long  getVersion(T&) { return version.fetch_add(1); }
    virtual std::vector<ItemStack const*> populate(T& container, int amount) = 0;
    virtual void       reset() {
        mCurrentIndex = mFromIndex;
        mFinished     = false;
    }
    virtual void afterPopulate(int count) {
        mCurrentIndex += count;
        if (count == 0 || mCurrentIndex >= 10000) mFinished = true;
    }
    virtual float getCollectingProgress() { return NAN; }

protected:
    ItemIterator(Finder containerFinder, int fromIndex)
    : mContainerFinder(std::move(containerFinder)),
      mFromIndex(fromIndex),
      mCurrentIndex(fromIndex) {}

    Finder mContainerFinder;
    int    mFromIndex;
    bool   mFinished = false;
    int    mCurrentIndex;
};

template <class T>
class SlottedItemIterator : public ItemIterator<T> {
public:
    std::vector<ItemStack const*> populate(T& container, int amount) override {
        int slotCount = getSlotCount(container);
        int toIndex   = this->mCurrentIndex > INT_MAX - amount ? INT_MAX : this->mCurrentIndex + amount;
        if (toIndex >= slotCount) {
            toIndex         = slotCount;
            this->mFinished = true;
            mProgress       = 1;
        } else {
            mProgress = float(this->mCurrentIndex - this->mFromIndex) / float(slotCount - this->mFromIndex);
        }
        std::vector<ItemStack const*> out;
        for (int slot = this->mCurrentIndex; slot < toIndex; ++slot) out.push_back(&getItemInSlot(container, slot));
        return out;
    }
    float getCollectingProgress() override { return mProgress; }

protected:
    using ItemIterator<T>::ItemIterator;
    virtual int              getSlotCount(T& container)            = 0;
    virtual ItemStack const& getItemInSlot(T& container, int slot) = 0;

    float mProgress = 0;
};

class ContainerItemIterator : public SlottedItemIterator<Container> {
public:
    ContainerItemIterator(Finder containerFinder, int fromIndex) : SlottedItemIterator(std::move(containerFinder), fromIndex) {}

protected:
    int              getSlotCount(Container& container) override { return container.getContainerSize(); }
    ItemStack const& getItemInSlot(Container& container, int slot) override { return container.getItem(slot); }
};

}
