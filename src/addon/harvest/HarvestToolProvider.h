// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "api/IJadeProvider.h"

class ItemStack;

namespace jade::addon::harvest {

using namespace jade::api;

struct ToolTier {
    std::string                 itemName;
    std::vector<std::string>    extraBlocks;
    bool                        alwaysFail = false;
};

struct ToolType {
    Identifier            uid;
    std::vector<ToolTier> tiers;
    bool                  skipInstaBreakingBlock = true;
};

class ToolTypeRegistry {
public:
    static ToolTypeRegistry& get();
    ToolType&                type(Identifier const& uid, bool skipInstaBreakingBlock = true);
    void                     clear() { mTypes.clear(); }
    [[nodiscard]] std::vector<ToolType> const& types() const { return mTypes; }

private:
    std::vector<ToolType> mTypes;
};

class HarvestToolProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<HarvestToolProvider> const INSTANCE = std::make_shared<HarvestToolProvider>();

    Identifier  getUid() const override { return JadeIds::MC_HARVEST_TOOL; }
    std::string getConfigIcon() const override { return "minecraft:diamond_pickaxe"; }
    int         getDefaultPriority() const override { return TooltipPosition::TAIL - 2000; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;

    std::vector<layout::LayoutElementPtr> getText(BlockAccessor& accessor, IPluginConfig& config);
    void                                  invalidateCache() { mResultCache.clear(); }

    static void registerVanillaTools(ToolTypeRegistry& registry);

private:
    std::vector<std::shared_ptr<ItemStack>> const& getTool(Block const& block);

    std::map<Block const*, std::vector<std::shared_ptr<ItemStack>>> mResultCache;
};

}
