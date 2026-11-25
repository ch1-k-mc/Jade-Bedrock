// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "overlay/OverlayRenderer.h"

#include <algorithm>

#include "api/config/IWailaConfig.h"
#include "impl/WailaClientRegistration.h"
#include "impl/ui/BoxElementImpl.h"
#include "overlay/DisplayHelper.h"
#include "overlay/WailaTickHandler.h"
#include "platform/render/GuiGraphics.h"
#include "util/ClientProxy.h"
#include "util/JadeKeys.h"

namespace jade::overlay {

using api::config::IWailaConfig;

api::ui::TooltipAnimation                 OverlayRenderer::animation;
float                                     OverlayRenderer::ticks = 0;
bool                                      OverlayRenderer::shown = false;
std::shared_ptr<impl::ui::BoxElementImpl> OverlayRenderer::sLingerTooltip;
float                                     OverlayRenderer::sDisappearTicks = 0;

bool OverlayRenderer::shouldShow() {
    if (!WailaTickHandler::instance().rootElement) return false;
    auto& general = IWailaConfig::get().general();
    if (!general.shouldDisplayTooltip()) return false;
    if (general.getDisplayMode() == api::config::DisplayMode::HOLD_KEY && !util::JadeKeys::showOverlay().isDown()) return false;
    if (general.getBossBarOverlapMode() == api::config::BossBarOverlapMode::HIDE_TOOLTIP && util::ClientProxy::getBossBarRect()) return false;
    return true;
}

bool OverlayRenderer::shouldShowImmediately(impl::ui::BoxElementImpl& box) {
    if (box.getTooltip().isEmpty()) return false;
    if (util::ClientProxy::shouldHideWithGui()) return false;
    box.updateExpectedRect(animation);
    if (util::ClientProxy::isHudHidden()) return false;
    return true;
}

void OverlayRenderer::renderOverlay478757(platform::render::GuiGraphics& graphics, float delta) {
    ticks += delta;
    realtimeDeltaTicks = delta;
    shown     = false;
    auto root = WailaTickHandler::instance().rootElement;
    bool show = shouldShow();
    auto& overlay = IWailaConfig::get().overlay();
    auto& general = IWailaConfig::get().general();
    if (root) sLingerTooltip = root;
    if (!root && sLingerTooltip) {
        sDisappearTicks += delta;
        if (sDisappearTicks < overlay.getDisappearingDelay()) {
            root = sLingerTooltip;
            show = true;
        }
    } else {
        sDisappearTicks = 0;
    }
    if (overlay.getAnimation() && sLingerTooltip) {
        root        = sLingerTooltip;
        float speed = general.isDebug() ? 0.1f : 0.6f;
        animation.showHideAlpha += (show ? speed : -speed) * delta;
        animation.showHideAlpha = std::clamp(animation.showHideAlpha, 0.0f, 1.0f);
    } else {
        animation.showHideAlpha = show ? 1.0f : 0.0f;
    }
    if (!root) return;
    if (animation.showHideAlpha < 0.1f || !shouldShowImmediately(*root)) {
        sLingerTooltip = nullptr;
        animation.rect.setWidth(0);
        return;
    }
    renderOverlay(root, graphics, -1, -1, delta);
}

void OverlayRenderer::renderOverlay(
    std::shared_ptr<impl::ui::BoxElementImpl> const& root,
    platform::render::GuiGraphics&                   graphics,
    int                                              mouseX,
    int                                              mouseY,
    float                                            partialTicks
) {
    root->updateRect(animation);
    auto& tickHandler = WailaTickHandler::instance();
    auto& reg         = impl::WailaClientRegistration::instance();
    if (tickHandler.state) {
        bool cancel = false;
        reg.beforeRenderCallback.call([&](auto const& cb) {
            if (!cancel && cb(*root, animation, graphics, *tickHandler.state->accessor)) cancel = true;
        });
        if (cancel) return;
        root->beforeRender(tickHandler.state->accessor.get(), partialTicks);
    }

    bool renderDebug = IWailaConfig::get().general().isDebug() && util::JadeKeys::showDetails().isDown() && false;
    if (renderDebug) {
        if (auto boss = util::ClientProxy::getBossBarRect()) overlay::DisplayHelper::INSTANCE.drawBorder(graphics, *boss, 2, 0x88FF00FF, true);
    }

    graphics.pushPose();
    auto const& rect = animation.rect;
    graphics.translate(rect.getX(), rect.getY());
    if (animation.scale != 1.0f) graphics.scale(animation.scale);
    root->render(graphics, mouseX, mouseY, partialTicks);
    if (renderDebug) root->renderDebug(graphics, mouseX, mouseY, partialTicks, {root.get(), true});
    if (tickHandler.state) reg.afterRenderCallback.call([&](auto const& cb) { cb(*root, animation, graphics, *tickHandler.state->accessor); });
    graphics.popPose();
    graphics.flush();
    shown = true;
}

void OverlayRenderer::clearLingerTooltip() { sLingerTooltip = nullptr; }

}
