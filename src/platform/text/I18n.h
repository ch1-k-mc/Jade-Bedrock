// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
// Includes code adapted from the JustEnoughItems Bedrock port (MIT; derived from JEI, Copyright (c) 2014-2015 mezz).
#pragma once

#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace jade::text {

class I18n {
public:
    static void load(std::filesystem::path const& langDir);

    static std::string get(std::string const& key);
    static bool has(std::string const& key);

    static std::string currentLanguage();
    static void        setLanguageOverride(std::string code);

    static std::string format(std::string const& pattern, std::vector<std::string> const& args);
    static std::string messageFormat(std::string const& pattern, std::vector<std::string> const& args);
    static std::string formatString(std::string const& key, std::vector<std::string> const& args) { return messageFormat(get(key), args); }

private:
    static bool lookupJade(std::string const& code, std::string const& key, std::string& out);

    static std::map<std::string, std::map<std::string, std::string>> sLanguages;
    static std::string                                                sOverride;
    static std::mutex                                                 sMutex;
};

}
