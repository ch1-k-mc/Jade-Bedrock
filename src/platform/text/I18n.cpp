// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
// Includes code adapted from the JustEnoughItems Bedrock port (MIT; derived from JEI, Copyright (c) 2014-2015 mezz).
#include "platform/text/I18n.h"

#include <algorithm>
#include <cctype>
#include <fstream>

#include "ll/api/mod/NativeMod.h"

#include "mc/locale/I18n.h"
#include "mc/locale/Localization.h"

#include "nlohmann/json.hpp"

namespace jade::text {

std::map<std::string, std::map<std::string, std::string>> I18n::sLanguages;
std::string                                                I18n::sOverride;
std::mutex                                                 I18n::sMutex;

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
}

void I18n::load(std::filesystem::path const& langDir) {
    std::lock_guard lock(sMutex);
    sLanguages.clear();
    std::error_code ec;
    for (auto const& entry : std::filesystem::directory_iterator(langDir, ec)) {
        if (entry.path().extension() != ".json") continue;
        std::ifstream in(entry.path());
        auto          j = nlohmann::json::parse(in, nullptr, false);
        if (!j.is_object()) continue;
        auto& table = sLanguages[lower(entry.path().stem().string())];
        for (auto const& [k, v] : j.items()) {
            if (v.is_string()) table[k] = v.get<std::string>();
        }
    }
    ll::mod::NativeMod::current()->getLogger().info("Jade: {} language files", sLanguages.size());
}

std::string I18n::currentLanguage() {
    {
        std::lock_guard lock(sMutex);
        if (!sOverride.empty()) return sOverride;
    }
    try {
        auto lang = getI18n().getCurrentLanguage();
        return lower(lang->mCode.get());
    } catch (...) {
        return "en_us";
    }
}

void I18n::setLanguageOverride(std::string code) {
    std::lock_guard lock(sMutex);
    sOverride = lower(std::move(code));
}

bool I18n::lookupJade(std::string const& code, std::string const& key, std::string& out) {
    std::lock_guard lock(sMutex);
    for (auto const& c : {code, std::string("en_us")}) {
        auto lang = sLanguages.find(c);
        if (lang == sLanguages.end()) continue;
        auto it = lang->second.find(key);
        if (it != lang->second.end()) {
            out = it->second;
            return true;
        }
    }
    return false;
}

std::string I18n::get(std::string const& key) {
    std::string out;
    if (lookupJade(currentLanguage(), key, out)) return out;
    try {
        std::string v = getI18n().get(key, nullptr);
        if (!v.empty()) return v;
    } catch (...) {}
    return key;
}

bool I18n::has(std::string const& key) {
    std::string out;
    if (lookupJade(currentLanguage(), key, out)) return true;
    try {
        return getI18n().get(key, nullptr) != key;
    } catch (...) {
        return false;
    }
}

std::string I18n::format(std::string const& pattern, std::vector<std::string> const& args) {
    std::string out;
    size_t      next = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        char c = pattern[i];
        if (c != '%' || i + 1 >= pattern.size()) {
            out.push_back(c);
            continue;
        }
        if (pattern[i + 1] == '%') {
            out.push_back('%');
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
            if (a < args.size()) out += args[a];
            i = j;
            continue;
        }
        out.push_back(c);
    }
    return out;
}

std::string I18n::messageFormat(std::string const& pattern, std::vector<std::string> const& args) {
    std::string out;
    bool        quoted = false;
    for (size_t i = 0; i < pattern.size(); ++i) {
        char c = pattern[i];
        if (c == '\'') {
            if (i + 1 < pattern.size() && pattern[i + 1] == '\'') {
                out.push_back('\'');
                ++i;
            } else {
                quoted = !quoted;
            }
            continue;
        }
        if (!quoted && c == '{') {
            size_t end = pattern.find('}', i);
            if (end != std::string::npos) {
                std::string inner = pattern.substr(i + 1, end - i - 1);
                auto        comma = inner.find(',');
                std::string num   = inner.substr(0, comma);
                bool        ok    = !num.empty() && std::all_of(num.begin(), num.end(), [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)); });
                if (ok) {
                    size_t idx = static_cast<size_t>(std::stoi(num));
                    out += idx < args.size() ? args[idx] : "{" + num + "}";
                    i = end;
                    continue;
                }
            }
        }
        out.push_back(c);
    }
    return out;
}

}
