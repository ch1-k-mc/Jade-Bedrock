// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/ui/FluidStackElement.h"

#include "overlay/DisplayHelper.h"

namespace jade::impl::ui {

void FluidStackElement::render(GuiGraphics& graphics, int, int, float) {
    if (mFluid.isEmpty()) return;
    auto const vol = api::view::JadeFluidObject::bucketVolume();
    if (!floatingRect) {
        overlay::DisplayHelper::INSTANCE.drawFluid(graphics, float(getX()), float(getY()), mFluid, float(width), float(height), vol);
    } else {
        overlay::DisplayHelper::INSTANCE.drawFluid(graphics, floatingRect->getX(), floatingRect->getY(), mFluid, floatingRect->getWidth(), floatingRect->getHeight(), vol);
    }
}

}
