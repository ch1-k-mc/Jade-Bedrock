// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "overlay/DisplayHelper.h"

#include "api/view/FluidView.h"
#include "platform/render/TextureManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "mc/world/item/ItemStack.h"

#include "api/config/IWailaConfig.h"
#include "api/theme/IThemeHelper.h"
#include "overlay/OverlayRenderer.h"
#include "platform/render/GuiGraphics.h"
#include "util/JadeFont.h"

namespace jade::overlay {

DisplayHelper DisplayHelper::INSTANCE;

using api::config::applyAlpha;

std::string DisplayHelper::formatNumber(double number, int maxFractionDigits) {
    double scale = std::pow(10.0, maxFractionDigits);
    double t     = std::trunc(number * scale) / scale;
    char   buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", maxFractionDigits, t);
    std::string s = buf;
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    if (s == "-0") s = "0";
    return s;
}

void DisplayHelper::fill(GuiGraphics& graphics, float minX, float minY, float maxX, float maxY, uint32_t color) {
    graphics.fill(minX, minY, maxX, maxY, color);
}

void DisplayHelper::drawItem(GuiGraphics& graphics, float x, float y, ItemStack const& stack, float scale, std::optional<std::string> const& text) {
    if (opacity() < 0.5f) return;
    graphics.pushPose();
    graphics.translate(x, y);
    graphics.scale(scale);
    graphics.renderItem(stack, 0, 0, 1.0f, 1.0f);
    if (!stack.isNull()) {
        int count = static_cast<int>(stack.mCount);
        if (count != 1 || text) {
            std::string s       = text ? *text : humanReadableNumber(count, "", false, false);
            bool        smaller = s.size() > 3;
            float       sc      = smaller ? 0.5f : 0.75f;
            int         tx      = smaller ? 32 : 22;
            int         ty      = smaller ? 23 : 13;
            graphics.pushPose();
            graphics.scale(sc);
            uint32_t color = api::theme::IThemeHelper::get().theme().text.itemAmountColor;
            graphics.drawText(s, float(tx - util::JadeFont::width(s)), float(ty), color, true);
            graphics.popPose();
        }
    }
    graphics.popPose();
}

void DisplayHelper::drawBorder(GuiGraphics& graphics, api::ui::Rect2f const& r, int width, uint32_t color, bool corner) {
    float minX = r.getX(), minY = r.getY(), maxX = r.getRight(), maxY = r.getBottom();
    float w    = static_cast<float>(width);
    fill(graphics, minX + w, minY, maxX - w, minY + w, color);
    fill(graphics, minX + w, maxY - w, maxX - w, maxY, color);
    if (corner) {
        fill(graphics, minX, minY, minX + w, maxY, color);
        fill(graphics, maxX - w, minY, maxX, maxY, color);
    } else {
        fill(graphics, minX, minY + w, minX + w, maxY - w, color);
        fill(graphics, maxX - w, minY + w, maxX, maxY - w, color);
    }
}

void DisplayHelper::drawText(GuiGraphics& graphics, text::Component const& text, float x, float y, uint32_t color) {
    bool shadow = api::config::IWailaConfig::get().overlay().getTheme().text.shadow;
    color       = applyAlpha(color, opacity());
    graphics.drawText(text, std::floor(x), std::floor(y), color, shadow);
}

void DisplayHelper::drawText(GuiGraphics& graphics, std::string const& text, float x, float y, uint32_t color) {
    drawText(graphics, text::Component::literal(text), x, y, color);
}

void DisplayHelper::blitSprite(GuiGraphics& graphics, std::string const& sprite, float x, float y, float w, float h, uint32_t color) {
    color = applyAlpha(color, opacity());
    graphics.blitSprite(api::theme::IThemeHelper::get().theme().mapSprite(sprite), x, y, w, h, color);
}

void DisplayHelper::blitSprite(
    GuiGraphics&       graphics,
    std::string const& sprite,
    float              spriteWidth,
    float              spriteHeight,
    float              uStart,
    float              vStart,
    float              uSize,
    float              vSize,
    float              x,
    float              y,
    float              width,
    float              height,
    uint32_t           color
) {
    if (width == 0 || height == 0) return;
    color = applyAlpha(color, opacity());
    graphics.blitSpriteRegion(
        api::theme::IThemeHelper::get().theme().mapSprite(sprite),
        spriteWidth,
        spriteHeight,
        uStart,
        vStart,
        uSize,
        vSize,
        x,
        y,
        width,
        height,
        color
    );
}

std::string DisplayHelper::humanReadableNumber(double number, std::string const& unit, bool milli, bool useDefaultFormatter) {
    if (std::abs(number) < 1.0e-5) return "0" + unit;
    std::string sb;
    if (number < 0) {
        number = -number;
        sb.push_back('-');
    }
    if (milli && number >= 1000) {
        number /= 1000;
        milli = false;
    }
    bool autoFormatter = !useDefaultFormatter;
    int  exp           = (autoFormatter && number < 10000) ? 0 : static_cast<int>(std::log10(number)) / 3;
    if (exp > 7) exp = 7;
    if (exp > 0) number /= std::pow(1000, exp);
    int digits = 2;
    if (autoFormatter) digits = number < 10 ? 2 : (number < 100 ? 1 : 0);
    sb += formatNumber(number, digits);
    if (exp == 0) {
        if (milli) sb.push_back('m');
    } else {
        sb.push_back("kMGTPEZ"[exp - 1]);
    }
    sb += unit;
    return sb;
}

void DisplayHelper::drawFluid(GuiGraphics& graphics, float x, float y, api::view::JadeFluidObject const& fluid, float width, float height, int64_t) {
    if (fluid.isEmpty() || width <= 0 || height <= 0) return;
    std::string const tex   = fluid.texture();
    uint32_t          color = api::config::applyAlpha(fluid.tint(), opacity());
    float frameV = 1.0f;
    if (auto size = platform::render::TextureManager::get().textureSize(tex); size && size->second > size->first && size->first > 0) {
        frameV = static_cast<float>(size->first) / static_cast<float>(size->second);
    }
    constexpr float tile = 16.0f;
    bool            ok   = true;
    for (float ty = 0; ty < height && ok; ty += tile) {
        float th = std::min(tile, height - ty);
        for (float tx = 0; tx < width && ok; tx += tile) {
            float tw = std::min(tile, width - tx);
            ok       = graphics.blit(tex, x + tx, y + ty, tw, th, 0, 0, tw / tile, th / tile * frameV, color);
        }
    }
    if (!ok) fill(graphics, x, y, x + width, y + height, api::config::applyAlpha(0xAAAAAAAAu, opacity()));
}

float DisplayHelper::opacity() const { return OverlayRenderer::animation.alpha; }
float DisplayHelper::backgroundOpacity() const { return OverlayRenderer::animation.showHideAlpha; }

}
