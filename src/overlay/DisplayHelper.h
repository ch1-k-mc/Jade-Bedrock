// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
// Includes code adapted from the JustEnoughItems Bedrock port (MIT; derived from JEI, Copyright (c) 2014-2015 mezz).
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "api/ui/Rect2f.h"
#include "platform/text/Component.h"

class ItemStack;

namespace jade::platform::render {
class GuiGraphics;
}

namespace jade::api::view {
struct JadeFluidObject;
}

namespace jade::overlay {

using platform::render::GuiGraphics;

class DisplayHelper {
public:
    static DisplayHelper INSTANCE;

    static void fill(GuiGraphics& graphics, float minX, float minY, float maxX, float maxY, uint32_t color);

    void drawItem(GuiGraphics& graphics, float x, float y, ItemStack const& stack, float scale, std::optional<std::string> const& text);
    void drawBorder(GuiGraphics& graphics, api::ui::Rect2f const& rect, int width, uint32_t color, bool corner);
    void drawText(GuiGraphics& graphics, text::Component const& text, float x, float y, uint32_t color);
    void drawText(GuiGraphics& graphics, std::string const& text, float x, float y, uint32_t color);

    void drawFluid(GuiGraphics& graphics, float x, float y, api::view::JadeFluidObject const& fluid, float width, float height, int64_t capacity);
    void blitSprite(GuiGraphics& graphics, std::string const& sprite, float x, float y, float w, float h, uint32_t color = 0xFFFFFFFF);
    void blitSprite(
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
        uint32_t           color = 0xFFFFFFFF
    );

    [[nodiscard]] std::string humanReadableNumber(double number, std::string const& unit, bool milli, bool useDefaultFormatter = true);
    [[nodiscard]] text::Component stripColor(text::Component const& c) const { return c.stripColor(); }

    [[nodiscard]] static std::string formatNumber(double number, int maxFractionDigits = 2);

    [[nodiscard]] float opacity() const;
    [[nodiscard]] float backgroundOpacity() const;
};

}
