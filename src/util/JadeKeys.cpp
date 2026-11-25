// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "util/JadeKeys.h"

#include <fstream>
#include <mutex>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/GuiData.h"

#include "nlohmann/json.hpp"

#include "api/config/IWailaConfig.h"
#include "gui/ConfigGui.h"
#include "impl/config/WailaConfig.h"
#include "platform/text/I18n.h"
#include "util/ClientProxy.h"

namespace jade::util {

namespace {

constexpr int VK_SHIFT_   = 0x10;
constexpr int VK_LSHIFT_  = 0xA0;
constexpr int VK_NUMPAD0_ = 0x60;

KeyMapping gOpenConfig{"config", VK_NUMPAD0_};
KeyMapping gShowOverlay{"show_overlay", VK_NUMPAD0_ + 1};
KeyMapping gToggleLiquid{"toggle_liquid", VK_NUMPAD0_ + 2};
KeyMapping gShowDetails{"show_details", VK_LSHIFT_};
KeyMapping gProfiles[4]{{"profile.0", 0}, {"profile.1", 0}, {"profile.2", 0}, {"profile.3", 0}};

std::mutex                          gMutex;
std::filesystem::path               gFile;
std::vector<ll::event::ListenerPtr> gListeners;

void chat(text::Component const& c) { ClientProxy::displayClientMessage(c.getString()); }

}

bool KeyMapping::consumeClick() {
    std::lock_guard lock(gMutex);
    if (mClicks == 0) return false;
    --mClicks;
    return true;
}

bool KeyMapping::matches(int vk) const {
    if (mKey == 0) return false;
    if (vk == mKey) return true;
    return (mKey == VK_LSHIFT_ && vk == VK_SHIFT_);
}

void KeyMapping::onKey(int vk, bool down) {
    if (!matches(vk)) return;
    std::lock_guard lock(gMutex);
    if (down && !mDown) ++mClicks;
    mDown = down;
}

KeyMapping& JadeKeys::openConfig() { return gOpenConfig; }
KeyMapping& JadeKeys::showOverlay() { return gShowOverlay; }
KeyMapping& JadeKeys::toggleLiquid() { return gToggleLiquid; }
KeyMapping& JadeKeys::showDetails() { return gShowDetails; }
KeyMapping& JadeKeys::useProfile(int index) { return gProfiles[index & 3]; }

std::vector<KeyMapping*> JadeKeys::all() {
    return {&gOpenConfig, &gShowOverlay, &gToggleLiquid, &gShowDetails, &gProfiles[0], &gProfiles[1], &gProfiles[2], &gProfiles[3]};
}

void JadeKeys::init(std::filesystem::path const& configDir) {
    gFile = configDir / "jade" / "keys.json";
    {
        std::ifstream in(gFile);
        if (in) {
            auto j = nlohmann::json::parse(in, nullptr, false);
            if (j.is_object())
                for (auto* k : all())
                    if (j.contains(k->getName()) && j[k->getName()].is_number_integer()) k->setKey(j[k->getName()].get<int>());
        }
    }
    save();
    auto& bus = ll::event::EventBus::getInstance();
    gListeners.push_back(bus.emplaceListener<ll::event::KeyInputEvent>([](ll::event::KeyInputEvent& e) {
        bool down = e.isDown();
        if (down && (ClientProxy::isScreenOpen() || gui::ConfigGui::isOpen())) return;
        for (auto* k : all()) k->onKey(e.keyCode(), down);
    }));
}

void JadeKeys::shutdown() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& l : gListeners) bus.removeListener(l);
    gListeners.clear();
}

void JadeKeys::save() {
    if (gFile.empty()) return;
    nlohmann::json j = nlohmann::json::object();
    for (auto* k : all()) j[k->getName()] = k->key();
    std::error_code ec;
    std::filesystem::create_directories(gFile.parent_path(), ec);
    std::ofstream out(gFile, std::ios::binary | std::ios::trunc);
    if (out) out << j.dump(2);
}

void JadeKeys::onKeyPressed() {
    auto& store = impl::config::ConfigStore::get();
    while (openConfig().consumeClick()) {
        store.invalidateConfig();
        gui::ConfigGui::open();
    }
    while (showOverlay().consumeClick()) {
        auto& general = api::config::IWailaConfig::get().general();
        if (general.getDisplayMode() == api::config::DisplayMode::TOGGLE) {
            general.setDisplayTooltip(!general.shouldDisplayTooltip());
            if (!general.shouldDisplayTooltip() && store.history().hintOverlayToggle) {
                chat(text::Component::translatable("toast.jade.toggle_hint.1"));
                chat(text::Component::translatable("toast.jade.toggle_hint.2", std::vector<std::string>{"Numpad 1"}));
                store.history().hintOverlayToggle = false;
            }
            api::config::IWailaConfig::get().save();
        }
    }
    while (toggleLiquid().consumeClick()) {
        auto& general = api::config::IWailaConfig::get().general();
        general.setDisplayFluids(!general.shouldDisplayFluids());
        api::config::IWailaConfig::get().save();
    }
    if (store.rootConfig().isEnableProfiles()) {
        for (int i = 0; i < 4; ++i)
            while (useProfile(i).consumeClick()) store.useProfile(i);
    }
}

}
