// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "api/ui/Element.h"
#include "api/ui/Orientation.h"
#include "api/ui/Rect2f.h"
#include "platform/text/Component.h"

class ItemStack;

namespace jade::impl::ui {

using namespace jade::api::ui;

std::shared_ptr<ItemStack> makeSharedItem(ItemStack const& stack);

class TextElementImpl : public TextElement {
public:
    explicit TextElementImpl(text::Component text);

    TextElement& scale(float scale) override;
    TextElement& alpha(float alpha) override;
    void         render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    std::string  getNarration() override { return mText.getString(); }
    std::string  getString() override { return mText.getString(); }
    void         setFreeSpace(int w, int h) override {
        width  = w;
        height = h;
    }
    [[nodiscard]] text::Component const& getText() const { return mText; }

protected:
    text::Component mText;
    float           mScale = 1;
    float           mAlpha = 1;
    int             mTextWidth;
};

class ItemStackElement : public Element {
public:
    ItemStackElement(std::shared_ptr<ItemStack> item, float scale, std::optional<std::string> text);
    static std::shared_ptr<ItemStackElement> of(ItemStack const& stack, float scale = 1, std::optional<std::string> text = std::nullopt);

    void        render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    std::string getNarration() override;
    [[nodiscard]] ItemStack const& getItem() const { return *mItem; }
    [[nodiscard]] std::optional<std::string> const& getText() const { return mText; }

private:
    std::shared_ptr<ItemStack> mItem;
    float                      mScale;
    std::optional<std::string> mText;
};

class ProgressOverlayElement : public ResizeableElement {
public:
    std::optional<Rect2f> floatingRect;
    virtual bool          canUseFloatingRect() { return true; }
    void                  setFreeSpace(int w, int h) override {
        width  = w;
        height = h;
    }
};

class SpriteElement : public ProgressOverlayElement {
public:
    SpriteElement(Identifier sprite, int width, int height);

    void        render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    std::string getNarration() override { return ""; }
    bool        canUseFloatingRect() override;
    void        setColor(int color) { mColor = static_cast<uint32_t>(color); }

    std::optional<Orientation> tiledOrientation;

private:
    Identifier const& mappedSprite();

    Identifier mSprite;
    int        mOWidth;
    int        mOHeight;
    uint32_t   mColor      = 0xFFFFFFFF;
    int        mGeneration = -1;
    Identifier mMappedSprite;
};

class SpacerElement : public ResizeableElement {
public:
    SpacerElement(int w, int h) {
        width  = w;
        height = h;
    }
    SpacerElement& wrapped(layout::LayoutElementPtr wrapped);
    SpacerElement& offsetWrapped(int x, int y);

    std::string getNarration() override { return ""; }
    void        render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    void        renderDebug(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks, RenderDebugContext const& context) override;
    void        setFreeSpace(int w, int h) override {
        width  = w;
        height = h;
    }
    void setX(int x) override;
    void setY(int y) override;
    bool isMouseOver(double mouseX, double mouseY) override;
    [[nodiscard]] layout::LayoutElementPtr const& getWrapped() const { return mWrapped; }

private:
    layout::LayoutElementPtr mWrapped;
    int                      mWrappedOffsetX = 0;
    int                      mWrappedOffsetY = 0;
};

class CompoundElement : public Element {
public:
    CompoundElement(ElementPtr large, ElementPtr smallElement);
    std::string getNarration() override { return mLarge->getNarration(); }
    void        render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    void        setX(int x) override;
    void        setY(int y) override;

protected:
    ElementPtr mLarge;
    ElementPtr mSmall;
};

class HorizontalLineElement : public ResizeableElement {
public:
    HorizontalLineElement();
    std::string getNarration() override { return ""; }
    void        render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    void        setFreeSpace(int w, int) override { width = w; }

    uint32_t color;
};

}
