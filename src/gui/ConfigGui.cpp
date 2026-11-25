// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "gui/ConfigGui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <optional>
#include <vector>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/GuiData.h"
#include "mc/deps/input/MouseAction.h"

#include "overlay/OverlayRenderer.h"

#include "api/ExternalPlugins.h"
#include "api/theme/Theme.h"
#include "impl/WailaClientRegistration.h"
#include "impl/config/WailaConfig.h"
#include "impl/config/entry/ConfigEntry.h"
#include "impl/theme/ThemeHelper.h"
#include "platform/render/GuiGraphics.h"
#include "platform/text/I18n.h"
#include "util/JadeKeys.h"

namespace jade::gui {

namespace {

using impl::config::ConfigStore;
using nlohmann::json;
using text::I18n;
using Page = ConfigGui::Page;

constexpr float ROW_H = 22, LIST_W = 320, CONTROL_W = 120, TOP = 34, BOTTOM = 46, BTN_H = 20;
constexpr int   VK_ESCAPE_ = 0x1B;

struct Row {
    enum class Kind { Title, Bool, Cycle, Slider, Key, Button, Action, Profile } kind = Kind::Title;
    std::string                                      label;
    std::string                                      key;
    std::vector<std::pair<std::string, std::string>> choices;
    float                                            min = 0, max = 1, step = 0;
    bool                                             integer    = false;
    bool                                             disabled   = false;
    bool                                             secondary  = false;
    util::KeyMapping*                                keyMapping = nullptr;
    std::function<void()>                            action;
    std::string                                      buttonText;
    std::string                                      descKey;
    std::string                                      note;
    std::function<std::string()>                     getText;
    std::function<void(std::string const&)>          setText;
    std::function<bool()>                            getBool;
    std::function<void(bool)>                        setBool;
    int                                              profile = -1;
};

struct Confirm {
    std::string           title, message, yes;
    std::function<void()> action;
};

Page                                gPage = Page::Home;
bool                                gOpen = false;
bool                                gAllowGrab = true;
bool                                gHooked    = false;
std::vector<Row>                    gRows;
std::vector<ll::event::ListenerPtr> gListeners;
float                               gScroll = 0, gMouseX = 0, gMouseY = 0, gW = 0, gH = 0;
int                                 gDragging = -1, gCapturing = -1;
std::string                         gSearch;
int                                 gFocus = -1;
std::optional<Confirm>              gConfirm;
bool                                gAdjusting = false, gAdjustDrag = false, gShift = false, gCtrl = false, gOverlayShown = false;
float                               gDragOffsetX = 0, gDragOffsetY = 0;
constexpr int                       FOCUS_SEARCH = 100;

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string tr(std::string const& key, std::string const& fallback) { return I18n::has(key) ? I18n::get(key) : fallback; }

Row title(char const* name) {
    Row r;
    r.label = I18n::get(std::string("config.jade.") + name);
    return r;
}

Row boolean(char const* name, char const* key) {
    Row r;
    r.kind    = Row::Kind::Bool;
    r.label   = I18n::get(std::string("config.jade.") + name);
    r.key     = key;
    r.descKey = std::string("config.jade.") + name + "_desc";
    return r;
}

Row cycle(char const* name, char const* key, std::vector<std::string> const& values) {
    Row r;
    r.kind    = Row::Kind::Cycle;
    r.label   = I18n::get(std::string("config.jade.") + name);
    r.key     = key;
    r.descKey = std::string("config.jade.") + name + "_desc";
    for (auto const& v : values) r.choices.emplace_back(v, tr(std::string("config.jade.") + name + "_" + lower(v), v));
    return r;
}

Row slider(char const* name, char const* key, float min, float max, float step = 0) {
    Row r;
    r.kind    = Row::Kind::Slider;
    r.label   = I18n::get(std::string("config.jade.") + name);
    r.key     = key;
    r.descKey = std::string("config.jade.") + name + "_desc";
    r.min     = min;
    r.max   = max;
    r.step  = step;
    return r;
}

Row button(std::string label, std::function<void()> action) {
    Row r;
    r.kind   = Row::Kind::Button;
    r.label  = std::move(label);
    r.action = std::move(action);
    return r;
}

Row actionRow(std::string label, std::string buttonText, std::function<void()> action) {
    Row r;
    r.kind       = Row::Kind::Action;
    r.label      = std::move(label);
    r.buttonText = std::move(buttonText);
    r.action     = std::move(action);
    return r;
}

std::string themeKey(std::string id) {
    std::replace(id.begin(), id.end(), ':', '.');
    return "jade.theme." + id;
}

void rebuildKeep();

void applyActiveTheme() {
    auto& overlay = ConfigStore::get().config().overlay();
    overlay.applyTheme(overlay.getActiveTheme());
}

void resetSettings() {
    for (auto* k : util::JadeKeys::all()) k->setKey(k->defaultKey());
    util::JadeKeys::save();
    ConfigStore::get().resetConfig();
    applyActiveTheme();
    rebuildKeep();
}

void buildGeneral() {
    gRows.push_back(title("general"));
    gRows.push_back(boolean("debug_mode", "general.debug"));
    gRows.push_back(boolean("display_tooltip", "general.displayTooltip"));
    gRows.push_back(boolean("display_entities", "general.displayEntities"));
    gRows.push_back(boolean("display_bosses", "general.displayBosses"));
    gRows.push_back(boolean("display_blocks", "general.displayBlocks"));
    gRows.push_back(cycle("display_fluids", "general.fluidMode", {"NONE", "ANY", "FALLBACK"}));
    gRows.push_back(cycle("display_mode", "general.displayMode", {"HOLD_KEY", "TOGGLE", "LITE"}));
    gRows.push_back(boolean("item_mod_name", "general.itemModNameTooltip"));
    gRows.push_back(boolean("hide_from_guis", "general.hideFromGUIs"));
    gRows.push_back(cycle("boss_bar_overlap", "general.bossBarOverlapMode", {"NO_OPERATION", "HIDE_BOSS_BAR", "HIDE_TOOLTIP", "PUSH_DOWN"}));
    gRows.push_back(slider("reach_distance", "general.extendedReach", 0, 20, 0.5f));
    gRows.push_back(cycle("perspective_mode", "general.perspectiveMode", {"CAMERA", "EYE"}));

    gRows.push_back(title("overlay"));
    gRows.push_back(actionRow(I18n::get("config.jade.overlay_pos"), I18n::get("config.jade.overlay_pos.adjust"), [] {
        gAdjusting  = true;
        gAdjustDrag = false;
    }));
    auto& overlay = ConfigStore::get().config().overlay();
    Row theme;
    theme.kind    = Row::Kind::Cycle;
    theme.label   = I18n::get("config.jade.overlay_theme");
    theme.key     = "overlay.activeTheme";
    theme.getText = [] { return std::string(ConfigStore::get().config().overlay().getTheme().mainId()); };
    theme.setText = [](std::string const& id) {
        auto& o = ConfigStore::get().config().overlay();
        o.applyTheme(id);
        if (o.getTheme().changeOpacity != 0) o.setAlpha(o.getTheme().changeOpacity);
        ConfigStore::get().saveConfig();
    };
    for (auto* t : impl::theme::ThemeHelper::INSTANCE.getThemes()) {
        if (t->styleId().empty()) theme.choices.emplace_back(t->fullId(), tr(themeKey(t->fullId()), t->fullId()));
    }
    gRows.push_back(std::move(theme));
    Row style;
    style.kind      = Row::Kind::Cycle;
    style.label     = I18n::get("config.jade.theme_style");
    style.key       = "overlay.themeStyle";
    style.secondary = true;
    style.getText   = [] { return std::string(ConfigStore::get().config().overlay().getTheme().fullId()); };
    style.setText   = [](std::string const& id) {
        ConfigStore::get().config().overlay().applyTheme(id);
        ConfigStore::get().saveConfig();
    };
    std::string mainId = overlay.getTheme().mainId();
    for (auto* t : impl::theme::ThemeHelper::INSTANCE.getThemes()) {
        if (std::string(t->mainId()) == mainId) style.choices.emplace_back(t->fullId(), tr(t->styleName, t->styleName));
    }
    style.disabled = style.choices.size() <= 1;
    gRows.push_back(std::move(style));
    gRows.push_back(slider("overlay_alpha", "overlay.alpha", 0, 1));
    gRows.push_back(slider("overlay_scale", "overlay.overlayScale", 0.2f, 2));
    gRows.push_back(cycle("display_item", "overlay.iconMode", {"TOP", "CENTERED", "INLINE", "HIDE"}));
    gRows.push_back(boolean("animation", "overlay.animation"));

    gRows.push_back(title("key_binds"));
    for (auto* k : util::JadeKeys::all()) {
        if (k->getName().starts_with("profile.")) continue;
        Row r;
        r.kind       = Row::Kind::Key;
        r.label      = tr("key.jade." + k->getName(), k->getName());
        r.keyMapping = k;
        gRows.push_back(std::move(r));
    }

    gRows.push_back(title("accessibility"));
    gRows.push_back(boolean("accessibility_plugin", "accessibility.enableAccessibilityPlugin"));
    gRows.push_back(boolean("always_list_mode", "accessibility.alwaysListMode"));
    gRows.push_back(slider("text_background_opacity", "accessibility.textBackgroundOpacity", 0, 1));
    gRows.push_back(boolean("flip_main_hand", "accessibility.flipMainHand"));

    gRows.push_back(title("danger_zone"));
    gRows.push_back(actionRow(I18n::get("config.jade.reload_plugins"), I18n::get("config.jade.reload_plugins.button"), [] {
        ConfigStore::get().invalidateConfig();
        applyActiveTheme();
        rebuildKeep();
    }));
    gRows.push_back(actionRow(I18n::get("config.jade.reset_settings"), I18n::get("gui.jade.reset"), [] {
        gConfirm = Confirm{I18n::get("config.jade.reset_settings"), I18n::get("config.jade.reset_settings.confirm"), I18n::get("gui.jade.reset"), resetSettings};
    }));
}

std::string profileName(int index) {
    std::string name = ConfigStore::get().profile(index).getName();
    if (name.starts_with("@")) return I18n::get(name.substr(1));
    return name.empty() ? I18n::get("config.jade.profile." + std::to_string(index)) : name;
}

void buildProfiles() {
    auto& store = ConfigStore::get();
    bool  on    = store.rootConfig().isEnableProfiles();
    gRows.push_back(title("profiles"));
    Row enable = boolean("enable_profiles", "root.enableProfiles");
    enable.getBool = [] { return ConfigStore::get().rootConfig().isEnableProfiles(); };
    enable.setBool = [](bool v) {
        std::string error;
        ConfigStore::get().setValue("root.enableProfiles", v, error);
        applyActiveTheme();
        rebuildKeep();
    };
    gRows.push_back(std::move(enable));
    for (int i = 0; i < 4; ++i) {
        Row r;
        r.kind     = Row::Kind::Profile;
        r.profile  = i;
        r.label    = I18n::get("config.jade.profile." + std::to_string(i));
        r.disabled = !on;
        gRows.push_back(std::move(r));
    }
    gRows.push_back(title("key_binds"));
    for (auto* k : util::JadeKeys::all()) {
        if (!k->getName().starts_with("profile.")) continue;
        Row r;
        r.kind       = Row::Kind::Key;
        r.label      = tr("key.jade." + k->getName(), k->getName());
        r.keyMapping = k;
        r.disabled   = !on;
        gRows.push_back(std::move(r));
    }
}

void buildPlugins() {
    namespace entry = impl::config::entry;
    std::string lastNamespace;
    for (auto const& [idRef, e] : impl::WailaClientRegistration::instance().configEntries) {
        std::string id  = idRef;
        auto        pos = id.find(':');
        std::string ns  = id.substr(0, pos);
        if (ns != lastNamespace) {
            lastNamespace = ns;
            Row t;
            t.label = tr("config.jade.plugin_" + ns, ns);
            gRows.push_back(std::move(t));
        }
        std::string langKey = id;
        std::replace(langKey.begin(), langKey.end(), ':', '.');
        Row r;
        r.key       = id;
        r.label     = api::external::displayName(id).value_or(tr("config.jade.plugin_" + langKey, id));
        r.disabled  = e->isSynced();
        r.secondary = pos != std::string::npos && id.find('.', pos) != std::string::npos;
        r.descKey   = "config.jade.plugin_" + langKey + "_desc";
        if (r.disabled) r.note = I18n::get("gui.jade.forced_plugin_config");
        else if (!impl::WailaClientRegistration::instance().isClientFeature(id)) r.note = I18n::get("gui.jade.server_feature");
        if (dynamic_cast<entry::BooleanConfigEntry*>(e.get())) {
            r.kind = Row::Kind::Bool;
        } else if (auto* en = dynamic_cast<entry::EnumConfigEntry*>(e.get())) {
            r.kind = Row::Kind::Cycle;
            for (auto const& v : en->names()) r.choices.emplace_back(v, tr("config.jade.plugin_" + langKey + "_" + lower(v), v));
        } else if (auto* i = dynamic_cast<entry::IntConfigEntry*>(e.get())) {
            r.kind    = Row::Kind::Slider;
            r.min     = float(i->min());
            r.max     = float(i->max());
            r.step    = 1;
            r.integer = true;
        } else if (auto* f = dynamic_cast<entry::FloatConfigEntry*>(e.get())) {
            r.kind = Row::Kind::Slider;
            r.min  = f->min();
            r.max  = f->max();
        } else {
            continue;
        }
        gRows.push_back(std::move(r));
    }
}

void applySearch() {
    if (gSearch.empty()) return;
    std::string      q = lower(gSearch);
    std::vector<Row> kept;
    Row const*       pendingTitle = nullptr;
    for (auto const& r : gRows) {
        if (r.kind == Row::Kind::Title) {
            pendingTitle = &r;
            continue;
        }
        if (lower(r.label).find(q) == std::string::npos && lower(r.key).find(q) == std::string::npos) continue;
        if (pendingTitle) kept.push_back(*pendingTitle);
        pendingTitle = nullptr;
        kept.push_back(r);
    }
    gRows = std::move(kept);
}

void rebuild() {
    gRows.clear();
    gDragging = gCapturing = -1;
    switch (gPage) {
    case Page::Home:
        gRows.push_back(button(I18n::get("gui.jade.jade_settings"), [] { ConfigGui::open(Page::General); }));
        gRows.push_back(button(I18n::get("gui.jade.plugin_settings"), [] { ConfigGui::open(Page::Plugins); }));
        gRows.push_back(button(I18n::get("gui.jade.profile_settings"), [] { ConfigGui::open(Page::Profiles); }));
        break;
    case Page::General: buildGeneral(); break;
    case Page::Plugins: buildPlugins(); break;
    case Page::Profiles: buildProfiles(); break;
    }
    applySearch();
}

void rebuildKeep() {
    float scroll = gScroll;
    rebuild();
    gScroll = scroll;
}

float contentHeight() { return float(gRows.size()) * ROW_H; }
float listLeft() { return std::floor((gW - LIST_W) / 2); }
float maxScroll() { return std::max(0.0f, contentHeight() - (gH - TOP - BOTTOM)); }
float rowY(size_t i) { return TOP + float(i) * ROW_H - gScroll; }
float controlX() { return listLeft() + LIST_W - CONTROL_W; }

struct Box {
    float x, y, w, h;
    [[nodiscard]] bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

Box controlBox(size_t i) {
    if (gRows[i].kind == Row::Kind::Button) return {std::floor((gW - 200) / 2), rowY(i) + 1, 200, BTN_H};
    return {controlX(), rowY(i) + 1, CONTROL_W, BTN_H};
}

Box doneBox() { return {std::floor((gW - 200) / 2), gH - BOTTOM + 7, 200, BTN_H}; }
Box previewBox() { return {10, gH - BOTTOM + 7, 85, BTN_H}; }
Box searchBox() { return {listLeft() + LIST_W - CONTROL_W, 8, CONTROL_W, 18}; }
Box profileNameBox(size_t i) { return {listLeft() + 70, rowY(i) + 1, 130, BTN_H}; }
Box profileUseBox(size_t i) { return {listLeft() + 205, rowY(i) + 1, 55, BTN_H}; }
Box profileSaveBox(size_t i) { return {listLeft() + 265, rowY(i) + 1, 55, BTN_H}; }
Box confirmYes() { return {std::floor(gW / 2) - 105, std::floor(gH / 2) + 18, 100, BTN_H}; }
Box confirmNo() { return {std::floor(gW / 2) + 5, std::floor(gH / 2) + 18, 100, BTN_H}; }
bool hasPreview() { return gPage == Page::General || gPage == Page::Plugins; }
bool hasSearch() { return hasPreview(); }
bool rowVisible(size_t i) { return rowY(i) >= TOP - 0.5f && rowY(i) + ROW_H <= gH - BOTTOM + 0.5f; }

Box overlayBox() {
    auto const& e = overlay::OverlayRenderer::animation.expectedRect;
    if (gOverlayShown && e.getWidth() > 0) return {e.getX(), e.getY(), e.getWidth(), e.getHeight()};
    auto const& o = ConfigStore::get().config().overlay();
    float       w = 100, h = 30;
    float       cx = o.getOverlayPosX() * gW - w * (o.getAnchorX() - 0.5f);
    float       cy = (1 - o.getOverlayPosY()) * gH - h * (o.getAnchorY() - 0.5f);
    return {cx - w / 2, cy - h / 2, w, h};
}

float calculateAnchor(float center, float size, float rectSize) {
    float anchor = center / size;
    if (anchor < 0.25f) return 0;
    if (anchor > 0.75f) return 1;
    float half      = rectSize / 2;
    float tolerance = std::min(15.0f, half / 2 - 3);
    if (std::abs(center + half - size / 2) < tolerance) return 1;
    if (std::abs(center - half - size / 2) < tolerance) return 0;
    return 0.5f;
}

float maybeSnap(float v, bool snap) { return snap && v > 0.475f && v < 0.525f ? 0.5f : v; }

void moveOverlay(float centerX, float centerY, bool snap) {
    Box   r       = overlayBox();
    float anchorX = calculateAnchor(centerX, gW, r.w);
    float anchorY = calculateAnchor(centerY, gH, r.h);
    float posX    = (centerX + r.w * (anchorX - 0.5f)) / gW;
    float posY    = 1 - (centerY + r.h * (anchorY - 0.5f)) / gH;
    auto& cfg     = ConfigStore::get().config();
    cfg.overlay().setOverlayPosX(cfg.accessibility().tryFlip(maybeSnap(posX, snap)));
    cfg.overlay().setOverlayPosY(maybeSnap(posY, snap));
    cfg.overlay().setAnchorX(cfg.accessibility().tryFlip(anchorX));
    cfg.overlay().setAnchorY(anchorY);
}

void adjustClick(float x, float y) {
    Box r = overlayBox();
    if (r.contains(x, y)) {
        gAdjustDrag  = true;
        gDragOffsetX = x - (r.x + r.w / 2);
        gDragOffsetY = y - (r.y + r.h / 2);
        return;
    }
    int xi = std::clamp(int(x / (gW / 3)), 0, 2), yi = std::clamp(int(y / (gH / 3)), 0, 2);
    if (xi == 1 && yi == 1) {
        gAdjusting = false;
        return;
    }
    auto& cfg = ConfigStore::get().config();
    cfg.overlay().setOverlayPosX(cfg.accessibility().tryFlip(float(xi) / 2));
    cfg.overlay().setOverlayPosY(1 - float(yi) / 2);
    cfg.overlay().setAnchorX(cfg.accessibility().tryFlip(float(xi) / 2));
    cfg.overlay().setAnchorY(float(yi) / 2);
    ConfigStore::get().saveConfig();
}

void adjustKey(int code) {
    int dx = code == 0x25 ? -1 : code == 0x27 ? 1 : 0, dy = code == 0x26 ? -1 : code == 0x28 ? 1 : 0;
    if (!dx && !dy) return;
    Box   r    = overlayBox();
    float step = gShift ? 20.0f : 2.0f;
    moveOverlay(r.x + r.w / 2 + float(dx) * step, r.y + r.h / 2 + float(dy) * step, false);
    ConfigStore::get().saveConfig();
}

std::string keyToText(int code) {
    if (code >= '0' && code <= '9') return std::string(1, char(code));
    if (code >= 'A' && code <= 'Z') return std::string(1, char(gShift ? code : code - 'A' + 'a'));
    if (code >= 0x60 && code <= 0x69) return std::string(1, char('0' + code - 0x60));
    if (code == 0xBE || code == 0x6E) return ".";
    if (code == 0xBD || code == 0x6D) return gShift ? "_" : "-";
    if (code == 0x20) return " ";
    return "";
}

void popUtf8(std::string& s) {
    while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) s.pop_back();
    if (!s.empty()) s.pop_back();
}

void focusedKey(int code) {
    std::string text = gFocus == FOCUS_SEARCH ? gSearch : ConfigStore::get().profile(gFocus).getName();
    if (text.starts_with("@")) text = I18n::get(text.substr(1));
    if (code == 0x08) popUtf8(text);
    else if (code == 0x0D) {
        gFocus = -1;
        return;
    } else {
        auto t = keyToText(code);
        if (t.empty() || text.size() >= 40) return;
        text += t;
    }
    if (gFocus == FOCUS_SEARCH) {
        gSearch = text;
        gScroll = 0;
        rebuild();
    } else {
        ConfigStore::get().renameProfile(gFocus, text);
    }
}

float numberOf(Row const& r) {
    auto v = ConfigStore::get().getValue(r.key);
    return v.is_number() ? v.get<float>() : r.min;
}

bool boolOf(Row const& r) {
    if (r.getBool) return r.getBool();
    auto v = ConfigStore::get().getValue(r.key);
    return v.is_boolean() && v.get<bool>();
}

std::string textOf(Row const& r) {
    if (r.getText) return r.getText();
    auto v = ConfigStore::get().getValue(r.key);
    return v.is_string() ? v.get<std::string>() : "";
}

void setValue(Row const& r, json const& v) {
    std::string error;
    ConfigStore::get().setValue(r.key, v, error);
}

void setSlider(Row const& r, Box const& b, float px) {
    float t = std::clamp((px - b.x) / b.w, 0.0f, 1.0f);
    float v = r.min + (r.max - r.min) * t;
    if (r.step > 0) v = std::floor(v / r.step + 0.5f) * r.step;
    v = std::clamp(v, r.min, r.max);
    if (r.integer) setValue(r, int(v));
    else setValue(r, v);
}

std::string keyName(int vk) {
    if (vk == 0) return I18n::get("key.keyboard.unknown") == "key.keyboard.unknown" ? "-" : I18n::get("key.keyboard.unknown");
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) return std::string(1, char(vk));
    if (vk >= 0x60 && vk <= 0x69) return "Numpad " + std::to_string(vk - 0x60);
    if (vk >= 0x70 && vk <= 0x87) return "F" + std::to_string(vk - 0x6F);
    if (vk == 0xA0 || vk == 0x10) return "Left Shift";
    if (vk == 0xA1) return "Right Shift";
    if (vk == 0xA2 || vk == 0x11) return "Left Ctrl";
    if (vk == 0xA4 || vk == 0x12) return "Left Alt";
    return "Key " + std::to_string(vk);
}

std::string valueText(Row const& r) {
    switch (r.kind) {
    case Row::Kind::Bool: return tr(boolOf(r) ? "options.on" : "options.off", boolOf(r) ? "ON" : "OFF");
    case Row::Kind::Cycle: {
        if (r.disabled && r.choices.size() <= 1) return I18n::get("jade.unavailable");
        auto v = textOf(r);
        for (auto const& [value, shown] : r.choices)
            if (value == v) return shown;
        return v.empty() ? "?" : v;
    }
    case Row::Kind::Action: return r.buttonText;
    case Row::Kind::Profile: return profileName(r.profile);
    case Row::Kind::Slider: {
        float v = numberOf(r);
        if (r.integer) return std::to_string(int(v));
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", v);
        return buf;
    }
    case Row::Kind::Key: return r.keyMapping ? keyName(r.keyMapping->key()) : "";
    default: return "";
    }
}

std::string descriptionOf(Row const& r) {
    std::string desc = !r.descKey.empty() && I18n::has(r.descKey) ? I18n::get(r.descKey) : "";
    if (!r.note.empty()) desc += (desc.empty() ? "" : "\n") + r.note;
    auto replaceAll = [&](std::string const& from, std::string const& to) {
        for (size_t p = desc.find(from); p != std::string::npos; p = desc.find(from, p + to.size())) desc.replace(p, from.size(), to);
    };
    replaceAll("${SHOW_DETAILS}", keyName(util::JadeKeys::showDetails().key()));
    replaceAll("${SHOW_OVERLAY}", keyName(util::JadeKeys::showOverlay().key()));
    return desc;
}

void clickRow(size_t i, float x, bool right) {
    Row const& r = gRows[i];
    if (r.disabled) return;
    switch (r.kind) {
    case Row::Kind::Bool: {
        bool next = !boolOf(r);
        if (r.setBool) {
            auto set = r.setBool;
            set(next);
        } else {
            setValue(r, next);
        }
        break;
    }
    case Row::Kind::Cycle: {
        if (r.choices.empty()) break;
        auto v   = textOf(r);
        int  n   = int(r.choices.size());
        int  cur = 0;
        for (int k = 0; k < n; ++k)
            if (r.choices[k].first == v) cur = k;
        std::string next = r.choices[(cur + (right ? n - 1 : 1)) % n].first;
        if (r.setText) {
            auto set = r.setText;
            set(next);
            rebuildKeep();
        } else {
            setValue(r, next);
        }
        break;
    }
    case Row::Kind::Slider:
        gDragging = int(i);
        setSlider(r, controlBox(i), x);
        break;
    case Row::Kind::Key: gCapturing = int(i); break;
    case Row::Kind::Action:
    case Row::Kind::Button: {
        auto action = r.action;
        action();
        break;
    }
    default: break;
    }
}

void clickProfile(size_t i, float x, float y) {
    int index = gRows[i].profile;
    if (gRows[i].disabled) return;
    bool current = ConfigStore::get().rootConfig().profileIndex == index;
    if (profileNameBox(i).contains(x, y)) {
        gFocus = index;
    } else if (!current && profileUseBox(i).contains(x, y)) {
        ConfigStore::get().useProfile(index);
        applyActiveTheme();
        rebuildKeep();
    } else if (!current && profileSaveBox(i).contains(x, y)) {
        auto save = [index] { ConfigStore::get().saveProfile(index); };
        if (gCtrl) save();
        else gConfirm = Confirm{I18n::get("gui.jade.save_profile.title"), I18n::format(I18n::get("gui.jade.save_profile.message"), {profileName(index)}), I18n::get("gui.jade.save"), save};
    }
}

void onClick(float x, float y, bool right) {
    if (gConfirm) {
        if (right) return;
        if (confirmYes().contains(x, y)) {
            auto action = gConfirm->action;
            gConfirm.reset();
            action();
        } else if (confirmNo().contains(x, y)) {
            gConfirm.reset();
        }
        return;
    }
    if (gAdjusting) {
        if (!right) adjustClick(x, y);
        return;
    }
    gFocus = -1;
    if (!right && doneBox().contains(x, y)) {
        ConfigGui::back();
        return;
    }
    if (!right && hasPreview() && previewBox().contains(x, y)) {
        auto& h          = ConfigStore::get().history();
        h.previewOverlay = !h.previewOverlay;
        ConfigStore::get().saveConfig();
        return;
    }
    if (!right && hasSearch() && searchBox().contains(x, y)) {
        gFocus = FOCUS_SEARCH;
        return;
    }
    if (y < TOP || y >= gH - BOTTOM) return;
    for (size_t i = 0; i < gRows.size(); ++i) {
        float top = rowY(i);
        if (top < TOP - 0.5f || top + ROW_H > gH - BOTTOM + 0.5f) continue;
        if (gRows[i].kind == Row::Kind::Profile) {
            if (!right && y >= top && y < top + ROW_H) {
                clickProfile(i, x, y);
                return;
            }
            continue;
        }
        if (gRows[i].kind != Row::Kind::Title && controlBox(i).contains(x, y)) {
            clickRow(i, x, right);
            return;
        }
    }
}

void drawButton(platform::render::GuiGraphics& g, Box const& b, std::string const& text, bool hovered, bool disabled) {
    g.fill(b.x, b.y, b.x + b.w, b.y + b.h, disabled ? 0xFF2A2A2A : hovered ? 0xFF5A5A5A : 0xFF3C3C3C);
    g.drawBorder(b.x, b.y, b.w, b.h, 1, hovered && !disabled ? 0xFFFFFFFF : 0xFF000000);
    float tw = float(g.textWidth(text));
    g.drawText(text, b.x + (b.w - tw) / 2, b.y + (b.h - 8) / 2, disabled ? 0xFF808080 : 0xFFFFFFFF, true);
}

bool hudOnTop() {
    auto ci = ll::service::getClientInstance();
    if (!ci) return false;
    auto name = ci->getTopScreenName();
    return name == "hud_screen" || name == "toast_screen";
}

float guiScale() {
    auto ci = ll::service::getClientInstance();
    if (!ci) return 1;
    float s = ci->getGuiData()->mGuiScale;
    return s > 0 ? s : 1;
}

void setGrab(bool allow) {
    if (gAllowGrab == allow) return;
    gAllowGrab = allow;
    auto ci    = ll::service::getClientInstance();
    if (!ci) return;
    if (allow) ci->grabMouse();
    else ci->releaseMouse();
}

}

LL_TYPE_INSTANCE_HOOK(JadeClientGrabMouseHook, ll::memory::HookPriority::Normal, ClientInstance, &ClientInstance::$grabMouse, void) {
    if (!gAllowGrab) return;
    origin();
}

void ConfigGui::install() {
    auto& bus = ll::event::EventBus::getInstance();
    gListeners.push_back(bus.emplaceListener<ll::event::KeyInputEvent>(
        [](ll::event::KeyInputEvent& e) {
            int code = e.keyCode();
            if (code == 0x10 || code == 0xA0 || code == 0xA1) gShift = e.isDown();
            if (code == 0x11 || code == 0xA2 || code == 0xA3) gCtrl = e.isDown();
            if (!gOpen || !hudOnTop()) return;
            if (!e.isDown()) return;
            if (gConfirm) {
                if (code == VK_ESCAPE_) gConfirm.reset();
            } else if (gAdjusting) {
                if (code == VK_ESCAPE_) {
                    gAdjusting = gAdjustDrag = false;
                    ConfigStore::get().saveConfig();
                } else {
                    adjustKey(code);
                }
            } else if (gFocus >= 0) {
                if (code == VK_ESCAPE_) gFocus = -1;
                else focusedKey(code);
            } else if (gCapturing >= 0) {
                if (auto* k = gRows[gCapturing].keyMapping) {
                    k->setKey(code == VK_ESCAPE_ ? 0 : code);
                    util::JadeKeys::save();
                }
                gCapturing = -1;
            } else if (code == VK_ESCAPE_) {
                back();
            }
            e.cancel();
        },
        ll::event::EventPriority::High
    ));
    gListeners.push_back(bus.emplaceListener<ll::event::MouseInputEvent>(
        [](ll::event::MouseInputEvent& e) {
            if (!gOpen || !hudOnTop()) return;
            float s      = guiScale();
            int   action = static_cast<int>(e.actionButtonId());
            if (action == MouseAction::ActionMove) {
                gMouseX = float(e.x()) / s;
                gMouseY = float(e.y()) / s;
                if (gAdjusting && gAdjustDrag) moveOverlay(gMouseX - gDragOffsetX, gMouseY - gDragOffsetY, !gCtrl);
                else if (gDragging >= 0) setSlider(gRows[gDragging], controlBox(gDragging), gMouseX);
            } else if (action == MouseAction::ActionWheel) {
                if (!gAdjusting && !gConfirm) gScroll = std::clamp(gScroll - (float(e.buttonData()) > 0 ? ROW_H * 2 : -ROW_H * 2), 0.0f, maxScroll());
            } else if (action == MouseAction::ActionLeft || action == MouseAction::ActionRight) {
                if (e.buttonData() == MouseAction::DataDown) onClick(gMouseX, gMouseY, action == MouseAction::ActionRight);
                else {
                    if (gAdjustDrag) ConfigStore::get().saveConfig();
                    gAdjustDrag = false;
                    gDragging   = -1;
                }
            }
            e.cancel();
        },
        ll::event::EventPriority::High
    ));
    if (!gHooked) {
        JadeClientGrabMouseHook::hook();
        gHooked = true;
    }
}

void ConfigGui::uninstall() {
    close();
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& l : gListeners) bus.removeListener(l);
    gListeners.clear();
    if (gHooked) {
        JadeClientGrabMouseHook::unhook();
        gHooked = false;
    }
}

void ConfigGui::open(Page page) {
    gPage   = page;
    gScroll = 0;
    gOpen   = true;
    gSearch.clear();
    gFocus = -1;
    gConfirm.reset();
    gAdjusting = gAdjustDrag = false;
    rebuild();
    setGrab(false);
}

void ConfigGui::back() {
    if (!gOpen) return;
    if (gPage == Page::Home) close();
    else open(Page::Home);
}

void ConfigGui::close() {
    if (!gOpen) return;
    gOpen = false;
    gRows.clear();
    gDragging = gCapturing = gFocus = -1;
    gConfirm.reset();
    gAdjusting = gAdjustDrag = false;
    gSearch.clear();
    setGrab(true);
}

bool ConfigGui::isOpen() { return gOpen; }

void ConfigGui::render(platform::render::GuiGraphics& g) {
    if (!gOpen) return;
    gW      = float(g.screenWidth());
    gH      = float(g.screenHeight());
    gScroll = std::clamp(gScroll, 0.0f, maxScroll());
    if (gAdjusting) {
        g.fill(0, 0, gW, gH, 0x80808080);
        std::string text = I18n::get("config.jade.overlay_pos.exit");
        float       tw   = float(g.textWidth(text)), x = (gW - tw) / 2, y = gH / 2 - 7;
        g.fill(x - 4, y - 4, x + tw + 4, y + 13, 0x88000000);
        g.drawText(text, x, y, 0xFFFFFFFF, false);
        auto const& o = ConfigStore::get().config().overlay();
        Box         r = overlayBox();
        if (o.getOverlayPosX() == 0.5f) g.fill(gW / 2, r.y - 5, gW / 2 + 1, r.y + r.h + 4, 0xFF0000FF);
        if (o.getOverlayPosY() == 0.5f) g.fill(r.x - 5, gH / 2, r.x + r.w + 4, gH / 2 + 1, 0xFF0000FF);
        g.flush();
        return;
    }
    g.fill(0, 0, gW, gH, 0xA0101010);

    std::string head = I18n::get(
        gPage == Page::Plugins    ? "gui.jade.plugin_settings"
        : gPage == Page::General  ? "gui.jade.jade_settings"
        : gPage == Page::Profiles ? "gui.jade.profile_settings"
                                  : "gui.jade.configuration"
    );
    g.drawText(head, (gW - float(g.textWidth(head))) / 2, 13, 0xFFFFFFFF, true);
    if (hasSearch()) {
        Box  b      = searchBox();
        bool active = gFocus == FOCUS_SEARCH;
        g.fill(b.x, b.y, b.x + b.w, b.y + b.h, 0xFF000000);
        g.drawBorder(b.x, b.y, b.w, b.h, 1, active ? 0xFFFFFFFF : 0xFFA0A0A0);
        if (gSearch.empty() && !active) g.drawText(I18n::get("gui.jade.search.hint"), b.x + 4, b.y + 5, 0xFF808080, false);
        else g.drawText(gSearch + (active ? "_" : ""), b.x + 4, b.y + 5, 0xFFFFFFFF, false);
    }
    if (gRows.empty()) {
        std::string none = I18n::get("gui.jade.no_results");
        g.drawText(none, (gW - float(g.textWidth(none))) / 2, TOP + 20, 0xFFA0A0A0, true);
    }

    bool inList = gMouseY >= TOP && gMouseY < gH - BOTTOM;
    for (size_t i = 0; i < gRows.size(); ++i) {
        float y = rowY(i);
        if (y < TOP - 0.5f || y + ROW_H > gH - BOTTOM + 0.5f) continue;
        Row const& r = gRows[i];
        if (r.kind == Row::Kind::Title) {
            g.drawText(r.label, (gW - float(g.textWidth(r.label))) / 2, y + 8, 0xFFFFFF55, true);
            continue;
        }
        if (r.kind == Row::Kind::Profile) {
            auto& root    = ConfigStore::get().rootConfig();
            bool  current = root.isEnableProfiles() && root.profileIndex == r.profile;
            g.drawText(r.label, listLeft(), y + 7, r.disabled ? 0xFF808080 : current ? 0xFFFFFF55 : 0xFFFFFFFF, true);
            Box name = profileNameBox(i), use = profileUseBox(i), save = profileSaveBox(i);
            g.fill(name.x, name.y, name.x + name.w, name.y + name.h, 0xFF000000);
            g.drawBorder(name.x, name.y, name.w, name.h, 1, gFocus == r.profile ? 0xFFFFFFFF : 0xFFA0A0A0);
            g.drawText(profileName(r.profile) + (gFocus == r.profile ? "_" : current ? I18n::get("gui.jade.profile.active") : ""), name.x + 4, name.y + 6, r.disabled ? 0xFF808080 : current ? 0xFFFFFF55 : 0xFFE0E0E0, false);
            drawButton(g, use, I18n::get("gui.jade.profile.use"), inList && use.contains(gMouseX, gMouseY), r.disabled || current);
            drawButton(g, save, I18n::get("gui.jade.save"), inList && save.contains(gMouseX, gMouseY), r.disabled || current);
            continue;
        }
        Box  b       = controlBox(i);
        bool hovered = inList && b.contains(gMouseX, gMouseY);
        if (r.kind == Row::Kind::Button) {
            drawButton(g, b, r.label, hovered, r.disabled);
            continue;
        }
        g.drawText(r.label, listLeft() + (r.secondary ? 10 : 0), y + 7, r.disabled ? 0xFF808080 : 0xFFFFFFFF, true);
        if (r.kind == Row::Kind::Slider) {
            drawButton(g, b, valueText(r), hovered, r.disabled);
            float t = r.max > r.min ? std::clamp((numberOf(r) - r.min) / (r.max - r.min), 0.0f, 1.0f) : 0;
            float k = b.x + (b.w - 6) * t;
            g.fill(k, b.y, k + 6, b.y + b.h, 0xFFC0C0C0);
        } else {
            drawButton(g, b, gCapturing == int(i) ? "> ... <" : valueText(r), hovered, r.disabled);
        }
    }
    Box done = doneBox();
    drawButton(g, done, gPage == Page::Home ? I18n::get("gui.jade.save_and_quit") : I18n::get("gui.jade.back"), done.contains(gMouseX, gMouseY), false);
    if (hasPreview()) {
        Box  p  = previewBox();
        bool on = ConfigStore::get().history().previewOverlay;
        drawButton(g, p, I18n::get("gui.jade.preview") + ": " + tr(on ? "options.on" : "options.off", on ? "ON" : "OFF"), p.contains(gMouseX, gMouseY), false);
    }
    g.flush();
}

bool ConfigGui::showPreview() {
    if (!gOpen || gConfirm) return false;
    if (gAdjusting) return true;
    if (gDragging >= 0 && gRows[gDragging].key == "overlay.overlayScale") return true;
    return hasPreview() && ConfigStore::get().history().previewOverlay;
}

void ConfigGui::renderTop(platform::render::GuiGraphics& g) {
    if (!gOpen) return;
    gOverlayShown = showPreview() && overlay::OverlayRenderer::shown;
    if (gAdjusting) {
        if (!gOverlayShown) {
            Box r = overlayBox();
            g.fill(r.x, r.y, r.x + r.w, r.y + r.h, 0xC0202020);
            g.drawBorder(r.x, r.y, r.w, r.h, 1, 0xFFFFFFFF);
            std::string text = I18n::get("gui.jade.preview");
            g.drawText(text, r.x + (r.w - float(g.textWidth(text))) / 2, r.y + 11, 0xFFFFFFFF, true);
        }
        g.flush();
        return;
    }
    if (gConfirm) {
        g.fill(0, 0, gW, gH, 0xC0101010);
        g.drawText(gConfirm->title, (gW - float(g.textWidth(gConfirm->title))) / 2, gH / 2 - 40, 0xFFFF5555, true);
        std::vector<std::string> lines(1);
        for (size_t i = 0; i < gConfirm->message.size();) {
            size_t n = 1;
            auto   c = static_cast<unsigned char>(gConfirm->message[i]);
            if (c >= 0xF0) n = 4;
            else if (c >= 0xE0) n = 3;
            else if (c >= 0xC0) n = 2;
            std::string ch = gConfirm->message.substr(i, n);
            if (g.textWidth(lines.back() + ch) > 300) lines.emplace_back();
            lines.back() += ch;
            i += n;
        }
        float y = gH / 2 - 22;
        for (auto const& l : lines) {
            g.drawText(l, (gW - float(g.textWidth(l))) / 2, y, 0xFFFFFFFF, true);
            y += 11;
        }
        drawButton(g, confirmYes(), gConfirm->yes, confirmYes().contains(gMouseX, gMouseY), false);
        drawButton(g, confirmNo(), I18n::get("gui.jade.cancel"), confirmNo().contains(gMouseX, gMouseY), false);
        g.flush();
        return;
    }
    if (gMouseY < TOP || gMouseY >= gH - BOTTOM) return;
    for (size_t i = 0; i < gRows.size(); ++i) {
        if (!rowVisible(i) || gMouseY < rowY(i) || gMouseY >= rowY(i) + ROW_H || gMouseX < listLeft() || gMouseX >= listLeft() + LIST_W) continue;
        Row const& r    = gRows[i];
        std::string desc = descriptionOf(r);
        if (desc.empty()) return;
        std::vector<std::string> lines(1);
        for (size_t k = 0; k < desc.size();) {
            if (desc[k] == '\n') {
                lines.emplace_back();
                ++k;
                continue;
            }
            size_t n = 1;
            auto   c = static_cast<unsigned char>(desc[k]);
            if (c >= 0xF0) n = 4;
            else if (c >= 0xE0) n = 3;
            else if (c >= 0xC0) n = 2;
            std::string ch = desc.substr(k, n);
            if (g.textWidth(lines.back() + ch) > 200) lines.emplace_back();
            lines.back() += ch;
            k += n;
        }
        float w = 0;
        for (auto const& l : lines) w = std::max(w, float(g.textWidth(l)));
        float h = float(lines.size()) * 11 + 6;
        float x = std::min(gMouseX + 10, gW - w - 10), y = std::min(gMouseY + 10, gH - h - 4);
        g.fill(x - 3, y - 3, x + w + 3, y + h - 3, 0xF0100010);
        g.drawBorder(x - 3, y - 3, w + 6, h, 1, 0xFF5000A0);
        for (size_t k = 0; k < lines.size(); ++k) g.drawText(lines[k], x, y + float(k) * 11, 0xFFFFFFFF, true);
        g.flush();
        return;
    }
}

json ConfigGui::debugState() {
    json j = {{"open", gOpen}, {"scroll", gScroll}, {"maxScroll", maxScroll()}, {"width", gW}, {"height", gH}, {"guiScale", guiScale()}, {"capturing", gCapturing}};
    j["page"] = gPage == Page::Home ? "home" : gPage == Page::General ? "general" : gPage == Page::Plugins ? "plugins" : "profiles";
    j["search"]    = gSearch;
    j["focus"]     = gFocus;
    j["adjusting"] = gAdjusting;
    j["confirm"]   = gConfirm.has_value();
    j["preview"]   = showPreview();
    if (gOpen && gW > 0) {
        Box p = previewBox(), sb = searchBox(), oy = confirmYes(), on = confirmNo(), ob = overlayBox();
        j["boxes"] = {{"preview", {p.x, p.y, p.w, p.h}}, {"search", {sb.x, sb.y, sb.w, sb.h}}, {"confirmYes", {oy.x, oy.y, oy.w, oy.h}}, {"confirmNo", {on.x, on.y, on.w, on.h}}, {"overlay", {ob.x, ob.y, ob.w, ob.h}}};
    }
    json rows = json::array();
    for (size_t i = 0; i < gRows.size(); ++i) {
        Row const& r = gRows[i];
        json       o = {{"label", r.label}, {"key", r.key}, {"kind", int(r.kind)}, {"disabled", r.disabled}, {"value", valueText(r)}, {"desc", descriptionOf(r)}};
        if (r.kind != Row::Kind::Title) {
            Box b    = r.kind == Row::Kind::Profile ? profileNameBox(i) : controlBox(i);
            o["box"] = {b.x, b.y, b.w, b.h};
            if (r.kind == Row::Kind::Profile) {
                Box u = profileUseBox(i), s = profileSaveBox(i);
                o["use"]  = {u.x, u.y, u.w, u.h};
                o["save"] = {s.x, s.y, s.w, s.h};
            }
        }
        rows.push_back(std::move(o));
    }
    j["rows"]  = std::move(rows);
    Box d      = doneBox();
    j["done"]  = {d.x, d.y, d.w, d.h};
    return j;
}

}
