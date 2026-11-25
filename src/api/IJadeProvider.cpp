// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "api/IJadeProvider.h"

#include <algorithm>

#include "mc/world/actor/Actor.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"

namespace jade::api {

BlockClass BlockClass::any() { return {"Block", [](Block const&) { return true; }, 0}; }

BlockClass BlockClass::ofTypes(std::vector<std::string> typeNames) {
    std::string name;
    for (auto const& t : typeNames) name += (name.empty() ? "" : ",") + t;
    return {name, [types = std::move(typeNames)](Block const& b) { return std::find(types.begin(), types.end(), b.getTypeName()) != types.end(); }, 1};
}

BlockClass BlockClass::of(std::string name, std::function<bool(Block const&)> test, int depth) { return {std::move(name), std::move(test), depth}; }

BlockActorClass BlockActorClass::any() { return {"BlockActor", [](BlockActor const&) { return true; }, 0}; }

BlockActorClass BlockActorClass::ofType(BlockActorType type, std::string name) {
    return {std::move(name), [type](BlockActor const& a) { return a.getType() == type; }, 1};
}

BlockActorClass BlockActorClass::ofTypes(std::vector<BlockActorType> types, std::string name) {
    return {std::move(name), [types = std::move(types)](BlockActor const& a) { return std::find(types.begin(), types.end(), a.getType()) != types.end(); }, 1};
}

EntityClass EntityClass::any() { return {"Actor", [](Actor const&) { return true; }, 0}; }

EntityClass EntityClass::ofIdentifiers(std::vector<std::string> ids, std::string name) {
    return {std::move(name), [ids = std::move(ids)](Actor const& a) { return std::find(ids.begin(), ids.end(), a.getTypeName()) != ids.end(); }, 1};
}

EntityClass EntityClass::of(std::string name, std::function<bool(Actor const&)> test, int depth) { return {std::move(name), std::move(test), depth}; }

}
