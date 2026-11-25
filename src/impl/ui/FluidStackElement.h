// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include "api/view/FluidView.h"
#include "impl/ui/Elements.h"

namespace jade::impl::ui {

class FluidStackElement : public ProgressOverlayElement {
public:
    explicit FluidStackElement(api::view::JadeFluidObject fluid) : mFluid(std::move(fluid)) { width = height = 16; }

    void        render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    std::string getNarration() override { return ""; }
    [[nodiscard]] api::view::JadeFluidObject const& getFluid() const { return mFluid; }

private:
    api::view::JadeFluidObject mFluid;
};

}
