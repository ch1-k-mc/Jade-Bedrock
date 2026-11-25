// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDataIDs.h"
#include "mc/world/actor/Motif.h"
#include "mc/world/actor/Painting.h"
#include "mc/world/actor/SynchedActorDataEntityWrapper.h"
#include "mc/world/attribute/AttributeInstance.h"
#include "mc/world/attribute/AttributeInstanceConstRef.h"
#include "mc/world/attribute/SharedAttributes.h"
#include "mc/world/item/ItemStack.h"

#include "api/theme/IThemeHelper.h"
#include "impl/ui/JadeUIInternal.h"
#include "overlay/DisplayHelper.h"
#include "platform/item/ScopedItemRegistry.h"

namespace jade::addon::vanilla {

using impl::ui::JadeUIInternal;
using text::Component;

void ArmorStandProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* e = accessor.getEntity();
    if (!e) return;
    platform::item::ScopedItemRegistry scoped;
    using Slot = SharedTypes::Legacy::ArmorSlot;
    ItemStack const* stacks[]{&e->getCarriedItem(), &e->getOffhandSlot(), &e->getArmor(Slot::Feet), &e->getArmor(Slot::Legs), &e->getArmor(Slot::Torso), &e->getArmor(Slot::Head)};
    bool empty = true;
    for (ItemStack const* stack : stacks) {
        if (stack->isNull()) continue;
        tooltip.add(JadeUIInternal::smallItem(*stack));
        tooltip.append(Component::literal(text::stripFormatting(stack->getHoverName())));
        tooltip.setLineMargin(-1, ui::ScreenDirection::DOWN, 0);
        empty = false;
    }
    if (!empty) tooltip.setLineMargin(-1, ui::ScreenDirection::DOWN, 1);
}

void PaintingProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* e = accessor.getEntity();
    if (!e || e->getTypeName() != "minecraft:painting") return;
    Motif const* motif = static_cast<Painting*>(e)->mMotif;
    if (!motif) return;
    tooltip.add(theme::IThemeHelper::get().warning(Component::literal(*motif->mName)));
}

namespace {

double getJumpHeight(double s) { return 4.53680079 * s * s + 1.61431730 * s - 0.22656224; }
double getSpeed(double speed) { return speed * 43.171815466666658 - 0.000000339999999; }

double const MAX_JUMP_HEIGHT    = getJumpHeight(1.0);
double const MAX_MOVEMENT_SPEED = getSpeed(0.3375);

Component switchText(std::string const& key, bool showMax, double value, double max) {
    auto& t         = theme::IThemeHelper::get();
    auto  valueText = t.info(overlay::DisplayHelper::formatNumber(value));
    if (showMax) {
        return Component::translatable(key, std::vector<Component>{Component::translatable("jade.fraction", std::vector<Component>{valueText, Component::literal(overlay::DisplayHelper::formatNumber(max))})});
    }
    return Component::translatable(key, std::vector<Component>{valueText});
}

std::optional<double> attributeValue(Actor& e, Attribute const& attribute) {
    auto                     ref  = e.getAttribute(attribute);
    AttributeInstance const* attr = ref.mPtr;
    if (!attr) return std::nullopt;
    return attr->mCurrentValue;
}

}

void HorseStatsProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* horse = accessor.getEntity();
    if (!horse) return;
    bool        showMax = accessor.showDetails();
    auto const& type    = horse->getTypeName();
    if (type == "minecraft:llama" || type == "minecraft:trader_llama") {
        tooltip.add(switchText("jade.llamaStrength", showMax, horse->mEntityData->getInt(static_cast<ushort>(ActorDataIDs::Strength)), 5));
        return;
    }
    if (type == "minecraft:camel") return;
    if (auto jump = attributeValue(*horse, SharedAttributes::JUMP_STRENGTH()); jump && *jump > 0) {
        tooltip.add(switchText("jade.horseStat.jump", showMax, getJumpHeight(*jump), MAX_JUMP_HEIGHT));
    }
    if (auto speed = attributeValue(*horse, SharedAttributes::MOVEMENT_SPEED())) {
        tooltip.add(switchText("jade.horseStat.speed", showMax, getSpeed(*speed), MAX_MOVEMENT_SPEED));
    }
}

}
