// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/ui/ProgressElement.h"

#include <algorithm>
#include <cmath>

#include "api/theme/IThemeHelper.h"
#include "impl/ui/Elements.h"
#include "impl/ui/JadeUIInternal.h"
#include "overlay/DisplayHelper.h"
#include "overlay/OverlayRenderer.h"
#include "overlay/WailaTickHandler.h"
#include "track/ProgressTrackInfo.h"
#include "util/JadeFont.h"

namespace jade::api::view {

int ProgressView::Part::themeColor() const {
    if (color != -1) return color;
    if (messageType) return static_cast<int>(theme::IThemeHelper::get().theme().progressColors.get(*messageType));
    return -1;
}

ProgressView ProgressView::read(Data const& data) {
    ProgressView view(impl::ui::JadeUIInternal::progressStyle(), ui::BoxStyle::nestedBox());
    view.parts = {PartBuilder().progress(data.progress).target(data.speed, data.target).messageType(data.messageType).build()};
    return view;
}

}

namespace jade::impl::ui {

using overlay::DisplayHelper;

namespace {
SpriteElement& defaultOverlay() {
    static SpriteElement e(JadeIds::JADE("progressbar"), 16, 16);
    return e;
}

void resizeElement(api::ui::Element& element, int x, int y, int w, int h) {
    element.setX(x);
    element.setY(y);
    if (auto* r = dynamic_cast<gui::ResizeableLayout*>(&element)) r->setFreeSpace(w, h);
}
}

ProgressElement::ProgressElement(api::view::ProgressView view) : mView(std::move(view)) {
    width  = 100;
    height = 8;
    if (mView.text) {
        width  = std::max(width, util::JadeFont::width(*mView.text) + 10);
        height = 14;
    }
    if (api::ui::ProgressStyle::isHorizontal(mView.style->direction())) {
        if (mView.style->fitContentX()) flexGrow(1);
        if (mView.style->fitContentY()) alignSelfStretch();
    } else {
        if (mView.style->fitContentY()) flexGrow(1);
        if (mView.style->fitContentX()) alignSelfStretch();
    }
}

ProgressElement::ProgressElement(api::view::ProgressView view, int w, int h) : mView(std::move(view)) {
    width  = w;
    height = h;
}

void ProgressElement::render(api::ui::GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) {
    mView.boxStyle.render(graphics, *this, float(getX()), float(getY()), float(width), float(height), DisplayHelper::INSTANCE.opacity());
    if (mTrack) {
        mTrack->setProgress(mView.parts);
        mTrack->update(overlay::OverlayRenderer::realtimeDeltaTicks);
    }
    int   borderWidth = mView.boxStyle.borderWidth();
    int   freeX       = getX() + borderWidth;
    int   freeY       = getY() + borderWidth;
    int   freeWidth   = width - borderWidth * 2;
    int   freeHeight  = height - borderWidth * 2;
    float progress    = 0;
    float start       = 0;
    for (size_t i = 0; i < mView.parts.size(); i++) {
        auto const& part         = mView.parts[i];
        float       partProgress = part.progress;
        if (partProgress <= 0) continue;
        if (mTrack) partProgress = mTrack->getSmoothProgress(part);
        progress = std::min(progress + partProgress, 1.0f);
        start = renderPart(graphics, mouseX, mouseY, partialTicks, part, partProgress, start, freeX, freeY, freeWidth, freeHeight, i == mView.parts.size() - 1);
        if (progress == 1.0f) break;
    }
    if (progress > 0 && mView.style->foreground()) {
        DisplayHelper::INSTANCE.blitSprite(graphics, *mView.style->foreground(), float(freeX), float(freeY), float(int(freeWidth - start)), float(freeHeight));
    }
    if (mView.text) {
        DisplayHelper::INSTANCE.drawText(graphics, *mView.text, float(getX() + 4), float(getY() + 3), api::theme::IThemeHelper::get().getNormalColor());
    }
}

float ProgressElement::renderPart(
    api::ui::GuiGraphics&                graphics,
    int                                  mouseX,
    int                                  mouseY,
    float                                partialTicks,
    api::view::ProgressView::Part const& part,
    float                                partProgress,
    float                                start,
    int                                  x,
    int                                  y,
    int                                  w,
    int                                  h,
    bool                                 isLast
) {
    float               partWidth        = std::min(partProgress * float(w), float(w) - start);
    int                 roundedPartWidth = static_cast<int>(std::ceil(partWidth));
    api::ui::Element*   overlay          = part.overlay.get();
    if (!overlay) {
        auto& d = defaultOverlay();
        d.setColor(part.themeColor());
        d.tiledOrientation = mView.style->direction() == api::ui::ScreenDirection::RIGHT ? api::ui::Orientation::HORIZONTAL : api::ui::Orientation::VERTICAL;
        overlay            = &d;
    }
    auto* element = dynamic_cast<ProgressOverlayElement*>(overlay);
    if (isLast && !mView.style->foreground() && element && element->canUseFloatingRect()) {
        element->floatingRect = api::ui::Rect2f(float(x) + start, float(y), partWidth, float(h));
        resizeElement(*element, x + int(start), y, roundedPartWidth, h);
        element->render(graphics, mouseX, mouseY, partialTicks);
        element->floatingRect.reset();
        return start + partWidth;
    }
    resizeElement(*overlay, x + int(start), y, roundedPartWidth, h);
    overlay->render(graphics, mouseX, mouseY, partialTicks);
    return start + float(roundedPartWidth);
}

void ProgressElement::updateSize() {
    if (getTag()) {
        auto parts    = mView.parts;
        bool canDec   = mView.style->canDecrease();
        int  w        = width;
        mTrack        = overlay::WailaTickHandler::instance().progressTracker.getOrCreate<track::ProgressTrackInfo>(*getTag(), [&] {
            return std::make_unique<track::ProgressTrackInfo>(parts, canDec, w);
        });
        mTrack->setExpectedWidth(width);
        width = mTrack->getWidth();
    } else {
        mTrack = nullptr;
    }
}

}
