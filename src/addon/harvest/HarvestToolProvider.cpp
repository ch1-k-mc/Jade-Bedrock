// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/harvest/HarvestToolProvider.h"

#include <algorithm>

#include "mc/world/actor/player/Player.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/GameType.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockType.h"

#include "api/theme/IThemeHelper.h"
#include "impl/ui/Elements.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/item/ScopedItemRegistry.h"
#include "util/CommonProxy.h"

namespace jade::addon::harvest {

using impl::ui::JadeUIInternal;
using text::Component;

ToolTypeRegistry& ToolTypeRegistry::get() {
    static ToolTypeRegistry instance;
    return instance;
}

ToolType& ToolTypeRegistry::type(Identifier const& uid, bool skipInstaBreakingBlock) {
    for (auto& t : mTypes)
        if (t.uid == uid) return t;
    mTypes.push_back(ToolType{uid, {}, skipInstaBreakingBlock});
    return mTypes.back();
}

void HarvestToolProvider::registerVanillaTools(ToolTypeRegistry& registry) {
    auto tiers = [](std::string const& kind) {
        std::vector<ToolTier> out;
        for (char const* material : {"wooden", "golden", "stone", "copper", "iron", "diamond", "netherite"}) {
            out.push_back(ToolTier{std::string("minecraft:") + material + "_" + kind, {}, false});
        }
        return out;
    };
    registry.type(JadeIds::JADE("pickaxe")).tiers = tiers("pickaxe");
    registry.type(JadeIds::JADE("axe")).tiers     = tiers("axe");
    registry.type(JadeIds::JADE("shovel")).tiers  = tiers("shovel");
    registry.type(JadeIds::JADE("hoe")).tiers     = tiers("hoe");
    registry.type(JadeIds::JADE("sword")).tiers   = {ToolTier{"minecraft:wooden_sword", {}, false}};
    registry.type(JadeIds::JADE("shears"), false).tiers = {ToolTier{"minecraft:shears", {"minecraft:trip_wire"}, false}};
}

namespace {

bool isEffectiveTool(ItemStack const& tool, Block const& block, bool requiresTool) {
    Item const* item = tool.mItem.get();
    if (!item) return false;
    if (requiresTool) return item->canDestroySpecial(block);
    return item->canDestroySpecial(block) || item->getDestroySpeed(tool, block) > 1.0f;
}

}

std::vector<std::shared_ptr<ItemStack>> const& HarvestToolProvider::getTool(Block const& block) {
    auto it = mResultCache.find(&block);
    if (it != mResultCache.end()) return it->second;
    platform::item::ScopedItemRegistry      scoped;
    std::vector<std::shared_ptr<ItemStack>> tools;
    bool requiresTool = block.getBlockType().mRequiresCorrectToolForDrops;
    for (auto const& type : ToolTypeRegistry::get().types()) {
        if (type.skipInstaBreakingBlock && !requiresTool && block.mDirectData->mDestroySpeed == 0) continue;
        for (auto const& tier : type.tiers) {
            if (tier.alwaysFail) continue;
            ItemStack stack;
            stack.reinit(tier.itemName, 1, 0);
            if (stack.isNull()) continue;
            bool extra = std::find(tier.extraBlocks.begin(), tier.extraBlocks.end(), block.getTypeName()) != tier.extraBlocks.end();
            if (extra || isEffectiveTool(stack, block, requiresTool)) {
                tools.push_back(impl::ui::makeSharedItem(stack));
                break;
            }
        }
    }
    return mResultCache.emplace(&block, std::move(tools)).first->second;
}

void HarvestToolProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) {
    Player& player   = accessor.getPlayer();
    auto    gameType = player.getPlayerGameType();
    if (!config.get(JadeIds::MC_HARVEST_TOOL_CREATIVE) && (gameType == GameType::Creative || gameType == GameType::Spectator)) return;
    Block const& block = accessor.getBlock();
    if (block.mDirectData->mDestroySpeed < 0) {
        if (!accessor.isServersideContent() && config.get(JadeIds::MC_SHOW_UNBREAKABLE)) {
            auto t = JadeUIInternal::text(theme::IThemeHelper::get().failure(Component::translatable("jade.harvest_tool.unbreakable")));
            t->narration("");
            tooltip.add(t);
        }
        return;
    }
    bool newLine  = config.get(JadeIds::MC_HARVEST_TOOL_NEW_LINE);
    auto elements = getText(accessor, config);
    if (elements.empty()) return;
    for (auto& e : elements)
        if (auto* el = dynamic_cast<ui::Element*>(e.get())) el->narration("");
    if (newLine) tooltip.add(elements);
    else tooltip.append(0, elements);
}

std::vector<layout::LayoutElementPtr> HarvestToolProvider::getText(BlockAccessor& accessor, IPluginConfig& config) {
    Block const& block    = accessor.getBlock();
    bool         requiresTool = block.getBlockType().mRequiresCorrectToolForDrops;
    if (!requiresTool && !config.get(JadeIds::MC_EFFECTIVE_TOOL)) return {};
    auto const& tools = getTool(block);
    if (tools.empty()) return {};
    int                                   offsetY = -3;
    bool                                  newLine = config.get(JadeIds::MC_HARVEST_TOOL_NEW_LINE);
    std::vector<layout::LayoutElementPtr> elements;
    auto spacer = JadeUIInternal::spacer(newLine ? -2 : 5, newLine ? 10 : 0);
    spacer->flexGrow(1000);
    elements.push_back(spacer);
    for (auto const& tool : tools) {
        auto item  = JadeUIInternal::item(*tool, 0.75f);
        auto moved = JadeUIInternal::size(JadeUIInternal::offset(item, -1, offsetY), 10, 0);
        moved->narration("");
        elements.push_back(moved);
    }
    bool canHarvest = util::CommonProxy::isCorrectToolForDrops(block, accessor.getPlayer());
    if (requiresTool || !canHarvest) {
        auto& t    = theme::IThemeHelper::get();
        auto  text = JadeUIInternal::text(canHarvest ? t.success(Component::literal("✔")) : t.danger(Component::literal("✕")));
        text->scale(0.75f);
        elements.push_back(JadeUIInternal::offset(JadeUIInternal::size(text, 0, 0), -3, 6 + offsetY));
    }
    return elements;
}

}
