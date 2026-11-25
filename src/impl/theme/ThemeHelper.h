// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <filesystem>
#include <map>
#include <memory>

#include "api/theme/Theme.h"

namespace jade::impl::theme {

using api::theme::Theme;

class ThemeHelper : public api::theme::IThemeHelper {
public:
    static ThemeHelper INSTANCE;

    void load(std::filesystem::path const& assetsDir);

    Theme&              theme() override;
    Theme&              getTheme(Identifier const& id) override;
    bool                hasTheme(Identifier const& id) override { return mThemes.contains(id); }
    std::vector<Theme*> getThemes() override;

    text::Component info(text::Component c) override;
    text::Component success(text::Component c) override;
    text::Component warning(text::Component c) override;
    text::Component danger(text::Component c) override;
    text::Component failure(text::Component c) override;
    text::Component title(text::Component c) override;
    text::Component modName(text::Component c) override;
    using IThemeHelper::danger;
    using IThemeHelper::failure;
    using IThemeHelper::info;
    using IThemeHelper::modName;
    using IThemeHelper::success;
    using IThemeHelper::title;
    using IThemeHelper::warning;

    std::shared_ptr<api::ui::TextElement> modNameElement(text::Component c) override;
    text::Component                       seconds(int ticks, float tickRate, bool alwaysOnePart) override;

    int  generation() override { return mGeneration; }
    void setTheme(Theme* theme);
    void setThemeOverride(Theme* theme) override;

private:
    text::Component color(text::Component c, uint32_t color);

    std::map<Identifier, std::unique_ptr<Theme>> mThemes;
    Theme*                                       mTheme         = nullptr;
    Theme*                                       mFallback      = nullptr;
    Theme*                                       mThemeOverride = nullptr;
    int                                          mGeneration    = 0;
    std::unique_ptr<Theme>                       mEmergency;
};

}
