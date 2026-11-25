// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>

#include "api/IJadeProvider.h"
#include "api/StreamServerDataProvider.h"
#include "api/IWailaPlugin.h"

namespace jade::addon::core {

using namespace jade::api;

enum class ModNameMode { ON, OFF, SMALLER };

}

namespace jade::api::config {
JADE_ENUM_NAMES(::jade::addon::core::ModNameMode, "ON", "OFF", "SMALLER")
}

namespace jade::addon::core {

class ObjectNameProvider : public virtual IToggleableProvider {
public:
    Identifier  getUid() const override { return JadeIds::CORE_OBJECT_NAME; }
    std::string getConfigIcon() const override { return "minecraft:name_tag"; }
    int         getDefaultPriority() const override { return TooltipPosition::HEAD - 100; }

    static text::Component getEntityName(Actor& entity, bool accessibilityDetails);

    class ForBlock;
    class ForEntity;
    class BlockData;
};

class ObjectNameProvider::BlockData : public ObjectNameProvider, public StreamServerDataProvider<BlockAccessor, std::string> {
public:
    static std::shared_ptr<BlockData> const INSTANCE;
    std::optional<std::string> streamData(BlockAccessor& accessor) override;
    void                       encode(CompoundTag& out, std::string const& value) override { out["name"] = value; }
    std::optional<std::string> decode(CompoundTag const& in) override;
    bool                       shouldRequestData(BlockAccessor& accessor) override;
};

class ObjectNameProvider::ForBlock : public ObjectNameProvider, public IBlockComponentProvider {
public:
    static std::shared_ptr<ForBlock> const INSTANCE;
    void appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class ObjectNameProvider::ForEntity : public ObjectNameProvider, public IEntityComponentProvider {
public:
    static std::shared_ptr<ForEntity> const INSTANCE;
    static void addName(ITooltip& tooltip, text::Component name);
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class ModNameProvider : public virtual IToggleableProvider {
public:
    Identifier getUid() const override { return JadeIds::CORE_MOD_NAME; }
    bool       isRequired() const override { return true; }
    int        getDefaultPriority() const override { return TooltipPosition::TAIL - 1; }

    class ForBlock;
    class ForEntity;
};

class ModNameProvider::ForBlock : public ModNameProvider, public IBlockComponentProvider {
public:
    static std::shared_ptr<ForBlock> const INSTANCE;
    void appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class ModNameProvider::ForEntity : public ModNameProvider, public IEntityComponentProvider {
public:
    static std::shared_ptr<ForEntity> const INSTANCE;
    void appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class DistanceProvider : public virtual IToggleableProvider {
public:
    Identifier getUid() const override { return JadeIds::CORE_DISTANCE; }
    bool       isRequired() const override { return true; }
    int        getDefaultPriority() const override { return -4600; }

    static std::string                     distance(Accessor& accessor);
    static std::shared_ptr<ui::TextElement> xyz(int x, int y, int z);
    static text::Component                 display(int i, int colorIndex);
    void append(ITooltip& tooltip, Accessor& accessor, BlockPos const& pos, IPluginConfig& config) const;

    class ForBlock;
    class ForEntity;
};

class DistanceProvider::ForBlock : public DistanceProvider, public IBlockComponentProvider {
public:
    static std::shared_ptr<ForBlock> const INSTANCE;
    void appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class DistanceProvider::ForEntity : public DistanceProvider, public IEntityComponentProvider {
public:
    static std::shared_ptr<ForEntity> const INSTANCE;
    void appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class BlockFaceProvider : public IBlockComponentProvider {
public:
    static std::shared_ptr<BlockFaceProvider> const INSTANCE;
    Identifier  getUid() const override { return JadeIds::CORE_BLOCK_FACE; }
    std::string getConfigIcon() const override { return "minecraft:compass"; }
    int         getDefaultPriority() const override { return TooltipPosition::HEAD - 100 + 30; }
    bool        enabledByDefault() const override { return false; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;

    static text::Component directionName(int direction);
};

class CorePlugin : public IWailaPlugin {
public:
    void register_(impl::WailaCommonRegistration& registration) override;
    void registerClient(impl::WailaClientRegistration& registration) override;
};

}
