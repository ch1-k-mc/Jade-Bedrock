// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/actor/BaseCommandBlock.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/CommandBlockActor.h"
#include "mc/world/level/block/actor/JukeboxBlockActor.h"

#include "platform/text/I18n.h"

namespace jade::addon::vanilla {

namespace {
JukeboxBlockActor* jukeboxOf(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    return be && be->getType() == BlockActorType::Jukebox ? static_cast<JukeboxBlockActor*>(be) : nullptr;
}
}

bool JukeboxProvider::shouldRequestData(BlockAccessor& accessor) { return jukeboxOf(accessor) != nullptr; }

std::optional<view::ItemData> JukeboxProvider::streamData(BlockAccessor& accessor) {
    auto* j = jukeboxOf(accessor);
    if (!j) return std::nullopt;
    ItemStack const& record = j->mRecord.get();
    return record.isNull() ? view::ItemData{} : view::ItemData::of(record);
}

void JukeboxProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto data = JukeboxProvider::INSTANCE->decodeFromData(accessor);
    if (!data) return;
    auto view = view::ItemView::fromData(*data);
    if (!view) {
        tooltip.add(text::Component::translatable("tooltip.jade.empty"));
        return;
    }
    std::string     type = view->item->getTypeName();
    std::string     song = type.substr(type.find(':') + 1);
    if (song.starts_with("music_disc_")) song = song.substr(11);
    std::string     key  = "item.record_" + song + ".desc";
    text::Component name = text::I18n::has(key) ? text::Component::literal(text::I18n::get(key))
                                                : text::Component::literal(text::stripFormatting(view->item->getHoverName()));
    tooltip.add(text::Component::translatable("record.nowPlaying", std::vector<text::Component>{name.stripColor()}));
}

namespace {
CommandBlockActor* commandBlockOf(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    return be && be->getType() == BlockActorType::CommandBlock ? static_cast<CommandBlockActor*>(be) : nullptr;
}
}

bool CommandBlockProvider::shouldRequestData(BlockAccessor& accessor) {
    return accessor.getPlayer().canUseOperatorBlocks() && commandBlockOf(accessor) != nullptr;
}

std::optional<std::string> CommandBlockProvider::streamData(BlockAccessor& accessor) {
    if (!accessor.getPlayer().canUseOperatorBlocks()) return std::nullopt;
    auto* cb = commandBlockOf(accessor);
    if (!cb) return std::nullopt;
    std::string command = cb->mBaseCB->mCommand;
    if (command.size() > 40) command = command.substr(0, 37) + "...";
    return command;
}

std::optional<std::string> CommandBlockProvider::decode(CompoundTag const& in) {
    if (!in.contains("command") || !in.at("command").hold<StringTag>()) return std::nullopt;
    return std::string(in.at("command").get<StringTag>());
}

void CommandBlockProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    std::string command = CommandBlockProvider::INSTANCE->decodeFromData(accessor).value_or("");
    if (command.find_first_not_of(" \t") == std::string::npos) return;
    tooltip.add(text::Component::literal("> " + command));
}

}
