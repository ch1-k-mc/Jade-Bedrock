// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include <cmath>
#include <cstdio>
#include <map>

#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/VanillaStates.h"
#include "mc/world/level/block/actor/BeehiveBlockActor.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/ComparatorBlockActor.h"

#include "api/theme/IThemeHelper.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/item/ScopedItemRegistry.h"
#include "util/BlockStates.h"

namespace jade::addon::vanilla {

using text::Component;

namespace {

bool isAny(Block const& b, std::initializer_list<char const*> names) {
    auto const& t = b.getTypeName();
    for (auto n : names)
        if (t == n) return true;
    return false;
}

std::vector<Component> args(std::initializer_list<Component> c) { return {c}; }

}

std::optional<int> RedstoneProvider::streamData(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    if (be && be->getType() == BlockActorType::Comparator) return static_cast<ComparatorBlockActor*>(be)->mOutput;
    return std::nullopt;
}

std::optional<int> RedstoneProvider::decode(CompoundTag const& in) { return static_cast<int>(in.at("signal")); }

void RedstoneProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    Block const& block = accessor.getBlock();
    auto&        t     = theme::IThemeHelper::get();
    if (isAny(block, {"minecraft:lever"})) {
        bool      on   = block.getState<bool>(VanillaStates::OpenBit()).value_or(false);
        Component info = on ? t.success(Component::translatable("tooltip.jade.state_on")) : t.danger(Component::translatable("tooltip.jade.state_off"));
        tooltip.add(Component::translatable("tooltip.jade.state", args({info})));
        return;
    }
    if (isAny(block, {"minecraft:unpowered_repeater", "minecraft:powered_repeater"})) {
        int delay = block.getState<int>(VanillaStates::RepeaterDelay()).value_or(0) + 1;
        tooltip.add(Component::translatable("tooltip.jade.delay", args({t.info(double(delay))})));
        return;
    }
    auto signal = RedstoneProvider::INSTANCE->decodeFromData(accessor);
    if (isAny(block, {"minecraft:unpowered_comparator", "minecraft:powered_comparator"})) {
        bool      subtract = util::BlockStates::boolState(block, "output_subtract_bit").value_or(false);
        Component modeInfo = t.info(Component::translatable(std::string("tooltip.jade.mode_") + (subtract ? "subtractor" : "comparator")));
        tooltip.add(Component::translatable("tooltip.jade.mode", args({modeInfo})));
        if (signal) tooltip.add(Component::translatable("tooltip.jade.power", args({t.info(double(*signal))})));
        return;
    }
    if (auto power = block.getState<int>(VanillaStates::RedstoneSignal())) {
        tooltip.add(Component::translatable("tooltip.jade.power", args({t.info(double(*power))})));
    }
}

namespace {

struct CropInfo {
    enum class State { Growth, Age } state;
    int max;
};

std::map<std::string, CropInfo> const& crops() {
    static std::map<std::string, CropInfo> const m{
        {"minecraft:wheat", {CropInfo::State::Growth, 7}},
        {"minecraft:carrots", {CropInfo::State::Growth, 7}},
        {"minecraft:potatoes", {CropInfo::State::Growth, 7}},
        {"minecraft:beetroot", {CropInfo::State::Growth, 7}},
        {"minecraft:melon_stem", {CropInfo::State::Growth, 7}},
        {"minecraft:pumpkin_stem", {CropInfo::State::Growth, 7}},
        {"minecraft:torchflower_crop", {CropInfo::State::Growth, 7}},
        {"minecraft:pitcher_crop", {CropInfo::State::Growth, 4}},
        {"minecraft:sweet_berry_bush", {CropInfo::State::Growth, 3}},
        {"minecraft:nether_wart", {CropInfo::State::Age, 3}},
        {"minecraft:cocoa", {CropInfo::State::Age, 2}},
    };
    return m;
}

void addMaturityTooltip(ITooltip& tooltip, float growthValue) {
    auto&     t = theme::IThemeHelper::get();
    Component component;
    if (growthValue < 1) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.0f%%", growthValue * 100);
        component = t.info(std::string(buf));
    } else {
        component = t.success(Component::translatable("tooltip.jade.crop_mature"));
    }
    tooltip.add(Component::translatable("tooltip.jade.crop_growth", args({component})));
}

}

ui::ElementPtr CropProgressProvider::getIcon(BlockAccessor& accessor, IPluginConfig&, ui::ElementPtr) {
    char const* item = nullptr;
    if (accessor.getBlock().getTypeName() == "minecraft:wheat") item = "minecraft:wheat";
    else if (accessor.getBlock().getTypeName() == "minecraft:beetroot") item = "minecraft:beetroot";
    if (!item) return nullptr;
    platform::item::ScopedItemRegistry scoped;
    ItemStack                          stack;
    stack.reinit(item, 1, 0);
    return stack.isNull() ? nullptr : impl::ui::JadeUIInternal::item(stack);
}

void CropProgressProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    Block const& block = accessor.getBlock();
    auto         it    = crops().find(block.getTypeName());
    if (it == crops().end()) return;
    auto const& info  = it->second;
    int         value = info.state == CropInfo::State::Growth ? block.getState<int>(VanillaStates::Growth()).value_or(0)
                                                              : block.getState<int>(VanillaStates::Age()).value_or(0);
    addMaturityTooltip(tooltip, std::min(1.0f, static_cast<float>(value) / static_cast<float>(info.max)));
}

namespace {
BeehiveBlockActor* beehiveOf(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    return be && be->getType() == BlockActorType::Beehive ? static_cast<BeehiveBlockActor*>(be) : nullptr;
}
constexpr int MAX_OCCUPANTS = 3;
}

bool BeehiveProvider::shouldRequestData(BlockAccessor& accessor) { return beehiveOf(accessor) != nullptr; }

std::optional<int> BeehiveProvider::streamData(BlockAccessor& accessor) {
    auto* hive = beehiveOf(accessor);
    if (!hive) return std::nullopt;
    int bees = static_cast<int>(hive->mOccupants->size());
    return bees >= MAX_OCCUPANTS ? bees : -bees;
}

std::optional<int> BeehiveProvider::decode(CompoundTag const& in) { return static_cast<int>(in.at("bees")); }

void BeehiveProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto& t     = theme::IThemeHelper::get();
    int   level = accessor.getBlock().getState<int>(VanillaStates::BeehiveHoneyLevel()).value_or(0);
    auto  value = Component::translatable("jade.fraction", std::vector<std::string>{std::to_string(level), "5"});
    tooltip.add(Component::translatable("jade.beehive.honey", args({level == 5 ? t.success(value) : t.info(value)})));
    auto b = BeehiveProvider::INSTANCE->decodeFromData(accessor);
    if (!b) return;
    bool full = *b > 0;
    int  bees = std::abs(*b);
    tooltip.add(Component::translatable("jade.beehive.bees", args({full ? t.success(std::to_string(bees)) : t.info(double(bees))})));
}

}
