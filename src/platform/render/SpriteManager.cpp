// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "platform/render/SpriteManager.h"

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace jade::platform::render {

namespace {

bool readPngSize(std::filesystem::path const& path, int& w, int& h) {
    std::ifstream in(path, std::ios::binary);
    unsigned char buf[24]{};
    if (!in.read(reinterpret_cast<char*>(buf), sizeof(buf))) return false;
    if (buf[0] != 0x89 || buf[1] != 'P' || buf[2] != 'N' || buf[3] != 'G') return false;
    auto be32 = [&](int o) { return (uint32_t(buf[o]) << 24) | (uint32_t(buf[o + 1]) << 16) | (uint32_t(buf[o + 2]) << 8) | uint32_t(buf[o + 3]); };
    w = static_cast<int>(be32(16));
    h = static_cast<int>(be32(20));
    return true;
}

void readScaling(std::filesystem::path const& mcmeta, SpriteInfo& info) {
    info.scaling        = {};
    info.scaling.width  = info.pixelWidth;
    info.scaling.height = info.pixelHeight;
    std::ifstream in(mcmeta);
    if (!in) return;
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_object() || !j.contains("gui") || !j["gui"].contains("scaling")) return;
    auto const& s    = j["gui"]["scaling"];
    std::string type = s.value("type", "stretch");
    if (type == "tile") info.scaling.type = SpriteScaling::Type::Tile;
    else if (type == "nine_slice") info.scaling.type = SpriteScaling::Type::NineSlice;
    info.scaling.width        = s.value("width", info.pixelWidth);
    info.scaling.height       = s.value("height", info.pixelHeight);
    info.scaling.stretchInner = s.value("stretch_inner", false);
    if (s.contains("border")) {
        auto const& b = s["border"];
        if (b.is_number_integer()) {
            int v                  = b.get<int>();
            info.scaling.borderLeft = info.scaling.borderTop = info.scaling.borderRight = info.scaling.borderBottom = v;
        } else if (b.is_object()) {
            info.scaling.borderLeft   = b.value("left", 0);
            info.scaling.borderTop    = b.value("top", 0);
            info.scaling.borderRight  = b.value("right", 0);
            info.scaling.borderBottom = b.value("bottom", 0);
        }
    }
}

}

SpriteManager& SpriteManager::get() {
    static SpriteManager instance;
    return instance;
}

SpriteInfo const& SpriteManager::sprite(std::string const& id) {
    std::lock_guard lock(mMutex);
    auto            it = mCache.find(id);
    if (it != mCache.end()) return it->second;
    SpriteInfo info;
    auto       colon = id.find(':');
    std::string ns   = colon == std::string::npos ? "minecraft" : id.substr(0, colon);
    std::string path = colon == std::string::npos ? id : id.substr(colon + 1);
    info.texturePath = ns + "/textures/gui/sprites/" + path + ".png";
    std::filesystem::path full = std::filesystem::path(mAssetsDir) / std::filesystem::path(info.texturePath);
    info.exists                = readPngSize(full, info.pixelWidth, info.pixelHeight);
    if (info.exists) readScaling(std::filesystem::path(full.string() + ".mcmeta"), info);
    return mCache.emplace(id, std::move(info)).first->second;
}

nlohmann::json SpriteManager::status() const {
    std::lock_guard lock(mMutex);
    nlohmann::json  j = nlohmann::json::object();
    for (auto const& [id, s] : mCache) {
        j[id] = {{"exists", s.exists}, {"size", {s.pixelWidth, s.pixelHeight}}, {"type", static_cast<int>(s.scaling.type)}};
    }
    return j;
}

void SpriteManager::clear() {
    std::lock_guard lock(mMutex);
    mCache.clear();
}

}
