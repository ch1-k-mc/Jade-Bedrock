// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "addon/bedrock/BedrockPlugin.h"

#include <cstdio>
#include <map>
#include <mutex>
#include <regex>
#include <vector>

#include "ll/api/service/Bedrock.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/deps/core/file/Path.h"
#include "mc/deps/core/file/PathView.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/resources/PackInstance.h"
#include "mc/resources/ResourcePack.h"
#include "mc/resources/ResourcePackManager.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/CauldronBlockActor.h"

#include "api/config/IWailaConfig.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"
#include "platform/text/I18n.h"
#include "util/BlockStates.h"

namespace jade::addon::bedrock {

using text::Component;
using text::I18n;

std::optional<CauldronInfo> CauldronProvider::streamData(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be || be->getType() != BlockActorType::Cauldron) return std::nullopt;
    auto*        c = static_cast<CauldronBlockActor*>(be);
    CauldronInfo info;
    info.potionId   = c->mPotionId;
    info.potionType = static_cast<int>(static_cast<PotionType>(c->mPotionType));
    info.hasColor   = c->mHasCustomColor;
    info.color      = c->mCustomColor;
    if (info.potionId < 0 && !info.hasColor) return std::nullopt;
    return info;
}

void CauldronProvider::encode(CompoundTag& out, CauldronInfo const& v) {
    out["potion"]   = v.potionId;
    out["type"]     = v.potionType;
    out["color"]    = v.color;
    out["hasColor"] = v.hasColor ? 1 : 0;
}

std::optional<CauldronInfo> CauldronProvider::decode(CompoundTag const& in) {
    CauldronInfo v;
    v.potionId   = static_cast<int>(in.at("potion"));
    v.potionType = static_cast<int>(in.at("type"));
    v.color      = static_cast<int>(in.at("color"));
    v.hasColor   = static_cast<int>(in.at("hasColor")) != 0;
    return v;
}

void CauldronProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto info = CauldronProvider::INSTANCE->decodeFromData(accessor);
    if (!info) return;
    if (info->potionId < 0 && info->hasColor) {
        char hex[16];
        snprintf(hex, sizeof(hex), "#%06X", static_cast<unsigned>(info->color) & 0xFFFFFF);
        tooltip.add(Component::literal(I18n::format(I18n::get("jade.bedrock.cauldron.color"), {hex})));
    }
}

void ComposterProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto level = util::BlockStates::intState(accessor.getBlock(), "composter_fill_level");
    if (!level) return;
    if (*level >= 8) tooltip.add(Component::literal(I18n::get("jade.bedrock.composter.ready")));
    else tooltip.add(Component::literal(I18n::format(I18n::get("jade.bedrock.composter.level"), {std::to_string(*level), "7"})));
}

void RespawnAnchorProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto charge = util::BlockStates::intState(accessor.getBlock(), "respawn_anchor_charge");
    if (!charge) return;
    tooltip.add(Component::literal(I18n::format(I18n::get("jade.bedrock.anchor.charge"), {std::to_string(*charge), "4"})));
}

namespace {

std::mutex                         gPackMutex;
bool                               gPacksBuilt = false;
std::map<std::string, std::string> gPackNames;

std::string cleanName(std::string name) {
    static std::regex const version(R"([ _-]*v?\d+\.\d+\.\d+\s*$)");
    return std::regex_replace(name, version, "");
}

void scanText(std::string const& text, std::string const& pack) {
    static std::regex const id(R"re("(?:identifier|[a-z0-9_.-]+:[a-z0-9_./-]+)"\s*:\s*(?:"([a-z0-9_.-]+):[^"]+")?)re");
    static std::regex const key(R"re("([a-z0-9_.-]+):[a-z0-9_./-]+"\s*:)re");
    for (std::sregex_iterator it(text.begin(), text.end(), id), end; it != end; ++it) {
        if ((*it)[1].matched && (*it)[1].str() != "minecraft") gPackNames.try_emplace((*it)[1].str(), pack);
    }
    for (std::sregex_iterator it(text.begin(), text.end(), key), end; it != end; ++it) {
        auto ns = (*it)[1].str();
        if (ns != "minecraft" && ns != "format_version") gPackNames.try_emplace(ns, pack);
    }
}

void scanManager(ResourcePackManager* manager) {
    if (!manager) return;
    manager->iteratePacks([](PackInstance const& inst) {
        ResourcePack const& pack = *inst.mPack.get();
        auto const&         impl = *pack.mImpl.get();
        if (impl.mIsBaseGamePack) return;
        std::string name = cleanName(impl.mNameWithVersionForTelemetry.get());
        if (name.empty()) return;
        std::string text;
        if (pack.getResource(Core::PathView("blocks.json"), text, 0)) scanText(text, name);
        for (char const* dir : {"blocks", "items", "entity", "entities"}) {
            pack.forEachIn(
                Core::Path(dir),
                [&](Core::Path const& file) {
                    std::string body;
                    if (file.getUtf8StdString().ends_with(".json") && pack.getResource(Core::PathView(file), body, 0)) scanText(body, name);
                },
                0,
                true
            );
        }
    });
}

}

std::optional<std::string> PackNames::find(std::string const& ns) {
    if (ns == "minecraft") return std::nullopt;
    auto& cfg = config::IWailaConfig::get().plugin();
    if (!cfg.get(PACK_NAME)) return std::nullopt;
    std::lock_guard lock(gPackMutex);
    if (!gPacksBuilt) {
        auto level = ll::service::getMultiPlayerLevel();
        if (!level) return std::nullopt;
        gPacksBuilt = true;
        scanManager(level->getClientResourcePackManager());
        scanManager(level->getServerResourcePackManager());
        if (auto server = ll::service::getLevel()) scanManager(server->getServerResourcePackManager());
    }
    auto it = gPackNames.find(ns);
    if (it == gPackNames.end()) return std::nullopt;
    return it->second;
}

void PackNames::reset() {
    std::lock_guard lock(gPackMutex);
    gPacksBuilt = false;
    gPackNames.clear();
}

void BedrockPlugin::register_(impl::WailaCommonRegistration& registration) {
    registration.registerBlockDataProvider(CauldronProvider::INSTANCE, BlockClass::ofTypes({"minecraft:cauldron"}));
}

void BedrockPlugin::registerClient(impl::WailaClientRegistration& registration) {
    registration.registerBlockComponent(CauldronProvider::Client::INSTANCE, BlockClass::ofTypes({"minecraft:cauldron"}));
    registration.registerBlockComponent(std::make_shared<ComposterProvider>(), BlockClass::ofTypes({"minecraft:composter"}));
    registration.markAsClientFeature(COMPOSTER);
    registration.registerBlockComponent(std::make_shared<RespawnAnchorProvider>(), BlockClass::ofTypes({"minecraft:respawn_anchor"}));
    registration.markAsClientFeature(ANCHOR);
    registration.addConfig(PACK_NAME, "minecraft:book", true);
    registration.markAsClientFeature(PACK_NAME);
}

}
