// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorCategory.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/attribute/AttributeInstance.h"
#include "mc/world/attribute/AttributeInstanceConstRef.h"
#include "mc/world/attribute/SharedAttributes.h"

#include "impl/ui/HealthElement.h"
#include "impl/ui/JadeUIInternal.h"

namespace jade::addon::vanilla {

namespace {
bool isLiving(Actor const& e) { return e.hasCategory(ActorCategory::Mob); }
}

bool EntityHealthAndArmorProvider::isHealthVisible(Actor const& entity) {
    auto const& t = entity.getTypeName();
    return t != "minecraft:armor_stand" && t != "minecraft:creaking";
}

bool EntityHealthAndArmorProvider::shouldRequestData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    return e && isLiving(*e) && isHealthVisible(*e);
}

std::optional<float> EntityHealthAndArmorProvider::streamData(EntityAccessor& accessor) {
    Actor* e = accessor.getEntity();
    if (!e || !isLiving(*e)) return std::nullopt;
    auto ref = e->getAttribute(SharedAttributes::ABSORPTION());
    AttributeInstance const* attr = ref.mPtr;
    float absorption = attr ? attr->mCurrentValue : 0.0f;
    return absorption > 0 ? absorption : 0.0f;
}

void EntityHealthAndArmorProvider::Client::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) {
    Actor* living = accessor.getEntity();
    if (!living || !isLiving(*living)) return;
    bool                        healthText = false, armorText = false;
    std::vector<ui::ElementPtr> elements;
    if (config.get(JadeIds::MC_ENTITY_HEALTH) && isHealthVisible(*living)) {
        float health     = static_cast<float>(living->getHealth());
        float maxHealth  = static_cast<float>(living->getMaxHealth());
        float absorption = EntityHealthAndArmorProvider::INSTANCE->decodeFromData(accessor).value_or(0.0f);
        auto  e          = std::make_shared<impl::ui::HealthElement>(impl::ui::HeartType::NORMAL, maxHealth, health, absorption);
        e->tag(JadeIds::MC_ENTITY_HEALTH);
        healthText = e->showText();
        elements.push_back(e);
    }
    int armor = static_cast<Mob*>(living)->getArmorValue();
    if (config.get(JadeIds::MC_ENTITY_ARMOR) && armor > 0) {
        auto e = std::make_shared<impl::ui::ArmorElement>(static_cast<float>(armor));
        e->tag(JadeIds::MC_ENTITY_ARMOR);
        armorText = e->showText();
        elements.push_back(e);
    }
    if (healthText && armorText) {
        tooltip.add(elements[0]);
        tooltip.append(impl::ui::JadeUIInternal::spacer(4, 0));
        tooltip.append(elements[1]);
    } else {
        for (auto& e : elements) tooltip.add(e);
    }
}

}
