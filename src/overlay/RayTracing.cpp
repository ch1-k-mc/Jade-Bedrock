// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "overlay/RayTracing.h"

#include <algorithm>

#include "mc/world/actor/ActorCategory.h"
#include "mc/world/actor/ActorType.h"

#include "mc/world/phys/AABB.h"
#include "mc/world/phys/AABBHitResult.h"

#include <cmath>

#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/phys/HitResult.h"

#include "api/config/IWailaConfig.h"
#include "impl/WailaCommonRegistration.h"

namespace jade::overlay {

RayTracing RayTracing::INSTANCE;

namespace {

float distSqr(Vec3 const& a, Vec3 const& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

BlockPos floorPos(Vec3 const& v) {
    return BlockPos{static_cast<int>(std::floor(v.x)), static_cast<int>(std::floor(v.y)), static_cast<int>(std::floor(v.z))};
}

bool isLiquidBlock(Block const& block) {
    auto const& t = block.getTypeName();
    return t == "minecraft:water" || t == "minecraft:flowing_water" || t == "minecraft:lava" || t == "minecraft:flowing_lava";
}

std::optional<api::HitResult> traceBlocks(BlockSource& region, Vec3 const& from, Vec3 const& dir, double maxDistance, bool stopAtLiquid) {
    double len = std::sqrt(double(dir.x) * dir.x + double(dir.y) * dir.y + double(dir.z) * dir.z);
    if (len < 1e-9) return std::nullopt;
    double dx = dir.x / len, dy = dir.y / len, dz = dir.z / len;
    int    x = int(std::floor(from.x)), y = int(std::floor(from.y)), z = int(std::floor(from.z));
    int    stepX = dx > 0 ? 1 : (dx < 0 ? -1 : 0), stepY = dy > 0 ? 1 : (dy < 0 ? -1 : 0), stepZ = dz > 0 ? 1 : (dz < 0 ? -1 : 0);
    auto   boundary = [](double p, double d, int c) { return d > 0 ? (c + 1 - p) / d : (d < 0 ? (p - c) / -d : 1e30); };
    double tMaxX = boundary(from.x, dx, x), tMaxY = boundary(from.y, dy, y), tMaxZ = boundary(from.z, dz, z);
    double tDeltaX = stepX ? 1.0 / std::abs(dx) : 1e30, tDeltaY = stepY ? 1.0 / std::abs(dy) : 1e30, tDeltaZ = stepZ ? 1.0 / std::abs(dz) : 1e30;
    int    face = -1;
    double t    = 0;
    while (t <= maxDistance) {
        if (face >= 0) {
            Block const& block = region.getBlock(BlockPos{x, y, z});
            bool         liquid = isLiquidBlock(block);
            if (!block.isAir() && (!liquid || stopAtLiquid)) {
                api::HitResult r;
                r.type      = api::HitResult::Type::BLOCK;
                r.blockPos  = BlockPos{x, y, z};
                r.direction = face;
                r.location  = Vec3(float(from.x + dx * t), float(from.y + dy * t), float(from.z + dz * t));
                r.liquid    = liquid;
                return r;
            }
        }
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            t = tMaxX;
            x += stepX;
            tMaxX += tDeltaX;
            face = stepX > 0 ? 4 : 5;
        } else if (tMaxY < tMaxZ) {
            t = tMaxY;
            y += stepY;
            tMaxY += tDeltaY;
            face = stepY > 0 ? 0 : 1;
        } else {
            t = tMaxZ;
            z += stepZ;
            tMaxZ += tDeltaZ;
            face = stepZ > 0 ? 2 : 3;
        }
    }
    return std::nullopt;
}

}

bool RayTracing::canBeTarget(Actor const& target, Actor const& viewEntity) {
    if (target.mRemoved) return false;
    if (target.isSpectator()) return false;
    if (&target == viewEntity.getVehicle()) return false;
    if (target.isInvisible()) return false;
    return !impl::WailaCommonRegistration::instance().entityTypeOperations().shouldHide(target.getTypeName());
}

void RayTracing::fire() {
    auto         ci     = ll::service::getClientInstance();
    LocalPlayer* player = ci ? ci->getLocalPlayer() : nullptr;
    if (!player) return;
    Level&       level  = player->getLevel();
    BlockSource& region = player->getDimensionBlockSource();
    ::HitResult& hit    = level.getHitResult();
    ::HitResult& liquid = level.getLiquidHitResult();
    Vec3         eye    = player->getEyePos();

    if (hit.mType == HitResultType::Entity) {
        if (Actor* e = hit.getEntity(); e && canBeTarget(*e, *player) && entityFilter(*e)) {
            api::HitResult r;
            r.type       = api::HitResult::Type::ENTITY;
            r.entityId   = e->getOrCreateUniqueID();
            r.location   = hit.mPos;
            r.blockPos   = floorPos(hit.mPos);
            mTarget      = r;
            mHitLocation = hit.mPos;
            return;
        }
    }

    if (hit.mType != HitResultType::Entity) {
        Vec3   view  = player->getViewVector(1.0f);
        double reach = double(player->getPickRange());
        if (hit.mType == HitResultType::Tile) reach = std::min(reach, double(eye.distanceTo(hit.mPos)));
        Vec3   end   = eye + view * float(reach);
        AABB   range(Vec3(std::min(eye.x, end.x), std::min(eye.y, end.y), std::min(eye.z, end.z)), Vec3(std::max(eye.x, end.x), std::max(eye.y, end.y), std::max(eye.z, end.z)));
        Actor* best     = nullptr;
        Vec3   bestPos  = end;
        double bestDist = reach;
        for (Actor* e : region.fetchEntities(player, range, true, false)) {
            if (!e || e == player || !(e->hasCategory(ActorCategory::Item) || e->getTypeName() == "minecraft:falling_block") || !canBeTarget(*e, *player) || !entityFilter(*e)) continue;
            AABB box = e->getAABB();
            box.min  = box.min - Vec3(0.1f, 0.1f, 0.1f);
            box.max  = box.max + Vec3(0.1f, 0.1f, 0.1f);
            AABBHitResult hit = box.clip(eye, end);
            if (!hit.mIsHit) continue;
            Vec3 point = hit.mPos;
            double d = eye.distanceTo(point);
            if (d < bestDist) {
                best     = e;
                bestDist = d;
                bestPos  = point;
            }
        }
        if (best) {
            api::HitResult r;
            r.type       = api::HitResult::Type::ENTITY;
            r.entityId   = best->getOrCreateUniqueID();
            r.location   = bestPos;
            r.blockPos   = floorPos(bestPos);
            mTarget      = r;
            mHitLocation = bestPos;
            return;
        }
    }

    auto fluidMode = api::config::IWailaConfig::get().general().getDisplayFluids();
    if (isLiquidBlock(region.getBlock(floorPos(eye)))) fluidMode = api::config::FluidMode::NONE;

    std::optional<api::HitResult> blockResult;
    if (hit.mType == HitResultType::Tile && !region.getBlock(hit.mBlock).isAir()) {
        api::HitResult r;
        r.type      = api::HitResult::Type::BLOCK;
        r.blockPos  = hit.mBlock;
        r.direction = hit.mFacing;
        r.location  = hit.mPos;
        blockResult = r;
    }
    std::optional<api::HitResult> liquidResult;
    if (liquid.mIsHitLiquid && !region.getBlock(liquid.mLiquid).isAir()) {
        api::HitResult r;
        r.type       = api::HitResult::Type::BLOCK;
        r.blockPos   = liquid.mLiquid;
        r.direction  = liquid.mLiquidFacing;
        r.location   = liquid.mLiquidPos;
        r.liquid     = true;
        liquidResult = r;
    }

    if (fluidMode == api::config::FluidMode::ANY && liquidResult
        && (!blockResult || distSqr(eye, liquidResult->location) <= distSqr(eye, blockResult->location))) {
        mTarget = liquidResult;
    } else if (blockResult) {
        mTarget = blockResult;
    } else if (fluidMode == api::config::FluidMode::FALLBACK && liquidResult) {
        mTarget = liquidResult;
    } else if (float extended = api::config::IWailaConfig::get().general().getExtendedReach();
               extended > 0 && hit.mType == HitResultType::NoHit && !liquidResult) {
        double reach = double(player->getPickRange()) + extended;
        auto   far   = traceBlocks(region, eye, player->getViewVector(1.0f), reach, fluidMode != api::config::FluidMode::NONE);
        if (!far && fluidMode == api::config::FluidMode::FALLBACK) far = traceBlocks(region, eye, player->getViewVector(1.0f), reach, true);
        if (far) {
            mTarget = far;
        } else {
            api::HitResult miss;
            miss.type     = api::HitResult::Type::MISS;
            miss.location = eye;
            miss.blockPos = floorPos(eye);
            mTarget       = miss;
        }
    } else {
        api::HitResult miss;
        miss.type     = api::HitResult::Type::MISS;
        miss.location = hit.mType == HitResultType::NoHit ? Vec3(eye.x, eye.y, eye.z) : hit.mPos;
        miss.blockPos = floorPos(miss.location);
        mTarget       = miss;
    }
    mHitLocation = mTarget->location;
}

}
