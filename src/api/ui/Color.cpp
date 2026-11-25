// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND Apache-2.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
// The original file is modified from csscolor4j (Apache-2.0, silentsoft). See licenses/csscolor4j.txt.
#include "api/ui/Color.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>
#include <sstream>
#include <vector>

namespace jade::api::ui {

namespace {

std::string trim(std::string s) {
    auto b = s.find_first_not_of(" \t\r\n");
    auto e = s.find_last_not_of(" \t\r\n");
    return b == std::string::npos ? "" : s.substr(b, e - b + 1);
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool contains(std::string const& s, char const* sub) { return s.find(sub) != std::string::npos; }

std::string replaceAll(std::string s, std::string const& from, std::string const& to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

double parseDouble(std::string value, double limit) {
    bool hasPercent = contains(value, "%");
    value           = trim(replaceAll(value, "%", ""));
    double number   = std::stod(value);
    return hasPercent ? number * (limit / 100.0) : number;
}

int parseInt(std::string const& value, double limit) {
    double number = parseDouble(value, limit);
    int    round  = static_cast<int>(std::lround(number));
    return round > limit ? static_cast<int>(limit) : round;
}

std::vector<std::string> split(std::string value) {
    value = replaceAll(value, "/", " ");
    std::string collapsed;
    bool        space = false;
    for (char c : value) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!space) collapsed.push_back(' ');
            space = true;
        } else {
            collapsed.push_back(c);
            space = false;
        }
    }
    value          = collapsed.substr(collapsed.find('(') + 1, collapsed.find(')') - collapsed.find('(') - 1);
    char const sep = contains(value, ",") ? ',' : ' ';
    std::vector<std::string> out;
    std::stringstream        ss(value);
    std::string              part;
    while (std::getline(ss, part, sep)) out.push_back(part);
    return out;
}

double toDegrees(std::string value) {
    value = trim(lower(value));
    if (contains(value, "deg")) return std::stod(trim(replaceAll(value, "deg", "")));
    if (contains(value, "grad")) return std::stod(trim(replaceAll(value, "grad", ""))) / 400.0 * 360.0;
    if (contains(value, "rad")) return std::stod(trim(replaceAll(value, "rad", ""))) * (180.0 / std::numbers::pi);
    if (contains(value, "turn")) return std::stod(trim(replaceAll(value, "turn", ""))) * 360.0;
    return parseDouble(value, 360);
}

std::string fill(std::string const& hex) {
    if (hex.size() == 3 || hex.size() == 4) {
        std::string v;
        for (char c : hex) {
            v.push_back(c);
            v.push_back(c);
        }
        return v;
    }
    return hex;
}

}

Color Color::rgb(int r, int g, int b, double opacity) {
    Color c;
    c.red     = r;
    c.green   = g;
    c.blue    = b;
    c.opacity = opacity;
    return c;
}

Color Color::hsl(double h, double s, double l, double opacity) {
    double c  = (1 - std::abs((2 * l) - 1)) * s;
    double hh = h / 60;
    double x  = c * (1 - std::abs(std::fmod(hh, 2.0) - 1));
    double rgb[3]{0, 0, 0};
    if (hh >= 0 && hh < 1) rgb[0] = c, rgb[1] = x;
    else if (hh >= 1 && hh < 2) rgb[0] = x, rgb[1] = c;
    else if (hh >= 2 && hh < 3) rgb[1] = c, rgb[2] = x;
    else if (hh >= 3 && hh < 4) rgb[1] = x, rgb[2] = c;
    else if (hh >= 4 && hh < 5) rgb[0] = x, rgb[2] = c;
    else if (hh >= 5 && hh < 6) rgb[0] = c, rgb[2] = x;
    double m     = l - (c / 2);
    Color  color = Color::rgb(int((rgb[0] + m) * 255), int((rgb[1] + m) * 255), int((rgb[2] + m) * 255), opacity);
    color.hue        = h;
    color.saturation = s;
    color.lightness  = l;
    return color;
}

Color Color::hex(std::string value) {
    value           = trim(value);
    std::string hex = value.starts_with("#") ? value.substr(1) : value;
    std::string f   = fill(hex);
    int         r   = std::stoi(f.substr(0, 2), nullptr, 16);
    int         g   = std::stoi(f.substr(2, 2), nullptr, 16);
    int         b   = std::stoi(f.substr(4, 2), nullptr, 16);
    double      a   = 1.0;
    if (f.size() == 8) a = std::stoi(f.substr(6, 2), nullptr, 16) / 255.0;
    return Color::rgb(r, g, b, a);
}

std::optional<Color> Color::valueOf(std::string value) {
    try {
        value = trim(lower(value));
        if (contains(value, "rgb")) {
            auto rgb = split(value);
            if (rgb.size() < 3) return std::nullopt;
            bool p0 = contains(rgb[0], "%"), p1 = contains(rgb[1], "%"), p2 = contains(rgb[2], "%");
            if ((p0 || p1 || p2) && !(p0 && p1 && p2)) return std::nullopt;
            double opacity = rgb.size() >= 4 ? parseDouble(rgb[3], 1) : 1.0;
            return Color::rgb(parseInt(rgb[0], 255), parseInt(rgb[1], 255), parseInt(rgb[2], 255), opacity);
        }
        if (contains(value, "hsl")) {
            auto hsl = split(value);
            if (hsl.size() < 3) return std::nullopt;
            double opacity = hsl.size() >= 4 ? parseDouble(hsl[3], 1) : 1.0;
            return Color::hsl(toDegrees(hsl[0]), parseDouble(hsl[1], 1), parseDouble(hsl[2], 1), opacity);
        }
        if (value == "transparent") return Color::rgb(0, 0, 0, 0);
        if (value.starts_with("#") || value.size() == 3 || value.size() == 4 || value.size() == 6 || value.size() == 8) return hex(value);
    } catch (...) {}
    return std::nullopt;
}

Color Color::rgb(uint32_t color) {
    double a = ((color >> 24) & 255) / 255.0;
    if (a == 0) a = 1;
    return Color::rgb(int((color >> 16) & 255), int((color >> 8) & 255), int(color & 255), a);
}

uint32_t Color::toInt() const {
    return ((uint32_t(int(opacity * 255)) & 0xff) << 24) + ((uint32_t(red) & 0xff) << 16) + ((uint32_t(green) & 0xff) << 8) + (uint32_t(blue) & 0xff);
}

}
