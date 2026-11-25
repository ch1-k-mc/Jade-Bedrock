// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

#include "api/JadeIds.h"
#include "platform/text/Component.h"

namespace jade::api::theme {
class Theme;
}

namespace jade::api::config {

template <class E>
struct EnumNames;

#define JADE_ENUM_NAMES(E, ...)                                                                                                   \
    template <>                                                                                                                   \
    struct EnumNames<E> {                                                                                                         \
        static constexpr auto values = std::to_array<std::string_view>({__VA_ARGS__});                                            \
    };

template <class E>
std::string_view enumName(E e) {
    return EnumNames<E>::values[static_cast<size_t>(e)];
}
template <class E>
std::optional<E> enumFromName(std::string_view name) {
    auto const& v = EnumNames<E>::values;
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == name) return static_cast<E>(i);
    return std::nullopt;
}

enum class IconMode { TOP, CENTERED, INLINE, HIDE };
enum class TTSMode { TOGGLE, PRESS };
enum class DisplayMode { HOLD_KEY, TOGGLE, LITE };
enum class FluidMode { NONE, ANY, FALLBACK };
enum class BossBarOverlapMode { NO_OPERATION, HIDE_BOSS_BAR, HIDE_TOOLTIP, PUSH_DOWN };
enum class PerspectiveMode { CAMERA, EYE };
enum class HandlerDisplayStyle { PROGRESS_BAR, ICON, PLAIN_TEXT };

JADE_ENUM_NAMES(IconMode, "TOP", "CENTERED", "INLINE", "HIDE")
JADE_ENUM_NAMES(TTSMode, "TOGGLE", "PRESS")
JADE_ENUM_NAMES(DisplayMode, "HOLD_KEY", "TOGGLE", "LITE")
JADE_ENUM_NAMES(FluidMode, "NONE", "ANY", "FALLBACK")
JADE_ENUM_NAMES(BossBarOverlapMode, "NO_OPERATION", "HIDE_BOSS_BAR", "HIDE_TOOLTIP", "PUSH_DOWN")
JADE_ENUM_NAMES(PerspectiveMode, "CAMERA", "EYE")
JADE_ENUM_NAMES(HandlerDisplayStyle, "PROGRESS_BAR", "ICON", "PLAIN_TEXT")

class IPluginConfig {
public:
    virtual ~IPluginConfig() = default;

    static bool       isPrimaryKey(Identifier const& key) { return key.substr(key.find(':') + 1).find('.') == std::string::npos; }
    static Identifier getPrimaryKey(Identifier const& key) { return key.substr(0, key.find('.', key.find(':') + 1)); }

    virtual bool        get(Identifier const& key)       = 0;
    virtual int         getInt(Identifier const& key)    = 0;
    virtual float       getFloat(Identifier const& key)  = 0;
    virtual std::string getString(Identifier const& key) = 0;
    template <class E>
    E getEnum(Identifier const& key) {
        return enumFromName<E>(getString(key)).value_or(static_cast<E>(0));
    }
    virtual bool                  set(Identifier const& key, nlohmann::json value) = 0;
    virtual nlohmann::json const& values() const                                   = 0;
};

uint32_t applyAlpha(uint32_t color, float alpha);

text::Style    styleFromJson(nlohmann::json const& j);
nlohmann::json styleToJson(text::Style const& s);

class IWailaConfig {
public:
    class General {
    public:
        void        setDisplayTooltip(bool v) { displayTooltip = v; }
        bool        getDisplayEntities() const { return displayEntities; }
        void        setDisplayEntities(bool v) { displayEntities = v; }
        bool        getDisplayBosses() const { return displayBosses; }
        void        setDisplayBosses(bool v) { displayBosses = v; }
        bool        getDisplayBlocks() const { return displayBlocks; }
        void        setDisplayBlocks(bool v) { displayBlocks = v; }
        void        setHideFromTabList(bool v) { hideFromTabList = v; }
        void        setHideFromGUIs(bool v) { hideFromGUIs = v; }
        void        setItemModNameTooltip(bool v) { itemModNameTooltip = v; }
        bool        shouldDisplayTooltip() const { return displayTooltip; }
        DisplayMode getDisplayMode() const { return displayMode; }
        void        setDisplayMode(DisplayMode v) { displayMode = v; }
        bool        shouldHideFromTabList() const { return hideFromTabList; }
        bool        shouldHideFromGUIs() const { return hideFromGUIs; }
        bool        shouldDisplayFluids() const { return fluidMode != FluidMode::NONE; }
        FluidMode   getDisplayFluids() const { return fluidMode; }
        void        setDisplayFluids(bool v) { fluidMode = v ? FluidMode::ANY : FluidMode::NONE; }
        void        setDisplayFluids(FluidMode v) { fluidMode = v; }
        bool        showItemModNameTooltip() const { return itemModNameTooltip; }
        float       getExtendedReach() const { return extendedReach; }
        void        setExtendedReach(float v);
        BossBarOverlapMode getBossBarOverlapMode() const { return bossBarOverlapMode; }
        void               setBossBarOverlapMode(BossBarOverlapMode v) { bossBarOverlapMode = v; }
        bool               isDebug() const { return debug; }
        void               setDebug(bool v) { debug = v; }
        bool               getBuiltinCamouflage() const { return builtinCamouflage; }
        void               setBuiltinCamouflage(bool v) { builtinCamouflage = v; }
        PerspectiveMode    getPerspectiveMode() const { return perspectiveMode; }
        void               setPerspectiveMode(PerspectiveMode v) { perspectiveMode = v; }

        void           fromJson(nlohmann::json const& j);
        nlohmann::json toJson() const;

    private:
        bool               displayTooltip     = true;
        bool               displayBlocks      = true;
        bool               displayEntities    = true;
        bool               displayBosses      = true;
        DisplayMode        displayMode        = DisplayMode::TOGGLE;
        FluidMode          fluidMode          = FluidMode::ANY;
        PerspectiveMode    perspectiveMode    = PerspectiveMode::CAMERA;
        float              extendedReach      = 0;
        bool               debug              = false;
        bool               itemModNameTooltip = true;
        BossBarOverlapMode bossBarOverlapMode = BossBarOverlapMode::PUSH_DOWN;
        bool               builtinCamouflage  = true;
        bool               hideFromTabList    = true;
        bool               hideFromGUIs       = true;
    };

    class Overlay {
    public:
        float getOverlayPosX() const;
        void  setOverlayPosX(float v) { overlayPosX = v; }
        float getOverlayPosY() const;
        void  setOverlayPosY(float v) { overlayPosY = v; }
        float getOverlayScale() const { return overlayScale; }
        void  setOverlayScale(float v);
        float getAnchorX() const;
        void  setAnchorX(float v) { overlayAnchorX = v; }
        float getAnchorY() const;
        void  setAnchorY(float v) { overlayAnchorY = v; }
        float getAutoScaleThreshold() const { return autoScaleThreshold; }
        float getAlpha() const { return alpha; }
        void  setAlpha(float v);
        theme::Theme&     getTheme();
        void              applyTheme(Identifier const& id);
        Identifier const& getActiveTheme() const { return activeTheme; }
        bool              shouldShowIcon() const { return iconMode != IconMode::HIDE; }
        IconMode          getIconMode() const { return iconMode; }
        void              setIconMode(IconMode v) { iconMode = v; }
        bool              getAnimation() const { return animation; }
        void              setAnimation(bool v) { animation = v; }
        float             getDisappearingDelay() const { return disappearingDelay; }
        void              setDisappearingDelay(float v) { disappearingDelay = v < 0 ? 0 : v; }

        void           fromJson(nlohmann::json const& j);
        nlohmann::json toJson() const;

        Identifier activeTheme = JadeIds::DEFAULT_THEME;

    private:
        float         overlayPosX        = 0.5f;
        float         overlayPosY        = 1.0f;
        float         overlayScale       = 1.0f;
        float         overlayAnchorX     = 0.5f;
        float         overlayAnchorY     = 0.0f;
        float         autoScaleThreshold = 0.4f;
        float         alpha              = 0.7f;
        IconMode      iconMode           = IconMode::TOP;
        bool          animation          = true;
        float         disappearingDelay  = 0;
        theme::Theme* activeThemeInstance = nullptr;
    };

    class Formatting {
    public:
        text::Style const& getItemModNameStyle() const { return itemModNameStyle; }
        void               setItemModNameStyle(text::Style s) { itemModNameStyle = s; }
        text::Component    registryName(std::string const& name) const;

        void           fromJson(nlohmann::json const& j);
        nlohmann::json toJson() const;

    private:
        text::Style itemModNameStyle{.color = 0xFF5555FF, .italic = true};
    };

    class Accessibility {
    public:
        bool    shouldEnableTextToSpeech() const { return enableTextToSpeech; }
        void    toggleTTS() { enableTextToSpeech = !enableTextToSpeech; }
        TTSMode getTTSMode() const { return ttsMode; }
        void    setTTSMode(TTSMode v) { ttsMode = v; }
        bool    getEnableAccessibilityPlugin() const { return enableAccessibilityPlugin; }
        void    setEnableAccessibilityPlugin(bool v) { enableAccessibilityPlugin = v; }
        bool    getAlwaysListMode() const { return alwaysListMode; }
        void    setAlwaysListMode(bool v) { alwaysListMode = v; }
        bool    getFlipMainHand() const { return flipMainHand; }
        void    setFlipMainHand(bool v) { flipMainHand = v; }
        float   tryFlip(float f) const;
        void    setTextBackgroundOpacity(float v);
        float   getTextBackgroundOpacity() const { return textBackgroundOpacity; }
        void    setNarrateKeys(bool v) { narrateKeys = v; }
        bool    getNarrateKeys() const { return narrateKeys; }

        void           fromJson(nlohmann::json const& j);
        nlohmann::json toJson() const;

    private:
        bool    enableTextToSpeech        = false;
        TTSMode ttsMode                   = TTSMode::TOGGLE;
        bool    enableAccessibilityPlugin = false;
        float   textBackgroundOpacity     = 0;
        bool    flipMainHand              = false;
        bool    narrateKeys               = false;
        bool    alwaysListMode            = false;
    };

    virtual ~IWailaConfig() = default;

    static IWailaConfig& get();

    virtual General&       general()       = 0;
    virtual Overlay&       overlay()       = 0;
    virtual Formatting&    formatting()    = 0;
    virtual Accessibility& accessibility() = 0;
    virtual IPluginConfig& plugin()        = 0;
    virtual void           save()          = 0;
    virtual void           invalidate()    = 0;
    virtual std::string    getName()       = 0;
    virtual void           setName(std::string name) = 0;
};

}
