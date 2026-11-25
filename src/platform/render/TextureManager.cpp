// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
// Includes code adapted from the JustEnoughItems Bedrock port (MIT; derived from JEI, Copyright (c) 2014-2015 mezz).
#include "platform/render/TextureManager.h"

#include "ll/api/mod/NativeMod.h"

#include "mc/client/renderer/screen/MinecraftUIRenderContext.h"
#include "mc/deps/core/file/PathView.h"
#include "mc/deps/core/resource/ResourceFileSystem.h"
#include "mc/deps/core/resource/ResourceLocation.h"
#include "mc/deps/minecraft_renderer/renderer/BedrockTextureData.h"
#include "mc/deps/minecraft_renderer/renderer/TexturePtr.h"

namespace jade::platform::render {

namespace {

struct Candidate {
    char const*        name;
    ResourceFileSystem fs;
    bool               absolute;
};

constexpr Candidate kCandidates[]{
    {"Raw+abs",           ResourceFileSystem::Raw,           true},
    {"RawPersistent+abs", ResourceFileSystem::RawPersistent, true},
    {"ExternalDir+abs",   ResourceFileSystem::ExternalDir,   true},
    {"UserPackage+abs",   ResourceFileSystem::UserPackage,   true},
};
constexpr Candidate kGameCandidates[]{
    {"UserPackage+rel", ResourceFileSystem::UserPackage, false},
    {"AppPackage+rel",  ResourceFileSystem::AppPackage,  false},
    {"Raw+rel",         ResourceFileSystem::Raw,         false},
};
constexpr std::string_view kGamePrefix = "game:";

constexpr auto kWaitPerCandidate = std::chrono::seconds(5);

bool hasFlag(TextureLoadState s, TextureLoadState f) { return (static_cast<uint8_t>(s) & static_cast<uint8_t>(f)) != 0; }

}

TextureManager& TextureManager::get() {
    static TextureManager instance;
    return instance;
}

std::string const& TextureManager::flushMaterialName() {
    static std::string const name = "ui_flush";
    return name;
}

mce::ClientTexture const* TextureManager::getClientTexture(MinecraftUIRenderContext& ctx, std::string const& relativePath) {
    std::lock_guard lock(mMutex);
    Entry& e = mEntries[relativePath];
    if (e.failed) return nullptr;
    bool             game       = relativePath.starts_with(kGamePrefix);
    Candidate const* candidates = game ? kGameCandidates : kCandidates;
    int const        kCount     = game ? static_cast<int>(std::size(kGameCandidates)) : static_cast<int>(std::size(kCandidates));
    while (e.attempt < kCount) {
        auto const& c = candidates[e.attempt];
        if (!e.texture) {
            std::string path;
            if (game) {
                path = relativePath.substr(kGamePrefix.size());
            } else {
                std::u8string u8 = (mAssetsDir / std::filesystem::path(relativePath)).generic_u8string();
                path             = c.absolute ? std::string(reinterpret_cast<char const*>(u8.c_str()), u8.size()) : relativePath;
            }
            ResourceLocation loc(Core::PathView(path), c.fs);
            e.texture = std::make_shared<mce::TexturePtr>(ctx.getTexture(loc, false));
            e.started = std::chrono::steady_clock::now();
            e.how     = c.name;
        }
        auto const& data = e.texture->mClientTexture.get();
        if (data) {
            bool missing = data->mIsMissingTexture == IsMissingTexture::Yes;
            bool loaded  = hasFlag(data->mTextureLoadState, TextureLoadState::LoadedBit);
            if (loaded && !missing) {
                if (!e.loaded) {
                    e.loaded = true;
                    e.width  = data->mTextureDescription->mWidth;
                    e.height = data->mTextureDescription->mHeight;
                    ll::mod::NativeMod::current()->getLogger().info("Jade: texture loaded ({}): {}", e.how, relativePath);
                }
                return &data->mClientTexture.get();
            }
            if (loaded && missing) {
                e.texture.reset();
                ++e.attempt;
                continue;
            }
        }
        if (std::chrono::steady_clock::now() - e.started < kWaitPerCandidate) return nullptr;
        e.texture.reset();
        ++e.attempt;
    }
    e.failed = true;
    ll::mod::NativeMod::current()->getLogger().warn("Jade: cannot load texture (tried all candidates): {}", relativePath);
    return nullptr;
}

nlohmann::json TextureManager::status() const {
    std::lock_guard lock(mMutex);
    nlohmann::json  j = nlohmann::json::object();
    for (auto const& [path, e] : mEntries) {
        j[path] = {{"loaded", e.loaded}, {"failed", e.failed}, {"how", e.how}, {"attempt", e.attempt}, {"size", {e.width, e.height}}};
    }
    return j;
}

std::optional<std::pair<uint32_t, uint32_t>> TextureManager::textureSize(std::string const& relativePath) const {
    std::lock_guard lock(mMutex);
    auto            it = mEntries.find(relativePath);
    if (it == mEntries.end() || !it->second.loaded) return std::nullopt;
    return std::make_pair(it->second.width, it->second.height);
}

void TextureManager::clear() {
    std::lock_guard lock(mMutex);
    mEntries.clear();
}

}
