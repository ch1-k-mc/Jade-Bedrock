// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/config/WailaConfig.h"

#include <algorithm>
#include <fstream>

#include "ll/api/mod/NativeMod.h"

#include "api/theme/IThemeHelper.h"
#include "api/theme/Theme.h"
#include "impl/WailaClientRegistration.h"
#include "impl/config/entry/ConfigEntry.h"
#include "impl/theme/ThemeHelper.h"

namespace jade::api::config {

namespace {

float clampf(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }

template <class E>
E readEnum(nlohmann::json const& j, char const* key, E def) {
    if (!j.contains(key) || !j[key].is_string()) return def;
    return enumFromName<E>(j[key].get<std::string>()).value_or(def);
}

template <class T>
T readOr(nlohmann::json const& j, char const* key, T def) {
    if (!j.contains(key)) return def;
    try {
        return j[key].get<T>();
    } catch (...) {
        return def;
    }
}

struct NamedColor {
    char const* name;
    uint32_t    rgb;
};
constexpr NamedColor kChatColors[] = {
    {"black", 0x000000},
    {"dark_blue", 0x0000AA},
    {"dark_green", 0x00AA00},
    {"dark_aqua", 0x00AAAA},
    {"dark_red", 0xAA0000},
    {"dark_purple", 0xAA00AA},
    {"gold", 0xFFAA00},
    {"gray", 0xAAAAAA},
    {"dark_gray", 0x555555},
    {"blue", 0x5555FF},
    {"green", 0x55FF55},
    {"aqua", 0x55FFFF},
    {"red", 0xFF5555},
    {"light_purple", 0xFF55FF},
    {"yellow", 0xFFFF55},
    {"white", 0xFFFFFF},
};

}

text::Style styleFromJson(nlohmann::json const& j) {
    text::Style s;
    if (!j.is_object()) return s;
    if (j.contains("color") && j["color"].is_string()) {
        std::string c = j["color"].get<std::string>();
        if (c.starts_with("#") && c.size() == 7) {
            s.color = 0xFF000000u | static_cast<uint32_t>(std::stoul(c.substr(1), nullptr, 16));
        } else {
            for (auto const& nc : kChatColors)
                if (c == nc.name) s.color = 0xFF000000u | nc.rgb;
        }
    }
    auto flag = [&](char const* key, std::optional<bool>& out) {
        if (j.contains(key) && j[key].is_boolean()) out = j[key].get<bool>();
    };
    flag("bold", s.bold);
    flag("italic", s.italic);
    flag("underlined", s.underlined);
    flag("strikethrough", s.strikethrough);
    flag("obfuscated", s.obfuscated);
    return s;
}

nlohmann::json styleToJson(text::Style const& s) {
    nlohmann::json j = nlohmann::json::object();
    if (s.color) {
        uint32_t rgb  = *s.color & 0xFFFFFF;
        bool     done = false;
        for (auto const& nc : kChatColors) {
            if (nc.rgb == rgb) {
                j["color"] = nc.name;
                done       = true;
                break;
            }
        }
        if (!done) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "#%06X", rgb);
            j["color"] = buf;
        }
    }
    if (s.bold) j["bold"] = *s.bold;
    if (s.italic) j["italic"] = *s.italic;
    if (s.underlined) j["underlined"] = *s.underlined;
    if (s.strikethrough) j["strikethrough"] = *s.strikethrough;
    if (s.obfuscated) j["obfuscated"] = *s.obfuscated;
    return j;
}

uint32_t applyAlpha(uint32_t color, float alpha) {
    if (alpha == 1) return color;
    if (alpha == 0) return 0;
    uint32_t prevAlpha = (color >> 24) & 0xFF;
    if (prevAlpha != 255) alpha *= static_cast<float>(prevAlpha) / 255.0f;
    uint32_t a = static_cast<uint32_t>(std::lround(clampf(alpha, 0, 1) * 255.0f));
    return (a << 24) | (color & 0x00FFFFFF);
}

void IWailaConfig::General::setExtendedReach(float v) { extendedReach = clampf(v, 0, 20); }

void IWailaConfig::General::fromJson(nlohmann::json const& j) {
    displayTooltip     = readOr(j, "displayTooltip", true);
    displayBlocks      = readOr(j, "displayBlocks", true);
    displayEntities    = readOr(j, "displayEntities", true);
    displayBosses      = readOr(j, "displayBosses", true);
    displayMode        = readEnum(j, "displayMode", DisplayMode::TOGGLE);
    fluidMode          = readEnum(j, "fluidMode", FluidMode::ANY);
    perspectiveMode    = readEnum(j, "perspectiveMode", PerspectiveMode::CAMERA);
    extendedReach      = clampf(readOr(j, "extendedReach", 0.0f), 0, 20);
    debug              = readOr(j, "debug", false);
    itemModNameTooltip = readOr(j, "itemModNameTooltip", true);
    bossBarOverlapMode = readEnum(j, "bossBarOverlapMode", BossBarOverlapMode::PUSH_DOWN);
    builtinCamouflage  = readOr(j, "builtinCamouflage", true);
    hideFromTabList    = readOr(j, "hideFromTabList", true);
    hideFromGUIs       = readOr(j, "hideFromGUIs", true);
}

nlohmann::json IWailaConfig::General::toJson() const {
    return {
        {"displayTooltip", displayTooltip},
        {"displayBlocks", displayBlocks},
        {"displayEntities", displayEntities},
        {"displayBosses", displayBosses},
        {"displayMode", enumName(displayMode)},
        {"fluidMode", enumName(fluidMode)},
        {"perspectiveMode", enumName(perspectiveMode)},
        {"extendedReach", extendedReach},
        {"debug", debug},
        {"itemModNameTooltip", itemModNameTooltip},
        {"bossBarOverlapMode", enumName(bossBarOverlapMode)},
        {"builtinCamouflage", builtinCamouflage},
        {"hideFromTabList", hideFromTabList},
        {"hideFromGUIs", hideFromGUIs},
    };
}

float IWailaConfig::Overlay::getOverlayPosX() const { return clampf(overlayPosX, 0, 1); }
float IWailaConfig::Overlay::getOverlayPosY() const { return clampf(overlayPosY, 0, 1); }
float IWailaConfig::Overlay::getAnchorX() const { return clampf(overlayAnchorX, 0, 1); }
float IWailaConfig::Overlay::getAnchorY() const { return clampf(overlayAnchorY, 0, 1); }
void  IWailaConfig::Overlay::setOverlayScale(float v) { overlayScale = clampf(v, 0.2f, 2.0f); }
void  IWailaConfig::Overlay::setAlpha(float v) { alpha = clampf(v, 0, 1); }

theme::Theme& IWailaConfig::Overlay::getTheme() {
    if (!activeThemeInstance) applyTheme(activeTheme);
    return *activeThemeInstance;
}

void IWailaConfig::Overlay::applyTheme(Identifier const& id) {
    auto& helper        = impl::theme::ThemeHelper::INSTANCE;
    activeThemeInstance = &helper.getTheme(id);
    activeTheme         = activeThemeInstance->fullId();
    helper.setTheme(activeThemeInstance);
}

void IWailaConfig::Overlay::fromJson(nlohmann::json const& j) {
    activeTheme        = readOr<std::string>(j, "activeTheme", JadeIds::DEFAULT_THEME);
    overlayPosX        = readOr(j, "overlayPosX", 0.5f);
    overlayPosY        = readOr(j, "overlayPosY", 1.0f);
    overlayScale       = clampf(readOr(j, "overlayScale", 1.0f), 0.2f, 2.0f);
    overlayAnchorX     = readOr(j, "overlayAnchorX", 0.5f);
    overlayAnchorY     = readOr(j, "overlayAnchorY", 0.0f);
    autoScaleThreshold = clampf(readOr(j, "autoScaleThreshold", 0.4f), 0, 1);
    alpha              = clampf(readOr(j, "alpha", 0.7f), 0, 1);
    iconMode           = readEnum(j, "iconMode", IconMode::TOP);
    animation          = readOr(j, "animation", true);
    disappearingDelay  = std::max(0.0f, readOr(j, "disappearingDelay", 0.0f));
    activeThemeInstance = nullptr;
}

nlohmann::json IWailaConfig::Overlay::toJson() const {
    return {
        {"activeTheme", activeTheme},
        {"overlayPosX", overlayPosX},
        {"overlayPosY", overlayPosY},
        {"overlayScale", overlayScale},
        {"overlayAnchorX", overlayAnchorX},
        {"overlayAnchorY", overlayAnchorY},
        {"autoScaleThreshold", autoScaleThreshold},
        {"alpha", alpha},
        {"iconMode", enumName(iconMode)},
        {"animation", animation},
        {"disappearingDelay", disappearingDelay},
    };
}

text::Component IWailaConfig::Formatting::registryName(std::string const& name) const {
    bool light = theme::IThemeHelper::get().isLightColorScheme();
    return text::Component::literal(name, text::Style{.color = light ? 0xFF555555u : 0xFFAAAAAAu});
}

void IWailaConfig::Formatting::fromJson(nlohmann::json const& j) {
    if (j.contains("itemModNameStyle")) itemModNameStyle = styleFromJson(j["itemModNameStyle"]);
}

nlohmann::json IWailaConfig::Formatting::toJson() const { return {{"itemModNameStyle", styleToJson(itemModNameStyle)}}; }

float IWailaConfig::Accessibility::tryFlip(float f) const {
    return f;
}

void IWailaConfig::Accessibility::setTextBackgroundOpacity(float v) { textBackgroundOpacity = v; }

void IWailaConfig::Accessibility::fromJson(nlohmann::json const& j) {
    enableTextToSpeech        = readOr(j, "enableTextToSpeech", false);
    ttsMode                   = readEnum(j, "ttsMode", TTSMode::TOGGLE);
    enableAccessibilityPlugin = readOr(j, "enableAccessibilityPlugin", false);
    textBackgroundOpacity     = clampf(readOr(j, "textBackgroundOpacity", 0.0f), 0, 1);
    flipMainHand              = readOr(j, "flipMainHand", false);
    narrateKeys               = readOr(j, "narrateKeys", false);
    alwaysListMode            = readOr(j, "alwaysListMode", false);
}

nlohmann::json IWailaConfig::Accessibility::toJson() const {
    return {
        {"enableTextToSpeech", enableTextToSpeech},
        {"ttsMode", enumName(ttsMode)},
        {"enableAccessibilityPlugin", enableAccessibilityPlugin},
        {"textBackgroundOpacity", textBackgroundOpacity},
        {"flipMainHand", flipMainHand},
        {"narrateKeys", narrateKeys},
        {"alwaysListMode", alwaysListMode},
    };
}

IWailaConfig& IWailaConfig::get() { return impl::config::ConfigStore::get().config(); }

}

namespace jade::impl::config {

namespace {
auto& logger() { return ll::mod::NativeMod::current()->getLogger(); }
}

nlohmann::json const& ClientPluginConfig::merged(Identifier const& key) {
    static nlohmann::json const kNull;
    auto                        it = mMergedValues.find(key);
    return it == mMergedValues.end() ? kNull : *it;
}

bool ClientPluginConfig::get(Identifier const& key) {
    auto const& v = merged(key);
    return v.is_boolean() ? v.get<bool>() : false;
}

int ClientPluginConfig::getInt(Identifier const& key) {
    auto const& v = merged(key);
    return v.is_number() ? static_cast<int>(v.get<double>()) : 0;
}

float ClientPluginConfig::getFloat(Identifier const& key) {
    auto const& v = merged(key);
    return v.is_number() ? v.get<float>() : 0.0f;
}

std::string ClientPluginConfig::getString(Identifier const& key) {
    auto const& v = merged(key);
    return v.is_string() ? v.get<std::string>() : std::string();
}

bool ClientPluginConfig::set(Identifier const& key, nlohmann::json value) {
    auto const* entry = WailaClientRegistration::instance().getConfigEntry(key);
    if (!entry) {
        logger().warn("Jade: ignoring unknown config entry: {}", key);
        return false;
    }
    try {
        value = entry->convertValue(value);
    } catch (...) {
        logger().warn("Jade: wrong value type for config entry: {} = {}", key, value.dump());
        return false;
    }
    if (!entry->isValidValue(value)) {
        logger().warn("Jade: config value out of range: {} = {}", key, value.dump());
        return false;
    }
    bool changed = !mValues.contains(key) || mValues[key] != value;
    mValues[key] = value;
    if (changed) ensureEntry(*entry);
    return true;
}

void ClientPluginConfig::ensureEntry(entry::ConfigEntry const& entry) {
    Identifier const& key = entry.id();
    nlohmann::json    value;
    if (!mValues.contains(key)) {
        value = mValues[key] = entry.defaultValue();
    } else {
        try {
            value = entry.convertValue(mValues[key]);
            if (!entry.isValidValue(value)) value = entry.defaultValue();
        } catch (...) {
            value = entry.defaultValue();
        }
        mValues[key] = value;
    }
    if (entry.isSynced()) value = entry.syncedValue();
    bool changed        = !mMergedValues.contains(key) || mMergedValues[key] != value;
    mMergedValues[key]  = value;
    if (changed) entry.notifyChange();
}

void ClientPluginConfig::fromJson(nlohmann::json const& j) {
    mValues       = j.is_object() ? j : nlohmann::json::object();
    mMergedValues = mValues;
}

void WailaConfig::save() { ConfigStore::get().saveConfig(); }
void WailaConfig::invalidate() { ConfigStore::get().invalidateConfig(); }

void WailaConfig::fixData() {
    for (auto const& [id, entry] : WailaClientRegistration::instance().configEntries) mPlugin.ensureEntry(*entry);
}

void WailaConfig::fromJson(nlohmann::json const& j) {
    mName = j.value("name", "");
    mGeneral.fromJson(j.value("general", nlohmann::json::object()));
    mOverlay.fromJson(j.value("overlay", nlohmann::json::object()));
    mFormatting.fromJson(j.value("formatting", nlohmann::json::object()));
    mAccessibility.fromJson(j.value("accessibility", nlohmann::json::object()));
    mPlugin.fromJson(j.value("plugin", nlohmann::json::object()));
}

nlohmann::json WailaConfig::toJson() const {
    return {
        {"name", mName},
        {"general", mGeneral.toJson()},
        {"overlay", mOverlay.toJson()},
        {"formatting", mFormatting.toJson()},
        {"accessibility", mAccessibility.toJson()},
        {"plugin", mPlugin.values()},
    };
}

void History::checkNewUser(int hash) {
    if (!usersHash.empty() && usersHash[0] == 0) return;
    if (std::find(usersHash.begin(), usersHash.end(), hash) != usersHash.end()) return;
    usersHash.push_back(hash);
}

void History::fromJson(nlohmann::json const& j) {
    previewOverlay         = j.value("previewOverlay", true);
    hintOverlayToggle      = j.value("hintOverlayToggle", true);
    hintNarratorToggle     = j.value("hintNarratorToggle", true);
    accessibilityModMemory = j.value("accessibilityModMemory", false);
    themesHash             = j.value("themesHash", 0);
    usersHash.clear();
    if (j.contains("usersHash") && j["usersHash"].is_array())
        for (auto const& v : j["usersHash"])
            if (v.is_number_integer()) usersHash.push_back(v.get<int>());
}

nlohmann::json History::toJson() const {
    return {
        {"previewOverlay", previewOverlay},
        {"hintOverlayToggle", hintOverlayToggle},
        {"hintNarratorToggle", hintNarratorToggle},
        {"accessibilityModMemory", accessibilityModMemory},
        {"themesHash", themesHash},
        {"usersHash", usersHash},
    };
}

void RootConfig::fromJson(nlohmann::json const& j) {
    WailaConfig::fromJson(j);
    enableProfiles = j.value("enableProfiles", false);
    profileIndex   = std::clamp(j.value("profileIndex", 0), 0, 3);
    history.fromJson(j.value("history", nlohmann::json::object()));
}

nlohmann::json RootConfig::toJson() const {
    auto j              = WailaConfig::toJson();
    j["enableProfiles"] = enableProfiles;
    j["profileIndex"]   = profileIndex;
    j["history"]        = history.toJson();
    return j;
}

ConfigStore& ConfigStore::get() {
    static ConfigStore instance;
    return instance;
}

std::filesystem::path ConfigStore::fileOf(int index) const {
    if (index == 0) return mDir / "jade" / "jade.json";
    return mDir / "jade" / "profiles" / std::to_string(index) / "jade.json";
}

WailaConfig& ConfigStore::profile(int index) {
    if (index <= 0 || index >= static_cast<int>(mProfiles.size()) || !mProfiles[index]) return *mRoot;
    return *mProfiles[index];
}

WailaConfig& ConfigStore::holder() {
    if (mRoot->isEnableProfiles() && mRoot->profileIndex > 0 && mRoot->profileIndex < static_cast<int>(mProfiles.size())) {
        return *mProfiles[mRoot->profileIndex];
    }
    return *mRoot;
}

WailaConfig& ConfigStore::config() { return holder(); }

void ConfigStore::loadIndex(int index) {
    WailaConfig&  cfg  = profile(index);
    auto          path = fileOf(index);
    std::ifstream in(path);
    if (in) {
        auto j = nlohmann::json::parse(in, nullptr, false);
        if (j.is_object()) {
            cfg.fromJson(j);
        } else {
            logger().warn("Jade: cannot read config file, using defaults: {}", path.filename().string());
        }
    }
    cfg.fixData();
}

void ConfigStore::saveIndex(int index) {
    auto            path = fileOf(index);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        logger().warn("Jade: cannot write config file: {}", path.filename().string());
        return;
    }
    out << profile(index).toJson().dump(2);
}

void ConfigStore::load(std::filesystem::path const& configDir) {
    mDir  = configDir;
    mRoot = std::make_unique<RootConfig>();
    mProfiles.clear();
    mProfiles.resize(4);
    for (int i = 1; i < 4; ++i) {
        mProfiles[i] = std::make_unique<WailaConfig>();
        if (!std::filesystem::exists(fileOf(i))) {
            auto& c = *mProfiles[i];
            c.fixData();
            if (i == 1) {
                c.setName("@jade.profile_preset.accessibility");
                c.accessibility().setEnableAccessibilityPlugin(true);
                c.accessibility().setAlwaysListMode(true);
                c.overlay().setAnimation(false);
                c.overlay().setAlpha(1);
                c.plugin().set(JadeIds::CORE_BLOCK_FACE, true);
                c.plugin().set(JadeIds::CORE_MOD_NAME, "OFF");
            } else if (i == 2) {
                c.setName("@jade.profile_preset.minimalism");
                c.general().setDisplayMode(api::config::DisplayMode::LITE);
                c.general().setBossBarOverlapMode(api::config::BossBarOverlapMode::HIDE_TOOLTIP);
                c.overlay().setAlpha(0);
                c.overlay().activeTheme = JadeIds::JADE("dark/slim");
                c.overlay().setIconMode(api::config::IconMode::INLINE);
                c.plugin().set(JadeIds::MC_BREAKING_PROGRESS, false);
                c.plugin().set(JadeIds::MC_HARVEST_TOOL, false);
                c.plugin().set(JadeIds::MC_ITEM_TOOLTIP, false);
            }
        }
    }
    for (int i = 0; i < 4; ++i) loadIndex(i);
    for (int i = 0; i < 4; ++i) saveIndex(i);
}

void ConfigStore::saveConfig() {
    int index = (mRoot->isEnableProfiles() && mRoot->profileIndex > 0) ? mRoot->profileIndex : 0;
    saveIndex(index);
    if (index != 0) saveIndex(0);
}

void ConfigStore::invalidateConfig() {
    int index = (mRoot->isEnableProfiles() && mRoot->profileIndex > 0) ? mRoot->profileIndex : 0;
    if (index == 0) {
        History keep = mRoot->history;
        mRoot->fromJson(nlohmann::json::object());
        loadIndex(0);
        mRoot->history = keep;
    } else {
        mProfiles[index] = std::make_unique<WailaConfig>();
        loadIndex(index);
    }
}

void ConfigStore::resetConfig() {
    int index      = (mRoot->isEnableProfiles() && mRoot->profileIndex > 0) ? mRoot->profileIndex : 0;
    int themesHash = history().themesHash;
    std::error_code ec;
    std::filesystem::remove(fileOf(index), ec);
    if (index == 0) {
        mRoot = std::make_unique<RootConfig>();
    } else {
        mProfiles[index] = std::make_unique<WailaConfig>();
    }
    loadIndex(index);
    history().themesHash = themesHash;
    saveConfig();
}

nlohmann::json ConfigStore::getValue(std::string const& key) {
    if (key.find(':') != std::string::npos) {
        auto const& v = config().plugin().values();
        return v.contains(key) ? v[key] : nlohmann::json();
    }
    auto j   = config().toJson();
    auto dot = key.find('.');
    if (dot == std::string::npos) return j.contains(key) ? j[key] : nlohmann::json();
    auto sec = key.substr(0, dot), k = key.substr(dot + 1);
    if (sec == "root") {
        auto r = mRoot->toJson();
        return r.contains(k) ? r[k] : nlohmann::json();
    }
    if (!j.contains(sec) || !j[sec].contains(k)) return nlohmann::json();
    return j[sec][k];
}

bool ConfigStore::setValue(std::string const& key, nlohmann::json const& value, std::string& error) {
    if (key.find(':') != std::string::npos) {
        if (!config().plugin().set(key, value)) {
            error = "unknown config entry or value out of range: " + key;
            return false;
        }
        saveConfig();
        return true;
    }
    auto dot = key.find('.');
    if (dot == std::string::npos) {
        error = "use section.key (e.g. general.displayMode)";
        return false;
    }
    auto sec = key.substr(0, dot), k = key.substr(dot + 1);
    if (sec == "root") {
        auto r = mRoot->toJson();
        if (!r.contains(k)) {
            error = "unknown key: " + key;
            return false;
        }
        r[k] = value;
        mRoot->fromJson(r);
        saveIndex(0);
        return true;
    }
    auto& cfg = config();
    auto  j   = cfg.toJson();
    if (!j.contains(sec) || !j[sec].is_object() || !j[sec].contains(k)) {
        error = "unknown key: " + key;
        return false;
    }
    j[sec][k] = value;
    if (sec == "general") cfg.general().fromJson(j[sec]);
    else if (sec == "overlay") {
        cfg.overlay().fromJson(j[sec]);
        cfg.overlay().applyTheme(cfg.overlay().activeTheme);
    } else if (sec == "formatting") cfg.formatting().fromJson(j[sec]);
    else if (sec == "accessibility") cfg.accessibility().fromJson(j[sec]);
    saveConfig();
    return true;
}

void ConfigStore::useProfile(int index) {
    mRoot->setEnableProfiles(true);
    mRoot->profileIndex = std::clamp(index, 0, 3);
    saveIndex(0);
}

void ConfigStore::saveProfile(int index) {
    if (index < 0 || index > 3) return;
    auto j = holder().toJson();
    if (index == 0) {
        auto root = mRoot->toJson();
        for (auto& [k, v] : j.items()) root[k] = v;
        mRoot->fromJson(root);
    } else {
        mProfiles[index]->fromJson(j);
    }
    saveIndex(index);
}

void ConfigStore::renameProfile(int index, std::string const& name) {
    if (index < 0 || index > 3) return;
    profile(index).setName(name);
    saveIndex(index);
}

}
