// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "api/ui/Element.h"

namespace jade::track {
class HealthTrackInfo;
}

namespace jade::impl::ui {

enum class HeartType { CONTAINER, NORMAL, FROZEN, ABSORBING };

class HealthElement : public api::ui::Element {
public:
    HealthElement(HeartType heartType, float maxHealth, float health, float absorption);

    void        render(api::ui::GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    std::string getNarration() override;
    [[nodiscard]] bool showText() const { return mText.has_value(); }
    [[nodiscard]] std::optional<std::string> const& getText() const { return mText; }

private:
    HeartType                  mHeartType;
    float                      mMaxHealth;
    float                      mHealth;
    float                      mAbsorption;
    std::optional<std::string> mText;
    int                        mIconsPerLine = 1;
    int                        mLineCount    = 1;
    int                        mIconCount    = 1;
    std::shared_ptr<track::HealthTrackInfo> mTrack;
};

class ArmorElement : public api::ui::Element {
public:
    explicit ArmorElement(float armor);

    void        render(api::ui::GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    std::string getNarration() override;
    [[nodiscard]] bool showText() const { return mText.has_value(); }

private:
    float                      mArmor;
    std::optional<std::string> mText;
    int                        mIconsPerLine = 1;
    int                        mLineCount    = 1;
    int                        mIconCount    = 1;
};

}
