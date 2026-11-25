// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace jade::util {

class KeyMapping {
public:
    KeyMapping(std::string name, int defaultKey) : mName(std::move(name)), mDefaultKey(defaultKey), mKey(defaultKey) {}

    [[nodiscard]] std::string const& getName() const { return mName; }
    [[nodiscard]] int                key() const { return mKey; }
    void                             setKey(int vk) { mKey = vk; }
    [[nodiscard]] int                defaultKey() const { return mDefaultKey; }
    [[nodiscard]] bool               isUnbound() const { return mKey == 0; }
    [[nodiscard]] bool               isDown() const { return mDown; }
    bool consumeClick();

    bool matches(int vk) const;
    void onKey(int vk, bool down);
    void release() { mDown = false; }

private:
    std::string mName;
    int         mDefaultKey;
    int         mKey;
    bool        mDown   = false;
    int         mClicks = 0;
};

class JadeKeys {
public:
    static KeyMapping& openConfig();
    static KeyMapping& showOverlay();
    static KeyMapping& toggleLiquid();
    static KeyMapping& showDetails();
    static KeyMapping& useProfile(int index);
    static std::vector<KeyMapping*> all();

    static bool isShowDetailsPressed() { return showDetails().isDown(); }

    static void init(std::filesystem::path const& configDir);
    static void shutdown();
    static void save();
    static void onKeyPressed();
};

}
