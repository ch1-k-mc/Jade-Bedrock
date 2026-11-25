// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "api/JadeIds.h"
#include "gui/JadeLinearLayout.h"
#include "platform/layout/Layout.h"

namespace jade::platform::render {
class GuiGraphics;
}

namespace jade::api {
class ITooltip;
class Accessor;
}

namespace jade::api::ui {

using platform::render::GuiGraphics;

enum class ScreenDirection { UP, DOWN, LEFT, RIGHT };
enum class MessageType { NORMAL, INFO, TITLE, SUCCESS, WARNING, DANGER, FAILURE };

class ResizeableElement;
class Rect2f;

class Element : public layout::LayoutElement {
public:
    struct RenderDebugContext {
        layout::LayoutElement* root;
        bool                   renderChildren;
    };

    std::shared_ptr<ResizeableElement> offset(int x, int y);
    std::shared_ptr<ResizeableElement> size(int width, int height);

    Element&                                  settings(layout::LayoutSettingsOperator settings);
    [[nodiscard]] layout::LayoutSettingsOperator const& getSettings() const { return mSettings; }

    Element&                                 alignSelfStart();
    Element&                                 alignSelfCenter();
    Element&                                 alignSelfEnd();
    Element&                                 alignSelfStretch();
    [[nodiscard]] std::optional<gui::LinearAlign> getAlignSelf() const { return mAlignSelf; }

    Element&                                tag(std::optional<Identifier> tag);
    [[nodiscard]] std::optional<Identifier> const& getTag() const { return mTag; }

    virtual std::string getNarration() = 0;
    std::string         cachedNarration();
    Element&            narration(std::string narration);
    Element&            refreshNarration();

    virtual void render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) = 0;
    virtual void renderDebug(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks, RenderDebugContext const& context);

    void setX(int x) override { mX = x; }
    void setY(int y) override { mY = y; }
    int  getX() final { return mX; }
    int  getY() final { return mY; }
    int  getWidth() final { return width; }
    int  getHeight() final { return height; }

    virtual bool isMouseOver(double mouseX, double mouseY) {
        return mouseX >= getX() && mouseX < getX() + getWidth() && mouseY >= getY() && mouseY < getY() + getHeight();
    }

protected:
    std::optional<Identifier> mTag;
    int                       width  = 0;
    int                       height = 0;

private:
    int                            mX = 0;
    int                            mY = 0;
    std::optional<std::string>     mNarration;
    layout::LayoutSettingsOperator mSettings;
    std::optional<gui::LinearAlign> mAlignSelf;
};

using ElementPtr = std::shared_ptr<Element>;

class ResizeableElement : public Element, public gui::ResizeableLayout {
public:
    ResizeableElement& flexGrow(int flexGrow) {
        setFlexGrow(flexGrow);
        return *this;
    }
    void         setFlexGrow(int flexGrow) override { mFlexGrow = flexGrow < 0 ? 0 : flexGrow; }
    int          getFlexGrow() override { return mFlexGrow; }
    void         setFreeSpace(int, int) override {}
    virtual void updateSize() {}

private:
    int mFlexGrow = 0;
};

class TextElement : public ResizeableElement {
public:
    virtual TextElement&      scale(float scale) = 0;
    virtual TextElement&      alpha(float alpha) = 0;
    [[nodiscard]] virtual std::string getString() = 0;
};

class BoxStyle;

class StyledElement {
public:
    virtual ~StyledElement()               = default;
    virtual ElementPtr getIcon()           = 0;
    virtual BoxStyle&  getStyle()          = 0;
};

class BoxElement;

struct BoxProgress {
    float                   progress = 0;
    MessageType             type     = MessageType::NORMAL;
    std::optional<uint32_t> color;
    float                   alpha = 1;
};

class IBoxProgressProvider {
public:
    virtual ~IBoxProgressProvider()                                                                       = default;
    virtual std::optional<BoxProgress> getProgress(BoxElement& box, Accessor& accessor, float partialTicks) = 0;
};

class BoxElement : public ResizeableElement, public StyledElement {
public:
    virtual void addProgressProvider(int priority, std::shared_ptr<IBoxProgressProvider> provider) = 0;
    virtual ITooltip& getTooltip()                                = 0;
    virtual void      setBoxProgress(MessageType type, float progress) = 0;
    virtual void      clearBoxProgress()                          = 0;
    virtual void      setIcon(ElementPtr icon)                    = 0;
};

template <class T>
std::shared_ptr<T> tagged(std::shared_ptr<T> e, std::optional<Identifier> tag) {
    e->tag(std::move(tag));
    return e;
}

}
