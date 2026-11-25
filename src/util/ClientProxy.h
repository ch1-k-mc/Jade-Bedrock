// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "api/ui/Rect2f.h"

class ScreenView;

namespace jade::util {

class ClientProxy {
public:
    static int guiScaledWidth();
    static int guiScaledHeight();
    static float guiScale();
    static int64_t millis();

    static std::optional<api::ui::Rect2f> getBossBarRect();
    static void                           updateBossBarRect(ScreenView& hudScreen);

    static std::string topScreenName();
    static bool isScreenOpen();
    static bool shouldHideWithGui();
    static bool isHudHidden();
    static void displayClientMessage(std::string const& message);
};

}
