// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "gui/JadeLinearLayout.h"

#include <algorithm>
#include <stdexcept>

#include "api/ui/Element.h"
#include "api/ui/Orientation.h"

namespace jade::gui {

using api::ui::Orientation;
namespace ori = api::ui::orientation;

namespace {

int axisLength(Orientation o, JadeLinearLayout::ChildContainer const& c) { return o == Orientation::HORIZONTAL ? c.getWidth() : c.getHeight(); }
int crossAxisLength(Orientation o, JadeLinearLayout::ChildContainer const& c) { return o == Orientation::HORIZONTAL ? c.getHeight() : c.getWidth(); }
void setPosition(Orientation o, JadeLinearLayout::ChildContainer const& c, int axis, int crossAxis) {
    if (o == Orientation::HORIZONTAL) {
        c.setX(axis, c.getWidth());
        c.setY(crossAxis, c.getHeight());
    } else {
        c.setX(crossAxis, c.getWidth());
        c.setY(axis, c.getHeight());
    }
}
void childSetFreeSpace(Orientation o, JadeLinearLayout::ChildContainer const& c, int axis, int crossAxis) {
    auto* r = dynamic_cast<ResizeableLayout*>(c.child.get());
    if (!r) throw std::logic_error("Child is not a ResizeableLayout");
    if (o == Orientation::HORIZONTAL) r->setFreeSpace(axis, crossAxis);
    else r->setFreeSpace(crossAxis, axis);
}

void align(LinearAlign a, Orientation o, JadeLinearLayout::ChildContainer const& child, int axisPos, int crossAxisPos, int crossAxisFreeSpace) {
    int axisLen      = axisLength(o, child);
    int crossAxisLen = crossAxisLength(o, child);
    switch (a) {
    case LinearAlign::START:
        break;
    case LinearAlign::CENTER:
        crossAxisPos += (crossAxisFreeSpace - crossAxisLen) / 2;
        break;
    case LinearAlign::END:
        crossAxisPos += crossAxisFreeSpace - crossAxisLen;
        break;
    case LinearAlign::STRETCH:
        childSetFreeSpace(o, child, axisLen, crossAxisFreeSpace);
        break;
    }
    setPosition(o, child, axisPos, crossAxisPos);
}

int calculateMargin(int m1, int m2) {
    if (m1 >= 0 && m2 >= 0) return std::max(m1, m2);
    if (m1 < 0 && m2 < 0) return std::min(m1, m2);
    return m1 + m2;
}

}

JadeLinearLayout::JadeLinearLayout(Orientation orientation) : AbstractLayout(0, 0, 0, 0), mOrientation(orientation) {}

std::shared_ptr<JadeLinearLayout> JadeLinearLayout::vertical() { return std::make_shared<JadeLinearLayout>(Orientation::VERTICAL); }
std::shared_ptr<JadeLinearLayout> JadeLinearLayout::horizontal() { return std::make_shared<JadeLinearLayout>(Orientation::HORIZONTAL); }

JadeLinearLayout& JadeLinearLayout::orientation(Orientation orientation) {
    mOrientation = orientation;
    mArranged    = false;
    return *this;
}

JadeLinearLayout& JadeLinearLayout::alignItems(LinearAlign align) {
    mAlignItems = align;
    mArranged   = false;
    return *this;
}

JadeLinearLayout& JadeLinearLayout::spacing(int i) {
    mDefaultHeadMargin = mDefaultTailMargin = i;
    mArranged                               = false;
    return *this;
}

JadeLinearLayout& JadeLinearLayout::setMinDimensions(int minWidth, int minHeight) { return setMinWidth(minWidth).setMinHeight(minHeight); }

JadeLinearLayout& JadeLinearLayout::setMinHeight(int minHeight) {
    mMinHeight = minHeight;
    mArranged  = false;
    return *this;
}

JadeLinearLayout& JadeLinearLayout::setMinWidth(int minWidth) {
    mMinWidth = minWidth;
    mArranged = false;
    return *this;
}

LayoutSettings JadeLinearLayout::newChildLayoutSettings(LayoutElementPtr const& element) const {
    LayoutSettings settings = newChildLayoutSettings();
    if (auto* e = dynamic_cast<api::ui::Element*>(element.get()); e && e->getSettings()) settings = e->getSettings()(settings);
    return settings;
}

LayoutElementPtr JadeLinearLayout::addChild(LayoutElementPtr element) {
    auto settings = newChildLayoutSettings(element);
    return addChild(std::move(element), settings);
}

LayoutElementPtr JadeLinearLayout::addChild(LayoutElementPtr element, LayoutSettings layoutSettings, std::function<void(ChildContainer&)> const& consumer) {
    auto container        = std::make_unique<ChildContainer>(element, layoutSettings);
    container->headMargin = mDefaultHeadMargin;
    container->tailMargin = mDefaultTailMargin;
    if (consumer) consumer(*container);
    mChildren.push_back(std::move(container));
    mArranged = false;
    return element;
}

void JadeLinearLayout::visitChildren(std::function<void(LayoutElementPtr const&)> const& consumer) {
    for (auto const& c : mChildren) consumer(c->child);
}

void JadeLinearLayout::arrangeElements() {
    if (mArranged) return;
    size_t const size = mChildren.size();
    if (size == 0) {
        width = height = 0;
        mArranged      = true;
        return;
    }
    Layout::arrangeElements();
    int              axis      = 0;
    int              crossAxis = 0;
    int              sumGrow   = 0;
    std::vector<int> margins;
    if (size == 1) {
        auto& child = *mChildren.front();
        axis        = axisLength(mOrientation, child);
        crossAxis   = crossAxisLength(mOrientation, child);
        sumGrow     = child.flexGrow;
    } else {
        margins.resize(size - 1);
        ChildContainer* last = nullptr;
        for (size_t i = 0; i < size; ++i) {
            auto& child = *mChildren[i];
            if (i != 0) {
                int margin     = calculateMargin(last->tailMargin, child.headMargin);
                margins[i - 1] = margin;
                axis += margin;
            }
            axis += axisLength(mOrientation, child);
            crossAxis = std::max(crossAxis, crossAxisLength(mOrientation, child));
            sumGrow += child.flexGrow;
            last = &child;
        }
        mArranged = true;
    }

    int minAxis        = mOrientation == Orientation::HORIZONTAL ? mMinWidth : mMinHeight;
    int extraAxisSpace = std::max(0, minAxis - axis);
    axis               = std::max(axis, minAxis);
    int minCrossAxis   = mOrientation == Orientation::HORIZONTAL ? mMinHeight : mMinWidth;
    crossAxis          = std::max(crossAxis, minCrossAxis);

    resolveFlexGrow(extraAxisSpace, crossAxis, sumGrow);

    int axisPos      = ori::getAxisPosition(mOrientation, *this);
    int crossAxisPos = ori::getCrossAxisPosition(mOrientation, *this);
    for (size_t i = 0; i < size; ++i) {
        auto& child          = *mChildren[i];
        int   childAxisLength = axisLength(mOrientation, child);
        if (i != 0) axisPos += margins[i - 1];
        align(child.alignSelf.value_or(mAlignItems), mOrientation, child, axisPos, crossAxisPos, crossAxis);
        axisPos += childAxisLength;
    }

    width  = mOrientation == Orientation::HORIZONTAL ? axis : crossAxis;
    height = mOrientation == Orientation::HORIZONTAL ? crossAxis : axis;
}

void JadeLinearLayout::removeChildren() {
    mChildren.clear();
    mArranged = false;
}

void JadeLinearLayout::resolveFlexGrow(int extraAxisSpace, int crossAxis, int sumGrow) {
    if (sumGrow == 0 || extraAxisSpace <= 0) return;
    std::vector<ChildContainer*> children;
    for (auto const& c : mChildren)
        if (c->flexGrow > 0) children.push_back(c.get());
    size_t const size = children.size();
    if (size == 1) {
        auto& child = *children.front();
        childSetFreeSpace(mOrientation, child, axisLength(mOrientation, child) + extraAxisSpace, crossAxis);
        return;
    }
    size_t            reachLimitAmount = 0;
    std::vector<bool> reachLimitFlags(size, false);
    bool              changed = true;
    while (changed && reachLimitAmount < size && extraAxisSpace > 0) {
        changed            = false;
        int virtualSumGrow = sumGrow;
        bool stop          = false;
        for (size_t i = 0; i < size; ++i) {
            auto& child = *children[i];
            if (reachLimitFlags[i]) continue;
            int childAxisLength = axisLength(mOrientation, child);
            int grow            = child.flexGrow;
            int extraChildAxis  = extraAxisSpace * grow / virtualSumGrow;
            if (extraChildAxis <= 0) continue;
            int newAxisLength = childAxisLength + extraChildAxis;
            childSetFreeSpace(mOrientation, child, newAxisLength, crossAxis);
            int childAxisLengthNow = axisLength(mOrientation, child);
            changed |= childAxisLength != childAxisLengthNow;
            if (newAxisLength > childAxisLengthNow) {
                reachLimitFlags[i] = true;
                ++reachLimitAmount;
                sumGrow -= grow;
                if (sumGrow <= 0) {
                    stop = true;
                    break;
                }
            }
            extraAxisSpace -= extraChildAxis;
            virtualSumGrow -= grow;
            if (virtualSumGrow <= 0) break;
        }
        if (stop) break;
    }
}

void JadeLinearLayout::setFreeSpace(int w, int h) {
    if (width >= w && height >= h) return;
    int oldMinWidth  = mMinWidth;
    int oldMinHeight = mMinHeight;
    mMinWidth        = std::max(mMinWidth, w);
    mMinHeight       = std::max(mMinHeight, h);
    mArranged        = false;
    arrangeElements();
    mMinWidth  = oldMinWidth;
    mMinHeight = oldMinHeight;
}

void JadeLinearLayout::setFlexGrow(int flexGrow) {
    mFlexGrow = std::max(0, flexGrow);
    mArranged = false;
}

int JadeLinearLayout::getHeight() {
    if (!mArranged) arrangeElements();
    return height;
}

int JadeLinearLayout::getWidth() {
    if (!mArranged) arrangeElements();
    return width;
}

LayoutWithPadding::LayoutWithPadding(LayoutElementPtr wrapped)
: mWrapped(std::move(wrapped)),
  mResizeable(dynamic_cast<ResizeableLayout*>(mWrapped.get()) != nullptr) {}

LayoutWithPadding::LayoutWithPadding(LayoutElementPtr wrapped, int l, int t, int r, int b) : LayoutWithPadding(std::move(wrapped)) {
    paddingLeft   = l;
    paddingTop    = t;
    paddingRight  = r;
    paddingBottom = b;
}

void LayoutWithPadding::visitChildren(std::function<void(LayoutElementPtr const&)> const& consumer) {
    if (auto* layout = dynamic_cast<layout::Layout*>(mWrapped.get())) layout->visitChildren(consumer);
    else consumer(mWrapped);
}

void LayoutWithPadding::arrangeElements() {
    if (auto* layout = dynamic_cast<layout::Layout*>(mWrapped.get())) {
        layout->arrangeElements();
        setPosition(layout->getX(), layout->getY());
    }
}

void LayoutWithPadding::removeChildren() {
    if (auto* layout = dynamic_cast<layout::Layout*>(mWrapped.get())) layout->removeChildren();
}

int  LayoutWithPadding::getWidth() { return mWrapped->getWidth() + paddingLeft + paddingRight; }
int  LayoutWithPadding::getHeight() { return mWrapped->getHeight() + paddingTop + paddingBottom; }
void LayoutWithPadding::setX(int x) { mWrapped->setX(x + paddingLeft); }
void LayoutWithPadding::setY(int y) { mWrapped->setY(y + paddingTop); }
void LayoutWithPadding::setPosition(int x, int y) { mWrapped->setPosition(x + paddingLeft, y + paddingTop); }
int  LayoutWithPadding::getX() { return mWrapped->getX() - paddingLeft; }
int  LayoutWithPadding::getY() { return mWrapped->getY() - paddingTop; }

void LayoutWithPadding::setFreeSpace(int w, int h) {
    if (!mResizeable) return;
    dynamic_cast<ResizeableLayout*>(mWrapped.get())->setFreeSpace(w - paddingLeft - paddingRight, h - paddingTop - paddingBottom);
    setPosition(mWrapped->getX(), mWrapped->getY());
}

void LayoutWithPadding::setFlexGrow(int flexGrow) {
    if (mResizeable) dynamic_cast<ResizeableLayout*>(mWrapped.get())->setFlexGrow(flexGrow);
}

int LayoutWithPadding::getFlexGrow() { return mResizeable ? dynamic_cast<ResizeableLayout*>(mWrapped.get())->getFlexGrow() : 0; }

}
