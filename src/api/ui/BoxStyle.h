// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <array>
#include <memory>
#include <optional>

#include "nlohmann/json.hpp"

#include "api/JadeIds.h"
#include "api/ui/ColorPalette.h"
#include "api/ui/Element.h"

namespace jade::api::ui {

class BoxStyle {
public:
    static constexpr std::array<int, 4> DEFAULT_PADDING{3, 3, 3, 3};

    BoxStyle() = default;
    BoxStyle(
        std::optional<std::array<float, 4>> boxProgressOffset,
        ColorPalette                        boxProgressColors,
        std::optional<std::array<int, 4>>   padding,
        int                                 borderWidth,
        std::optional<Identifier>           sprite,
        std::optional<Identifier>           withIconSprite,
        bool                                tooltip
    );

    static BoxStyle fromJson(nlohmann::json const& j);

    static BoxStyle& nestedBox();
    static BoxStyle& viewGroup();
    static BoxStyle& transparent();
    static BoxStyle  simple(std::optional<Identifier> sprite, std::optional<std::array<int, 4>> padding);
    static BoxStyle  tooltip(std::optional<Identifier> sprite, std::optional<std::array<int, 4>> padding, int borderWidth = 1);
    static BoxStyle  sprite(std::optional<Identifier> sprite, std::optional<std::array<int, 4>> padding, int borderWidth = 1);

    static BoxStyle const DEFAULT_NESTED_BOX;
    static BoxStyle const DEFAULT_VIEW_GROUP;

    [[nodiscard]] float boxProgressOffset(ScreenDirection dir) const;
    [[nodiscard]] int   padding(ScreenDirection dir) const;
    void                render(GuiGraphics& graphics, StyledElement& element, float x, float y, float w, float h, float alpha) const;
    [[nodiscard]] int   borderWidth() const { return mBorderWidth; }
    [[nodiscard]] BoxStyle copy() const { return *this; }

    std::optional<std::array<float, 4>> boxProgressOffsetValues;
    std::optional<std::array<int, 4>>   paddingValues = DEFAULT_PADDING;
    int                                 mBorderWidth  = 1;
    ColorPalette                        boxProgressColors;
    std::optional<Identifier>           spriteId;
    std::optional<Identifier>           withIconSprite;
    bool                                isTooltip = false;
};

Identifier tooltipBackgroundSprite(Identifier const& id);
Identifier tooltipFrameSprite(Identifier const& id);

}
