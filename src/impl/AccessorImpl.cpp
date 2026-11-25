// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/AccessorImpl.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"

#include "impl/ui/Elements.h"
#include "platform/item/ScopedItemRegistry.h"

namespace jade::impl {

namespace {

ItemStack const& emptyStack() {
    static ItemStack* empty = [] {
        platform::item::ScopedItemRegistry scoped;
        return new ItemStack();
    }();
    return *empty;
}

int readInt(CompoundTag const& tag, char const* key, int def) {
    if (!tag.contains(key)) return def;
    try {
        return static_cast<int>(tag.at(key));
    } catch (...) {
        return def;
    }
}

}

template <class Base>
AccessorImpl<Base>::AccessorImpl(AccessorInit&& init)
: mLevel(init.level),
  mPlayer(init.player),
  mHit(init.hit),
  mServerConnected(init.serverConnected),
  mShowDetails(init.showDetails),
  mVerify(init.verify) {
    setServerData(std::move(init.serverData));
}

template <class Base>
AccessorImpl<Base>::~AccessorImpl() = default;

template <class Base>
void AccessorImpl<Base>::setServerData(std::unique_ptr<CompoundTag> data) {
    mServerData = data ? std::move(data) : std::make_unique<CompoundTag>();
}

template class AccessorImpl<api::BlockAccessor>;
template class AccessorImpl<api::EntityAccessor>;
template class AccessorImpl<api::EmptyAccessor>;

BlockAccessorImpl::BlockAccessorImpl(AccessorInit&& init, Block const& block, BlockSource& region)
: AccessorImpl(std::move(init)),
  mBlock(&block),
  mRegion(&region) {}

BlockActor* BlockAccessorImpl::getBlockEntity() { return mRegion->getBlockEntity(getPosition()); }

ItemStack const& BlockAccessorImpl::getPickedResult() {
    if (!mPicked) {
        platform::item::ScopedItemRegistry scoped;
        auto                               stack = std::make_unique<ItemStack>();
        if (!mBlock->isAir()) {
            ItemInstance inst = mBlock->asItemInstance(*mRegion, getPosition(), true);
            if (!inst.isNull() && inst.mItem.get()) stack->reinit(*inst.mItem.get(), 1, inst.getAuxValue());
        }
        mPicked = ui::makeSharedItem(*stack);
    }
    return *mPicked;
}

void const* BlockAccessorImpl::getTarget() {
    if (auto* be = getBlockEntity()) return be;
    return nullptr;
}

bool BlockAccessorImpl::verifyData(CompoundTag const& data) {
    if (!mVerify) return true;
    auto pos = getPosition();
    return readInt(data, "x", 0) == pos.x && readInt(data, "y", 0) == pos.y && readInt(data, "z", 0) == pos.z;
}

EntityAccessorImpl::EntityAccessorImpl(AccessorInit&& init) : AccessorImpl(std::move(init)) {}

Actor* EntityAccessorImpl::getEntity() { return mLevel->fetchEntity(mHit.entityId, false); }

ItemStack const& EntityAccessorImpl::getPickedResult() {
    if (!mPicked) {
        Actor* e = getEntity();
        if (e && e->hasCategory(ActorCategory::Item)) {
            mPicked = ui::makeSharedItem(static_cast<ItemActor*>(e)->mItem.get());
        } else {
            return emptyStack();
        }
    }
    return *mPicked;
}

bool EntityAccessorImpl::verifyData(CompoundTag const& data) {
    if (!mVerify) return true;
    if (!data.contains("EntityId")) return false;
    try {
        return static_cast<int64>(data.at("EntityId")) == mHit.entityId.rawID;
    } catch (...) {
        return false;
    }
}

ItemStack const& EmptyAccessorImpl::getPickedResult() { return emptyStack(); }

}
