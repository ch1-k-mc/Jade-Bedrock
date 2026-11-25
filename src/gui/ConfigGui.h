// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <string>

#include "nlohmann/json.hpp"

namespace jade::platform::render {
class GuiGraphics;
}

namespace jade::gui {

class ConfigGui {
public:
    enum class Page { Home, General, Plugins, Profiles };

    static void install();
    static void uninstall();

    static void open(Page page = Page::Home);
    static void back();
    static void close();
    [[nodiscard]] static bool isOpen();

    static void render(platform::render::GuiGraphics& g);
    [[nodiscard]] static bool showPreview();
    static void renderTop(platform::render::GuiGraphics& g);

    [[nodiscard]] static nlohmann::json debugState();
};

}
