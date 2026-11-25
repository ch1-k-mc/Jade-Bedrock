// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>

#include "api/ui/Element.h"
#include "api/view/ProgressView.h"

namespace jade::track {
class ProgressTrackInfo;
}

namespace jade::impl::ui {

class ProgressElement : public api::ui::ResizeableElement, public api::ui::StyledElement {
public:
    explicit ProgressElement(api::view::ProgressView view);
    ProgressElement(api::view::ProgressView view, int width, int height);

    void               render(api::ui::GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    std::string        getNarration() override { return mView.text ? mView.text->getString() : ""; }
    api::ui::ElementPtr getIcon() override { return nullptr; }
    api::ui::BoxStyle&  getStyle() override { return mView.boxStyle; }
    void               setFreeSpace(int w, int h) override {
        width  = w;
        height = h;
    }
    void updateSize() override;

    [[nodiscard]] api::view::ProgressView const& getView() const { return mView; }

private:
    float renderPart(
        api::ui::GuiGraphics&                  graphics,
        int                                    mouseX,
        int                                    mouseY,
        float                                  partialTicks,
        api::view::ProgressView::Part const&   part,
        float                                  partProgress,
        float                                  start,
        int                                    x,
        int                                    y,
        int                                    w,
        int                                    h,
        bool                                   isLast
    );

    api::view::ProgressView    mView;
    std::shared_ptr<track::ProgressTrackInfo> mTrack;
};

}
