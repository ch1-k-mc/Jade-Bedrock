// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/access/AccessibilityPlugin.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorFlags.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/VanillaStates.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/SignBlockActor.h"

#include "mc/world/level/Level.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/phys/AABB.h"
#include "platform/text/I18n.h"
#include "addon/core/CoreProviders.h"
#include "impl/WailaClientRegistration.h"
#include "platform/item/ScopedItemRegistry.h"
#include "util/BlockStates.h"

namespace jade::addon::access {

using text::Component;

namespace {
int const OBJECT_NAME_PRIORITY = TooltipPosition::HEAD - 100;

class BlockDetails final : public BlockDetailsProvider {
public:
    int         getDefaultPriority() const override { return OBJECT_NAME_PRIORITY + 10; }
    std::string getConfigIcon() const override { return "minecraft:book"; }
};
class EntityDetails final : public EntityDetailsProvider {
public:
    int         getDefaultPriority() const override { return OBJECT_NAME_PRIORITY + 10; }
    std::string getConfigIcon() const override { return "minecraft:book"; }
};
}

void SignProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be || (be->getType() != BlockActorType::Sign && be->getType() != BlockActorType::HangingSign)) return;
    auto* sign = static_cast<SignBlockActor*>(be);
    auto  side = sign->getSideFacingPlayer(accessor.getPlayer());
    tooltip.add(Component::translatable(std::string("jade.access.sign.") + (side == SignTextSide::Front ? "front" : "back")));
    std::stringstream ss(sign->getMessage(side));
    int               i = 0;
    for (std::string line; std::getline(ss, line) && i < 4;) {
        ++i;
        if (accessor.showDetails()) tooltip.add(Component::translatable("jade.access.sign.line" + std::to_string(i), std::vector<Component>{Component::literal(line)}));
        else tooltip.add(Component::literal(line));
    }
}

void HeldItemProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* e = accessor.getEntity();
    if (!e) return;
    platform::item::ScopedItemRegistry scoped;
    ItemStack const&                   main = e->getCarriedItem();
    ItemStack const&                   off  = e->getOffhandSlot();
    if (!main.isNull()) tooltip.add(Component::translatable("jade.access.held_item.main", std::vector<Component>{Component::literal(main.getHoverName())}));
    if (!off.isNull()) tooltip.add(Component::translatable("jade.access.held_item.off", std::vector<Component>{Component::literal(off.getHoverName())}));
}

void BlockDetailsProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    Block const& block      = accessor.getBlock();
    auto const&  type       = block.getTypeName();
    std::string  objectName = tooltip.getString(JadeIds::CORE_OBJECT_NAME);
    auto         replace    = [&](char const* key) { AccessibilityPlugin::replaceTitle(tooltip, objectName, key); };
    if (auto open = block.getState<bool>(VanillaStates::OpenBit()); open && type != "minecraft:barrel") replace(*open ? "block.door_open" : "block.door_closed");
    if (block.getState<bool>(VanillaStates::Lit()).value_or(false) || type.starts_with("minecraft:lit_")) replace("block.lit");
    if (util::BlockStates::boolState(block, "end_portal_eye_bit").value_or(false)) replace("block.eye");
    if (block.getState<int>(VanillaStates::MoisturizedAmount()).value_or(0) == 7) replace("block.hydrated");
    if (block.getState<bool>(VanillaStates::UpsideDownBit()).value_or(false) && type.ends_with("_stairs")) replace("block.upside_down");
    bool powered = type == "minecraft:powered_repeater" || (type.ends_with("rail") && util::BlockStates::boolState(block, "rail_data_bit").value_or(false))
                || (type.ends_with("_shelf") && block.getState<bool>(VanillaStates::PoweredBit()).value_or(false));
    if (powered) replace("block.powered");
}

using util::BlockStates::intState;
using util::BlockStates::statesOf;
using util::BlockStates::stringState;

void BlockAmountProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) {
    if (!config.get(JadeIds::ACCESS_BLOCK_DETAILS)) return;
    auto const* states = statesOf(accessor.getBlock());
    int amount = -1;
    if (auto v = intState(states, "cluster_count")) amount = *v + 1;
    else if (auto v = intState(states, "candles")) amount = *v + 1;
    else if (auto v = stringState(states, "turtle_egg_count")) amount = *v == "one_egg" ? 1 : *v == "two_egg" ? 2 : *v == "three_egg" ? 3 : *v == "four_egg" ? 4 : -1;
    if (amount >= 0) tooltip.add(Component::literal(text::I18n::formatString("jade.access.block.amount", {std::to_string(amount)})));
    if (auto v = intState(states, "bite_counter")) tooltip.add(Component::literal(text::I18n::formatString("jade.access.block.bites", {std::to_string(*v)})));
    if (auto v = intState(states, "height"); v && accessor.getBlock().getTypeName() == "minecraft:snow_layer") {
        tooltip.add(Component::literal(text::I18n::formatString("jade.access.block.layers", {std::to_string(*v + 1)})));
    }
    if (auto v = intState(states, "fill_level")) tooltip.add(Component::literal(text::I18n::formatString("jade.access.block.level", {std::to_string(*v)})));
}

void BlockDetailsBodyProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto const& type = accessor.getBlock().getTypeName();
    if (type.ends_with("campfire") || type == "minecraft:decorated_pot" || type.ends_with("dripleaf") || type == "minecraft:pink_petals") return;
    auto const* states = statesOf(accessor.getBlock());
    int         facing = -1;
    if (auto d = stringState(states, "minecraft:cardinal_direction")) facing = *d == "north" ? 2 : *d == "south" ? 3 : *d == "west" ? 4 : *d == "east" ? 5 : -1;
    else if (auto d = stringState(states, "minecraft:facing_direction")) facing = *d == "down" ? 0 : *d == "up" ? 1 : *d == "north" ? 2 : *d == "south" ? 3 : *d == "west" ? 4 : *d == "east" ? 5 : -1;
    else if (auto d = intState(states, "facing_direction")) facing = *d;
    if (facing < 0 || facing > 5) return;
    tooltip.add(Component::translatable("jade.access.block.facing", std::vector<Component>{core::BlockFaceProvider::directionName(facing)}));
}

void EntityDetailsProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* e = accessor.getEntity();
    if (!e) return;
    std::string objectName = tooltip.getString(JadeIds::CORE_OBJECT_NAME);
    auto        replace    = [&](char const* key) { AccessibilityPlugin::replaceTitle(tooltip, objectName, key); };
    auto const& type       = e->getTypeName();
    if (type == "minecraft:creeper" && e->getStatusFlag(ActorFlags::Powered)) replace("creeper.powered");
    if (type == "minecraft:wither" && e->getStatusFlag(ActorFlags::Powered)) replace("wither.powered");
    if (type == "minecraft:bee" && e->getStatusFlag(ActorFlags::Angry)) replace("entity.angry");
    if (e->isBaby()) replace("entity.baby");
    if (e->getStatusFlag(ActorFlags::Sheared)) replace("entity.sheared");
    if (e->getStatusFlag(ActorFlags::Saddled)) replace("entity.saddled");
}

namespace {

int poseIdOf(Actor& e) {
    if (e.getTypeName() == "minecraft:axolotl" && e.getStatusFlag(ActorFlags::PlayingDead)) return 1000;
    if (e.getTypeName() == "minecraft:armadillo" && e.getStatusFlag(ActorFlags::Rolling)) return 1001;
    if (e.getStatusFlag(ActorFlags::Sleeping)) return 2;
    if (e.getStatusFlag(ActorFlags::Sitting)) return 10;
    if (e.getStatusFlag(ActorFlags::Swimming)) return 3;
    if (e.getStatusFlag(ActorFlags::Sneaking)) return 5;
    if (e.getHealth() <= 0) return 7;
    return 0;
}

}

void EntityDetailsBodyProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) {
    if (!config.get(JadeIds::ACCESS_ENTITY_DETAILS)) return;
    Actor* e = accessor.getEntity();
    if (!e) return;
    int pose = poseIdOf(*e);
    if (pose != 0) {
        std::string key = "jade.access.entity.pose." + std::to_string(pose);
        if (text::I18n::has(key)) tooltip.add(Component::translatable("jade.access.entity.pose", std::vector<Component>{Component::translatable(key)}));
    }
    if (e->hasPassenger()) {
        AABB box  = e->getAABB();
        AABB near(box.min - Vec3(3, 3, 3), box.max + Vec3(3, 3, 3));
        int  count = 0;
        for (Actor* other : e->getDimensionBlockSource().fetchEntities(e, near, true, false)) {
            if (other && other->isRiding(e)) ++count;
        }
        if (count > 0) tooltip.add(Component::literal(text::I18n::formatString("jade.access.entity.passengers", {std::to_string(count)})));
    }
    if (e->isLeashed()) {
        if (Actor* holder = accessor.getLevel().fetchEntity(e->getLeashHolder(), false)) {
            std::string target;
            if (holder->getTypeName() == "minecraft:leash_knot") {
                BlockPos p(holder->getPosition());
                target = core::DistanceProvider::xyz(p.x, p.y, p.z)->getString();
            } else {
                target = core::ObjectNameProvider::getEntityName(*holder, false).getString();
            }
            tooltip.add(Component::translatable("jade.access.entity.leashed_to", std::vector<Component>{Component::literal(target)}));
        }
    }
}

namespace {

std::map<std::string, std::vector<char const*>> const& variantNames() {
    static std::map<std::string, std::vector<char const*>> const names{
        {"minecraft:cat", {"white", "black", "red", "siamese", "british_shorthair", "calico", "persian", "ragdoll", "tabby", "all_black", "jellie"}},
        {"minecraft:frog", {"temperate", "cold", "warm"}},
        {"minecraft:axolotl", {"lucy", "cyan", "gold", "wild", "blue"}},
        {"minecraft:parrot", {"red_blue", "blue", "green", "yellow_blue", "gray"}},
        {"minecraft:llama", {"creamy", "white", "brown", "gray"}},
        {"minecraft:trader_llama", {"creamy", "white", "brown", "gray"}},
        {"minecraft:horse", {"white", "creamy", "chestnut", "brown", "black", "gray", "dark_brown"}},
        {"minecraft:fox", {"red", "snow"}},
        {"minecraft:mooshroom", {"red", "brown"}},
        {"minecraft:rabbit", {"brown", "white", "black", "white_splotched", "gold", "salt"}},
    };
    return names;
}

std::string spaced(std::string s) {
    std::replace(s.begin(), s.end(), '_', ' ');
    std::replace(s.begin(), s.end(), '.', ' ');
    return s;
}

}

void EntityVariantProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* e = accessor.getEntity();
    if (!e) return;
    auto const& type = e->getTypeName();
    auto        it   = variantNames().find(type);
    if (it == variantNames().end()) return;
    int variant = e->getVariant();
    if (variant < 0 || variant >= static_cast<int>(it->second.size())) return;
    tooltip.add(Component::translatable("jade.access.entity.variant", std::vector<Component>{Component::literal(spaced(it->second[variant]))}));
    if (type == "minecraft:horse") {
        static std::vector<char const*> const MARKINGS{"none", "white", "white_field", "white_dots", "black_dots"};
        int mark = e->getMarkVariant();
        if (mark >= 0 && mark < static_cast<int>(MARKINGS.size())) {
            tooltip.add(Component::translatable("jade.access.entity.horse_markings", std::vector<Component>{Component::literal(spaced(MARKINGS[mark]))}));
        }
    }
}

void AccessibilityPlugin::registerClient(impl::WailaClientRegistration& registration) {
    registration.registerBlockComponent(
        std::make_shared<SignProvider>(),
        BlockClass::of("SignBlock", [](Block const& b) {
            auto const& t = b.getTypeName();
            return t.ends_with("_sign") || t == "minecraft:standing_sign" || t == "minecraft:wall_sign";
        })
    );
    registration.markAsClientFeature(JadeIds::ACCESS_SIGN);
    registration.registerBlockComponent(std::make_shared<BlockDetails>(), BlockClass::any());
    registration.markAsClientFeature(JadeIds::ACCESS_BLOCK_DETAILS);
    registration.registerBlockComponent(std::make_shared<BlockDetailsBodyProvider>(), BlockClass::any());
    registration.registerBlockComponent(std::make_shared<BlockAmountProvider>(), BlockClass::any());
    registration.markAsClientFeature(JadeIds::ACCESS_BLOCK_AMOUNT);
    registration.registerEntityComponent(std::make_shared<EntityDetails>(), EntityClass::any());
    registration.registerEntityComponent(std::make_shared<EntityDetailsBodyProvider>(), EntityClass::any());
    registration.markAsClientFeature(JadeIds::ACCESS_ENTITY_DETAILS);
    registration.registerEntityComponent(
        std::make_shared<EntityVariantProvider>(),
        EntityClass::of("LivingEntity", [](Actor const& a) { return a.hasCategory(ActorCategory::Mob); })
    );
    registration.markAsClientFeature(JadeIds::ACCESS_ENTITY_VARIANT);
    registration.registerEntityComponent(
        std::make_shared<HeldItemProvider>(),
        EntityClass::of("LivingEntity", [](Actor const& a) { return a.hasCategory(ActorCategory::Mob); })
    );
    registration.markAsClientFeature(JadeIds::ACCESS_HELD_ITEM);
}

}
