// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <optional>
#include <string>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/block/Block.h"

namespace jade::util::BlockStates {

inline CompoundTag const* statesOf(Block const& block) {
    auto const& ser = block.mSerializationId.get();
    if (!ser.contains("states") || !ser.at("states").hold<CompoundTag>()) return nullptr;
    return &ser.at("states").get<CompoundTag>();
}

inline std::optional<int> intState(CompoundTag const* states, char const* name) {
    if (!states || !states->contains(name)) return std::nullopt;
    auto const& v = states->at(name);
    if (v.hold<StringTag>()) return std::nullopt;
    return static_cast<int>(v);
}

inline std::optional<std::string> stringState(CompoundTag const* states, char const* name) {
    if (!states || !states->contains(name) || !states->at(name).hold<StringTag>()) return std::nullopt;
    return std::string(states->at(name).get<StringTag>());
}

inline std::optional<int> intState(Block const& block, char const* name) { return intState(statesOf(block), name); }

inline std::optional<bool> boolState(Block const& block, char const* name) {
    auto v = intState(block, name);
    if (!v) return std::nullopt;
    return *v != 0;
}

}
