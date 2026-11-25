// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND Apache-2.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
// The original file is modified from csscolor4j (Apache-2.0, silentsoft). See licenses/csscolor4j.txt.
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace jade::api::ui {

class Color {
public:
    static Color rgb(int red, int green, int blue, double opacity = 1.0);
    static Color hsl(double hue, double saturation, double lightness, double opacity = 1.0);
    static Color hex(std::string value);
    static std::optional<Color> valueOf(std::string value);
    static Color rgb(uint32_t argb);

    [[nodiscard]] int      getRed() const { return red; }
    [[nodiscard]] int      getGreen() const { return green; }
    [[nodiscard]] int      getBlue() const { return blue; }
    [[nodiscard]] double   getOpacity() const { return opacity; }
    [[nodiscard]] uint32_t toInt() const;

private:
    int    red = 0, green = 0, blue = 0;
    double hue = 0, saturation = 0, lightness = 0;
    double opacity = 0;
};

}
