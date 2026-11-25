// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "nlohmann/json.hpp"

#include "api/JadeIds.h"
#include "api/ui/BoxStyle.h"
#include "api/ui/ColorPalette.h"
#include "platform/text/Component.h"

namespace jade::api::theme {

using ui::BoxStyle;
using ui::ColorPalette;

struct TextSetting {
    ColorPalette colors         = ColorPalette::DEFAULT;
    bool         shadow         = true;
    text::Style  modNameStyle;
    uint32_t     itemAmountColor = 0xFFFFFFFF;

    static TextSetting fromJson(nlohmann::json const& j);
};

class SneakyDetails {
public:
    Identifier  sprite            = JadeIds::JADE("details_arrow");
    int         width             = 7;
    int         height            = 5;
    float       offsetX           = 0;
    float       offsetY           = 0;
    std::string animation         = "breath";
    float       animationDistance = 1;
    float       animationPeriod   = 12;

    static SneakyDetails fromJson(nlohmann::json const& j);
    void render(ui::GuiGraphics& graphics, float partialTicks, ui::BoxElement& element) const;
};

class Theme {
public:
    std::optional<Identifier>        id;
    std::string                      styleName = "jade.default";
    BoxStyle                         tooltipStyle;
    BoxStyle                         nestedBoxStyle = BoxStyle::DEFAULT_NESTED_BOX;
    BoxStyle                         viewGroupStyle = BoxStyle::DEFAULT_VIEW_GROUP;
    TextSetting                      text;
    float                            changeOpacity    = 0;
    bool                             lightColorScheme = false;
    std::optional<Identifier>        iconSlotSprite;
    int                              iconSlotInflation = 0;
    std::shared_ptr<ui::BoxElement>  iconSlotSpriteCache;
    SneakyDetails                    sneakyDetails;
    ColorPalette                     progressColors = ColorPalette::DEFAULT;
    std::map<Identifier, Identifier> spriteMapping;

    static std::unique_ptr<Theme> fromJson(nlohmann::json const& j);

    [[nodiscard]] Identifier  fullId() const { return *id; }
    [[nodiscard]] Identifier  mainId() const;
    [[nodiscard]] std::string styleId() const;
    [[nodiscard]] Identifier  mapSprite(Identifier const& sprite) const;
    ui::ElementPtr modifyIcon(ui::ElementPtr icon);
};

class IThemeHelper {
public:
    virtual ~IThemeHelper() = default;
    static IThemeHelper& get();

    virtual Theme&       theme() = 0;
    uint32_t             getNormalColor() { return theme().text.colors.normal; }
    virtual Theme&       getTheme(Identifier const& id) = 0;
    virtual bool         hasTheme(Identifier const& id) = 0;
    virtual std::vector<Theme*> getThemes()             = 0;

    virtual text::Component info(text::Component c)    = 0;
    virtual text::Component success(text::Component c) = 0;
    virtual text::Component warning(text::Component c) = 0;
    virtual text::Component danger(text::Component c)  = 0;
    virtual text::Component failure(text::Component c) = 0;
    virtual text::Component title(text::Component c)   = 0;
    virtual text::Component modName(text::Component c) = 0;
    text::Component         info(std::string const& s) { return info(text::Component::literal(s)); }
    text::Component         success(std::string const& s) { return success(text::Component::literal(s)); }
    text::Component         warning(std::string const& s) { return warning(text::Component::literal(s)); }
    text::Component         danger(std::string const& s) { return danger(text::Component::literal(s)); }
    text::Component         failure(std::string const& s) { return failure(text::Component::literal(s)); }
    text::Component         title(std::string const& s) { return title(text::Component::literal(s)); }
    text::Component         modName(std::string const& s) { return modName(text::Component::literal(s)); }
    text::Component info(double number);

    virtual std::shared_ptr<ui::TextElement> modNameElement(text::Component c) = 0;
    virtual text::Component                  seconds(int ticks, float tickRate, bool alwaysOnePart = false) = 0;

    bool        isLightColorScheme() { return theme().lightColorScheme; }
    virtual int generation()         = 0;
    virtual void setThemeOverride(Theme* theme) = 0;
};

}
