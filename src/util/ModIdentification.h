// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <optional>
#include <string>

class Block;
class Actor;
class ItemStack;

namespace jade::util {

class ModIdentification {
public:
    static constexpr int NAME_MAX_WIDTH = 160;

    static void                       invalidateCache();
    static std::optional<std::string> getModFullName(std::string const& ns);
    static std::optional<std::string> getModName(std::string const& ns, int maxWidth = NAME_MAX_WIDTH);
    static std::string                getModNameOfId(std::string const& id);
    static std::string                getModName(Block const& block);
    static std::string                getModName(ItemStack const& stack);
    static std::string                getModName(Actor const& entity);
    static std::string                cutName(std::string fullName, int maxWidth);
};

}
