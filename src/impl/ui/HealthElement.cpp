// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/ui/HealthElement.h"

#include <cmath>

#include "api/config/IWailaConfig.h"
#include "api/theme/IThemeHelper.h"
#include "overlay/DisplayHelper.h"
#include "overlay/OverlayRenderer.h"
#include "overlay/WailaTickHandler.h"
#include "platform/render/GuiGraphics.h"
#include "platform/text/Component.h"
#include "track/HealthTrackInfo.h"
#include "util/JadeFont.h"

namespace jade::impl::ui {

using overlay::DisplayHelper;

namespace {

int ceilI(float v) { return static_cast<int>(std::ceil(v)); }
int floorI(float v) { return static_cast<int>(std::floor(v)); }

std::string heartSprite(HeartType type, bool half, bool blinking) {
    std::string base;
    switch (type) {
    case HeartType::CONTAINER: return blinking ? "game:textures/ui/heart_blink" : "game:textures/ui/heart_background";
    case HeartType::FROZEN: base = "game:textures/ui/freeze_heart"; break;
    case HeartType::ABSORBING: return half ? "game:textures/ui/absorption_heart_half" : "game:textures/ui/absorption_heart";
    default: base = "game:textures/ui/heart"; break;
    }
    if (blinking) base += "_flash";
    if (half) base += "_half";
    return base;
}

void blit(api::ui::GuiGraphics& graphics, std::string const& tex, int x, int y) {
    uint32_t color = api::config::applyAlpha(0xFFFFFFFFu, DisplayHelper::INSTANCE.opacity());
    graphics.blit(tex, float(x), float(y), 9, 9, 0, 0, 1, 1, color);
}

}

HealthElement::HealthElement(HeartType heartType, float maxHealth, float health, float absorption)
: mHeartType(heartType),
  mMaxHealth(maxHealth),
  mHealth(health),
  mAbsorption(absorption) {
    auto& config    = api::config::IWailaConfig::get().plugin();
    int   iconCount = ceilI(maxHealth) + ceilI(absorption);
    if (iconCount > config.getInt(JadeIds::MC_ENTITY_HEALTH_MAX_FOR_RENDER)) {
        health += absorption;
        if (!config.get(JadeIds::MC_ENTITY_HEALTH_SHOW_FRACTIONS)) {
            maxHealth = std::ceil(maxHealth);
            health    = std::ceil(health);
        }
        if (absorption > 0) mHeartType = HeartType::ABSORBING;
        mText = DisplayHelper::formatNumber(health) + "/" + DisplayHelper::formatNumber(maxHealth);
    } else {
        int maxHeartsPerLine = config.getInt(JadeIds::MC_ENTITY_HEALTH_ICONS_PER_LINE);
        if (maxHeartsPerLine <= 0) maxHeartsPerLine = 10;
        iconCount     = ceilI(float(iconCount) * 0.5f);
        mIconCount    = iconCount;
        mIconsPerLine = std::min(maxHeartsPerLine, iconCount);
        mLineCount    = ceilI(float(iconCount) / float(maxHeartsPerLine));
    }
    if (showText()) {
        width  = util::JadeFont::width(*mText) + 10;
        height = 9;
    } else {
        width  = 8 * mIconsPerLine + 1;
        height = 5 + 4 * mLineCount;
    }
}

void HealthElement::render(api::ui::GuiGraphics& graphics, int, int, float) {
    float health         = std::round(mHealth);
    float hearts         = health * 0.5f;
    float lastHearts     = hearts;
    float lastAbsorption = mAbsorption;
    bool  blinking       = false;
    if (!mTrack && getTag()) {
        float a = mAbsorption;
        mTrack  = overlay::WailaTickHandler::instance().progressTracker.getOrCreate<track::HealthTrackInfo>(*getTag(), [&] {
            return std::make_unique<track::HealthTrackInfo>(health, a);
        });
    }
    if (mTrack) {
        mTrack->setHealth(health, mAbsorption);
        mTrack->update(overlay::OverlayRenderer::realtimeDeltaTicks);
        lastHearts     = mTrack->getLastHealth() * 0.5f;
        lastAbsorption = mTrack->getLastAbsorption();
        blinking       = mTrack->isBlinking();
    }
    int         xOffset   = (mIconCount - 1) % mIconsPerLine * 8;
    int         yOffset   = mLineCount * 4 - 4;
    std::string container = heartSprite(HeartType::CONTAINER, false, blinking);
    for (int i = mIconCount; i > 0; --i) {
        int xPos = getX() + xOffset;
        int yPos = getY() + yOffset;
        blit(graphics, container, xPos, yPos);
        bool      renderAbsorb  = i > ceilI(mMaxHealth * 0.5f);
        HeartType curType       = mHeartType;
        float     curHearts     = hearts;
        float     curLastHearts = lastHearts;
        if (renderAbsorb) {
            curType       = HeartType::ABSORBING;
            curHearts     = (std::ceil(mMaxHealth) + mAbsorption) * 0.5f;
            curLastHearts = (std::ceil(mMaxHealth) + lastAbsorption) * 0.5f;
        }
        if (i <= floorI(curHearts)) blit(graphics, heartSprite(curType, false, false), xPos, yPos);
        if (float(i) > curHearts) {
            if (i <= floorI(curLastHearts)) blit(graphics, heartSprite(curType, false, true), xPos, yPos);
            else if (float(i) > curLastHearts && float(i) < curLastHearts + 1) blit(graphics, heartSprite(curType, true, true), xPos, yPos);
            if (float(i) < curHearts + 1) blit(graphics, heartSprite(curType, true, false), xPos, yPos);
        }
        xOffset -= 8;
        if (xOffset < 0) {
            xOffset = mIconsPerLine * 8 - 8;
            yOffset -= 4;
        }
    }
    if (showText()) DisplayHelper::INSTANCE.drawText(graphics, *mText, float(getX() + 10), float(getY() + 1), api::theme::IThemeHelper::get().getNormalColor());
}

std::string HealthElement::getNarration() {
    return text::Component::translatable("narration.jade.health", std::vector<std::string>{std::to_string(ceilI(mHealth))}).getString();
}

ArmorElement::ArmorElement(float armor) : mArmor(armor) {
    auto& config = api::config::IWailaConfig::get().plugin();
    if (armor > float(config.getInt(JadeIds::MC_ENTITY_ARMOR_MAX_FOR_RENDER))) {
        if (!config.get(JadeIds::MC_ENTITY_HEALTH_SHOW_FRACTIONS)) armor = std::ceil(armor);
        mText = DisplayHelper::formatNumber(armor);
    } else {
        armor *= 0.5f;
        int maxPerLine = config.getInt(JadeIds::MC_ENTITY_HEALTH_ICONS_PER_LINE);
        if (maxPerLine <= 0) maxPerLine = 10;
        mIconCount    = ceilI(armor);
        mIconsPerLine = std::min(maxPerLine, mIconCount);
        mLineCount    = ceilI(armor / float(maxPerLine));
    }
    if (showText()) {
        width  = util::JadeFont::width(*mText) + 10;
        height = 9;
    } else {
        width  = 8 * mIconsPerLine + 1;
        height = 5 + 4 * mLineCount;
    }
}

void ArmorElement::render(api::ui::GuiGraphics& graphics, int, int, float) {
    int x = getX(), y = getY();
    int xOffset = (mIconCount - 1) % mIconsPerLine * 8;
    int yOffset = mLineCount * 4 - 4;
    for (int i = mIconCount; i > 0; --i) {
        blit(graphics, "game:textures/ui/armor_empty", x + xOffset, y + yOffset);
        if (i <= floorI(mArmor)) blit(graphics, "game:textures/ui/armor_full", x + xOffset, y + yOffset);
        if (float(i) > mArmor && float(i) < mArmor + 1) blit(graphics, "game:textures/ui/armor_half", x + xOffset, y + yOffset);
        xOffset -= 8;
        if (xOffset < 0) {
            xOffset = mIconsPerLine * 8 - 8;
            yOffset -= 4;
        }
    }
    if (showText()) DisplayHelper::INSTANCE.drawText(graphics, *mText, float(x + 10), float(y + 1), api::theme::IThemeHelper::get().getNormalColor());
}

std::string ArmorElement::getNarration() {
    return text::Component::translatable("narration.jade.armor", std::vector<std::string>{std::to_string(ceilI(mArmor))}).getString();
}

}
