// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include <algorithm>

#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/world/gamemode/GameMode.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"

#include "impl/WailaClientRegistration.h"
#include "overlay/OverlayRenderer.h"
#include "util/CommonProxy.h"

namespace jade::addon::vanilla {

void BreakingProgressProvider::registerTo(impl::WailaClientRegistration& registration) {
    registration.addTooltipCollectedCallback(0, [](ui::BoxElement& root, Accessor&) { root.addProgressProvider(100, INSTANCE); });
}

std::optional<ui::BoxProgress> BreakingProgressProvider::getProgress(ui::BoxElement&, Accessor&, float) {
    if (!config::IWailaConfig::get().plugin().get(JadeIds::MC_BREAKING_PROGRESS)) {
        mProgressAlpha = 0;
        return std::nullopt;
    }
    auto         ci     = ll::service::getClientInstance();
    LocalPlayer* player = ci ? ci->getLocalPlayer() : nullptr;
    GameMode*    gm     = player ? player->mGameMode.get() : nullptr;
    if (!gm) {
        mProgressAlpha = 0;
        return std::nullopt;
    }
    BlockPos const& pos      = gm->mDestroyBlockPos;
    float           progress = gm->mDestroyProgress;
    if ((pos.x == -1 && pos.y == -1 && pos.z == -1) || progress <= 0) {
        mProgressAlpha = 0;
        return std::nullopt;
    }
    Block const& block      = player->getDimensionBlockSource().getBlock(pos);
    bool         canHarvest = util::CommonProxy::isCorrectToolForDrops(block, *player);
    mProgressAlpha          = std::min(mProgressAlpha + overlay::OverlayRenderer::realtimeDeltaTicks * 0.1f, 0.6f);
    if (progress >= 1) {
        mProgressAlpha = progress = 1;
    } else {
        progress = std::clamp(progress, 0.0f, 1.0f);
    }
    return ui::BoxProgress{progress, canHarvest ? ui::MessageType::TITLE : ui::MessageType::FAILURE, std::nullopt, mProgressAlpha};
}

}
