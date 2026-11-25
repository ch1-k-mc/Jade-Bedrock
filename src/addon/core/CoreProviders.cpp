// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/core/CoreProviders.h"

#include <cmath>
#include <cstdio>

#include <Windows.h>

#include "mc/server/commands/CommandUtils.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/component/IVanillaMainBlockActorComponent.h"
#include "mc/safety/RedactableString.h"

#include "api/theme/IThemeHelper.h"
#include "impl/AccessorImpl.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"
#include "impl/ui/Elements.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/item/ScopedItemRegistry.h"
#include "platform/text/I18n.h"
#include "util/ModIdentification.h"

namespace jade::addon::core {

using text::Component;

std::shared_ptr<ObjectNameProvider::ForBlock> const  ObjectNameProvider::ForBlock::INSTANCE  = std::make_shared<ObjectNameProvider::ForBlock>();
std::shared_ptr<ObjectNameProvider::ForEntity> const ObjectNameProvider::ForEntity::INSTANCE = std::make_shared<ObjectNameProvider::ForEntity>();
std::shared_ptr<ObjectNameProvider::BlockData> const ObjectNameProvider::BlockData::INSTANCE = std::make_shared<ObjectNameProvider::BlockData>();
std::shared_ptr<ModNameProvider::ForBlock> const     ModNameProvider::ForBlock::INSTANCE     = std::make_shared<ModNameProvider::ForBlock>();
std::shared_ptr<ModNameProvider::ForEntity> const    ModNameProvider::ForEntity::INSTANCE    = std::make_shared<ModNameProvider::ForEntity>();
std::shared_ptr<DistanceProvider::ForBlock> const    DistanceProvider::ForBlock::INSTANCE    = std::make_shared<DistanceProvider::ForBlock>();
std::shared_ptr<DistanceProvider::ForEntity> const   DistanceProvider::ForEntity::INSTANCE   = std::make_shared<DistanceProvider::ForEntity>();
std::shared_ptr<BlockFaceProvider> const             BlockFaceProvider::INSTANCE             = std::make_shared<BlockFaceProvider>();

namespace {

bool hasControlDown() { return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0; }

std::string itemHoverName(ItemStack const& stack) {
    platform::item::ScopedItemRegistry scoped;
    return stack.getHoverName();
}

}

Component ObjectNameProvider::getEntityName(Actor& entity, bool accessibilityDetails) {
    std::string const& customName = entity.getNameTag();
    if (!customName.empty() && !accessibilityDetails) return Component::literal(customName);
    std::string displayName;
    if (entity.hasCategory(ActorCategory::Item)) {
        displayName = itemHoverName(static_cast<ItemActor&>(entity).mItem.get());
    } else {
        displayName = CommandUtils::getActorName(entity);
    }
    return Component::literal(displayName);
}

void ObjectNameProvider::ForBlock::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    std::optional<Component> name;
    if (auto custom = BlockData::INSTANCE->decodeFromData(accessor)) name = Component::literal(*custom);
    auto& ops = impl::WailaCommonRegistration::instance().blockOperations();
    if (ops.shouldPick(accessor.getBlock().getTypeName())) {
        ItemStack const& pick = accessor.getPickedResult();
        if (!pick.isNull()) name = Component::literal(itemHoverName(pick));
    }
    if (!name) {
        std::string key = accessor.getBlock().getDescriptionId();
        std::string n   = accessor.getBlock().buildDescriptionName();
        if (!n.empty() && n != key) {
            name = Component::literal(n);
        } else {
            ItemStack const& pick = accessor.getPickedResult();
            name = pick.isNull() ? Component::literal(key) : Component::literal(itemHoverName(pick));
        }
    }
    ForEntity::addName(tooltip, *name);
}

void ObjectNameProvider::ForEntity::addName(ITooltip& tooltip, Component name) {
    name = theme::IThemeHelper::get().title(name);
    if (config::IWailaConfig::get().overlay().getIconMode() != config::IconMode::INLINE) {
        tooltip.add(name);
        return;
    }
    auto                icon = tooltip.getIcon();
    api::ui::ElementPtr newIcon;
    if (auto* item = dynamic_cast<impl::ui::ItemStackElement*>(icon.get())) newIcon = impl::ui::JadeUIInternal::smallItem(item->getItem());
    if (!newIcon) {
        tooltip.add(name);
        return;
    }
    newIcon->tag(JadeIds::CORE_ROOT_ICON);
    tooltip.add(std::static_pointer_cast<layout::LayoutElement>(newIcon));
    tooltip.append(name);
}

void ObjectNameProvider::ForEntity::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) {
    Actor* entity = accessor.getEntity();
    if (!entity) return;
    bool details = config::IWailaConfig::get().accessibility().getEnableAccessibilityPlugin() && config.get(JadeIds::ACCESS_ENTITY_DETAILS);
    addName(tooltip, getEntityName(*entity, details));
}

std::optional<std::string> ObjectNameProvider::BlockData::streamData(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be) return std::nullopt;
    auto* main = be->_getMainComponent();
    if (!main) return std::nullopt;
    std::string const& name = main->getCustomName().mUnredactedString;
    if (name.empty()) return std::nullopt;
    return name;
}

std::optional<std::string> ObjectNameProvider::BlockData::decode(CompoundTag const& in) {
    if (!in.contains("name")) return std::nullopt;
    std::string const& s = in.at("name");
    return s;
}

bool ObjectNameProvider::BlockData::shouldRequestData(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    return be && be->_getMainComponent();
}

void ModNameProvider::ForBlock::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) {
    if (config.getEnum<ModNameMode>(JadeIds::CORE_MOD_NAME) == ModNameMode::OFF) return;
    std::string modName;
    if (impl::WailaCommonRegistration::instance().blockOperations().shouldPick(accessor.getBlock().getTypeName())) {
        ItemStack const& pick = accessor.getPickedResult();
        if (!pick.isNull()) modName = util::ModIdentification::getModName(pick);
    }
    if (modName.empty()) modName = util::ModIdentification::getModName(accessor.getBlock());
    if (!modName.empty()) tooltip.add(std::static_pointer_cast<layout::LayoutElement>(theme::IThemeHelper::get().modNameElement(Component::literal(modName))));
}

void ModNameProvider::ForEntity::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) {
    if (config.getEnum<ModNameMode>(JadeIds::CORE_MOD_NAME) == ModNameMode::OFF) return;
    Actor* entity = accessor.getEntity();
    if (!entity) return;
    std::string modName = util::ModIdentification::getModName(*entity);
    if (!modName.empty()) tooltip.add(std::static_pointer_cast<layout::LayoutElement>(theme::IThemeHelper::get().modNameElement(Component::literal(modName))));
}

namespace {
constexpr uint32_t kColors[] = {0xef9a9a, 0xa5d6a7, 0x90caf9, 0xb02a37, 0x198754, 0x0a58ca};

std::string fmt1(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    std::string s = buf;
    if (s.size() > 2 && s.substr(s.size() - 2) == ".0") s.resize(s.size() - 2);
    return s;
}
}

std::string DistanceProvider::distance(Accessor& accessor) {
    Vec3 eye = accessor.getPlayer().getEyePos();
    Vec3 hit = accessor.getHitResult().location;
    double dx = eye.x - hit.x, dy = eye.y - hit.y, dz = eye.z - hit.z;
    return fmt1(std::sqrt(dx * dx + dy * dy + dz * dz));
}

Component DistanceProvider::display(int i, int colorIndex) {
    if (theme::IThemeHelper::get().isLightColorScheme()) colorIndex += 3;
    return Component::literal(std::to_string(i), text::Style{.color = 0xFF000000u | kColors[colorIndex]});
}

std::shared_ptr<ui::TextElement> DistanceProvider::xyz(int x, int y, int z) {
    auto display_ = Component::translatable("jade.blockpos", std::vector<Component>{display(x, 0), display(y, 1), display(z, 2)});
    auto text     = impl::ui::JadeUIInternal::text(display_);
    text->narration(text::I18n::formatString("narration.jade.blockpos", {std::to_string(x), std::to_string(y), std::to_string(z)}));
    return text;
}

void DistanceProvider::append(ITooltip& tooltip, Accessor& accessor, BlockPos const& pos, IPluginConfig& config) const {
    bool        dist        = config.get(JadeIds::CORE_DISTANCE);
    std::string distanceVal = dist ? distance(accessor) : "";
    if (config.get(JadeIds::CORE_COORDINATES)) {
        if (config.get(JadeIds::CORE_REL_COORDINATES) && hasControlDown()) {
            Vec3     eye = accessor.getPlayer().getEyePos();
            BlockPos e{static_cast<int>(std::floor(eye.x)), static_cast<int>(std::floor(eye.y)), static_cast<int>(std::floor(eye.z))};
            tooltip.add(std::static_pointer_cast<layout::LayoutElement>(xyz(pos.x - e.x, pos.y - e.y, pos.z - e.z)));
        } else {
            tooltip.add(std::static_pointer_cast<layout::LayoutElement>(xyz(pos.x, pos.y, pos.z)));
        }
        if (dist) tooltip.append(Component::translatable("jade.distance1", std::vector<std::string>{distanceVal}));
    } else if (dist) {
        tooltip.add(Component::translatable("jade.distance2", std::vector<std::string>{distanceVal}));
    }
}

void DistanceProvider::ForBlock::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) {
    append(tooltip, accessor, accessor.getPosition(), config);
}

void DistanceProvider::ForEntity::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) {
    Actor* entity = accessor.getEntity();
    if (!entity) return;
    Vec3 p = entity->getPosition();
    append(tooltip, accessor, BlockPos{static_cast<int>(std::floor(p.x)), static_cast<int>(std::floor(p.y)), static_cast<int>(std::floor(p.z))}, config);
}

Component BlockFaceProvider::directionName(int direction) {
    static char const* const kNames[] = {"down", "up", "north", "south", "west", "east"};
    return Component::translatable(std::string("jade.") + kNames[std::clamp(direction, 0, 5)]);
}

void BlockFaceProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    int side = accessor.getSide();
    tooltip.replace(JadeIds::CORE_OBJECT_NAME, [side](ElementLines lists) {
        if (lists.empty()) return lists;
        lists.back().push_back(std::static_pointer_cast<layout::LayoutElement>(
            impl::ui::JadeUIInternal::text(Component::translatable("jade.blockFace", std::vector<Component>{directionName(side)}))
        ));
        return lists;
    });
}

void CorePlugin::register_(impl::WailaCommonRegistration& registration) {
    registration.registerBlockDataProvider(ObjectNameProvider::BlockData::INSTANCE, BlockActorClass::any());
}

void CorePlugin::registerClient(impl::WailaClientRegistration& registration) {
    registration.registerAccessorHandler(AccessorType::EMPTY, std::make_unique<impl::EmptyAccessorClientHandler>());
    registration.registerAccessorHandler(AccessorType::BLOCK, std::make_unique<impl::BlockAccessorClientHandler>());
    registration.registerAccessorHandler(AccessorType::ENTITY, std::make_unique<impl::EntityAccessorClientHandler>());

    registration.addConfig(JadeIds::CORE_DISTANCE, "minecraft:spyglass", false);
    registration.addConfig(JadeIds::CORE_COORDINATES, "minecraft:compass", false);
    registration.addConfig(JadeIds::CORE_REL_COORDINATES, "", false);
    registration.addConfig(JadeIds::CORE_MOD_NAME, "minecraft:name_tag", std::string("ON"), {"ON", "OFF", "SMALLER"});
    registration.addConfig(JadeIds::CORE_TRANSLATE_MOD_NAME, "", true);
    registration.addConfigListener(JadeIds::CORE_TRANSLATE_MOD_NAME, [](Identifier const&) { util::ModIdentification::invalidateCache(); });

    registration.registerBlockComponent(ObjectNameProvider::ForBlock::INSTANCE, BlockClass::any());
    registration.registerBlockComponent(ModNameProvider::ForBlock::INSTANCE, BlockClass::any());
    registration.registerBlockComponent(DistanceProvider::ForBlock::INSTANCE, BlockClass::any());
    registration.registerBlockComponent(BlockFaceProvider::INSTANCE, BlockClass::any());

    registration.registerEntityComponent(ObjectNameProvider::ForEntity::INSTANCE, EntityClass::any());
    registration.registerEntityComponent(ModNameProvider::ForEntity::INSTANCE, EntityClass::any());
    registration.registerEntityComponent(DistanceProvider::ForEntity::INSTANCE, EntityClass::any());

    registration.markAsClientFeature(JadeIds::CORE_OBJECT_NAME);
    registration.markAsClientFeature(JadeIds::CORE_DISTANCE);
    registration.markAsClientFeature(JadeIds::CORE_COORDINATES);
    registration.markAsClientFeature(JadeIds::CORE_REL_COORDINATES);
    registration.markAsClientFeature(JadeIds::CORE_MOD_NAME);
    registration.markAsClientFeature(JadeIds::CORE_BLOCK_FACE);

    auto& ops = impl::WailaCommonRegistration::instance().blockOperations();
    for (char const* id :
         {"lit_furnace", "lit_blast_furnace", "lit_smoker", "lit_redstone_ore", "lit_deepslate_redstone_ore", "lit_redstone_lamp",
          "unlit_redstone_torch", "powered_repeater", "unpowered_repeater", "powered_comparator", "unpowered_comparator",
          "daylight_detector_inverted", "standing_sign", "wall_sign", "redstone_wire", "tripwire", "wall_banner", "standing_banner"}) {
        ops.pick(id);
    }
}

}
