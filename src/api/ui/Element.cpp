// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "api/ui/Element.h"

#include "impl/ui/JadeUIInternal.h"
#include "overlay/DisplayHelper.h"
#include "platform/render/GuiGraphics.h"

namespace jade::api::ui {

std::shared_ptr<ResizeableElement> Element::offset(int x, int y) {
    return impl::ui::JadeUIInternal::offset(std::static_pointer_cast<Element>(shared_from_this()), x, y);
}

std::shared_ptr<ResizeableElement> Element::size(int w, int h) {
    return impl::ui::JadeUIInternal::size(std::static_pointer_cast<Element>(shared_from_this()), w, h);
}

Element& Element::settings(layout::LayoutSettingsOperator settings) {
    mSettings = std::move(settings);
    return *this;
}

Element& Element::alignSelfStart() {
    mAlignSelf = gui::LinearAlign::START;
    return *this;
}
Element& Element::alignSelfCenter() {
    mAlignSelf = gui::LinearAlign::CENTER;
    return *this;
}
Element& Element::alignSelfEnd() {
    mAlignSelf = gui::LinearAlign::END;
    return *this;
}
Element& Element::alignSelfStretch() {
    mAlignSelf = gui::LinearAlign::STRETCH;
    return *this;
}

Element& Element::tag(std::optional<Identifier> tag) {
    mTag = std::move(tag);
    return *this;
}

std::string Element::cachedNarration() {
    if (!mNarration) mNarration = getNarration();
    return *mNarration;
}

Element& Element::narration(std::string narration) {
    mNarration = std::move(narration);
    return *this;
}

Element& Element::refreshNarration() {
    mNarration.reset();
    return *this;
}

void Element::renderDebug(GuiGraphics& graphics, int, int, float, RenderDebugContext const&) {
    overlay::DisplayHelper::INSTANCE.drawBorder(graphics, Rect2f::of(*this), 1, 0x88FF0000, true);
}

}
