// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
// Includes code adapted from the JustEnoughItems Bedrock port (MIT; derived from JEI, Copyright (c) 2014-2015 mezz).
#include "platform/render/GuiGraphics.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/CaretMeasureData.h"
#include "mc/client/gui/FontHandle.h"
#include "mc/client/gui/GuiData.h"
#include "mc/client/gui/ScreenSizeData.h"
#include "mc/client/gui/TextAlignment.h"
#include "mc/client/gui/TextMeasureData.h"
#include "mc/client/renderer/BaseActorRenderContext.h"
#include "mc/client/renderer/actor/ItemRenderer.h"
#include "mc/client/renderer/screen/MinecraftUIRenderContext.h"
#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/input/RectangleArea.h"
#include "mc/deps/minecraft_renderer/renderer/BedrockTextureData.h"
#include "mc/deps/minecraft_renderer/renderer/TexturePtr.h"
#include "mc/world/item/ItemStack.h"

#include "platform/render/SpriteManager.h"
#include "platform/render/TextureManager.h"

namespace jade::platform::render {

namespace {

mce::Color toColor(uint32_t argb) {
    float a = static_cast<float>((argb >> 24) & 0xFF) / 255.0f;
    float r = static_cast<float>((argb >> 16) & 0xFF) / 255.0f;
    float g = static_cast<float>((argb >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>(argb & 0xFF) / 255.0f;
    return mce::Color(r, g, b, a);
}

float alphaOf(uint32_t argb) { return static_cast<float>((argb >> 24) & 0xFF) / 255.0f; }

}

struct GuiGraphics::Impl {
    Font*                                 font = nullptr;
    std::optional<BaseActorRenderContext> actorContext;
};

GuiGraphics::GuiGraphics(MinecraftUIRenderContext& ctx, ClientInstance& ci) : mCtx(ctx), mClient(ci), mImpl(new Impl) {
    auto guiData  = ci.getGuiData();
    mGuiScale     = guiData->mGuiScale;
    Vec2 uiSize   = guiData->mScreenSizeData.get().clientUIScreenSize.get();
    mScreenWidth  = static_cast<int>(uiSize.x);
    mScreenHeight = static_cast<int>(uiSize.y);
}

GuiGraphics::~GuiGraphics() {
    flush();
    if (!mScissorStack.empty()) mCtx.disableScissorTest();
    delete mImpl;
}

Font& GuiGraphics::font() {
    if (!mImpl->font) {
        FontHandle handle = mClient.getFontHandle();
        mImpl->font       = handle.mDefaultFont.get();
    }
    return *mImpl->font;
}

void GuiGraphics::pushPose() { mPoseStack.push_back(mPose); }
void GuiGraphics::popPose() {
    if (mPoseStack.empty()) return;
    mPose = mPoseStack.back();
    mPoseStack.pop_back();
}
void GuiGraphics::translate(float x, float y) {
    mPose.tx += x * mPose.s;
    mPose.ty += y * mPose.s;
}
void GuiGraphics::scale(float s) { mPose.s *= s; }

void GuiGraphics::flushText() {
    if (!mTextPending) return;
    mTextPending = false;
    mCtx.flushText(0.0f, std::nullopt);
}

void GuiGraphics::flush() {
    flushImages();
    flushText();
}

void GuiGraphics::flushImages() {
    if (!mImagesPending) return;
    mImagesPending = false;
    static HashedString const material(TextureManager::flushMaterialName());
    mce::Color                c = toColor(mPendingTint);
    mCtx.flushImages(mce::Color(c.r, c.g, c.b, 1.0f), alphaOf(mPendingTint), material);
}

void GuiGraphics::fill(float x0, float y0, float x1, float y1, uint32_t argb) {
    if (x1 <= x0 || y1 <= y0 || alphaOf(argb) <= 0) return;
    flush();
    RectangleArea rect{sx(x0), sx(x1), sy(y0), sy(y1)};
    mCtx.fillRectangle(rect, toColor(argb), alphaOf(argb));
}

void GuiGraphics::drawBorder(float x, float y, float w, float h, float bw, uint32_t argb) {
    fill(x, y, x + w, y + bw, argb);
    fill(x, y + h - bw, x + w, y + h, argb);
    fill(x, y + bw, x + bw, y + h - bw, argb);
    fill(x + w - bw, y + bw, x + w, y + h - bw, argb);
}

void GuiGraphics::drawText(text::Component const& text, float x, float y, uint32_t defaultArgb, bool shadow) {
    if (text.isEmpty() || alphaOf(defaultArgb) <= 0.004f) return;
    flushImages();
    float const fontSize = mPose.s;
    float       offset   = 0;
    for (auto const& span : text.spans()) {
        if (span.text.empty()) continue;
        std::string str   = span.style.formatCodes() + span.text;
        uint32_t    color = span.style.color ? ((*span.style.color & 0x00FFFFFF) | (defaultArgb & 0xFF000000)) : defaultArgb;
        float       px    = sx(x + offset);
        float       py    = sy(y);
        RectangleArea    rect{px, static_cast<float>(mScreenWidth) + 1000.0f, py, py + LINE_HEIGHT * fontSize + 2};
        TextMeasureData  tmd{fontSize, 0.0f, shadow, false, false, ui::TextAlignment::Left};
        CaretMeasureData cmd{0, false};
        float const width = static_cast<float>(mCtx.getLineLength(font(), str, 1.0f, false));
        mCtx.drawText(font(), rect, std::move(str), toColor(color), alphaOf(color), ui::TextAlignment::Left, tmd, cmd);
        mTextPending = true;
        offset += width;
    }
}

void GuiGraphics::drawText(std::string const& text, float x, float y, uint32_t argb, bool shadow) {
    drawText(text::Component::literal(text), x, y, argb, shadow);
}

int GuiGraphics::textWidth(text::Component const& text) {
    int w = 0;
    for (auto const& span : text.spans()) {
        if (span.text.empty()) continue;
        w += mCtx.getLineLength(font(), span.style.formatCodes() + span.text, 1.0f, false);
    }
    return w;
}

int GuiGraphics::textWidth(std::string const& text) { return text.empty() ? 0 : mCtx.getLineLength(font(), text, 1.0f, false); }

bool GuiGraphics::blit(std::string const& texturePath, float x, float y, float w, float h, float u0, float v0, float u1, float v1, uint32_t tint) {
    if (w <= 0 || h <= 0 || alphaOf(tint) <= 0.004f) return false;
    auto const* texture = TextureManager::get().getClientTexture(mCtx, texturePath);
    if (!texture) return false;
    flushText();
    if (mImagesPending && (mPendingTexture != texture || mPendingTint != tint)) flushImages();
    mPendingTexture = texture;
    mPendingTint    = tint;
    mCtx.drawImage(
        *texture,
        glm::vec2(sx(x), sy(y)),
        glm::vec2(w * mPose.s, h * mPose.s),
        glm::vec2(u0, v0),
        glm::vec2(u1 - u0, v1 - v0),
        false
    );
    mImagesPending = true;
    return true;
}

bool GuiGraphics::blitSprite(std::string const& spriteId, float x, float y, float w, float h, uint32_t tint) {
    auto const& info = SpriteManager::get().sprite(spriteId);
    if (!info.exists || w <= 0 || h <= 0) return false;
    auto const& sc = info.scaling;
    switch (sc.type) {
    case SpriteScaling::Type::Stretch:
        return blit(info.texturePath, x, y, w, h, 0, 0, 1, 1, tint);
    case SpriteScaling::Type::Tile: {
        float const tw = static_cast<float>(std::max(sc.width, 1)), th = static_cast<float>(std::max(sc.height, 1));
        bool        ok = true;
        for (float ty = 0; ty < h; ty += th) {
            float ch = std::min(th, h - ty);
            for (float tx = 0; tx < w; tx += tw) {
                float cw = std::min(tw, w - tx);
                ok       = blit(info.texturePath, x + tx, y + ty, cw, ch, 0, 0, cw / tw, ch / th, tint) && ok;
            }
        }
        return ok;
    }
    case SpriteScaling::Type::NineSlice: {
        float l = static_cast<float>(sc.borderLeft), t = static_cast<float>(sc.borderTop);
        float r = static_cast<float>(sc.borderRight), b = static_cast<float>(sc.borderBottom);
        l                 = std::min(l, w / 2);
        r                 = std::min(r, w - l);
        t                 = std::min(t, h / 2);
        b                 = std::min(b, h - t);
        float const sw    = static_cast<float>(std::max(sc.width, 1)), sh = static_cast<float>(std::max(sc.height, 1));
        float const xs[4] = {x, x + l, x + w - r, x + w};
        float const ys[4] = {y, y + t, y + h - b, y + h};
        float const us[4] = {0.0f, l / sw, (sw - r) / sw, 1.0f};
        float const vs[4] = {0.0f, t / sh, (sh - b) / sh, 1.0f};
        bool        ok    = true;
        for (int iy = 0; iy < 3; ++iy) {
            for (int ix = 0; ix < 3; ++ix) {
                float cw = xs[ix + 1] - xs[ix];
                float ch = ys[iy + 1] - ys[iy];
                if (cw <= 0 || ch <= 0) continue;
                ok = blit(info.texturePath, xs[ix], ys[iy], cw, ch, us[ix], vs[iy], us[ix + 1], vs[iy + 1], tint) && ok;
            }
        }
        return ok;
    }
    }
    return false;
}

bool GuiGraphics::blitSpriteRegion(
    std::string const& spriteId,
    float              spriteW,
    float              spriteH,
    float              uStart,
    float              vStart,
    float              uSize,
    float              vSize,
    float              x,
    float              y,
    float              w,
    float              h,
    uint32_t           tint
) {
    auto const& info = SpriteManager::get().sprite(spriteId);
    if (!info.exists || spriteW <= 0 || spriteH <= 0) return false;
    return blit(info.texturePath, x, y, w, h, uStart / spriteW, vStart / spriteH, (uStart + uSize) / spriteW, (vStart + vSize) / spriteH, tint);
}

void GuiGraphics::applyScissor() {
    if (mScissorStack.empty()) {
        mCtx.disableScissorTest();
        return;
    }
    auto const&   r = mScissorStack.back();
    RectangleArea rect{r.x0, r.x1, r.y0, r.y1};
    mCtx.enableScissorTest(rect);
}

void GuiGraphics::enableScissor(float x0, float y0, float x1, float y1) {
    flush();
    ScreenRect r{sx(x0), sy(y0), sx(x1), sy(y1)};
    if (!mScissorStack.empty()) {
        auto const& p = mScissorStack.back();
        r.x0          = std::max(r.x0, p.x0);
        r.y0          = std::max(r.y0, p.y0);
        r.x1          = std::max(r.x0, std::min(r.x1, p.x1));
        r.y1          = std::max(r.y0, std::min(r.y1, p.y1));
    }
    mScissorStack.push_back(r);
    applyScissor();
}

void GuiGraphics::disableScissor() {
    flush();
    if (!mScissorStack.empty()) mScissorStack.pop_back();
    applyScissor();
}

void GuiGraphics::renderItem(ItemStack const& stack, float x, float y, float scale, float alpha) {
    if (stack.isNull()) return;
    ItemRenderer* renderer = mClient.getItemRenderer();
    if (!renderer) return;
    flush();
    if (!mImpl->actorContext) {
        mImpl->actorContext.emplace(mCtx.mScreenContext, mClient, mClient.getMinecraftGame());
    }
    bool const scissored = !mScissorStack.empty();
    if (scissored) mCtx.disableScissorTest();
    renderer->renderGuiItemNew(*mImpl->actorContext, stack, 0, sx(x), sy(y), false, alpha, 1.0f, scale * mPose.s, 0);
    if (scissored) applyScissor();
}

}
