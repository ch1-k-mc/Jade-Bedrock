// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include <cctype>
#include <sstream>

#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/VanillaStates.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/NoteBlockActor.h"

#include "addon/access/AccessibilityPlugin.h"
#include "api/theme/IThemeHelper.h"
#include "platform/text/I18n.h"
#include "util/BlockStates.h"

namespace jade::addon::vanilla {

namespace {

bool boolOf(CompoundTag const& tag, char const* key) {
    if (!tag.contains(key)) return false;
    return static_cast<int>(tag.at(key)) != 0;
}

}

void TNTStabilityProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    if (accessor.getBlock().getState<bool>(VanillaStates::ExplodeBit()).value_or(false)) {
        tooltip.add(theme::IThemeHelper::get().danger(text::Component::translatable("jade.tnt.unstable")));
    }
}

std::optional<bool> HopperLockProvider::streamData(BlockAccessor& accessor) {
    return util::BlockStates::boolState(accessor.getBlock(), "toggle_bit").value_or(false);
}

std::optional<bool> HopperLockProvider::decode(CompoundTag const& in) { return boolOf(in, "locked"); }

void HopperLockProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) {
    if (!HopperLockProvider::INSTANCE->decodeFromData(accessor).value_or(false)) return;
    if (config.get(JadeIds::MC_REDSTONE)
        || (config::IWailaConfig::get().accessibility().getEnableAccessibilityPlugin() && config.get(JadeIds::ACCESS_BLOCK_DETAILS))) {
        std::string objectName = tooltip.getString(JadeIds::CORE_OBJECT_NAME);
        access::AccessibilityPlugin::replaceTitle(tooltip, objectName, "block.locked");
    }
}

namespace {

char const* const PITCH[]{"F♯/G♭", "G", "G♯/A♭", "A", "A♯/B♭", "B", "C", "C♯/D♭", "D", "D♯/E♭", "E", "F"};
char const* const OCTAVE[]{"§f", "§e", "§6"};
char const* const OCTAVE_LIGHT[]{"§5", "§1", "§9"};

using Instrument = SharedTypes::v1_26_20::NoteBlockInstrument;

char const* instrumentName(Instrument i) {
    static char const* const names[]{
        "harp",   "basedrum", "snare",  "hat",      "bass",          "flute",  "bell",
        "guitar", "chime",    "xylophone", "iron_xylophone", "cow_bell", "didgeridoo", "bit",
        "banjo",  "pling",    "trumpet", "trumpet_exposed", "trumpet_weathered", "trumpet_oxidized",
        "zombie", "skeleton", "creeper", "dragon", "wither_skeleton", "piglin"};
    auto idx = static_cast<size_t>(i);
    return idx < std::size(names) ? names[idx] : "custom_head";
}

bool isTunable(Instrument i) { return static_cast<int>(i) < static_cast<int>(Instrument::Zombie); }

std::string capitalizeWords(std::string s) {
    for (auto& c : s)
        if (c == '_') c = ' ';
    bool start = true;
    for (auto& c : s) {
        if (start && std::isalpha(static_cast<unsigned char>(c))) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        start = c == ' ';
    }
    return s;
}

NoteBlockActor* noteBlockOf(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    return be && be->getType() == BlockActorType::Music ? static_cast<NoteBlockActor*>(be) : nullptr;
}

}

std::optional<int> NoteBlockProvider::streamData(BlockAccessor& accessor) {
    auto* nb = noteBlockOf(accessor);
    if (!nb) return std::nullopt;
    return static_cast<int>(nb->mNote);
}

std::optional<int> NoteBlockProvider::decode(CompoundTag const& in) { return static_cast<int>(in.at("note")); }

void NoteBlockProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto instrument = NoteBlockActor::getInstrument(accessor.getRegion(), accessor.getPosition()).value_or(Instrument::Harp);
    std::string serialized = instrumentName(instrument);
    std::string key        = "jade.instrument." + serialized;
    std::string name       = text::I18n::has(key) ? text::I18n::get(key) : capitalizeWords(serialized);
    if (isTunable(instrument)) {
        auto note = NoteBlockProvider::INSTANCE->decodeFromData(accessor);
        if (!note) {
            if (auto* nb = noteBlockOf(accessor)) note = static_cast<int>(nb->mNote);
        }
        if (note) {
            int         n      = std::clamp(*note, 0, 24);
            char const* octave = (theme::IThemeHelper::get().isLightColorScheme() ? OCTAVE_LIGHT : OCTAVE)[n / 12];
            tooltip.add(text::Component::literal(name + " " + octave + PITCH[n % 12]));
            return;
        }
    }
    tooltip.add(text::Component::literal(name));
}

}
