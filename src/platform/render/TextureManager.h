// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
// Includes code adapted from the JustEnoughItems Bedrock port (MIT; derived from JEI, Copyright (c) 2014-2015 mezz).
#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <string>

#include "nlohmann/json.hpp"

class MinecraftUIRenderContext;
namespace mce {
class ClientTexture;
class TexturePtr;
}

namespace jade::platform::render {

class TextureManager {
public:
    static TextureManager& get();

    void setAssetsDir(std::filesystem::path dir) { mAssetsDir = std::move(dir); }

    mce::ClientTexture const* getClientTexture(MinecraftUIRenderContext& ctx, std::string const& relativePath);

    [[nodiscard]] std::optional<std::pair<uint32_t, uint32_t>> textureSize(std::string const& relativePath) const;

    static std::string const& flushMaterialName();

    [[nodiscard]] nlohmann::json status() const;
    void                         clear();

private:
    struct Entry {
        std::shared_ptr<mce::TexturePtr> texture;
        int                              attempt = 0;
        std::chrono::steady_clock::time_point started{};
        bool                             loaded  = false;
        bool                             failed  = false;
        std::string                      how;
        uint32_t                         width  = 0;
        uint32_t                         height = 0;
    };

    std::filesystem::path        mAssetsDir;
    std::map<std::string, Entry> mEntries;
    mutable std::mutex           mMutex;
};

}
