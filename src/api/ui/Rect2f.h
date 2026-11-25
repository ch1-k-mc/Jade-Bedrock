// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <algorithm>

#include "platform/layout/Layout.h"

namespace jade::api::ui {

class Rect2f {
public:
    Rect2f() = default;
    Rect2f(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}

    static Rect2f of(layout::LayoutElement& element) {
        return {float(element.getX()), float(element.getY()), float(element.getWidth()), float(element.getHeight())};
    }

    [[nodiscard]] bool intersects(Rect2f const& o) const {
        return x < o.x + o.width && x + width > o.x && y < o.y + o.height && y + height > o.y;
    }
    [[nodiscard]] Rect2f intersect(Rect2f const& o) const {
        float x0 = std::max(x, o.x), y0 = std::max(y, o.y);
        float x1 = std::min(x + width, o.x + o.width), y1 = std::min(y + height, o.y + o.height);
        return {x0, y0, std::max(0.f, x1 - x0), std::max(0.f, y1 - y0)};
    }

    [[nodiscard]] float getX() const { return x; }
    [[nodiscard]] float getY() const { return y; }
    void                setX(float v) { x = v; }
    void                setY(float v) { y = v; }
    [[nodiscard]] float getWidth() const { return width; }
    [[nodiscard]] float getHeight() const { return height; }
    void                setWidth(float v) { width = v; }
    void                setHeight(float v) { height = v; }
    void                setPosition(float px, float py) {
        x = px;
        y = py;
    }
    [[nodiscard]] bool  contains(float px, float py) const { return px >= x && px < x + width && py >= y && py < y + height; }
    [[nodiscard]] Rect2f copy() const { return *this; }
    void                copy(Rect2f const& src) { *this = src; }
    [[nodiscard]] float getRight() const { return x + width; }
    [[nodiscard]] float getBottom() const { return y + height; }
    bool                operator==(Rect2f const&) const = default;

private:
    float x = 0, y = 0, width = 0, height = 0;
};

}
