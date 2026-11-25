// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "platform/layout/Layout.h"

namespace jade::api::ui {
enum class Orientation;
}

namespace jade::gui {

using layout::LayoutElement;
using layout::LayoutElementPtr;
using layout::LayoutSettings;

class ResizeableLayout {
public:
    virtual ~ResizeableLayout()                     = default;
    virtual void setFreeSpace(int width, int height) = 0;
    virtual void setFlexGrow(int flexGrow)          = 0;
    virtual int  getFlexGrow()                      = 0;
};

enum class LinearAlign { START, CENTER, END, STRETCH };

class JadeLinearLayout : public layout::AbstractLayout, public ResizeableLayout {
public:
    class ChildContainer : public AbstractChildWrapper {
    public:
        ChildContainer(LayoutElementPtr element, LayoutSettings settings) : AbstractChildWrapper(std::move(element), settings) {}
        int                        headMargin = 0;
        int                        tailMargin = 0;
        int                        flexGrow   = 0;
        std::optional<LinearAlign> alignSelf;
    };

    explicit JadeLinearLayout(api::ui::Orientation orientation);

    static std::shared_ptr<JadeLinearLayout> vertical();
    static std::shared_ptr<JadeLinearLayout> horizontal();

    JadeLinearLayout& orientation(api::ui::Orientation orientation);
    JadeLinearLayout& alignItems(LinearAlign align);
    JadeLinearLayout& spacing(int i);
    JadeLinearLayout& setMinDimensions(int minWidth, int minHeight);
    JadeLinearLayout& setMinHeight(int minHeight);
    JadeLinearLayout& setMinWidth(int minWidth);

    [[nodiscard]] LayoutSettings newChildLayoutSettings() const { return mDefaultChildLayoutSettings.copy(); }
    [[nodiscard]] LayoutSettings newChildLayoutSettings(LayoutElementPtr const& element) const;
    LayoutSettings&              defaultChildLayoutSetting() { return mDefaultChildLayoutSettings; }

    LayoutElementPtr addChild(LayoutElementPtr element);
    LayoutElementPtr addChild(LayoutElementPtr element, LayoutSettings layoutSettings, std::function<void(ChildContainer&)> const& consumer = {});

    void visitChildren(std::function<void(LayoutElementPtr const&)> const& consumer) override;
    void arrangeElements() override;
    void removeChildren() override;

    void setFreeSpace(int width, int height) override;
    void setFlexGrow(int flexGrow) override;
    int  getFlexGrow() override { return mFlexGrow; }

    int getHeight() override;
    int getWidth() override;

private:
    void resolveFlexGrow(int extraAxisSpace, int crossAxis, int sumGrow);

    api::ui::Orientation                         mOrientation;
    LinearAlign                                  mAlignItems = LinearAlign::START;
    std::vector<std::unique_ptr<ChildContainer>> mChildren;
    LayoutSettings                               mDefaultChildLayoutSettings;
    int                                          mDefaultHeadMargin = 0;
    int                                          mDefaultTailMargin = 0;
    int                                          mMinWidth          = 0;
    int                                          mMinHeight         = 0;
    int                                          mFlexGrow          = 0;
    bool                                         mArranged          = false;
};

class LayoutWithPadding : public layout::Layout, public ResizeableLayout {
public:
    explicit LayoutWithPadding(LayoutElementPtr wrapped);
    LayoutWithPadding(LayoutElementPtr wrapped, int paddingLeft, int paddingTop, int paddingRight, int paddingBottom);

    void visitChildren(std::function<void(LayoutElementPtr const&)> const& consumer) override;
    void arrangeElements() override;
    void removeChildren() override;

    int  getWidth() override;
    int  getHeight() override;
    void setX(int x) override;
    void setY(int y) override;
    void setPosition(int x, int y) override;
    int  getX() override;
    int  getY() override;

    void setFreeSpace(int width, int height) override;
    void setFlexGrow(int flexGrow) override;
    int  getFlexGrow() override;

    [[nodiscard]] LayoutElementPtr const& wrapped() const { return mWrapped; }

    int paddingLeft   = 0;
    int paddingTop    = 0;
    int paddingRight  = 0;
    int paddingBottom = 0;

private:
    LayoutElementPtr mWrapped;
    bool             mResizeable;
};

}
