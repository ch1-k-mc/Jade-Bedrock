// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components/AgeableComponent.h"
#include "mc/entity/components_json_legacy/BreedableComponent.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorCategory.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActorType.h"

#include "addon/harvest/HarvestToolProvider.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"

namespace jade::addon::vanilla {

namespace {

BlockClass furnaceBlocks() {
    return BlockClass::ofTypes(
        {"minecraft:furnace", "minecraft:lit_furnace", "minecraft:blast_furnace", "minecraft:lit_blast_furnace", "minecraft:smoker", "minecraft:lit_smoker"}
    );
}
BlockClass tntBlocks() { return BlockClass::ofTypes({"minecraft:tnt", "minecraft:underwater_tnt"}); }
BlockClass hopperBlocks() { return BlockClass::ofTypes({"minecraft:hopper"}); }
BlockClass noteBlocks() { return BlockClass::ofTypes({"minecraft:noteblock"}); }
BlockClass jukeboxBlocks() { return BlockClass::ofTypes({"minecraft:jukebox"}); }
BlockClass beehiveBlocks() { return BlockClass::ofTypes({"minecraft:beehive", "minecraft:bee_nest"}); }

BlockClass brewingStandBlocks() { return BlockClass::ofTypes({"minecraft:brewing_stand"}); }
BlockClass lecternBlocks() { return BlockClass::ofTypes({"minecraft:lectern"}); }
BlockClass enchantingTableBlocks() { return BlockClass::ofTypes({"minecraft:enchanting_table"}); }

BlockClass spawnerBlocks() { return BlockClass::ofTypes({"minecraft:mob_spawner", "minecraft:trial_spawner"}); }
BlockClass trialSpawnerBlocks() { return BlockClass::ofTypes({"minecraft:trial_spawner"}); }

BlockClass itemFrameBlocks() { return BlockClass::ofTypes({"minecraft:frame", "minecraft:glow_frame"}); }
BlockClass signBlocks() {
    return BlockClass::of("SignBlock", [](Block const& b) {
        auto const& t = b.getTypeName();
        return t.ends_with("_sign") || t == "minecraft:standing_sign" || t == "minecraft:wall_sign";
    });
}
BlockClass skullAndPotBlocks() {
    return BlockClass::of("AbstractSkullBlock", [](Block const& b) {
        auto const& t = b.getTypeName();
        return t.ends_with("_skull") || t.ends_with("_head") || t == "minecraft:skull" || t == "minecraft:decorated_pot";
    });
}

BlockClass shelfBlocks() {
    return BlockClass::of("ShelfBlock", [](Block const& b) {
        auto const& t = b.getTypeName();
        return t == "minecraft:chiseled_bookshelf" || t.ends_with("_shelf");
    });
}

EntityClass livingEntities() {
    return EntityClass::of("LivingEntity", [](Actor const& a) { return a.hasCategory(ActorCategory::Mob); });
}
EntityClass ageableEntities() { return EntityClass::of("AgeableMob", [](Actor const& a) { return a.hasCategory(ActorCategory::Mob); }); }
EntityClass breedableEntities() {
    return EntityClass::of("Animal", [](Actor const& a) {
        return a.getTypeName() == "minecraft:allay" || a.hasCategory(ActorCategory::Animal) || a.hasCategory(ActorCategory::Villager);
    });
}

BlockClass commandBlocks() { return BlockClass::ofTypes({"minecraft:command_block", "minecraft:repeating_command_block", "minecraft:chain_command_block"}); }

}

void VanillaPlugin::register_(impl::WailaCommonRegistration& registration) {
    registration.registerBlockDataProvider(CommandBlockProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::CommandBlock, "CommandBlockEntity"));
    registration.registerBlockDataProvider(HopperLockProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::Hopper, "HopperBlockEntity"));
    registration.registerBlockDataProvider(JukeboxProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::Jukebox, "JukeboxBlockEntity"));
    registration.registerBlockDataProvider(
        FurnaceProvider::INSTANCE,
        BlockActorClass::ofTypes({BlockActorType::Furnace, BlockActorType::BlastFurnace, BlockActorType::Smoker}, "AbstractFurnaceBlockEntity")
    );
    registration.registerBlockDataProvider(BeehiveProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::Beehive, "BeehiveBlockEntity"));
    registration.registerBlockDataProvider(RedstoneProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::Comparator, "ComparatorBlockEntity"));
    registration.registerBlockDataProvider(BrewingStandProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::BrewingStand, "BrewingStandBlockEntity"));
    registration.registerBlockDataProvider(LecternProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::Lectern, "LecternBlockEntity"));
    registration.registerItemStorage(CampfireProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::Campfire, "CampfireBlockEntity"));
    registration.registerBlockDataProvider(MobSpawnerCooldownProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::TrialSpawner, "TrialSpawnerBlockEntity"));
    registration.registerBlockDataProvider(
        ShelfProvider::INSTANCE,
        BlockActorClass::ofTypes({BlockActorType::ChiseledBookshelf, BlockActorType::Shelf}, "ChiseledBookShelfBlockEntity")
    );
    registration.registerEntityDataProvider(EntityHealthAndArmorProvider::INSTANCE, EntityClass::any());
    registration.registerEntityDataProvider(StatusEffectsProvider::INSTANCE, livingEntities());
    registration.registerEntityDataProvider(MobGrowthProvider::INSTANCE, ageableEntities());
    registration.registerEntityDataProvider(CopperGolemWaxedProvider::INSTANCE, EntityClass::ofIdentifiers({"minecraft:copper_golem"}, "CopperGolem"));
    registration.registerEntityDataProvider(MobBreedingProvider::INSTANCE, breedableEntities());
    registration.registerEntityDataProvider(AnimalOwnerProvider::INSTANCE, EntityClass::any());
    registration.registerEntityDataProvider(NextEntityDropProvider::INSTANCE, EntityClass::ofIdentifiers({"minecraft:chicken", "minecraft:armadillo", "minecraft:sniffer"}, "Chicken"));
    registration.registerEntityDataProvider(ZombieVillagerProvider::INSTANCE, EntityClass::ofIdentifiers({"minecraft:zombie_villager_v2", "minecraft:zombie_villager"}, "ZombieVillager"));
    registration.registerEntityDataProvider(PetArmorProvider::INSTANCE, livingEntities());
    registration.registerBlockDataProvider(NoteBlockProvider::INSTANCE, BlockActorClass::ofType(BlockActorType::Music, "NoteBlockEntity"));
}

void VanillaPlugin::registerClient(impl::WailaClientRegistration& registration) {
    registration.addConfig(JadeIds::MC_EFFECTIVE_TOOL, "", true);
    registration.addConfig(JadeIds::MC_HARVEST_TOOL_NEW_LINE, "", false);
    registration.addConfig(JadeIds::MC_SHOW_UNBREAKABLE, "", false);
    registration.addConfig(JadeIds::MC_HARVEST_TOOL_CREATIVE, "", false);
    registration.addConfig(JadeIds::MC_BREAKING_PROGRESS, "minecraft:iron_pickaxe", true);
    registration.addConfig(JadeIds::MC_ENTITY_HEALTH, "minecraft:golden_apple", true);
    registration.addConfig(JadeIds::MC_ENTITY_ARMOR, "minecraft:iron_chestplate", true);
    registration.addConfig(JadeIds::MC_ENTITY_ARMOR_MAX_FOR_RENDER, "", 20, 0, 200, false);
    registration.addConfig(JadeIds::MC_ENTITY_HEALTH_MAX_FOR_RENDER, "", 40, 0, 200, false);
    registration.addConfig(JadeIds::MC_ENTITY_HEALTH_ICONS_PER_LINE, "", 10, 5, 40, false);
    registration.addConfig(JadeIds::MC_ENTITY_HEALTH_SHOW_FRACTIONS, "", false);
    registration.addConfig(JadeIds::MC_POTION_EFFECTS_LIMIT, "", 7, 1, 99, false);
    registration.addConfigEnum(JadeIds::MC_PET_ARMOR, "minecraft:wolf_armor", PetArmorMode::SHOW_DAMAGEABLE);
    registration.registerBlockComponent(TNTStabilityProvider::INSTANCE, tntBlocks());
    registration.registerBlockComponent(NoteBlockProvider::Client::INSTANCE, noteBlocks());
    registration.registerBlockComponent(CommandBlockProvider::Client::INSTANCE, commandBlocks());
    registration.registerBlockComponent(FurnaceProvider::Client::INSTANCE, furnaceBlocks());
    registration.registerBlockComponent(HopperLockProvider::Client::INSTANCE, hopperBlocks());
    registration.registerBlockComponent(JukeboxProvider::Client::INSTANCE, jukeboxBlocks());
    registration.registerBlockComponent(BeehiveProvider::Client::INSTANCE, beehiveBlocks());
    registration.registerBlockComponent(RedstoneProvider::Client::INSTANCE, BlockClass::any());
    registration.registerBlockComponent(CropProgressProvider::INSTANCE, BlockClass::any());
    registration.registerBlockComponent(BrewingStandProvider::Client::INSTANCE, brewingStandBlocks());
    registration.registerBlockComponent(LecternProvider::Client::INSTANCE, lecternBlocks());
    registration.registerBlockComponent(EnchantmentPowerProvider::INSTANCE, BlockClass::any());
    registration.registerBlockComponent(TotalEnchantmentPowerProvider::INSTANCE, enchantingTableBlocks());
    registration.registerBlockComponent(MobSpawnerProvider::INSTANCE, spawnerBlocks());
    registration.registerBlockComponent(MobSpawnerCooldownProvider::Client::INSTANCE, trialSpawnerBlocks());
    registration.registerBlockComponent(ItemFrameProvider::INSTANCE, itemFrameBlocks());
    registration.registerBlockComponent(WaxedProvider::INSTANCE, signBlocks());
    registration.registerBlockIcon(WaxedProvider::INSTANCE, signBlocks());
    registration.registerBlockIcon(ItemBERProvider::INSTANCE, skullAndPotBlocks());
    registration.registerBlockComponent(harvest::HarvestToolProvider::INSTANCE, BlockClass::any());
    registration.registerBlockComponent(ShelfProvider::Client::INSTANCE, shelfBlocks());
    registration.registerBlockIcon(ShelfProvider::Client::INSTANCE, shelfBlocks());
    registration.registerEntityComponent(EntityHealthAndArmorProvider::Client::INSTANCE, EntityClass::any());
    registration.registerEntityComponent(StatusEffectsProvider::Client::INSTANCE, livingEntities());
    registration.registerEntityComponent(MobGrowthProvider::Client::INSTANCE, ageableEntities());
    registration.registerEntityComponent(CopperGolemWaxedProvider::Client::INSTANCE, EntityClass::ofIdentifiers({"minecraft:copper_golem"}, "CopperGolem"));
    registration.registerEntityIcon(CopperGolemWaxedProvider::Client::INSTANCE, EntityClass::ofIdentifiers({"minecraft:copper_golem"}, "CopperGolem"));
    registration.registerEntityComponent(MobBreedingProvider::Client::INSTANCE, breedableEntities());
    registration.registerEntityComponent(AnimalOwnerProvider::Client::INSTANCE, EntityClass::any());
    registration.registerEntityComponent(ArmorStandProvider::INSTANCE, EntityClass::ofIdentifiers({"minecraft:armor_stand"}, "ArmorStand"));
    registration.registerEntityIcon(FallingBlockProvider::INSTANCE, EntityClass::ofIdentifiers({"minecraft:falling_block"}, "FallingBlockEntity"));
    registration.registerEntityComponent(PaintingProvider::INSTANCE, EntityClass::ofIdentifiers({"minecraft:painting"}, "Painting"));
    registration.registerEntityComponent(NextEntityDropProvider::Client::INSTANCE, EntityClass::ofIdentifiers({"minecraft:chicken", "minecraft:armadillo", "minecraft:sniffer"}, "Chicken"));
    registration.registerEntityComponent(ZombieVillagerProvider::Client::INSTANCE, EntityClass::ofIdentifiers({"minecraft:zombie_villager_v2", "minecraft:zombie_villager"}, "ZombieVillager"));
    registration.registerEntityComponent(PetArmorProvider::Client::INSTANCE, livingEntities());
    registration.registerEntityComponent(
        ItemTooltipProvider::INSTANCE,
        EntityClass::of("ItemEntity", [](Actor const& a) { return a.hasCategory(ActorCategory::Item); })
    );
    registration.registerEntityComponent(
        HorseStatsProvider::INSTANCE,
        EntityClass::ofIdentifiers(
            {"minecraft:horse", "minecraft:donkey", "minecraft:mule", "minecraft:zombie_horse", "minecraft:skeleton_horse", "minecraft:llama", "minecraft:trader_llama", "minecraft:camel"},
            "AbstractHorse"
        )
    );
    registration.registerEntityComponent(
        VillagerProfessionProvider::INSTANCE,
        EntityClass::ofIdentifiers({"minecraft:villager_v2", "minecraft:zombie_villager_v2", "minecraft:villager", "minecraft:zombie_villager"}, "Villager")
    );
    registration.registerItemStorageClient(CampfireProvider::INSTANCE);
    BreakingProgressProvider::registerTo(registration);

    registration.markAsClientFeature(JadeIds::MC_TNT_STABILITY);
    registration.markAsClientFeature(JadeIds::MC_NOTE_BLOCK);
    registration.markAsClientFeature(JadeIds::MC_CROP_PROGRESS);
    registration.markAsClientFeature(JadeIds::MC_ENCHANTMENT_POWER);
    registration.markAsClientFeature(JadeIds::MC_TOTAL_ENCHANTMENT_POWER);
    registration.markAsClientFeature(JadeIds::MC_MOB_SPAWNER);
    registration.markAsClientFeature(JadeIds::MC_ITEM_FRAME);
    registration.markAsClientFeature(JadeIds::MC_WAXED);
    registration.markAsClientFeature(JadeIds::MC_BREAKING_PROGRESS);
    registration.markAsClientFeature(JadeIds::MC_EFFECTIVE_TOOL);
    registration.markAsClientFeature(JadeIds::MC_HARVEST_TOOL_NEW_LINE);
    registration.markAsClientFeature(JadeIds::MC_SHOW_UNBREAKABLE);
    registration.markAsClientFeature(JadeIds::MC_HARVEST_TOOL_CREATIVE);
    registration.markAsClientFeature(JadeIds::MC_HARVEST_TOOL);
    registration.markAsClientFeature(JadeIds::MC_ENTITY_ARMOR_MAX_FOR_RENDER);
    registration.markAsClientFeature(JadeIds::MC_ENTITY_HEALTH_MAX_FOR_RENDER);
    registration.markAsClientFeature(JadeIds::MC_ENTITY_HEALTH_ICONS_PER_LINE);
    registration.markAsClientFeature(JadeIds::MC_ENTITY_HEALTH_SHOW_FRACTIONS);
    registration.markAsClientFeature(JadeIds::MC_ENTITY_ARMOR);
    registration.markAsClientFeature(JadeIds::MC_HORSE_STATS);
    registration.markAsClientFeature(JadeIds::MC_ARMOR_STAND);
    registration.markAsClientFeature(JadeIds::MC_PAINTING);
    registration.markAsClientFeature(JadeIds::MC_ITEM_TOOLTIP);

    auto& tools = harvest::ToolTypeRegistry::get();
    tools.clear();
    harvest::HarvestToolProvider::registerVanillaTools(tools);
    harvest::HarvestToolProvider::INSTANCE->invalidateCache();

    auto block = text::Component::translatable("config.jade.plugin_minecraft.block");
    auto entity = text::Component::translatable("config.jade.plugin_minecraft.entity");
    registration.setConfigCategoryOverride(JadeIds::MC_ENTITY_ARMOR, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_ENTITY_HEALTH, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_MOB_BREEDING, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_ANIMAL_OWNER, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_ARMOR_STAND, std::vector<text::Component>{block, entity});
    registration.setConfigCategoryOverride(JadeIds::MC_PAINTING, std::vector<text::Component>{block, entity});
    registration.setConfigCategoryOverride(JadeIds::MC_HORSE_STATS, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_ITEM_TOOLTIP, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_NEXT_ENTITY_DROP, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_PET_ARMOR, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_ZOMBIE_VILLAGER, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_VILLAGER_PROFESSION, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_MOB_GROWTH, entity);
    registration.setConfigCategoryOverride(JadeIds::MC_POTION_EFFECTS, entity);
    impl::WailaCommonRegistration::instance().priorities.putUnsafe(JadeIds::MC_ENTITY_ARMOR, -4499);
    registration.setConfigCategoryOverride(JadeIds::MC_BEEHIVE, block);
    registration.setConfigCategoryOverride(JadeIds::MC_BREAKING_PROGRESS, block);
    registration.setConfigCategoryOverride(JadeIds::MC_BREWING_STAND, block);
    registration.setConfigCategoryOverride(JadeIds::MC_COMMAND_BLOCK, block);
    registration.setConfigCategoryOverride(JadeIds::MC_CROP_PROGRESS, block);
    registration.setConfigCategoryOverride(JadeIds::MC_ENCHANTMENT_POWER, block);
    registration.setConfigCategoryOverride(JadeIds::MC_FURNACE, block);
    registration.setConfigCategoryOverride(JadeIds::MC_HARVEST_TOOL, block);
    registration.setConfigCategoryOverride(JadeIds::MC_ITEM_FRAME, block);
    registration.setConfigCategoryOverride(JadeIds::MC_JUKEBOX, block);
    registration.setConfigCategoryOverride(JadeIds::MC_LECTERN, block);
    registration.setConfigCategoryOverride(JadeIds::MC_MOB_SPAWNER, block);
    registration.setConfigCategoryOverride(JadeIds::MC_NOTE_BLOCK, block);
    registration.setConfigCategoryOverride(JadeIds::MC_REDSTONE, block);
    registration.setConfigCategoryOverride(JadeIds::MC_SHELF, block);
    registration.setConfigCategoryOverride(JadeIds::MC_TNT_STABILITY, block);
    registration.setConfigCategoryOverride(JadeIds::MC_TOTAL_ENCHANTMENT_POWER, block);
    registration.setConfigCategoryOverride(JadeIds::MC_WAXED, block);
}

}
