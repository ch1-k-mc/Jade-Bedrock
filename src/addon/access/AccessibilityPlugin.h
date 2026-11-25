// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <string>

#include "api/IJadeProvider.h"
#include "api/ITooltip.h"
#include "api/IWailaPlugin.h"
#include "api/JadeIds.h"
#include "api/theme/IThemeHelper.h"
#include "platform/text/I18n.h"

namespace jade::addon::access {

using namespace jade::api;

class AccessibilityPlugin : public IWailaPlugin {
public:
    void registerClient(impl::WailaClientRegistration& registration) override;

    static void replaceTitle(ITooltip& tooltip, std::string const& , std::string const& key) {
        std::string message = tooltip.getString(JadeIds::CORE_OBJECT_NAME);
        std::string full    = "jade.access." + key;
        if (message.find_first_not_of(" \t") == std::string::npos || !text::I18n::has(full)) return;
        auto title = theme::IThemeHelper::get().title(text::I18n::formatString(full, {message, ""}));
        tooltip.replace(JadeIds::CORE_OBJECT_NAME, title);
    }
};

class SignProvider : public IBlockComponentProvider {
public:
    Identifier  getUid() const override { return JadeIds::ACCESS_SIGN; }
    std::string getConfigIcon() const override { return "minecraft:oak_sign"; }
    void        appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class HeldItemProvider : public IEntityComponentProvider {
public:
    Identifier  getUid() const override { return JadeIds::ACCESS_HELD_ITEM; }
    std::string getConfigIcon() const override { return "minecraft:diamond_sword"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class BlockDetailsProvider : public IBlockComponentProvider {
public:
    Identifier getUid() const override { return JadeIds::ACCESS_BLOCK_DETAILS; }
    void       appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class BlockAmountProvider : public IBlockComponentProvider {
public:
    Identifier getUid() const override { return JadeIds::ACCESS_BLOCK_AMOUNT; }
    bool       isRequired() const override { return true; }
    void       appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class BlockDetailsBodyProvider : public IBlockComponentProvider {
public:
    Identifier getUid() const override { return JadeIds::ACCESS_BLOCK_DETAILS_BODY; }
    bool       isRequired() const override { return true; }
    void       appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig& config) override;
};

class EntityDetailsBodyProvider : public IEntityComponentProvider {
public:
    Identifier getUid() const override { return JadeIds::ACCESS_ENTITY_DETAILS_BODY; }
    bool       isRequired() const override { return true; }
    int        getDefaultPriority() const override { return 3333; }
    void       appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class EntityVariantProvider : public IEntityComponentProvider {
public:
    Identifier  getUid() const override { return JadeIds::ACCESS_ENTITY_VARIANT; }
    std::string getConfigIcon() const override { return "minecraft:blue_egg"; }
    void        appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

class EntityDetailsProvider : public IEntityComponentProvider {
public:
    Identifier getUid() const override { return JadeIds::ACCESS_ENTITY_DETAILS; }
    void       appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig& config) override;
};

}
