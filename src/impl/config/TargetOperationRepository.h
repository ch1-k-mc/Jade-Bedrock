// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <filesystem>
#include <map>
#include <mutex>
#include <regex>
#include <string>
#include <vector>

namespace jade::impl::config {

enum class TargetOperation { HIDE, PICK };

class TargetOperationRepository {
public:
    TargetOperationRepository(std::string fileName, std::vector<std::string> defaultValues)
    : mFileName(std::move(fileName)),
      mDefaultValues(std::move(defaultValues)) {}

    void reload(std::filesystem::path const& configDir);

    [[nodiscard]] bool shouldHide(std::string const& id) const;
    [[nodiscard]] bool shouldPick(std::string const& id) const;
    void               hide(std::string const& id) { mBuiltIn[normalize(id)] = TargetOperation::HIDE; }
    void               pick(std::string const& id) { mBuiltIn[normalize(id)] = TargetOperation::PICK; }

    static std::string normalize(std::string const& id) { return id.find(':') == std::string::npos ? "minecraft:" + id : id; }

private:
    std::string                            mFileName;
    std::vector<std::string>               mDefaultValues;
    std::map<std::string, TargetOperation> mBuiltIn;
    std::map<std::string, TargetOperation> mOperations;
    std::vector<std::regex>                mPatterns;
    mutable std::mutex                     mMutex;
};

}
