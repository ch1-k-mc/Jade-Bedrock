// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/theme/ThemeHelper.h"

#include <cmath>
#include <fstream>

#include "ll/api/mod/NativeMod.h"

#include "api/config/IWailaConfig.h"
#include "impl/config/WailaConfig.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/text/I18n.h"

namespace jade::impl::theme {

ThemeHelper ThemeHelper::INSTANCE;

namespace {
auto& logger() { return ll::mod::NativeMod::current()->getLogger(); }

int javaHash(std::string const& s) {
    int h = 0;
    for (unsigned char c : s) h = 31 * h + c;
    return h;
}
}

void ThemeHelper::load(std::filesystem::path const& assetsDir) {
    std::vector<Identifier> existingKeys;
    for (auto const& [k, _] : mThemes) existingKeys.push_back(k);
    mThemes.clear();
    mTheme         = nullptr;
    mFallback      = nullptr;
    mThemeOverride = nullptr;
    Theme*          enable = nullptr;
    std::error_code ec;
    for (auto const& nsDir : std::filesystem::directory_iterator(assetsDir, ec)) {
        if (!nsDir.is_directory()) continue;
        auto themeDir = nsDir.path() / "jade_themes";
        if (!std::filesystem::is_directory(themeDir)) continue;
        std::string ns = nsDir.path().filename().string();
        for (auto const& f : std::filesystem::recursive_directory_iterator(themeDir, ec)) {
            if (!f.is_regular_file() || f.path().extension() != ".json") continue;
            auto rel = std::filesystem::relative(f.path(), themeDir).replace_extension().generic_string();
            Identifier    id = ns + ":" + rel;
            std::ifstream in(f.path());
            auto          j = nlohmann::json::parse(in, nullptr, false);
            if (!j.is_object()) {
                logger().warn("Jade: cannot read theme: {}", id);
                continue;
            }
            int version = j.value("version", 0);
            if (version < 200 || version > 299) {
                logger().warn("Jade: skipping theme {} with unsupported version {}", id, version);
                continue;
            }
            auto theme = Theme::fromJson(j);
            if (!theme) {
                logger().warn("Jade: theme {} has no tooltipStyle", id);
                continue;
            }
            theme->id    = id;
            Theme* raw   = theme.get();
            mThemes[id]  = std::move(theme);
            bool existed = std::find(existingKeys.begin(), existingKeys.end(), id) != existingKeys.end();
            if (!enable && j.value("autoEnable", false) && !existed) enable = raw;
        }
    }
    auto it = mThemes.find(JadeIds::DEFAULT_THEME);
    if (it != mThemes.end()) {
        mFallback = it->second.get();
    } else {
        logger().error("Jade: default theme {} is missing", JadeIds::DEFAULT_THEME);
        mEmergency     = std::make_unique<Theme>();
        mEmergency->id = JadeIds::DEFAULT_THEME;
        mFallback      = mEmergency.get();
    }
    int hash = 0;
    for (auto const& [id, _] : mThemes) hash = 31 * hash + javaHash(id);
    auto& store  = config::ConfigStore::get();
    auto& config = store.config().overlay();
    if (hash != store.history().themesHash) {
        if (hash != 0 && enable) {
            config.activeTheme = enable->fullId();
            logger().info("Jade: theme {} enabled automatically", enable->fullId());
            if (enable->changeOpacity != 0) config.setAlpha(enable->changeOpacity);
        }
        store.history().themesHash = hash;
        store.saveConfig();
    }
    config.applyTheme(config.activeTheme);
    logger().info("Jade: {} themes (active: {})", mThemes.size(), config.activeTheme);
}

Theme& ThemeHelper::theme() {
    if (mThemeOverride) return *mThemeOverride;
    if (mTheme) return *mTheme;
    if (mFallback) return *mFallback;
    if (!mEmergency) {
        mEmergency     = std::make_unique<Theme>();
        mEmergency->id = JadeIds::DEFAULT_THEME;
    }
    return *mEmergency;
}

Theme& ThemeHelper::getTheme(Identifier const& id) {
    auto it = mThemes.find(id);
    if (it != mThemes.end()) return *it->second;
    if (mFallback) return *mFallback;
    return theme();
}

std::vector<Theme*> ThemeHelper::getThemes() {
    std::vector<Theme*> out;
    for (auto const& [_, t] : mThemes) out.push_back(t.get());
    return out;
}

text::Component ThemeHelper::color(text::Component c, uint32_t color) {
    std::optional<uint32_t> first;
    bool                    uniform = true;
    bool                    seen    = false;
    for (auto const& s : c.spans()) {
        if (!seen) {
            first = s.style.color;
            seen  = true;
        } else if (s.style.color != first) {
            uniform = false;
            break;
        }
    }
    if (uniform) return c.setColor(color);
    return c.withStyle(text::Style{.color = color});
}

text::Component ThemeHelper::info(text::Component c) { return color(std::move(c), theme().text.colors.info); }
text::Component ThemeHelper::success(text::Component c) { return color(std::move(c), theme().text.colors.success); }
text::Component ThemeHelper::warning(text::Component c) { return color(std::move(c), theme().text.colors.warning); }
text::Component ThemeHelper::danger(text::Component c) { return color(std::move(c), theme().text.colors.danger); }
text::Component ThemeHelper::failure(text::Component c) { return color(std::move(c), theme().text.colors.failure); }
text::Component ThemeHelper::title(text::Component c) { return color(c.stripColor(), theme().text.colors.title); }

text::Component ThemeHelper::modName(text::Component c) {
    auto const& itemStyle  = api::config::IWailaConfig::get().formatting().getItemModNameStyle();
    auto const& themeStyle = theme().text.modNameStyle;
    return c.withStyle(themeStyle.applyTo(itemStyle));
}

std::shared_ptr<api::ui::TextElement> ThemeHelper::modNameElement(text::Component c) {
    auto e       = ui::JadeUIInternal::text(modName(std::move(c)));
    bool smaller = api::config::IWailaConfig::get().plugin().getString(JadeIds::CORE_MOD_NAME) == "SMALLER";
    e->scale(smaller ? 0.75f : 1.0f);
    return e;
}

text::Component ThemeHelper::seconds(int ticks, float tickRate, bool alwaysOnePart) {
    using text::Component;
    int s = static_cast<int>(std::floor(ticks / tickRate));
    if (s >= 3600) {
        int hours = s / 3600;
        s %= 3600;
        int minutes = s / 60;
        if (alwaysOnePart || minutes == 0) return info(Component::literal(text::I18n::formatString("jade.hours", {std::to_string(hours)})));
        return info(Component::literal(text::I18n::formatString("jade.hours_minutes", {std::to_string(hours), std::to_string(minutes)})));
    }
    if (s >= 60) {
        int minutes = s / 60;
        s %= 60;
        if (alwaysOnePart || s == 0) return info(Component::literal(text::I18n::formatString("jade.minutes", {std::to_string(minutes)})));
        return info(Component::literal(text::I18n::formatString("jade.minutes_seconds", {std::to_string(minutes), std::to_string(s)})));
    }
    return info(Component::literal(text::I18n::formatString("jade.seconds", {std::to_string(s)})));
}

void ThemeHelper::setTheme(Theme* theme) {
    if (mTheme == theme) return;
    mTheme = theme;
    ++mGeneration;
}

void ThemeHelper::setThemeOverride(Theme* theme) {
    if (mThemeOverride == theme) return;
    ++mGeneration;
    mThemeOverride = theme;
}

}
