// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace jade::text {

struct Style {
    std::optional<uint32_t> color;
    std::optional<bool>     bold;
    std::optional<bool>     italic;
    std::optional<bool>     underlined;
    std::optional<bool>     strikethrough;
    std::optional<bool>     obfuscated;

    [[nodiscard]] bool  isEmpty() const;
    [[nodiscard]] Style applyTo(Style const& base) const;
    [[nodiscard]] Style withColor(std::optional<uint32_t> c) const;
    [[nodiscard]] std::string formatCodes() const;
    bool operator==(Style const&) const = default;
};

struct Span {
    std::string text;
    Style       style;
};

class Component {
public:
    Component() = default;

    static Component empty() { return {}; }
    static Component literal(std::string text, Style style = {});
    static Component translatable(std::string const& key, std::vector<Component> const& args = {});
    static Component translatable(std::string const& key, std::vector<std::string> const& args);

    Component& append(Component const& other);
    Component& append(std::string const& text) { return append(literal(text)); }
    Component& withStyle(Style const& style);
    Component& withColor(uint32_t color) { return withStyle(Style{.color = color}); }
    Component& setColor(std::optional<uint32_t> color);
    [[nodiscard]] Component copy() const { return *this; }

    [[nodiscard]] std::vector<Span> const& spans() const { return mSpans; }
    [[nodiscard]] bool                     isEmpty() const;
    [[nodiscard]] std::string getString() const;
    [[nodiscard]] Style getStyle() const;

    [[nodiscard]] Component stripColor() const;

private:
    std::vector<Span> mSpans;
};

std::string stripFormatting(std::string const& s);
std::string stripColorCodes(std::string const& s);

}
