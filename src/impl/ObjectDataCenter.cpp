// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/ObjectDataCenter.h"

#include <algorithm>
#include <set>

#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ServerThreadExecutor.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/item/registry/ItemRegistryManager.h"
#include "mc/world/item/registry/ItemRegistryRef.h"

#include "api/IJadeProvider.h"
#include "impl/AccessorImpl.h"
#include "impl/WailaCommonRegistration.h"
#include "overlay/WailaTickHandler.h"
#include "platform/MainThread.h"
#include "util/ClientProxy.h"
#include "util/WailaExceptionHandler.h"

namespace jade::impl {

namespace {

auto& logger() { return ll::mod::NativeMod::current()->getLogger(); }

std::set<Identifier> uidsOf(std::vector<std::shared_ptr<api::IJadeProvider>> const& providers) {
    std::set<Identifier> out;
    for (auto const& p : providers) out.insert(p->getUid());
    return out;
}

void deliver(std::shared_ptr<CompoundTag> tag) {
    platform::MainThread::get().post([tag = std::move(tag)] { overlay::WailaTickHandler::instance().setData(tag); });
}

Player* serverPlayer(Level& level, ActorUniqueID id) {
    Actor* a = level.fetchEntity(id, false);
    return a && a->isPlayer() ? static_cast<Player*>(a) : nullptr;
}

}

void ObjectDataCenter::set(api::Accessor& accessor) {
    void const* object = accessor.getTarget();
    if (!object && accessor.getAccessorType() == api::AccessorType::BLOCK) object = &static_cast<api::BlockAccessor&>(accessor).getBlock();
    if (object != sLastObject) {
        overlay::WailaTickHandler::instance().progressTracker.clear();
        sLastObject = object;
        requestServerData();
    }
}

bool ObjectDataCenter::isTimeElapsed(int64_t time) { return util::ClientProxy::millis() - timeLastUpdate >= time; }

void ObjectDataCenter::resetTimer() { timeLastUpdate = util::ClientProxy::millis(); }

void ObjectDataCenter::disconnect() {
    serverConnected = false;
    sLastObject     = nullptr;
    overlay::WailaTickHandler::instance().clearState();
}

void ObjectDataCenter::requestBlockData(api::BlockAccessor& accessor, std::vector<std::shared_ptr<api::IJadeProvider>> const& providers) {
    struct Request {
        api::HitResult       hit;
        int                  dim;
        bool                 showDetails;
        ActorUniqueID        playerId;
        std::set<Identifier> uids;
        CompoundTag          data;
    };
    auto req         = std::make_shared<Request>();
    req->hit         = accessor.getHitResult();
    req->dim         = static_cast<int>(accessor.getPlayer().getDimensionId());
    req->showDetails = accessor.showDetails();
    req->playerId    = accessor.getPlayer().getOrCreateUniqueID();
    req->uids        = uidsOf(providers);
    req->data        = accessor.getServerData();

    ll::thread::ServerThreadExecutor::getDefault().execute([req] {
        auto level = ll::service::getLevel();
        if (!level || level->isClientSide()) return;
        ItemRegistryRef                         registry = level->getItemRegistry();
        ItemRegistryManager::ScopedItemRegistry scoped(registry.mWeakRegistry);
        auto tag = std::make_shared<CompoundTag>(req->data);
        auto pos = req->hit.blockPos;
        (*tag)["x"] = pos.x;
        (*tag)["y"] = pos.y;
        (*tag)["z"] = pos.z;
        auto dimRef = level->getDimension(static_cast<DimensionType>(req->dim)).lock();
        if (!dimRef) return deliver(tag);
        BlockSource& region = dimRef->getBlockSourceFromMainChunkSource();
        Player*      player = serverPlayer(*level, req->playerId);
        if (!player || !region.hasChunksAt(pos, 0, false)) return deliver(tag);
        Block const& block = region.getBlock(pos);
        (*tag)["BlockId"]  = block.getTypeName();
        AccessorInit init;
        init.level           = level.as_ptr();
        init.player          = player;
        init.hit             = req->hit;
        init.serverConnected = true;
        init.showDetails     = req->showDetails;
        init.serverData      = std::make_unique<CompoundTag>(*tag);
        BlockAccessorImpl serverAccessor(std::move(init), block, region);
        for (auto const& provider : WailaCommonRegistration::instance().blockDataProvidersOf(block, serverAccessor.getBlockEntity(), true)) {
            if (!req->uids.contains(provider->getUid())) continue;
            try {
                provider->appendServerData(serverAccessor.getServerData(), serverAccessor);
            } catch (std::exception const& e) {
                util::WailaExceptionHandler::handleErr(e, provider.get());
            }
        }
        deliver(std::make_shared<CompoundTag>(serverAccessor.getServerData()));
    });
}

void ObjectDataCenter::requestEntityData(api::EntityAccessor& accessor, std::vector<std::shared_ptr<api::IJadeProvider>> const& providers) {
    struct Request {
        api::HitResult       hit;
        bool                 showDetails;
        ActorUniqueID        playerId;
        std::set<Identifier> uids;
        CompoundTag          data;
    };
    auto req         = std::make_shared<Request>();
    req->hit         = accessor.getHitResult();
    req->showDetails = accessor.showDetails();
    req->playerId    = accessor.getPlayer().getOrCreateUniqueID();
    req->uids        = uidsOf(providers);
    req->data        = accessor.getServerData();

    ll::thread::ServerThreadExecutor::getDefault().execute([req] {
        auto level = ll::service::getLevel();
        if (!level || level->isClientSide()) return;
        ItemRegistryRef                         registry = level->getItemRegistry();
        ItemRegistryManager::ScopedItemRegistry scoped(registry.mWeakRegistry);
        auto tag             = std::make_shared<CompoundTag>(req->data);
        (*tag)["EntityId"]   = static_cast<int64>(req->hit.entityId.rawID);
        Player* player       = serverPlayer(*level, req->playerId);
        Actor*  entity       = level->fetchEntity(req->hit.entityId, false);
        if (!player || !entity) return deliver(tag);
        AccessorInit init;
        init.level           = level.as_ptr();
        init.player          = player;
        init.hit             = req->hit;
        init.serverConnected = true;
        init.showDetails     = req->showDetails;
        init.serverData      = std::make_unique<CompoundTag>(*tag);
        EntityAccessorImpl serverAccessor(std::move(init));
        for (auto const& provider : WailaCommonRegistration::instance().entityDataProvidersOf(*entity)) {
            if (!req->uids.contains(provider->getUid())) continue;
            try {
                provider->appendServerData(serverAccessor.getServerData(), serverAccessor);
            } catch (std::exception const& e) {
                util::WailaExceptionHandler::handleErr(e, provider.get());
            }
        }
        deliver(std::make_shared<CompoundTag>(serverAccessor.getServerData()));
    });
}

}
