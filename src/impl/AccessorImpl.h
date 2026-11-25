// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>

#include "api/Accessor.h"

namespace jade::impl {

struct AccessorInit {
    Level*                       level  = nullptr;
    Player*                      player = nullptr;
    std::unique_ptr<CompoundTag> serverData;
    api::HitResult               hit;
    bool                         serverConnected = false;
    bool                         showDetails     = false;
    bool                         verify          = false;
};

template <class Base>
class AccessorImpl : public Base {
public:
    explicit AccessorImpl(AccessorInit&& init);
    ~AccessorImpl() override;

    Level&                getLevel() override { return *mLevel; }
    Player&               getPlayer() override { return *mPlayer; }
    CompoundTag&          getServerData() override { return *mServerData; }
    void                  setServerData(std::unique_ptr<CompoundTag> data) override;
    api::HitResult const& getHitResult() const override { return mHit; }
    bool                  isServerConnected() const override { return mServerConnected; }
    bool                  showDetails() const override { return mShowDetails; }
    bool                  shouldVerifyData() const override { return mVerify; }
    void                  requireVerification() { mVerify = true; }

protected:
    Level*                       mLevel;
    Player*                      mPlayer;
    std::unique_ptr<CompoundTag> mServerData;
    api::HitResult               mHit;
    bool                         mServerConnected;
    bool                         mShowDetails;
    bool                         mVerify;
    std::shared_ptr<ItemStack>   mPicked;
};

class BlockAccessorImpl : public AccessorImpl<api::BlockAccessor> {
public:
    BlockAccessorImpl(AccessorInit&& init, Block const& block, BlockSource& region);

    Block const&     getBlock() override { return *mBlock; }
    BlockActor*      getBlockEntity() override;
    BlockSource&     getRegion() override { return *mRegion; }
    ItemStack const& getPickedResult() override;
    void const*      getTarget() override;
    bool             verifyData(CompoundTag const& data) override;

private:
    Block const* mBlock;
    BlockSource* mRegion;
};

class EntityAccessorImpl : public AccessorImpl<api::EntityAccessor> {
public:
    explicit EntityAccessorImpl(AccessorInit&& init);

    Actor*           getEntity() override;
    Actor*           getRawEntity() override { return getEntity(); }
    ItemStack const& getPickedResult() override;
    void const*      getTarget() override { return getEntity(); }
    bool             verifyData(CompoundTag const& data) override;
};

class EmptyAccessorImpl : public AccessorImpl<api::EmptyAccessor> {
public:
    explicit EmptyAccessorImpl(AccessorInit&& init) : AccessorImpl(std::move(init)) {}
    ItemStack const& getPickedResult() override;
    void const*      getTarget() override { return nullptr; }
    bool             verifyData(CompoundTag const&) override { return true; }
};

}
