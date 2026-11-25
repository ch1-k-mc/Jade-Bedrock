// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <filesystem>

namespace jade {

class JadeClient {
public:
    static bool init(std::filesystem::path const& assetsDir, std::filesystem::path const& configDir);
    static void shutdown();

    [[nodiscard]] static std::filesystem::path const& configDir();
    [[nodiscard]] static std::filesystem::path const& assetsDir();
};

}
