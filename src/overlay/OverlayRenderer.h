// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>

#include "api/ui/TooltipAnimation.h"

namespace jade::platform::render {
class GuiGraphics;
}
namespace jade::impl::ui {
class BoxElementImpl;
}

namespace jade::overlay {

class OverlayRenderer {
public:
    static api::ui::TooltipAnimation animation;
    static float                     ticks;
    static bool                      shown;
    static inline float              realtimeDeltaTicks = 0;

    static bool shouldShow();
    static bool shouldShowImmediately(impl::ui::BoxElementImpl& box);
    static void renderOverlay478757(platform::render::GuiGraphics& graphics, float delta);
    static void renderOverlay(std::shared_ptr<impl::ui::BoxElementImpl> const& root, platform::render::GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks);
    static void clearLingerTooltip();

private:
    static std::shared_ptr<impl::ui::BoxElementImpl> sLingerTooltip;
    static float                                     sDisappearTicks;
};

}
