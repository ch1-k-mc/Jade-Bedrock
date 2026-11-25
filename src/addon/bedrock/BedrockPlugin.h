// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "api/IJadeProvider.h"
#include "api/IWailaPlugin.h"
#include "api/StreamServerDataProvider.h"

namespace jade::addon::bedrock {

using namespace jade::api;

inline Identifier const CAULDRON  = "jade_bedrock:cauldron";
inline Identifier const COMPOSTER = "jade_bedrock:composter";
inline Identifier const ANCHOR    = "jade_bedrock:respawn_anchor";
inline Identifier const PACK_NAME = "jade_bedrock:pack_name";

struct CauldronInfo {
    int potionId   = -1;
    int potionType = 0;
    int color      = 0;
    bool hasColor  = false;
};

class CauldronProvider : public StreamServerDataProvider<BlockAccessor, CauldronInfo> {
public:
    static inline std::shared_ptr<CauldronProvider> const INSTANCE = std::make_shared<CauldronProvider>();
    Identifier                  getUid() const override { return CAULDRON; }
    std::optional<CauldronInfo> streamData(BlockAccessor& accessor) override;
    void                        encode(CompoundTag& out, CauldronInfo const& value) override;
    std::optional<CauldronInfo> decode(CompoundTag const& in) override;

    class Client;
};

class CauldronProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return CAULDRON; }
    std::string getConfigIcon() const override { return "minecraft:cauldron"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class ComposterProvider : public IBlockComponentProvider {
public:
    Identifier  getUid() const override { return COMPOSTER; }
    std::string getConfigIcon() const override { return "minecraft:composter"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class RespawnAnchorProvider : public IBlockComponentProvider {
public:
    Identifier  getUid() const override { return ANCHOR; }
    std::string getConfigIcon() const override { return "minecraft:respawn_anchor"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class PackNames {
public:
    [[nodiscard]] static std::optional<std::string> find(std::string const& ns);
    static void                                     reset();
};

class BedrockPlugin : public IWailaPlugin {
public:
    void register_(impl::WailaCommonRegistration& registration) override;
    void registerClient(impl::WailaClientRegistration& registration) override;
};

}
