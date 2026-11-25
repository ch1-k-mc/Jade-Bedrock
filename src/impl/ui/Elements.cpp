// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/ui/Elements.h"

#include <cmath>

#include "mc/world/item/ItemStack.h"

#include "api/config/IWailaConfig.h"
#include "api/theme/IThemeHelper.h"
#include "api/ui/Color.h"
#include "overlay/DisplayHelper.h"
#include "platform/item/ScopedItemRegistry.h"
#include "platform/render/GuiGraphics.h"
#include "platform/render/SpriteManager.h"
#include "util/JadeFont.h"

namespace jade::impl::ui {

using overlay::DisplayHelper;
using util::JadeFont;

std::shared_ptr<ItemStack> makeSharedItem(ItemStack const& stack) {
    platform::item::ScopedItemRegistry scoped;
    auto*                              copy = new ItemStack(stack);
    copy->mShowPickUp = false;
    return std::shared_ptr<ItemStack>(copy, [](ItemStack* p) {
        platform::item::ScopedItemRegistry scoped;
        delete p;
    });
}

TextElementImpl::TextElementImpl(text::Component t) : mText(std::move(t)) {
    width = mTextWidth = std::max(JadeFont::width(mText), 0);
    height             = JadeFont::lineHeight - 1;
}

TextElement& TextElementImpl::scale(float s) {
    mScale = s;
    width  = std::max(static_cast<int>(std::lround(JadeFont::width(mText) * s)), 0);
    height = static_cast<int>(std::lround(JadeFont::lineHeight * s)) - 1;
    return *this;
}

TextElement& TextElementImpl::alpha(float a) {
    mAlpha = a;
    return *this;
}

void TextElementImpl::render(GuiGraphics& graphics, int, int, float) {
    int      x           = getX();
    uint32_t normalColor = api::config::applyAlpha(api::theme::IThemeHelper::get().getNormalColor(), mAlpha);
    if (mScale != 1) {
        graphics.pushPose();
        graphics.translate(float(x), getY() + mScale);
        graphics.scale(mScale);
        DisplayHelper::INSTANCE.drawText(graphics, mText, 0, 0, normalColor);
        graphics.popPose();
    } else {
        DisplayHelper::INSTANCE.drawText(graphics, mText, float(x), float(getY()), normalColor);
    }
}

ItemStackElement::ItemStackElement(std::shared_ptr<ItemStack> item, float scale, std::optional<std::string> text)
: mItem(std::move(item)),
  mScale(scale == 0 ? 1 : scale),
  mText(std::move(text)) {
    width = height = static_cast<int>(std::floor(18 * mScale));
}

std::shared_ptr<ItemStackElement> ItemStackElement::of(ItemStack const& stack, float scale, std::optional<std::string> text) {
    return std::make_shared<ItemStackElement>(makeSharedItem(stack), scale, std::move(text));
}

void ItemStackElement::render(GuiGraphics& graphics, int, int, float) {
    if (!mItem || mItem->isNull()) return;
    DisplayHelper::INSTANCE.drawItem(graphics, float(getX() + 1), float(getY() + 1), *mItem, mScale, mText);
}

std::string ItemStackElement::getNarration() {
    if (!mItem || mItem->isNull()) return "";
    platform::item::ScopedItemRegistry scoped;
    return std::to_string(static_cast<int>(mItem->mCount)) + " " + text::stripFormatting(mItem->getHoverName());
}

SpriteElement::SpriteElement(Identifier sprite, int w, int h) : mSprite(std::move(sprite)), mOWidth(w), mOHeight(h) {
    width  = w;
    height = h;
}

Identifier const& SpriteElement::mappedSprite() {
    auto& helper = api::theme::IThemeHelper::get();
    if (mGeneration != helper.generation()) {
        mGeneration   = helper.generation();
        mMappedSprite = helper.theme().mapSprite(mSprite);
    }
    return mMappedSprite;
}

bool SpriteElement::canUseFloatingRect() {
    return platform::render::SpriteManager::get().sprite(mappedSprite()).scaling.type == platform::render::SpriteScaling::Type::Stretch;
}

void SpriteElement::render(GuiGraphics& graphics, int, int, float) {
    namespace ori = api::ui::orientation;
    if (tiledOrientation) {
        Orientation o    = *tiledOrientation;
        Rect2f      rect = floatingRect ? *floatingRect : Rect2f::of(*this);
        float axisLength = ori::getAxisLength(o, rect), axisPosition = ori::getAxisPosition(o, rect);
        float crossAxisLength = ori::getCrossAxisLength(o, rect), crossAxisPosition = ori::getCrossAxisPosition(o, rect);
        float tileAxisStep = o == Orientation::HORIZONTAL ? float(mOWidth) : float(mOHeight);
        for (float start = 0; start < axisLength; start += tileAxisStep) {
            float end      = std::min(start + tileAxisStep, axisLength);
            float tileSize = end - start;
            ori::setPosition(o, rect, axisPosition + start, crossAxisPosition);
            ori::setSize(o, rect, tileSize, crossAxisLength);
            float tw = float(mOWidth), th = float(mOHeight);
            if (o == Orientation::HORIZONTAL) tw = tileSize;
            else th = tileSize;
            DisplayHelper::INSTANCE
                .blitSprite(graphics, mappedSprite(), float(mOWidth), float(mOHeight), 0, 0, tw, th, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), mColor);
        }
        return;
    }
    if (!floatingRect) {
        DisplayHelper::INSTANCE.blitSprite(graphics, mappedSprite(), float(getX()), float(getY()), float(width), float(height), mColor);
    } else {
        DisplayHelper::INSTANCE.blitSprite(
            graphics,
            mappedSprite(),
            floatingRect->getX(),
            floatingRect->getY(),
            floatingRect->getWidth(),
            floatingRect->getHeight(),
            mColor
        );
    }
}

SpacerElement& SpacerElement::wrapped(layout::LayoutElementPtr w) {
    mWrapped = std::move(w);
    if (auto* element = dynamic_cast<Element*>(mWrapped.get())) {
        narration(element->cachedNarration());
        if (!mTag) tag(element->getTag());
    }
    return *this;
}

SpacerElement& SpacerElement::offsetWrapped(int x, int y) {
    mWrappedOffsetX = x;
    mWrappedOffsetY = y;
    if (mWrapped && (x != 0 || y != 0)) {
        mWrapped->setX(getX() + mWrappedOffsetX);
        mWrapped->setY(getY() + mWrappedOffsetY);
    }
    return *this;
}

void SpacerElement::render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) {
    if (auto* r = dynamic_cast<Element*>(mWrapped.get())) r->render(graphics, mouseX, mouseY, partialTicks);
}

void SpacerElement::renderDebug(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks, RenderDebugContext const& context) {
    Element::renderDebug(graphics, mouseX, mouseY, partialTicks, context);
    if (auto* e = dynamic_cast<Element*>(mWrapped.get())) e->renderDebug(graphics, mouseX, mouseY, partialTicks, context);
    if (mWrapped) DisplayHelper::INSTANCE.drawBorder(graphics, Rect2f::of(*this), 1, 0x880000FF, true);
}

void SpacerElement::setX(int x) {
    Element::setX(x);
    if (mWrapped) mWrapped->setX(x + mWrappedOffsetX);
}

void SpacerElement::setY(int y) {
    Element::setY(y);
    if (mWrapped) mWrapped->setY(y + mWrappedOffsetY);
}

bool SpacerElement::isMouseOver(double mouseX, double mouseY) {
    if (!mWrapped) return false;
    auto r = mWrapped->getRectangle();
    return mouseX >= r.x && mouseX < r.right() && mouseY >= r.y && mouseY < r.bottom();
}

CompoundElement::CompoundElement(ElementPtr large, ElementPtr small) : mLarge(std::move(large)), mSmall(std::move(small)) {
    width  = mLarge->getWidth();
    height = mLarge->getHeight();
}

void CompoundElement::render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) {
    mLarge->render(graphics, mouseX, mouseY, partialTicks);
    graphics.pushPose();
    mSmall->render(graphics, mouseX, mouseY, partialTicks);
    graphics.popPose();
}

void CompoundElement::setX(int x) {
    Element::setX(x);
    mLarge->setX(x);
    mSmall->setX(x + mLarge->getWidth() - mSmall->getWidth());
}

void CompoundElement::setY(int y) {
    Element::setY(y);
    mLarge->setY(y);
    mSmall->setY(y + mLarge->getHeight() - mSmall->getHeight());
}

HorizontalLineElement::HorizontalLineElement() : color(api::theme::IThemeHelper::get().getNormalColor()) {
    width  = 10;
    height = 4;
}

void HorizontalLineElement::render(GuiGraphics& graphics, int, int, float) {
    int   x     = getX();
    int   y     = getY() + height / 2;
    auto& theme = api::theme::IThemeHelper::get().theme();
    if (theme.text.shadow && !theme.lightColorScheme) {
        auto c      = api::ui::Color::rgb(color);
        auto shadow = api::ui::Color::rgb(c.getRed() / 4, c.getGreen() / 4, c.getBlue() / 4, c.getOpacity());
        DisplayHelper::fill(graphics, x + 2.5f, y + 0.5f, x + width - 1.5f, y + 1.0f, shadow.toInt());
    }
    DisplayHelper::fill(graphics, x + 2.0f, float(y), float(x + width - 2), y + 0.5f, color);
}

}
