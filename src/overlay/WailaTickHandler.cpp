// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "overlay/WailaTickHandler.h"

#include <cmath>

#include "ll/api/service/Bedrock.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"

#include "api/config/IWailaConfig.h"
#include "api/theme/IThemeHelper.h"
#include "impl/AccessorImpl.h"
#include "impl/ObjectDataCenter.h"
#include "impl/Tooltip.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"
#include "impl/ui/BoxElementImpl.h"
#include "overlay/OverlayRenderer.h"
#include "overlay/RayTracing.h"
#include "platform/item/ScopedItemRegistry.h"
#include "util/ClientProxy.h"
#include "util/JadeKeys.h"

namespace jade::overlay {

using api::config::IWailaConfig;

WailaTickHandler& WailaTickHandler::instance() {
    static WailaTickHandler handler;
    return handler;
}

WailaTickHandler::State WailaTickHandler::State::create(
    std::optional<State> const&,
    std::shared_ptr<api::Accessor> accessor,
    api::AccessorClientHandler&    handler,
    std::shared_ptr<CompoundTag>   data
) {
    State s;
    s.accessor = std::move(accessor);
    s.handler  = &handler;
    s.data     = (data && s.accessor->verifyData(*data)) ? std::move(data) : nullptr;
    return s;
}

api::ui::ElementPtr WailaTickHandler::State::getIcon() const {
    if (!accessor || !handler) return nullptr;
    return handler->getIcon(*accessor);
}

bool WailaTickHandler::State::verifyData(CompoundTag const* d) const { return !d || accessor->verifyData(*d); }

void WailaTickHandler::clearState() {
    state.reset();
    rootElement.reset();
    progressTracker.clear();
}

void WailaTickHandler::setData(std::shared_ptr<CompoundTag> tag) {
    if (!state || !tag || !state->verifyData(tag.get())) return;
    state->data = std::move(tag);
}

std::shared_ptr<api::Accessor> WailaTickHandler::createEmpty(api::HitResult const& hit) {
    auto            ci     = ll::service::getClientInstance();
    LocalPlayer*    player = ci->getLocalPlayer();
    impl::AccessorInit init;
    init.level  = &player->getLevel();
    init.player = player;
    init.hit    = hit;
    init.hit.type = api::HitResult::Type::MISS;
    return std::make_shared<impl::EmptyAccessorImpl>(std::move(init));
}

void WailaTickHandler::tickClient() {
    auto         ci     = ll::service::getClientInstance();
    LocalPlayer* player = ci ? ci->getLocalPlayer() : nullptr;
    if (!player) {
        OverlayRenderer::clearLingerTooltip();
        clearState();
        return;
    }
    platform::item::ScopedItemRegistry scoped(*ci);
    progressTracker.tick();
    auto& config = IWailaConfig::get().general();
    if (!config.shouldDisplayTooltip()) {
        clearState();
        return;
    }
    if (util::ClientProxy::shouldHideWithGui()) return;

    RayTracing::INSTANCE.fire();
    auto const& target = RayTracing::INSTANCE.getTarget();
    if (!target) {
        clearState();
        return;
    }

    Level&                         level  = player->getLevel();
    BlockSource&                   region = player->getDimensionBlockSource();
    auto&                          reg    = impl::WailaClientRegistration::instance();
    std::shared_ptr<api::Accessor> accessor;
    auto serverLevel = ll::service::getLevel();
    bool connected   = serverLevel && !serverLevel->isClientSide();
    bool showDetails = util::JadeKeys::isShowDetailsPressed();

    if (target->type == api::HitResult::Type::BLOCK) {
        Block const& block = region.getBlock(target->blockPos);
        if (block.isAir()) {
            accessor = createEmpty(*target);
        } else {
            impl::AccessorInit init;
            init.level           = &level;
            init.player          = player;
            init.hit             = *target;
            init.serverConnected = connected;
            init.showDetails     = showDetails;
            init.verify          = true;
            if (auto* d = getData()) init.serverData = std::make_unique<CompoundTag>(*d);
            accessor             = std::make_shared<impl::BlockAccessorImpl>(std::move(init), block, region);
        }
    } else if (target->type == api::HitResult::Type::ENTITY) {
        impl::AccessorInit init;
        init.level           = &level;
        init.player          = player;
        init.hit             = *target;
        init.serverConnected = connected;
        init.showDetails     = showDetails;
        init.verify          = true;
        if (auto* d = getData()) init.serverData = std::make_unique<CompoundTag>(*d);
        accessor             = std::make_shared<impl::EntityAccessorImpl>(std::move(init));
        if (!static_cast<api::EntityAccessor&>(*accessor).getEntity()) accessor = createEmpty(*target);
    } else {
        accessor = createEmpty(*target);
    }

    {
        auto                           originalAccessor = accessor;
        std::shared_ptr<api::Accessor> originalEmpty;
        if (originalAccessor->getAccessorType() == api::AccessorType::EMPTY) originalEmpty = originalAccessor;
        reg.rayTraceCallback.call([&](auto const& cb) {
            accessor = cb(*target, accessor, originalAccessor);
            if (!accessor) {
                if (!originalEmpty) originalEmpty = createEmpty(originalAccessor->getHitResult());
                accessor = originalEmpty;
            }
        });
    }

    if (!accessor->verifyData(accessor->getServerData())) accessor->setServerData(nullptr);
    impl::ObjectDataCenter::set(*accessor);
    auto& handler = reg.getAccessorHandler(accessor->getAccessorType());
    if (!handler.shouldDisplay(*accessor)) {
        clearState();
        return;
    }
    std::shared_ptr<CompoundTag> prevData = state ? state->data : nullptr;
    state                                 = State::create(state, accessor, handler, prevData);
    if (accessor->isServerConnected()) {
        auto data = std::make_unique<CompoundTag>(accessor->getServerData());
        accessor->setServerData(nullptr);
        auto providers = handler.shouldRequestData(*accessor);
        if (impl::ObjectDataCenter::isTimeElapsed(impl::ObjectDataCenter::rateLimiter)) {
            impl::ObjectDataCenter::resetTimer();
            if (!providers.empty()) handler.requestData(*accessor, providers);
        }
        if (!providers.empty() && !getData()) return;
        accessor->setServerData(std::move(data));
    }

    auto&                themes = api::theme::IThemeHelper::get();
    api::theme::Theme*   theme  = &IWailaConfig::get().overlay().getTheme();
    api::theme::Theme*   holder = theme;
    bool                 cancel = false;
    reg.beforeTooltipCollectCallback.call([&](auto const& cb) {
        if (!cancel && !cb(holder, *accessor)) cancel = true;
    });
    if (cancel) return;
    if (holder && holder != theme) themes.setThemeOverride(holder);

    auto tooltip = std::make_shared<impl::Tooltip>();
    tooltip->setIcon(state->getIcon());
    if (config.getDisplayMode() == api::config::DisplayMode::LITE && !util::JadeKeys::isShowDetailsPressed()) {
        impl::Tooltip dummyTooltip;
        auto&         priorities = impl::WailaCommonRegistration::instance().priorities;
        handler.gatherComponents(*accessor, [&](api::IJadeProvider const& p) -> api::ITooltip& {
            if (std::abs(priorities.byValue(p)) > 5000) return *tooltip;
            return dummyTooltip;
        });
        if (!dummyTooltip.isEmpty()) tooltip->sneakyDetails = true;
    } else {
        handler.gatherComponents(*accessor, [&](api::IJadeProvider const&) -> api::ITooltip& { return *tooltip; });
    }

    tooltip->setIcon(themes.theme().modifyIcon(tooltip->getIcon()));
    auto newElement = std::make_shared<impl::ui::BoxElementImpl>(tooltip, themes.theme().tooltipStyle);
    newElement->tag(JadeIds::ROOT);
    reg.tooltipCollectedCallback.call([&](auto const& cb) { cb(*newElement, *accessor); });
    if (newElement->tooltip().isDirty) newElement->updateSize();
    if (!rootElement || rootElement->layout->getX() != newElement->layout->getX() || rootElement->layout->getY() != newElement->layout->getY()
        || rootElement->layout->getWidth() != newElement->layout->getWidth()
        || rootElement->layout->getHeight() != newElement->layout->getHeight()) {
        OverlayRenderer::animation.startRect.copy(OverlayRenderer::animation.rect);
        OverlayRenderer::animation.startTime = util::ClientProxy::millis();
    }
    rootElement = newElement;
    themes.setThemeOverride(nullptr);
}

}
