// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>

#include "mc/deps/core/math/Vec3.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/world/level/BlockPos.h"

class Level;
class Player;
class Block;
class BlockSource;
class BlockActor;
class Actor;
class CompoundTag;
class ItemStack;

namespace jade::api {

struct HitResult {
    enum class Type { MISS, BLOCK, ENTITY };
    Type          type = Type::MISS;
    BlockPos      blockPos{0, 0, 0};
    int           direction = 1;
    Vec3          location{0, 0, 0};
    ActorUniqueID entityId{};
    bool          liquid = false;
};

enum class AccessorType { BLOCK, ENTITY, EMPTY };

class Accessor {
public:
    virtual ~Accessor() = default;

    virtual Level&  getLevel()  = 0;
    virtual Player& getPlayer() = 0;
    virtual CompoundTag&   getServerData()                                    = 0;
    virtual void           setServerData(std::unique_ptr<CompoundTag> data)   = 0;
    virtual HitResult const& getHitResult() const                             = 0;
    virtual bool           isServerConnected() const                          = 0;
    virtual ItemStack const& getPickedResult()            = 0;
    virtual bool             showDetails() const          = 0;
    virtual void const*      getTarget()                  = 0;
    virtual AccessorType     getAccessorType() const      = 0;
    virtual bool             verifyData(CompoundTag const& data) = 0;
    virtual bool             shouldVerifyData() const     = 0;
    virtual float            tickRate() const { return 20.0f; }
    bool isServersideContent() const { return false; }
};

class BlockAccessor : public Accessor {
public:
    virtual Block const& getBlock()       = 0;
    Block const&         getBlockState() { return getBlock(); }
    virtual BlockActor*  getBlockEntity() = 0;
    virtual BlockPos     getPosition() const { return getHitResult().blockPos; }
    virtual int          getSide() const { return getHitResult().direction; }
    virtual BlockSource& getRegion()      = 0;
    AccessorType         getAccessorType() const override { return AccessorType::BLOCK; }
};

class EntityAccessor : public Accessor {
public:
    virtual Actor* getEntity()    = 0;
    virtual Actor* getRawEntity() = 0;
    AccessorType   getAccessorType() const override { return AccessorType::ENTITY; }
};

class EmptyAccessor : public Accessor {
public:
    AccessorType getAccessorType() const override { return AccessorType::EMPTY; }
};

}
