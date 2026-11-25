// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <stdexcept>

#include "api/ui/Rect2f.h"
#include "gui/JadeLinearLayout.h"
#include "platform/layout/Layout.h"

namespace jade::api::ui {

enum class Orientation { HORIZONTAL, VERTICAL };

namespace orientation {

inline int getAxisPosition(Orientation o, layout::LayoutElement& e) { return o == Orientation::HORIZONTAL ? e.getX() : e.getY(); }
inline int getCrossAxisPosition(Orientation o, layout::LayoutElement& e) { return o == Orientation::HORIZONTAL ? e.getY() : e.getX(); }
inline int getAxisLength(Orientation o, layout::LayoutElement& e) { return o == Orientation::HORIZONTAL ? e.getWidth() : e.getHeight(); }
inline int getCrossAxisLength(Orientation o, layout::LayoutElement& e) { return o == Orientation::HORIZONTAL ? e.getHeight() : e.getWidth(); }
inline void setPosition(Orientation o, layout::LayoutElement& e, int axis, int crossAxis) {
    if (o == Orientation::HORIZONTAL) e.setPosition(axis, crossAxis);
    else e.setPosition(crossAxis, axis);
}
inline void setFreeSpace(Orientation o, layout::LayoutElement& e, int axis, int crossAxis) {
    auto* r = dynamic_cast<gui::ResizeableLayout*>(&e);
    if (!r) throw std::logic_error("Element is not a ResizeableLayout");
    if (o == Orientation::HORIZONTAL) r->setFreeSpace(axis, crossAxis);
    else r->setFreeSpace(crossAxis, axis);
}

inline float getAxisLength(Orientation o, Rect2f const& r) { return o == Orientation::HORIZONTAL ? r.getWidth() : r.getHeight(); }
inline float getCrossAxisLength(Orientation o, Rect2f const& r) { return o == Orientation::HORIZONTAL ? r.getHeight() : r.getWidth(); }
inline float getAxisPosition(Orientation o, Rect2f const& r) { return o == Orientation::HORIZONTAL ? r.getX() : r.getY(); }
inline float getCrossAxisPosition(Orientation o, Rect2f const& r) { return o == Orientation::HORIZONTAL ? r.getY() : r.getX(); }
inline void  setPosition(Orientation o, Rect2f& r, float axis, float crossAxis) {
    if (o == Orientation::HORIZONTAL) r.setPosition(axis, crossAxis);
    else r.setPosition(crossAxis, axis);
}
inline void setSize(Orientation o, Rect2f& r, float axis, float crossAxis) {
    if (o == Orientation::HORIZONTAL) {
        r.setWidth(axis);
        r.setHeight(crossAxis);
    } else {
        r.setWidth(crossAxis);
        r.setHeight(axis);
    }
}

}

}
