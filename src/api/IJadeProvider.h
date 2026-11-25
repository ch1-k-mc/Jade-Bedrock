// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "api/Accessor.h"
#include "api/ITooltip.h"
#include "api/JadeIds.h"
#include "api/config/IWailaConfig.h"

enum class BlockActorType : uchar;
enum class ActorType : int;

namespace jade::api {

using config::IPluginConfig;

namespace TooltipPosition {
inline constexpr int HEAD = -10000;
inline constexpr int BODY = 0;
inline constexpr int TAIL = 10000;
}

class IJadeProvider {
public:
    virtual ~IJadeProvider()               = default;
    virtual Identifier getUid() const      = 0;
    virtual int        getDefaultPriority() const { return TooltipPosition::BODY; }
};

class IToggleableProvider : public virtual IJadeProvider {
public:
    virtual std::string getConfigIcon() const { return ""; }
    virtual bool        isRequired() const { return false; }
    virtual bool        enabledByDefault() const { return true; }
};

template <class T>
class IComponentProvider : public virtual IToggleableProvider {
public:
    virtual ui::ElementPtr getIcon(T& accessor, IPluginConfig& config, ui::ElementPtr currentIcon) { return nullptr; }
    virtual void           appendTooltip(ITooltip& tooltip, T& accessor, IPluginConfig& config) = 0;
};

using IBlockComponentProvider  = IComponentProvider<BlockAccessor>;
using IEntityComponentProvider = IComponentProvider<EntityAccessor>;

template <class T>
class IServerDataProvider : public virtual IJadeProvider {
public:
    virtual void appendServerData(CompoundTag& data, T& accessor) = 0;
    virtual bool shouldRequestData(T& accessor) { return true; }
};

struct BlockClass {
    std::string                              name;
    std::function<bool(Block const&)>        test;
    int                                      depth = 0;

    static BlockClass any();
    static BlockClass ofTypes(std::vector<std::string> typeNames);
    static BlockClass of(std::string name, std::function<bool(Block const&)> test, int depth = 1);
};

struct BlockActorClass {
    std::string                              name;
    std::function<bool(BlockActor const&)>   test;
    int                                      depth = 0;

    static BlockActorClass any();
    static BlockActorClass ofType(BlockActorType type, std::string name);
    static BlockActorClass ofTypes(std::vector<BlockActorType> types, std::string name);
};

struct EntityClass {
    std::string                              name;
    std::function<bool(Actor const&)>        test;
    int                                      depth = 0;

    static EntityClass any();
    static EntityClass ofIdentifiers(std::vector<std::string> ids, std::string name);
    static EntityClass of(std::string name, std::function<bool(Actor const&)> test, int depth = 1);
};

}
