// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/config/TargetOperationRepository.h"

#include <fstream>

#include "ll/api/mod/NativeMod.h"

#include "nlohmann/json.hpp"

namespace jade::impl::config {

void TargetOperationRepository::reload(std::filesystem::path const& configDir) {
    std::lock_guard lock(mMutex);
    mOperations = mBuiltIn;
    mPatterns.clear();
    auto                     path = configDir / "jade" / (mFileName + ".json");
    std::vector<std::string> values;
    bool                     loaded = false;
    {
        std::ifstream in(path);
        if (in) {
            auto j = nlohmann::json::parse(in, nullptr, false);
            if (j.is_object() && j.contains("values") && j["values"].is_array()) {
                for (auto const& v : j["values"])
                    if (v.is_string()) values.push_back(v.get<std::string>());
                loaded = true;
            }
        }
    }
    if (!loaded) {
        values = mDefaultValues;
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (out) out << nlohmann::json{{"values", values}}.dump(2);
    }
    for (auto const& value : values) {
        try {
            if (value.size() > 1 && value.front() == '/' && value.back() == '/') {
                mPatterns.emplace_back(value.substr(1, value.size() - 2));
            } else {
                mOperations[normalize(value)] = TargetOperation::HIDE;
            }
        } catch (std::exception const& e) {
            ll::mod::NativeMod::current()->getLogger().error("Jade: cannot read hide-list entry: {} ({})", value, e.what());
        }
    }
}

bool TargetOperationRepository::shouldHide(std::string const& id) const {
    std::lock_guard lock(mMutex);
    auto            it = mOperations.find(id);
    if (it != mOperations.end()) return it->second == TargetOperation::HIDE;
    for (auto const& p : mPatterns)
        if (std::regex_search(id, p)) return true;
    return false;
}

bool TargetOperationRepository::shouldPick(std::string const& id) const {
    std::lock_guard lock(mMutex);
    auto            it = mOperations.find(id);
    return it != mOperations.end() && it->second == TargetOperation::PICK;
}

}
