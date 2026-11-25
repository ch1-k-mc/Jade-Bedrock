// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "api/ui/BoxStyle.h"

#include <cmath>

#include "api/theme/IThemeHelper.h"
#include "api/theme/Theme.h"
#include "api/ui/Color.h"
#include "platform/render/GuiGraphics.h"

namespace jade::api::ui {

namespace {

uint32_t readColor(nlohmann::json const& j, char const* key, uint32_t def) {
    if (!j.contains(key)) return def;
    if (j[key].is_string()) {
        if (auto c = Color::valueOf(j[key].get<std::string>())) return c->toInt();
        return def;
    }
    if (j[key].is_number_integer()) return static_cast<uint32_t>(j[key].get<int64_t>());
    return def;
}

uint32_t white(float alpha) {
    uint32_t a = static_cast<uint32_t>(std::lround(std::min(std::max(alpha, 0.f), 1.f) * 255.f));
    return (a << 24) | 0xFFFFFF;
}

Identifier withPath(Identifier const& id, std::string const& path) {
    auto colon = id.find(':');
    return (colon == std::string::npos ? std::string("minecraft") : id.substr(0, colon)) + ":" + path;
}

std::string pathOf(Identifier const& id) {
    auto colon = id.find(':');
    return colon == std::string::npos ? id : id.substr(colon + 1);
}

}

ColorPalette ColorPalette::fromJson(nlohmann::json const& j) {
    ColorPalette p;
    if (!j.is_object()) return p;
    p.normal  = readColor(j, "normal", DEFAULT.normal);
    p.info    = readColor(j, "info", DEFAULT.info);
    p.title   = readColor(j, "title", DEFAULT.title);
    p.success = readColor(j, "success", DEFAULT.success);
    p.warning = readColor(j, "warning", DEFAULT.warning);
    p.danger  = readColor(j, "danger", DEFAULT.danger);
    p.failure = readColor(j, "failure", DEFAULT.failure);
    return p;
}

Identifier tooltipBackgroundSprite(Identifier const& id) { return withPath(id, "tooltip/" + pathOf(id) + "_background"); }
Identifier tooltipFrameSprite(Identifier const& id) { return withPath(id, "tooltip/" + pathOf(id) + "_frame"); }

BoxStyle const BoxStyle::DEFAULT_NESTED_BOX = BoxStyle::sprite(JadeIds::JADE("nested_box"), std::nullopt);
BoxStyle const BoxStyle::DEFAULT_VIEW_GROUP = BoxStyle::sprite(JadeIds::JADE("view_group"), std::array<int, 4>{2, 2, 2, 2}, 0);

BoxStyle::BoxStyle(
    std::optional<std::array<float, 4>> boxProgressOffset,
    ColorPalette                        colors,
    std::optional<std::array<int, 4>>   padding,
    int                                 borderWidth,
    std::optional<Identifier>           sprite,
    std::optional<Identifier>           withIcon,
    bool                                tooltip
)
: boxProgressOffsetValues(boxProgressOffset),
  paddingValues(padding ? padding : std::optional<std::array<int, 4>>(DEFAULT_PADDING)),
  mBorderWidth(borderWidth),
  boxProgressColors(colors),
  spriteId(std::move(sprite)),
  withIconSprite(std::move(withIcon)),
  isTooltip(tooltip) {}

BoxStyle BoxStyle::fromJson(nlohmann::json const& j) {
    std::optional<std::array<float, 4>> offset;
    if (j.contains("boxProgressOffset") && j["boxProgressOffset"].is_array() && j["boxProgressOffset"].size() == 4) {
        std::array<float, 4> a{};
        for (size_t i = 0; i < 4; ++i) a[i] = j["boxProgressOffset"][i].get<float>();
        offset = a;
    }
    std::optional<std::array<int, 4>> padding;
    if (j.contains("padding") && j["padding"].is_array() && j["padding"].size() == 4) {
        std::array<int, 4> a{};
        for (size_t i = 0; i < 4; ++i) a[i] = j["padding"][i].get<int>();
        padding = a;
    }
    ColorPalette colors = j.contains("boxProgressColors") ? ColorPalette::fromJson(j["boxProgressColors"]) : ColorPalette::DEFAULT;
    std::optional<Identifier> sprite, withIcon;
    if (j.contains("sprite") && j["sprite"].is_string()) sprite = j["sprite"].get<std::string>();
    if (j.contains("withIconSprite") && j["withIconSprite"].is_string()) withIcon = j["withIconSprite"].get<std::string>();
    return BoxStyle(offset, colors, padding, j.value("borderWidth", 1), sprite, withIcon, j.value("tooltip", false));
}

BoxStyle& BoxStyle::nestedBox() { return theme::IThemeHelper::get().theme().nestedBoxStyle; }
BoxStyle& BoxStyle::viewGroup() { return theme::IThemeHelper::get().theme().viewGroupStyle; }
BoxStyle& BoxStyle::transparent() {
    static BoxStyle t = sprite(std::nullopt, std::nullopt, 0);
    return t;
}

BoxStyle BoxStyle::simple(std::optional<Identifier> sprite, std::optional<std::array<int, 4>> padding) {
    return BoxStyle(std::nullopt, ColorPalette::DEFAULT, padding, 1, std::move(sprite), std::nullopt, false);
}

BoxStyle BoxStyle::tooltip(std::optional<Identifier> sprite, std::optional<std::array<int, 4>> padding, int borderWidth) {
    return BoxStyle(std::nullopt, ColorPalette::DEFAULT, padding, borderWidth, std::move(sprite), std::nullopt, true);
}

BoxStyle BoxStyle::sprite(std::optional<Identifier> sprite, std::optional<std::array<int, 4>> padding, int borderWidth) {
    return BoxStyle(std::nullopt, ColorPalette::DEFAULT, padding, borderWidth, std::move(sprite), std::nullopt, false);
}

float BoxStyle::boxProgressOffset(ScreenDirection dir) const {
    return boxProgressOffsetValues ? (*boxProgressOffsetValues)[static_cast<size_t>(dir)] : 0.0f;
}

int BoxStyle::padding(ScreenDirection dir) const { return (paddingValues ? *paddingValues : DEFAULT_PADDING)[static_cast<size_t>(dir)]; }

void BoxStyle::render(GuiGraphics& graphics, StyledElement& element, float x, float y, float w, float h, float alpha) const {
    std::optional<Identifier> texture = spriteId;
    if (withIconSprite && element.getIcon()) texture = withIconSprite;
    if (!texture) return;
    float rx = std::round(x), ry = std::round(y), rw = std::round(w), rh = std::round(h);
    uint32_t col = white(alpha);
    if (isTooltip) {
        rx -= 9;
        ry -= 9;
        rw += 18;
        rh += 18;
        graphics.blitSprite(tooltipBackgroundSprite(*texture), rx, ry, rw, rh, col);
        graphics.blitSprite(tooltipFrameSprite(*texture), rx, ry, rw, rh, col);
    } else {
        graphics.blitSprite(*texture, rx, ry, rw, rh, col);
    }
}

}
