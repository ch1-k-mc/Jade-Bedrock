// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "platform/text/Component.h"

#include <cctype>

#include "platform/text/I18n.h"

namespace jade::text {

namespace {

bool isSection(std::string const& s, size_t i) {
    return i + 1 < s.size() && static_cast<unsigned char>(s[i]) == 0xC2 && static_cast<unsigned char>(s[i + 1]) == 0xA7;
}

bool isColorCode(char c) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return std::isdigit(static_cast<unsigned char>(c)) || (c >= 'a' && c <= 'j') || c == 'm' || c == 'n' || (c >= 'p' && c <= 'v');
}

template <class Pred>
std::string stripIf(std::string const& s, Pred pred) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (isSection(s, i) && i + 2 < s.size() && pred(s[i + 2])) {
            i += 2;
            continue;
        }
        out.push_back(s[i]);
    }
    return out;
}

}

std::string stripFormatting(std::string const& s) {
    return stripIf(s, [](char) { return true; });
}

std::string stripColorCodes(std::string const& s) { return stripIf(s, isColorCode); }

bool Style::isEmpty() const { return !color && !bold && !italic && !underlined && !strikethrough && !obfuscated; }

Style Style::applyTo(Style const& base) const {
    Style r = base;
    if (color) r.color = color;
    if (bold) r.bold = bold;
    if (italic) r.italic = italic;
    if (underlined) r.underlined = underlined;
    if (strikethrough) r.strikethrough = strikethrough;
    if (obfuscated) r.obfuscated = obfuscated;
    return r;
}

Style Style::withColor(std::optional<uint32_t> c) const {
    Style r = *this;
    r.color = c;
    return r;
}

std::string Style::formatCodes() const {
    std::string s;
    if (obfuscated.value_or(false)) s += "\xC2\xA7k";
    if (bold.value_or(false)) s += "\xC2\xA7l";
    if (italic.value_or(false)) s += "\xC2\xA7o";
    return s;
}

Component Component::literal(std::string text, Style style) {
    Component c;
    if (!text.empty()) c.mSpans.push_back({std::move(text), style});
    return c;
}

Component Component::translatable(std::string const& key, std::vector<std::string> const& args) {
    std::vector<Component> comps;
    comps.reserve(args.size());
    for (auto const& a : args) comps.push_back(literal(a));
    return translatable(key, comps);
}

Component Component::translatable(std::string const& key, std::vector<Component> const& args) {
    std::string const pattern = I18n::get(key);
    Component         out;
    std::string       buf;
    size_t            next  = 0;
    auto              flush = [&] {
        if (!buf.empty()) out.mSpans.push_back({std::move(buf), {}});
        buf.clear();
    };
    for (size_t i = 0; i < pattern.size(); ++i) {
        char c = pattern[i];
        if (c != '%' || i + 1 >= pattern.size()) {
            buf.push_back(c);
            continue;
        }
        if (pattern[i + 1] == '%') {
            buf.push_back('%');
            ++i;
            continue;
        }
        size_t j     = i + 1;
        int    index = -1;
        while (j < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[j]))) ++j;
        if (j > i + 1 && j < pattern.size() && pattern[j] == '$') {
            index = std::stoi(pattern.substr(i + 1, j - i - 1)) - 1;
            ++j;
        } else {
            j = i + 1;
        }
        if (j < pattern.size() && (pattern[j] == 's' || pattern[j] == 'd')) {
            size_t a = index >= 0 ? static_cast<size_t>(index) : next++;
            flush();
            if (a < args.size()) out.append(args[a]);
            i = j;
            continue;
        }
        buf.push_back(c);
    }
    flush();
    return out;
}

Component& Component::append(Component const& other) {
    mSpans.insert(mSpans.end(), other.mSpans.begin(), other.mSpans.end());
    return *this;
}

Component& Component::withStyle(Style const& style) {
    for (auto& s : mSpans) s.style = s.style.applyTo(style);
    return *this;
}

Component& Component::setColor(std::optional<uint32_t> color) {
    for (auto& s : mSpans) s.style.color = color;
    return *this;
}

bool Component::isEmpty() const {
    for (auto const& s : mSpans)
        if (!s.text.empty()) return false;
    return true;
}

std::string Component::getString() const {
    std::string out;
    for (auto const& s : mSpans) out += stripFormatting(s.text);
    return out;
}

Style Component::getStyle() const { return mSpans.empty() ? Style{} : mSpans.front().style; }

Component Component::stripColor() const {
    Component out;
    for (auto const& s : mSpans) {
        std::string t = stripColorCodes(s.text);
        if (t.empty()) continue;
        out.mSpans.push_back({std::move(t), s.style.withColor(std::nullopt)});
    }
    return out;
}

}
