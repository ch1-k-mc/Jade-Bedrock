// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <string>

namespace jade {

using Identifier = std::string;

namespace JadeIds {

inline Identifier JADE(std::string const& path) { return "jade:" + path; }
inline Identifier ACCESS(std::string const& path) { return "jade_access:" + path; }
inline Identifier MC(std::string const& path) { return "minecraft:" + path; }

inline Identifier const ROOT = "minecraft:root";
inline Identifier const DEFAULT_THEME = "jade:dark";
inline Identifier const UI_MAIN = "jade:main";
inline Identifier const PACKET_RECEIVE_DATA = "jade:receive_data";
inline Identifier const PACKET_SERVER_HANDSHAKE = "jade:server_handshake";
inline Identifier const PACKET_CLIENT_HANDSHAKE = "jade:client_handshake";
inline Identifier const PACKET_REQUEST_ENTITY = "jade:request_entity";
inline Identifier const PACKET_REQUEST_BLOCK = "jade:request_block";
inline Identifier const PACKET_SHOW_OVERLAY = "jade:show_overlay";
inline Identifier const CORE_ROOT_ICON = "jade:root_icon";
inline Identifier const CORE_OBJECT_NAME = "jade:object_name";
inline Identifier const CORE_MOD_NAME = "jade:mod_name";
inline Identifier const CORE_TRANSLATE_MOD_NAME = "jade:mod_name.translated";
inline Identifier const CORE_DISTANCE = "jade:distance";
inline Identifier const CORE_COORDINATES = "jade:coordinates";
inline Identifier const CORE_REL_COORDINATES = "jade:coordinates.rel";
inline Identifier const CORE_BLOCK_FACE = "jade:block_face";
inline Identifier const DEBUG_REGISTRY_NAME = "jade:registry_name";
inline Identifier const DEBUG_SPECIAL_REGISTRY_NAME = "jade:registry_name.special";
inline Identifier const DEBUG_BLOCK_PROPERTIES = "jade:block_properties";
inline Identifier const DEBUG_BLOCK_STATES = "jade:block_states";
inline Identifier const DEBUG_LOOT_TABLE = "jade:loot_table";
inline Identifier const DEBUG_ENTITY_ATTRIBUTES = "jade:entity_attributes";
inline Identifier const UNIVERSAL_ITEM_STORAGE = "minecraft:item_storage";
inline Identifier const UNIVERSAL_ITEM_STORAGE_DEFAULT = "minecraft:item_storage.default";
inline Identifier const UNIVERSAL_ITEM_STORAGE_DETAILED_AMOUNT = "minecraft:item_storage.detailed_amount";
inline Identifier const UNIVERSAL_ITEM_STORAGE_NORMAL_AMOUNT = "minecraft:item_storage.normal_amount";
inline Identifier const UNIVERSAL_ITEM_STORAGE_SHOW_NAME_AMOUNT = "minecraft:item_storage.show_name_amount";
inline Identifier const UNIVERSAL_ITEM_STORAGE_ITEMS_PER_LINE = "minecraft:item_storage.items_per_line";
inline Identifier const UNIVERSAL_ITEM_STORAGE_SORT = "minecraft:item_storage.sort";
inline Identifier const UNIVERSAL_FLUID_STORAGE = "minecraft:fluid_storage";
inline Identifier const UNIVERSAL_FLUID_STORAGE_DEFAULT = "minecraft:fluid_storage.default";
inline Identifier const UNIVERSAL_FLUID_STORAGE_DETAILED = "minecraft:fluid_storage.detailed";
inline Identifier const UNIVERSAL_FLUID_STORAGE_STYLE = "minecraft:fluid_storage.style";
inline Identifier const UNIVERSAL_ENERGY_STORAGE = "minecraft:energy_storage";
inline Identifier const UNIVERSAL_ENERGY_STORAGE_DEFAULT = "minecraft:energy_storage.default";
inline Identifier const UNIVERSAL_ENERGY_STORAGE_DETAILED = "minecraft:energy_storage.detailed";
inline Identifier const UNIVERSAL_ENERGY_STORAGE_STYLE = "minecraft:energy_storage.style";
inline Identifier const UNIVERSAL_PROGRESS = "minecraft:progress";
inline Identifier const UNIVERSAL_HIDE_THINGS = "jade:hide_things";
inline Identifier const MC_ANIMAL_OWNER = "minecraft:animal_owner";
inline Identifier const MC_ARMOR_STAND = "minecraft:armor_stand";
inline Identifier const MC_BEEHIVE = "minecraft:beehive";
inline Identifier const MC_BLOCK_DISPLAY = "minecraft:block_display";
inline Identifier const MC_BREAKING_PROGRESS = "minecraft:breaking_progress";
inline Identifier const MC_BREWING_STAND = "minecraft:brewing_stand";
inline Identifier const MC_CAMPFIRE = "minecraft:campfire";
inline Identifier const MC_COMMAND_BLOCK = "minecraft:command_block";
inline Identifier const MC_CROP_PROGRESS = "minecraft:crop_progress";
inline Identifier const MC_ENCHANTMENT_POWER = "minecraft:enchantment_power";
inline Identifier const MC_ENTITY_ARMOR = "minecraft:entity_armor";
inline Identifier const MC_ENTITY_ARMOR_MAX_FOR_RENDER = "minecraft:entity_armor.max_for_render";
inline Identifier const MC_ENTITY_HEALTH = "minecraft:entity_health";
inline Identifier const MC_ENTITY_HEALTH_MAX_FOR_RENDER = "minecraft:entity_health.max_for_render";
inline Identifier const MC_ENTITY_HEALTH_ICONS_PER_LINE = "minecraft:entity_health.icons_per_line";
inline Identifier const MC_ENTITY_HEALTH_SHOW_FRACTIONS = "minecraft:entity_health.show_fractions";
inline Identifier const MC_FALLING_BLOCK = "minecraft:falling_block";
inline Identifier const MC_FURNACE = "minecraft:furnace";
inline Identifier const MC_HARVEST_TOOL = "minecraft:harvest_tool";
inline Identifier const MC_HARVEST_TOOL_NEW_LINE = "minecraft:harvest_tool.new_line";
inline Identifier const MC_EFFECTIVE_TOOL = "minecraft:harvest_tool.effective_tool";
inline Identifier const MC_SHOW_UNBREAKABLE = "minecraft:harvest_tool.show_unbreakable";
inline Identifier const MC_HARVEST_TOOL_CREATIVE = "minecraft:harvest_tool.creative";
inline Identifier const MC_HOPPER_LOCK = "minecraft:hopper_lock";
inline Identifier const MC_HORSE_STATS = "minecraft:horse_stats";
inline Identifier const MC_ITEM_BER = "minecraft:item_ber";
inline Identifier const MC_ITEM_DISPLAY = "minecraft:item_display";
inline Identifier const MC_ITEM_FRAME = "minecraft:item_frame";
inline Identifier const MC_ITEM_TOOLTIP = "minecraft:item_tooltip";
inline Identifier const MC_JUKEBOX = "minecraft:jukebox";
inline Identifier const MC_LECTERN = "minecraft:lectern";
inline Identifier const MC_MOB_BREEDING = "minecraft:mob_breeding";
inline Identifier const MC_MOB_GROWTH = "minecraft:mob_growth";
inline Identifier const MC_MOB_SPAWNER = "minecraft:mob_spawner";
inline Identifier const MC_MOB_SPAWNER_COOLDOWN = "minecraft:mob_spawner.cooldown";
inline Identifier const MC_NEXT_ENTITY_DROP = "minecraft:next_entity_drop";
inline Identifier const MC_NOTE_BLOCK = "minecraft:note_block";
inline Identifier const MC_PAINTING = "minecraft:painting";
inline Identifier const MC_PET_ARMOR = "minecraft:pet_armor";
inline Identifier const MC_PLAYER_HEAD = "minecraft:player_head";
inline Identifier const MC_POTION_EFFECTS = "minecraft:potion_effects";
inline Identifier const MC_POTION_EFFECTS_LIMIT = "minecraft:potion_effects.limit";
inline Identifier const MC_REDSTONE = "minecraft:redstone";
inline Identifier const MC_SHELF = "minecraft:shelf";
inline Identifier const MC_SULFUR_CUBE = "minecraft:sulfur_cube";
inline Identifier const MC_TNT_STABILITY = "minecraft:tnt_stability";
inline Identifier const MC_TOTAL_ENCHANTMENT_POWER = "minecraft:total_enchantment_power";
inline Identifier const MC_VILLAGER_PROFESSION = "minecraft:villager_profession";
inline Identifier const MC_VILLAGER_RESTOCK = "minecraft:villager_restock";
inline Identifier const MC_WAXED = "minecraft:waxed";
inline Identifier const MC_ZOMBIE_VILLAGER = "minecraft:zombie_villager";
inline Identifier const ACCESS_SIGN = "jade_access:sign";
inline Identifier const ACCESS_BLOCK_DETAILS = "jade_access:block";
inline Identifier const ACCESS_BLOCK_DETAILS_BODY = "jade_access:block_body";
inline Identifier const ACCESS_BLOCK_AMOUNT = "jade_access:block_amount";
inline Identifier const ACCESS_ENTITY_DETAILS = "jade_access:entity";
inline Identifier const ACCESS_ENTITY_DETAILS_BODY = "jade_access:entity_body";
inline Identifier const ACCESS_ENTITY_VARIANT = "jade_access:entity_variant";
inline Identifier const ACCESS_HELD_ITEM = "jade_access:held_item";
inline Identifier const ACCESS_NPC_DESCRIPTION = "jade_access:npc_description";

}

}
