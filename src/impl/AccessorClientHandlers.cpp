// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "api/AccessorClientHandler.h"

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/Block.h"

#include "api/config/IWailaConfig.h"
#include "impl/ObjectDataCenter.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/item/ScopedItemRegistry.h"
#include "util/WailaExceptionHandler.h"

namespace jade::api {

bool AccessorClientHandler::isEnabled(IToggleableProvider const& provider) {
    auto& cfg = config::IWailaConfig::get();
    if (!cfg.accessibility().getEnableAccessibilityPlugin() && provider.getUid().starts_with("jade_access:")) return false;
    if (provider.isRequired()) return true;
    return cfg.plugin().get(provider.getUid());
}

}

namespace jade::impl {

using api::Accessor;
using api::IJadeProvider;

namespace {

std::vector<std::shared_ptr<IJadeProvider>> toBase(auto const& list) {
    std::vector<std::shared_ptr<IJadeProvider>> out;
    out.reserve(list.size());
    for (auto const& p : list) out.push_back(p);
    return out;
}

api::ui::ElementPtr elementFromLiquid(Block const& block) {
    std::string const& type = block.getTypeName();
    char const*        item = nullptr;
    if (type == "minecraft:water" || type == "minecraft:flowing_water") item = "minecraft:water_bucket";
    else if (type == "minecraft:lava" || type == "minecraft:flowing_lava") item = "minecraft:lava_bucket";
    if (!item) return nullptr;
    platform::item::ScopedItemRegistry scoped;
    ItemStack                          stack;
    stack.reinit(item, 1, 0);
    if (stack.isNull()) return nullptr;
    return ui::JadeUIInternal::item(stack);
}

}

bool BlockAccessorClientHandler::shouldDisplay(Accessor&) { return api::config::IWailaConfig::get().general().getDisplayBlocks(); }

std::vector<std::shared_ptr<IJadeProvider>> BlockAccessorClientHandler::shouldRequestData(Accessor& a) {
    auto& accessor  = static_cast<api::BlockAccessor&>(a);
    auto& common    = WailaCommonRegistration::instance();
    auto  providers = common.blockDataProvidersOf(accessor.getBlock(), accessor.getBlockEntity(), !accessor.isServersideContent());
    std::vector<std::shared_ptr<IJadeProvider>> out;
    for (auto const& p : providers) {
        if (!common.blockDataProviders.isMapped(p)) continue;
        try {
            if (p->shouldRequestData(accessor)) out.push_back(p);
        } catch (std::exception const& e) {
            util::WailaExceptionHandler::handleErr(e, p.get());
        }
    }
    return out;
}

void BlockAccessorClientHandler::requestData(Accessor& accessor, std::vector<std::shared_ptr<IJadeProvider>> const& providers) {
    ObjectDataCenter::requestBlockData(static_cast<api::BlockAccessor&>(accessor), providers);
}

api::ui::ElementPtr BlockAccessorClientHandler::getIcon(Accessor& a) {
    auto&        accessor = static_cast<api::BlockAccessor&>(a);
    Block const& block    = accessor.getBlock();
    if (block.isAir()) return nullptr;
    api::ui::ElementPtr icon;
    ItemStack const&    pick = accessor.getPickedResult();
    if (!pick.isNull()) icon = ui::JadeUIInternal::item(pick);
    if (!icon) icon = elementFromLiquid(block);
    for (auto const& provider : WailaClientRegistration::instance().getBlockIconProviders(block)) {
        try {
            auto element = provider->getIcon(accessor, api::config::IWailaConfig::get().plugin(), icon);
            if (element) icon = element;
        } catch (std::exception const& e) {
            util::WailaExceptionHandler::handleErr(e, provider.get());
        }
    }
    return icon;
}

void BlockAccessorClientHandler::gatherComponents(Accessor& a, std::function<api::ITooltip&(IJadeProvider const&)> const& tooltipProvider) {
    auto& accessor = static_cast<api::BlockAccessor&>(a);
    for (auto const& provider : WailaClientRegistration::instance().getBlockProviders(accessor.getBlock())) {
        auto& tooltip = tooltipProvider(*provider);
        try {
            ui::JadeUIInternal::setContextUid(provider->getUid());
            provider->appendTooltip(tooltip, accessor, api::config::IWailaConfig::get().plugin());
        } catch (std::exception const& e) {
            util::WailaExceptionHandler::handleErr(e, provider.get(), &tooltip);
        }
        ui::JadeUIInternal::setContextUid(std::nullopt);
    }
}

bool EntityAccessorClientHandler::shouldDisplay(Accessor& a) {
    auto& accessor = static_cast<api::EntityAccessor&>(a);
    auto& general  = api::config::IWailaConfig::get().general();
    if (!general.getDisplayEntities()) return false;
    Actor* entity = accessor.getEntity();
    if (!entity) return false;
    if (!general.getDisplayBosses()) {
        auto const& type = entity->getTypeName();
        if (type == "minecraft:ender_dragon" || type == "minecraft:wither") return false;
    }
    if (entity->hasCategory(ActorCategory::Item)) {
        platform::item::ScopedItemRegistry scoped;
        if (static_cast<ItemActor*>(entity)->mItem->isNull()) return false;
    }
    return true;
}

std::vector<std::shared_ptr<IJadeProvider>> EntityAccessorClientHandler::shouldRequestData(Accessor& a) {
    auto&  accessor = static_cast<api::EntityAccessor&>(a);
    Actor* entity   = accessor.getEntity();
    if (!entity) return {};
    auto&                                       common = WailaCommonRegistration::instance();
    std::vector<std::shared_ptr<IJadeProvider>> out;
    for (auto const& p : common.entityDataProvidersOf(*entity)) {
        if (!common.entityDataProviders.isMapped(p)) continue;
        try {
            if (p->shouldRequestData(accessor)) out.push_back(p);
        } catch (std::exception const& e) {
            util::WailaExceptionHandler::handleErr(e, p.get());
        }
    }
    return out;
}

void EntityAccessorClientHandler::requestData(Accessor& accessor, std::vector<std::shared_ptr<IJadeProvider>> const& providers) {
    ObjectDataCenter::requestEntityData(static_cast<api::EntityAccessor&>(accessor), providers);
}

api::ui::ElementPtr EntityAccessorClientHandler::getIcon(Accessor& a) {
    auto&  accessor = static_cast<api::EntityAccessor&>(a);
    Actor* entity   = accessor.getEntity();
    if (!entity) return nullptr;
    api::ui::ElementPtr icon;
    ItemStack const&    stack = accessor.getPickedResult();
    if (!stack.isNull()) icon = ui::JadeUIInternal::item(stack);
    for (auto const& provider : WailaClientRegistration::instance().getEntityIconProviders(*entity)) {
        try {
            auto element = provider->getIcon(accessor, api::config::IWailaConfig::get().plugin(), icon);
            if (element) icon = element;
        } catch (std::exception const& e) {
            util::WailaExceptionHandler::handleErr(e, provider.get());
        }
    }
    return icon;
}

void EntityAccessorClientHandler::gatherComponents(Accessor& a, std::function<api::ITooltip&(IJadeProvider const&)> const& tooltipProvider) {
    auto&  accessor = static_cast<api::EntityAccessor&>(a);
    Actor* entity   = accessor.getEntity();
    if (!entity) return;
    for (auto const& provider : WailaClientRegistration::instance().getEntityProviders(*entity)) {
        auto& tooltip = tooltipProvider(*provider);
        try {
            ui::JadeUIInternal::setContextUid(provider->getUid());
            provider->appendTooltip(tooltip, accessor, api::config::IWailaConfig::get().plugin());
        } catch (std::exception const& e) {
            util::WailaExceptionHandler::handleErr(e, provider.get(), &tooltip);
        }
        ui::JadeUIInternal::setContextUid(std::nullopt);
    }
}

}
