// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "platform/layout/Layout.h"

namespace jade::layout {

void Layout::arrangeElements() {
    visitChildren([](LayoutElementPtr const& child) {
        if (auto* layout = dynamic_cast<Layout*>(child.get())) layout->arrangeElements();
    });
}

void AbstractLayout::setX(int x) {
    int dx = x - mX;
    visitChildren([dx](LayoutElementPtr const& child) { child->setX(child->getX() + dx); });
    mX = x;
}

void AbstractLayout::setY(int y) {
    int dy = y - mY;
    visitChildren([dy](LayoutElementPtr const& child) { child->setY(child->getY() + dy); });
    mY = y;
}

void AbstractLayout::AbstractChildWrapper::setX(int x, int w) const {
    float from = static_cast<float>(layoutSettings.paddingLeft);
    float to   = static_cast<float>(w - child->getWidth() - layoutSettings.paddingRight);
    int   off  = static_cast<int>(from + layoutSettings.xAlignment * (to - from));
    child->setX(off + x);
}

void AbstractLayout::AbstractChildWrapper::setY(int y, int h) const {
    float from = static_cast<float>(layoutSettings.paddingTop);
    float to   = static_cast<float>(h - child->getHeight() - layoutSettings.paddingBottom);
    int   off  = static_cast<int>(from + layoutSettings.yAlignment * (to - from));
    child->setY(off + y);
}

}
