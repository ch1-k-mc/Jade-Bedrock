// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "api/theme/Theme.h"

#include <cmath>

#include "api/config/IWailaConfig.h"
#include "api/ui/Color.h"
#include "impl/Tooltip.h"
#include "impl/theme/ThemeHelper.h"
#include "impl/ui/BoxElementImpl.h"
#include "overlay/DisplayHelper.h"
#include "overlay/OverlayRenderer.h"
#include "platform/render/GuiGraphics.h"
#include "platform/render/SpriteManager.h"

namespace jade::api::theme {

IThemeHelper& IThemeHelper::get() { return impl::theme::ThemeHelper::INSTANCE; }

text::Component IThemeHelper::info(double number) { return info(overlay::DisplayHelper::formatNumber(number)); }

TextSetting TextSetting::fromJson(nlohmann::json const& j) {
    TextSetting t;
    if (!j.is_object()) return t;
    if (j.contains("colors")) t.colors = ColorPalette::fromJson(j["colors"]);
    t.shadow = j.value("shadow", true);
    if (j.contains("modNameStyle")) t.modNameStyle = config::styleFromJson(j["modNameStyle"]);
    if (j.contains("itemAmountColor") && j["itemAmountColor"].is_string()) {
        if (auto c = ui::Color::valueOf(j["itemAmountColor"].get<std::string>())) t.itemAmountColor = c->toInt();
    }
    return t;
}

SneakyDetails SneakyDetails::fromJson(nlohmann::json const& j) {
    SneakyDetails s;
    if (!j.is_object() || j.value("type", "simple") != "simple") return s;
    s.sprite            = j.value("sprite", s.sprite);
    s.width             = j.value("width", s.width);
    s.height            = j.value("height", s.height);
    s.offsetX           = j.value("offsetX", 0.0f);
    s.offsetY           = j.value("offsetY", 0.0f);
    s.animation         = j.value("animation", std::string());
    s.animationDistance = j.value("animationDistance", 1.0f);
    s.animationPeriod   = j.value("animationPeriod", 12.0f);
    return s;
}

void SneakyDetails::render(ui::GuiGraphics& graphics, float, ui::BoxElement& element) const {
    float x     = element.getX() + element.getWidth() / 2.0f - width / 2.0f + offsetX;
    float y     = element.getY() + element.getHeight() - height / 2.0f + offsetY;
    float alpha = 1.0f;
    if (animation == "breath") {
        float breath = std::fmod(overlay::OverlayRenderer::ticks / 5, animationPeriod) - 2;
        if (breath > 4) return;
        alpha = 1 - std::abs(breath) / 2;
        y += animationDistance * breath;
        if (alpha < 0.016f) return;
    }
    graphics.pushPose();
    graphics.translate(x, y);
    overlay::DisplayHelper::INSTANCE.blitSprite(graphics, sprite, 0, 0, float(width), float(height), config::applyAlpha(0xFFFFFFFF, alpha));
    graphics.popPose();
}

std::unique_ptr<Theme> Theme::fromJson(nlohmann::json const& j) {
    if (!j.contains("tooltipStyle")) return nullptr;
    auto t       = std::make_unique<Theme>();
    t->styleName = j.value("styleName", std::string("jade.default"));
    if (t->styleName.empty()) t->styleName = "jade.default";
    t->tooltipStyle = BoxStyle::fromJson(j["tooltipStyle"]);
    if (j.contains("nestedBoxStyle")) t->nestedBoxStyle = BoxStyle::fromJson(j["nestedBoxStyle"]);
    if (j.contains("viewGroupStyle")) t->viewGroupStyle = BoxStyle::fromJson(j["viewGroupStyle"]);
    if (j.contains("text")) t->text = TextSetting::fromJson(j["text"]);
    t->changeOpacity    = std::min(std::max(j.value("changeOpacity", 0.0f), 0.0f), 1.0f);
    t->lightColorScheme = j.value("lightColorScheme", false);
    if (j.contains("iconSlotSprite") && j["iconSlotSprite"].is_string()) t->iconSlotSprite = j["iconSlotSprite"].get<std::string>();
    t->iconSlotInflation = j.value("iconSlotInflation", 0);
    if (j.contains("sneakyDetails")) t->sneakyDetails = SneakyDetails::fromJson(j["sneakyDetails"]);
    if (j.contains("progressColors")) t->progressColors = ColorPalette::fromJson(j["progressColors"]);
    if (j.contains("spriteMapping") && j["spriteMapping"].is_object()) {
        for (auto const& [k, v] : j["spriteMapping"].items())
            if (v.is_string()) t->spriteMapping[k] = v.get<std::string>();
    }
    return t;
}

Identifier Theme::mainId() const {
    auto colon = id->find(':');
    auto slash = id->find('/', colon + 1);
    return slash == std::string::npos ? *id : id->substr(0, slash);
}

std::string Theme::styleId() const {
    auto colon = id->find(':');
    auto slash = id->find('/', colon + 1);
    return slash == std::string::npos ? std::string() : id->substr(slash + 1);
}

Identifier Theme::mapSprite(Identifier const& sprite) const {
    auto it = spriteMapping.find(sprite);
    return it == spriteMapping.end() ? sprite : it->second;
}

ui::ElementPtr Theme::modifyIcon(ui::ElementPtr icon) {
    if (!icon) return nullptr;
    auto& overlay = config::IWailaConfig::get().overlay();
    if (!overlay.shouldShowIcon() || overlay.getIconMode() == config::IconMode::INLINE) return nullptr;
    if (iconSlotSprite) {
        if (!iconSlotSpriteCache) {
            auto const&        info = platform::render::SpriteManager::get().sprite(*iconSlotSprite);
            std::array<int, 4> padding{};
            padding.fill(iconSlotInflation);
            if (info.scaling.type == platform::render::SpriteScaling::Type::NineSlice) {
                padding[0] += info.scaling.borderTop;
                padding[1] += info.scaling.borderRight;
                padding[2] += info.scaling.borderBottom;
                padding[3] += info.scaling.borderLeft;
            }
            iconSlotSpriteCache = std::make_shared<impl::ui::BoxElementImpl>(std::make_shared<impl::Tooltip>(), BoxStyle::simple(iconSlotSprite, padding));
        }
        auto& tooltip1 = iconSlotSpriteCache->getTooltip();
        tooltip1.clear();
        tooltip1.add(icon);
        iconSlotSpriteCache->updateSize();
        icon = iconSlotSpriteCache;
    }
    icon->tag(JadeIds::CORE_ROOT_ICON);
    return icon;
}

}
