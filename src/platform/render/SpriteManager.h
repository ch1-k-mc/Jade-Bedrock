// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "nlohmann/json.hpp"

namespace jade::platform::render {

struct SpriteScaling {
    enum class Type { Stretch, Tile, NineSlice };
    Type type   = Type::Stretch;
    int  width  = 0;
    int  height = 0;
    int  borderLeft = 0, borderTop = 0, borderRight = 0, borderBottom = 0;
    bool stretchInner = false;
};

struct SpriteInfo {
    std::string   texturePath;
    int           pixelWidth  = 0;
    int           pixelHeight = 0;
    SpriteScaling scaling;
    bool          exists = false;
};

class SpriteManager {
public:
    static SpriteManager& get();

    void setAssetsDir(std::string dir) { mAssetsDir = std::move(dir); }
    SpriteInfo const& sprite(std::string const& id);

    [[nodiscard]] nlohmann::json status() const;
    void                         clear();

private:
    std::string                       mAssetsDir;
    std::map<std::string, SpriteInfo> mCache;
    mutable std::mutex                mMutex;
};

}
