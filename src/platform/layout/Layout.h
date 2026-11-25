// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <functional>
#include <memory>

#include "platform/render/Rect.h"

namespace jade::layout {

class LayoutElement : public std::enable_shared_from_this<LayoutElement> {
public:
    virtual ~LayoutElement() = default;

    virtual void setX(int x)              = 0;
    virtual void setY(int y)              = 0;
    virtual int  getX()                   = 0;
    virtual int  getY()                   = 0;
    virtual int  getWidth()               = 0;
    virtual int  getHeight()              = 0;
    virtual void setPosition(int x, int y) {
        setX(x);
        setY(y);
    }
    [[nodiscard]] platform::render::Rect getRectangle() { return {getX(), getY(), getWidth(), getHeight()}; }
};

using LayoutElementPtr = std::shared_ptr<LayoutElement>;

class Layout : public LayoutElement {
public:
    virtual void visitChildren(std::function<void(LayoutElementPtr const&)> const& consumer) = 0;
    virtual void arrangeElements();
    virtual void removeChildren() {}
};

struct LayoutSettings {
    int   paddingLeft   = 0;
    int   paddingTop    = 0;
    int   paddingRight  = 0;
    int   paddingBottom = 0;
    float xAlignment    = 0;
    float yAlignment    = 0;

    LayoutSettings& padding(int all) { return padding(all, all, all, all); }
    LayoutSettings& padding(int horizontal, int vertical) { return padding(horizontal, vertical, horizontal, vertical); }
    LayoutSettings& padding(int left, int top, int right, int bottom) {
        paddingLeft   = left;
        paddingTop    = top;
        paddingRight  = right;
        paddingBottom = bottom;
        return *this;
    }
    LayoutSettings& paddingHorizontal(int h) {
        paddingLeft  = h;
        paddingRight = h;
        return *this;
    }
    LayoutSettings& paddingVertical(int v) {
        paddingTop    = v;
        paddingBottom = v;
        return *this;
    }
    LayoutSettings& alignHorizontally(float a) {
        xAlignment = a;
        return *this;
    }
    LayoutSettings& alignVertically(float a) {
        yAlignment = a;
        return *this;
    }
    LayoutSettings& alignHorizontallyLeft() { return alignHorizontally(0); }
    LayoutSettings& alignHorizontallyCenter() { return alignHorizontally(0.5f); }
    LayoutSettings& alignHorizontallyRight() { return alignHorizontally(1); }
    LayoutSettings& alignVerticallyTop() { return alignVertically(0); }
    LayoutSettings& alignVerticallyMiddle() { return alignVertically(0.5f); }
    LayoutSettings& alignVerticallyBottom() { return alignVertically(1); }
    [[nodiscard]] LayoutSettings copy() const { return *this; }
};

using LayoutSettingsOperator = std::function<LayoutSettings(LayoutSettings)>;

class AbstractLayout : public Layout {
public:
    AbstractLayout(int x, int y, int width, int height) : mX(x), mY(y), width(width), height(height) {}

    void setX(int x) override;
    void setY(int y) override;
    int  getX() override { return mX; }
    int  getY() override { return mY; }
    int  getWidth() override { return width; }
    int  getHeight() override { return height; }

    class AbstractChildWrapper {
    public:
        AbstractChildWrapper(LayoutElementPtr child, LayoutSettings settings) : child(std::move(child)), layoutSettings(settings) {}
        virtual ~AbstractChildWrapper() = default;

        [[nodiscard]] int getWidth() const { return child->getWidth() + layoutSettings.paddingLeft + layoutSettings.paddingRight; }
        [[nodiscard]] int getHeight() const { return child->getHeight() + layoutSettings.paddingTop + layoutSettings.paddingBottom; }
        void setX(int x, int width) const;
        void setY(int y, int height) const;

        LayoutElementPtr child;
        LayoutSettings   layoutSettings;
    };

protected:
    int mX;
    int mY;
    int width;
    int height;
};

}
