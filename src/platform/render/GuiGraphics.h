// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
// Includes code adapted from the JustEnoughItems Bedrock port (MIT; derived from JEI, Copyright (c) 2014-2015 mezz).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "platform/render/Rect.h"
#include "platform/text/Component.h"

class MinecraftUIRenderContext;
class ClientInstance;
class ItemStack;
class Font;

namespace jade::platform::render {

class GuiGraphics {
public:
    static constexpr int LINE_HEIGHT = 9;

    GuiGraphics(MinecraftUIRenderContext& ctx, ClientInstance& ci);
    ~GuiGraphics();
    GuiGraphics(GuiGraphics const&)            = delete;
    GuiGraphics& operator=(GuiGraphics const&) = delete;

    [[nodiscard]] int   screenWidth() const { return mScreenWidth; }
    [[nodiscard]] int   screenHeight() const { return mScreenHeight; }
    [[nodiscard]] float guiScale() const { return mGuiScale; }

    void  pushPose();
    void  popPose();
    void  translate(float x, float y);
    void  scale(float s);
    [[nodiscard]] float poseScale() const { return mPose.s; }

    void fill(float x0, float y0, float x1, float y1, uint32_t argb);
    void fill(Rect const& r, uint32_t argb) { fill(float(r.x), float(r.y), float(r.right()), float(r.bottom()), argb); }
    void drawBorder(float x, float y, float w, float h, float width, uint32_t argb);

    void drawText(text::Component const& text, float x, float y, uint32_t defaultArgb, bool shadow);
    void drawText(std::string const& text, float x, float y, uint32_t argb, bool shadow);
    [[nodiscard]] int textWidth(text::Component const& text);
    [[nodiscard]] int textWidth(std::string const& text);

    bool blit(std::string const& texturePath, float x, float y, float w, float h, float u0, float v0, float u1, float v1, uint32_t tint = 0xFFFFFFFF);
    bool blitSprite(std::string const& spriteId, float x, float y, float w, float h, uint32_t tint = 0xFFFFFFFF);
    bool blitSpriteRegion(
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
        uint32_t           tint = 0xFFFFFFFF
    );

    void enableScissor(float x0, float y0, float x1, float y1);
    void disableScissor();

    void renderItem(ItemStack const& stack, float x, float y, float scale = 1.0f, float alpha = 1.0f);

    void flushImages();
    void flush();

    [[nodiscard]] MinecraftUIRenderContext& context() { return mCtx; }
    [[nodiscard]] ClientInstance&           client() { return mClient; }

private:
    struct Pose {
        float tx = 0, ty = 0, s = 1;
    };
    struct ScreenRect {
        float x0, y0, x1, y1;
    };

    Font&      font();
    float      sx(float x) const { return mPose.tx + x * mPose.s; }
    float      sy(float y) const { return mPose.ty + y * mPose.s; }
    void       applyScissor();
    void       flushText();

    MinecraftUIRenderContext& mCtx;
    ClientInstance&           mClient;
    int                       mScreenWidth  = 0;
    int                       mScreenHeight = 0;
    float                     mGuiScale     = 1.0f;
    Pose                      mPose;
    std::vector<Pose>         mPoseStack;
    std::vector<ScreenRect>   mScissorStack;
    bool                      mTextPending    = false;
    bool                      mImagesPending  = false;
    void const*               mPendingTexture = nullptr;
    uint32_t                  mPendingTint    = 0xFFFFFFFF;
    struct Impl;
    Impl* mImpl = nullptr;
};

}
