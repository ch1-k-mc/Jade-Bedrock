// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>

#include "api/IJadeProvider.h"
#include "api/IWailaPlugin.h"
#include "api/StreamServerDataProvider.h"
#include "api/view/IExtensionProvider.h"
#include "api/view/ItemView.h"

namespace jade::addon::vanilla {

enum class PetArmorMode { OFF, SHOW_ALL, SHOW_DAMAGEABLE };

}

namespace jade::api::config {
JADE_ENUM_NAMES(::jade::addon::vanilla::PetArmorMode, "OFF", "SHOW_ALL", "SHOW_DAMAGEABLE")
}

namespace jade::addon::vanilla {

using namespace jade::api;

struct FurnaceData {
    int                                progress = 0;
    int                                total    = 0;
    std::array<view::ItemData, 3>      inventory;
};

class FurnaceProvider : public StreamServerDataProvider<BlockAccessor, FurnaceData> {
public:
    static inline std::shared_ptr<FurnaceProvider> const INSTANCE = std::make_shared<FurnaceProvider>();
    Identifier                 getUid() const override { return JadeIds::MC_FURNACE; }
    bool                       shouldRequestData(BlockAccessor& accessor) override;
    std::optional<FurnaceData> streamData(BlockAccessor& accessor) override;
    void                       encode(CompoundTag& out, FurnaceData const& value) override;
    std::optional<FurnaceData> decode(CompoundTag const& in) override;

    class Client;
};

class FurnaceProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_FURNACE; }
    std::string getConfigIcon() const override { return "minecraft:furnace"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class TNTStabilityProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<TNTStabilityProvider> const INSTANCE = std::make_shared<TNTStabilityProvider>();
    Identifier  getUid() const override { return JadeIds::MC_TNT_STABILITY; }
    std::string getConfigIcon() const override { return "minecraft:tnt"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class HopperLockProvider : public StreamServerDataProvider<BlockAccessor, bool> {
public:
    static inline std::shared_ptr<HopperLockProvider> const INSTANCE = std::make_shared<HopperLockProvider>();
    Identifier          getUid() const override { return JadeIds::MC_HOPPER_LOCK; }
    std::optional<bool> streamData(BlockAccessor& accessor) override;
    void                encode(CompoundTag& out, bool const& value) override { out["locked"] = value; }
    std::optional<bool> decode(CompoundTag const& in) override;

    class Client;
};

class HopperLockProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier getUid() const override { return JadeIds::MC_HOPPER_LOCK; }
    bool       isRequired() const override { return true; }
    int        getDefaultPriority() const override { return TooltipPosition::HEAD - 100 + 10; }
    void       appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class NoteBlockProvider : public StreamServerDataProvider<BlockAccessor, int> {
public:
    static inline std::shared_ptr<NoteBlockProvider> const INSTANCE = std::make_shared<NoteBlockProvider>();
    Identifier         getUid() const override { return JadeIds::MC_NOTE_BLOCK; }
    std::optional<int> streamData(BlockAccessor& accessor) override;
    void               encode(CompoundTag& out, int const& value) override { out["note"] = value; }
    std::optional<int> decode(CompoundTag const& in) override;

    class Client;
};

class NoteBlockProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_NOTE_BLOCK; }
    std::string getConfigIcon() const override { return "minecraft:noteblock"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class JukeboxProvider : public StreamServerDataProvider<BlockAccessor, view::ItemData> {
public:
    static inline std::shared_ptr<JukeboxProvider> const INSTANCE = std::make_shared<JukeboxProvider>();
    Identifier                    getUid() const override { return JadeIds::MC_JUKEBOX; }
    bool                          shouldRequestData(BlockAccessor& accessor) override;
    std::optional<view::ItemData> streamData(BlockAccessor& accessor) override;
    void                          encode(CompoundTag& out, view::ItemData const& value) override { view::ViewCodec<view::ItemData>::encode(out, value); }
    std::optional<view::ItemData> decode(CompoundTag const& in) override { return view::ViewCodec<view::ItemData>::decode(in); }

    class Client;
};

class JukeboxProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_JUKEBOX; }
    std::string getConfigIcon() const override { return "minecraft:jukebox"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class CommandBlockProvider : public StreamServerDataProvider<BlockAccessor, std::string> {
public:
    static inline std::shared_ptr<CommandBlockProvider> const INSTANCE = std::make_shared<CommandBlockProvider>();
    Identifier                 getUid() const override { return JadeIds::MC_COMMAND_BLOCK; }
    bool                       shouldRequestData(BlockAccessor& accessor) override;
    std::optional<std::string> streamData(BlockAccessor& accessor) override;
    void                       encode(CompoundTag& out, std::string const& value) override { out["command"] = value; }
    std::optional<std::string> decode(CompoundTag const& in) override;

    class Client;
};

class CommandBlockProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_COMMAND_BLOCK; }
    std::string getConfigIcon() const override { return "minecraft:command_block"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class RedstoneProvider : public StreamServerDataProvider<BlockAccessor, int> {
public:
    static inline std::shared_ptr<RedstoneProvider> const INSTANCE = std::make_shared<RedstoneProvider>();
    Identifier         getUid() const override { return JadeIds::MC_REDSTONE; }
    std::optional<int> streamData(BlockAccessor& accessor) override;
    void               encode(CompoundTag& out, int const& value) override { out["signal"] = value; }
    std::optional<int> decode(CompoundTag const& in) override;

    class Client;
};

class RedstoneProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_REDSTONE; }
    std::string getConfigIcon() const override { return "minecraft:redstone"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class CropProgressProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<CropProgressProvider> const INSTANCE = std::make_shared<CropProgressProvider>();
    Identifier     getUid() const override { return JadeIds::MC_CROP_PROGRESS; }
    std::string    getConfigIcon() const override { return "minecraft:wheat"; }
    ui::ElementPtr getIcon(BlockAccessor& accessor, IPluginConfig& config, ui::ElementPtr currentIcon) override;
    void           appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class BeehiveProvider : public StreamServerDataProvider<BlockAccessor, int> {
public:
    static inline std::shared_ptr<BeehiveProvider> const INSTANCE = std::make_shared<BeehiveProvider>();
    Identifier         getUid() const override { return JadeIds::MC_BEEHIVE; }
    bool               shouldRequestData(BlockAccessor& accessor) override;
    std::optional<int> streamData(BlockAccessor& accessor) override;
    void               encode(CompoundTag& out, int const& value) override { out["bees"] = value; }
    std::optional<int> decode(CompoundTag const& in) override;

    class Client;
};

class BeehiveProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_BEEHIVE; }
    std::string getConfigIcon() const override { return "minecraft:beehive"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

struct BrewingStandData {
    int fuel = 0;
    int time = 0;
};

class BrewingStandProvider : public StreamServerDataProvider<BlockAccessor, BrewingStandData> {
public:
    static inline std::shared_ptr<BrewingStandProvider> const INSTANCE = std::make_shared<BrewingStandProvider>();
    Identifier                      getUid() const override { return JadeIds::MC_BREWING_STAND; }
    bool                            shouldRequestData(BlockAccessor& accessor) override;
    std::optional<BrewingStandData> streamData(BlockAccessor& accessor) override;
    void                            encode(CompoundTag& out, BrewingStandData const& value) override {
        out["fuel"] = value.fuel;
        out["time"] = value.time;
    }
    std::optional<BrewingStandData> decode(CompoundTag const& in) override;

    class Client;
};

class BrewingStandProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_BREWING_STAND; }
    std::string getConfigIcon() const override { return "minecraft:brewing_stand"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class LecternProvider : public StreamServerDataProvider<BlockAccessor, view::ItemData> {
public:
    static inline std::shared_ptr<LecternProvider> const INSTANCE = std::make_shared<LecternProvider>();
    Identifier                    getUid() const override { return JadeIds::MC_LECTERN; }
    bool                          shouldRequestData(BlockAccessor& accessor) override;
    std::optional<view::ItemData> streamData(BlockAccessor& accessor) override;
    void                          encode(CompoundTag& out, view::ItemData const& value) override { view::ViewCodec<view::ItemData>::encode(out, value); }
    std::optional<view::ItemData> decode(CompoundTag const& in) override { return view::ViewCodec<view::ItemData>::decode(in); }

    class Client;
};

class LecternProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_LECTERN; }
    std::string getConfigIcon() const override { return "minecraft:lectern"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class CampfireProvider : public view::IServerExtensionProvider<view::ItemData>,
                         public view::IClientExtensionProvider<view::ItemData, view::ItemView> {
public:
    static inline std::shared_ptr<CampfireProvider> const INSTANCE = std::make_shared<CampfireProvider>();
    Identifier getUid() const override { return JadeIds::MC_CAMPFIRE; }
    std::optional<view::ViewGroups<view::ItemData>> getGroups(Accessor& accessor) override;
    std::vector<view::ClientViewGroup<view::ItemView>>
    getClientGroups(Accessor& accessor, view::ViewGroups<view::ItemData> const& groups) override;
};

class EnchantmentPowerProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<EnchantmentPowerProvider> const INSTANCE = std::make_shared<EnchantmentPowerProvider>();
    Identifier   getUid() const override { return JadeIds::MC_ENCHANTMENT_POWER; }
    std::string  getConfigIcon() const override { return "minecraft:enchanting_table"; }
    int          getDefaultPriority() const override { return -400; }
    void         appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
    static float getPower(BlockSource& region, BlockPos const& pos);
};

class TotalEnchantmentPowerProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<TotalEnchantmentPowerProvider> const INSTANCE = std::make_shared<TotalEnchantmentPowerProvider>();
    Identifier  getUid() const override { return JadeIds::MC_TOTAL_ENCHANTMENT_POWER; }
    std::string getConfigIcon() const override { return "minecraft:enchanting_table"; }
    int         getDefaultPriority() const override { return -400; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class MobSpawnerProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<MobSpawnerProvider> const INSTANCE = std::make_shared<MobSpawnerProvider>();
    Identifier  getUid() const override { return JadeIds::MC_MOB_SPAWNER; }
    std::string getConfigIcon() const override { return "minecraft:mob_spawner"; }
    int         getDefaultPriority() const override { return TooltipPosition::HEAD - 100 + 10; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class MobSpawnerCooldownProvider : public StreamServerDataProvider<BlockAccessor, int> {
public:
    static inline std::shared_ptr<MobSpawnerCooldownProvider> const INSTANCE = std::make_shared<MobSpawnerCooldownProvider>();
    Identifier         getUid() const override { return JadeIds::MC_MOB_SPAWNER_COOLDOWN; }
    bool               shouldRequestData(BlockAccessor& accessor) override;
    std::optional<int> streamData(BlockAccessor& accessor) override;
    void               encode(CompoundTag& out, int const& value) override { out["cooldown"] = value; }
    std::optional<int> decode(CompoundTag const& in) override { return static_cast<int>(in.at("cooldown")); }

    class Client;
};

class MobSpawnerCooldownProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier getUid() const override { return JadeIds::MC_MOB_SPAWNER_COOLDOWN; }
    bool       isRequired() const override { return true; }
    void       appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class ItemFrameProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<ItemFrameProvider> const INSTANCE = std::make_shared<ItemFrameProvider>();
    Identifier  getUid() const override { return JadeIds::MC_ITEM_FRAME; }
    std::string getConfigIcon() const override { return "minecraft:frame"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class WaxedProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<WaxedProvider> const INSTANCE = std::make_shared<WaxedProvider>();
    Identifier     getUid() const override { return JadeIds::MC_WAXED; }
    std::string    getConfigIcon() const override { return "minecraft:honeycomb"; }
    int            getDefaultPriority() const override { return TooltipPosition::HEAD - 100 + 10; }
    ui::ElementPtr getIcon(BlockAccessor& accessor, IPluginConfig& config, ui::ElementPtr currentIcon) override;
    void           appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
    static ui::ElementPtr waxedIcon(ui::ElementPtr icon, bool waxed);
};

class CopperGolemWaxedProvider : public StreamServerDataProvider<EntityAccessor, int> {
public:
    static constexpr int WAXED = 4;
    static inline std::shared_ptr<CopperGolemWaxedProvider> const INSTANCE = std::make_shared<CopperGolemWaxedProvider>();
    Identifier         getUid() const override { return JadeIds::MC_WAXED; }
    std::optional<int> streamData(EntityAccessor& accessor) override;
    void               encode(CompoundTag& out, int const& value) override { out["state"] = value; }
    std::optional<int> decode(CompoundTag const& in) override { return static_cast<int>(in.at("state")); }

    class Client;
};

class CopperGolemWaxedProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier     getUid() const override { return JadeIds::MC_WAXED; }
    std::string    getConfigIcon() const override { return "minecraft:honeycomb"; }
    int            getDefaultPriority() const override { return TooltipPosition::HEAD - 100 + 10; }
    ui::ElementPtr getIcon(EntityAccessor& accessor, IPluginConfig& config, ui::ElementPtr currentIcon) override;
    void           appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class ItemBERProvider : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<ItemBERProvider> const INSTANCE = std::make_shared<ItemBERProvider>();
    Identifier     getUid() const override { return JadeIds::MC_ITEM_BER; }
    bool           isRequired() const override { return true; }
    ui::ElementPtr getIcon(BlockAccessor& accessor, IPluginConfig& config, ui::ElementPtr currentIcon) override;
    void           appendTooltip(ITooltip&, BlockAccessor&, IPluginConfig&) override {}
};

class BreakingProgressProvider : public ui::IBoxProgressProvider {
public:
    static inline std::shared_ptr<BreakingProgressProvider> const INSTANCE = std::make_shared<BreakingProgressProvider>();
    static void                   registerTo(impl::WailaClientRegistration& registration);
    std::optional<ui::BoxProgress> getProgress(ui::BoxElement& box, Accessor& accessor, float partialTicks) override;

private:
    float mProgressAlpha = 0;
};

class ShelfProvider : public StreamServerDataProvider<BlockAccessor, view::ItemData> {
public:
    static inline std::shared_ptr<ShelfProvider> const INSTANCE = std::make_shared<ShelfProvider>();
    Identifier                    getUid() const override { return JadeIds::MC_SHELF; }
    bool                          shouldRequestData(BlockAccessor& accessor) override;
    std::optional<view::ItemData> streamData(BlockAccessor& accessor) override;
    void                          encode(CompoundTag& out, view::ItemData const& value) override { view::ViewCodec<view::ItemData>::encode(out, value); }
    std::optional<view::ItemData> decode(CompoundTag const& in) override { return view::ViewCodec<view::ItemData>::decode(in); }

    class Client;
};

class ShelfProvider::Client : public IBlockComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier     getUid() const override { return JadeIds::MC_SHELF; }
    std::string    getConfigIcon() const override { return "minecraft:chiseled_bookshelf"; }
    int            getDefaultPriority() const override { return TooltipPosition::BODY + 1000 + 1; }
    ui::ElementPtr getIcon(BlockAccessor& accessor, IPluginConfig& config, ui::ElementPtr currentIcon) override;
    void           appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class EntityHealthAndArmorProvider : public StreamServerDataProvider<EntityAccessor, float> {
public:
    static inline std::shared_ptr<EntityHealthAndArmorProvider> const INSTANCE = std::make_shared<EntityHealthAndArmorProvider>();
    Identifier           getUid() const override { return JadeIds::MC_ENTITY_HEALTH; }
    int                  getDefaultPriority() const override { return -8000; }
    bool                 shouldRequestData(EntityAccessor& accessor) override;
    std::optional<float> streamData(EntityAccessor& accessor) override;
    void                 encode(CompoundTag& out, float const& value) override { out["absorption"] = value; }
    std::optional<float> decode(CompoundTag const& in) override { return static_cast<float>(in.at("absorption")); }
    static bool          isHealthVisible(Actor const& entity);

    class Client;
};

class EntityHealthAndArmorProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier getUid() const override { return JadeIds::MC_ENTITY_HEALTH; }
    int        getDefaultPriority() const override { return -8000; }
    bool       isRequired() const override { return true; }
    void       appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

struct EffectData {
    int id        = 0;
    int amplifier = 0;
    int ticks     = 0;
};

class StatusEffectsProvider : public StreamServerDataProvider<EntityAccessor, std::vector<EffectData>> {
public:
    static inline std::shared_ptr<StatusEffectsProvider> const INSTANCE = std::make_shared<StatusEffectsProvider>();
    Identifier                             getUid() const override { return JadeIds::MC_POTION_EFFECTS; }
    bool                                   shouldRequestData(EntityAccessor& accessor) override;
    std::optional<std::vector<EffectData>> streamData(EntityAccessor& accessor) override;
    void                                   encode(CompoundTag& out, std::vector<EffectData> const& value) override;
    std::optional<std::vector<EffectData>> decode(CompoundTag const& in) override;

    class Client;
};

class StatusEffectsProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier             getUid() const override { return JadeIds::MC_POTION_EFFECTS; }
    std::string            getConfigIcon() const override { return "minecraft:potion"; }
    void                   appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
    static text::Component getEffectName(EffectData const& effect);
};

class MobGrowthProvider : public StreamServerDataProvider<EntityAccessor, int> {
public:
    static constexpr int PAUSED = -2;
    static inline std::shared_ptr<MobGrowthProvider> const INSTANCE = std::make_shared<MobGrowthProvider>();
    Identifier         getUid() const override { return JadeIds::MC_MOB_GROWTH; }
    bool               shouldRequestData(EntityAccessor& accessor) override;
    std::optional<int> streamData(EntityAccessor& accessor) override;
    void               encode(CompoundTag& out, int const& value) override { out["time"] = value; }
    std::optional<int> decode(CompoundTag const& in) override { return static_cast<int>(in.at("time")); }

    class Client;
};

class MobGrowthProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_MOB_GROWTH; }
    std::string getConfigIcon() const override { return "minecraft:wheat"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class MobBreedingProvider : public StreamServerDataProvider<EntityAccessor, int> {
public:
    static constexpr int IN_LOVE = -1;
    static inline std::shared_ptr<MobBreedingProvider> const INSTANCE = std::make_shared<MobBreedingProvider>();
    Identifier         getUid() const override { return JadeIds::MC_MOB_BREEDING; }
    std::optional<int> streamData(EntityAccessor& accessor) override;
    void               encode(CompoundTag& out, int const& value) override { out["time"] = value; }
    std::optional<int> decode(CompoundTag const& in) override { return static_cast<int>(in.at("time")); }

    class Client;
};

class MobBreedingProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_MOB_BREEDING; }
    std::string getConfigIcon() const override { return "minecraft:wheat"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class AnimalOwnerProvider : public StreamServerDataProvider<EntityAccessor, std::string> {
public:
    static inline std::shared_ptr<AnimalOwnerProvider> const INSTANCE = std::make_shared<AnimalOwnerProvider>();
    Identifier                 getUid() const override { return JadeIds::MC_ANIMAL_OWNER; }
    bool                       shouldRequestData(EntityAccessor& accessor) override;
    std::optional<std::string> streamData(EntityAccessor& accessor) override;
    void                       encode(CompoundTag& out, std::string const& value) override { out["name"] = value; }
    std::optional<std::string> decode(CompoundTag const& in) override;

    class Client;
};

class AnimalOwnerProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_ANIMAL_OWNER; }
    std::string getConfigIcon() const override { return "minecraft:lead"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class VillagerProfessionProvider : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<VillagerProfessionProvider> const INSTANCE = std::make_shared<VillagerProfessionProvider>();
    Identifier  getUid() const override { return JadeIds::MC_VILLAGER_PROFESSION; }
    std::string getConfigIcon() const override { return "minecraft:emerald"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class ArmorStandProvider : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<ArmorStandProvider> const INSTANCE = std::make_shared<ArmorStandProvider>();
    Identifier  getUid() const override { return JadeIds::MC_ARMOR_STAND; }
    std::string getConfigIcon() const override { return "minecraft:armor_stand"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class FallingBlockProvider : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<FallingBlockProvider> const INSTANCE = std::make_shared<FallingBlockProvider>();
    Identifier     getUid() const override { return JadeIds::MC_FALLING_BLOCK; }
    bool           isRequired() const override { return true; }
    void           appendTooltip(ITooltip&, EntityAccessor&, IPluginConfig&) override {}
    ui::ElementPtr getIcon(EntityAccessor& accessor, IPluginConfig& config, ui::ElementPtr currentIcon) override;
    static Block const* blockOf(Actor& entity);
};

class PaintingProvider : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<PaintingProvider> const INSTANCE = std::make_shared<PaintingProvider>();
    Identifier  getUid() const override { return JadeIds::MC_PAINTING; }
    std::string getConfigIcon() const override { return "minecraft:painting"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class HorseStatsProvider : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<HorseStatsProvider> const INSTANCE = std::make_shared<HorseStatsProvider>();
    Identifier  getUid() const override { return JadeIds::MC_HORSE_STATS; }
    std::string getConfigIcon() const override { return "minecraft:saddle"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class NextEntityDropProvider : public IServerDataProvider<EntityAccessor> {
public:
    static inline std::shared_ptr<NextEntityDropProvider> const INSTANCE = std::make_shared<NextEntityDropProvider>();
    Identifier getUid() const override { return JadeIds::MC_NEXT_ENTITY_DROP; }
    bool       shouldRequestData(EntityAccessor& accessor) override;
    void       appendServerData(CompoundTag& tag, EntityAccessor& accessor) override;

    class Client;
};

class NextEntityDropProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_NEXT_ENTITY_DROP; }
    std::string getConfigIcon() const override { return "minecraft:egg"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class ZombieVillagerProvider : public StreamServerDataProvider<EntityAccessor, int> {
public:
    static inline std::shared_ptr<ZombieVillagerProvider> const INSTANCE = std::make_shared<ZombieVillagerProvider>();
    Identifier         getUid() const override { return JadeIds::MC_ZOMBIE_VILLAGER; }
    std::optional<int> streamData(EntityAccessor& accessor) override;
    void               encode(CompoundTag& out, int const& value) override { out["time"] = value; }
    std::optional<int> decode(CompoundTag const& in) override { return static_cast<int>(in.at("time")); }

    class Client;
};

class ZombieVillagerProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier  getUid() const override { return JadeIds::MC_ZOMBIE_VILLAGER; }
    std::string getConfigIcon() const override { return "minecraft:rotten_flesh"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class PetArmorProvider : public StreamServerDataProvider<EntityAccessor, view::ItemData> {
public:
    static inline std::shared_ptr<PetArmorProvider> const INSTANCE = std::make_shared<PetArmorProvider>();
    Identifier                    getUid() const override { return JadeIds::MC_PET_ARMOR; }
    bool                          shouldRequestData(EntityAccessor& accessor) override;
    std::optional<view::ItemData> streamData(EntityAccessor& accessor) override;
    void                          encode(CompoundTag& out, view::ItemData const& value) override { view::ViewCodec<view::ItemData>::encode(out, value); }
    std::optional<view::ItemData> decode(CompoundTag const& in) override { return view::ViewCodec<view::ItemData>::decode(in); }

    class Client;
};

class PetArmorProvider::Client : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<Client> const INSTANCE = std::make_shared<Client>();
    Identifier getUid() const override { return JadeIds::MC_PET_ARMOR; }
    bool       isRequired() const override { return true; }
    void       appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class ItemTooltipProvider : public IEntityComponentProvider {
public:
    static inline std::shared_ptr<ItemTooltipProvider> const INSTANCE = std::make_shared<ItemTooltipProvider>();
    Identifier  getUid() const override { return JadeIds::MC_ITEM_TOOLTIP; }
    std::string getConfigIcon() const override { return "minecraft:paper"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class VanillaPlugin : public IWailaPlugin {
public:
    void register_(impl::WailaCommonRegistration& registration) override;
    void registerClient(impl::WailaClientRegistration& registration) override;
};

}
