// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/WailaClientRegistration.h"

#include <algorithm>
#include <stdexcept>

#include "mc/world/actor/Actor.h"
#include "mc/world/level/block/Block.h"

#include "impl/WailaCommonRegistration.h"
#include "impl/config/WailaConfig.h"

namespace jade::impl {

namespace {
std::unique_ptr<WailaClientRegistration> gInstance;

bool isAccess(Identifier const& id) { return id.starts_with("jade_access:"); }

template <class List, class Filter>
List filtered(List const& in, Filter f) {
    List out;
    for (auto const& p : in)
        if (f(*p)) out.push_back(p);
    return out;
}
}

WailaClientRegistration& WailaClientRegistration::instance() {
    if (!gInstance) gInstance = std::make_unique<WailaClientRegistration>();
    return *gInstance;
}

void WailaClientRegistration::reset() { gInstance = std::make_unique<WailaClientRegistration>(); }

void WailaClientRegistration::registerBlockIcon(std::shared_ptr<api::IBlockComponentProvider> provider, api::BlockClass blockClass) {
    WailaCommonRegistration::instance().priorities.put(*provider);
    tryAddConfig(*provider);
    blockIconProviders.register_(std::move(blockClass), std::move(provider));
}

void WailaClientRegistration::registerBlockComponent(std::shared_ptr<api::IBlockComponentProvider> provider, api::BlockClass blockClass) {
    WailaCommonRegistration::instance().priorities.put(*provider);
    tryAddConfig(*provider);
    blockComponentProviders.register_(std::move(blockClass), std::move(provider));
}

void WailaClientRegistration::registerEntityIcon(std::shared_ptr<api::IEntityComponentProvider> provider, api::EntityClass entityClass) {
    WailaCommonRegistration::instance().priorities.put(*provider);
    tryAddConfig(*provider);
    entityIconProviders.register_(std::move(entityClass), std::move(provider));
}

void WailaClientRegistration::registerEntityComponent(std::shared_ptr<api::IEntityComponentProvider> provider, api::EntityClass entityClass) {
    WailaCommonRegistration::instance().priorities.put(*provider);
    tryAddConfig(*provider);
    entityComponentProviders.register_(std::move(entityClass), std::move(provider));
}

std::vector<std::shared_ptr<api::IBlockComponentProvider>> WailaClientRegistration::getBlockProviders(Block const& block) {
    return filtered(blockComponentProviders.get(block, block.getTypeName()), api::AccessorClientHandler::isEnabled);
}

std::vector<std::shared_ptr<api::IBlockComponentProvider>> WailaClientRegistration::getBlockIconProviders(Block const& block) {
    return filtered(blockIconProviders.get(block, block.getTypeName()), api::AccessorClientHandler::isEnabled);
}

std::vector<std::shared_ptr<api::IEntityComponentProvider>> WailaClientRegistration::getEntityProviders(Actor const& entity) {
    return filtered(entityComponentProviders.get(entity, entity.getTypeName()), api::AccessorClientHandler::isEnabled);
}

std::vector<std::shared_ptr<api::IEntityComponentProvider>> WailaClientRegistration::getEntityIconProviders(Actor const& entity) {
    return filtered(entityIconProviders.get(entity, entity.getTypeName()), api::AccessorClientHandler::isEnabled);
}

void WailaClientRegistration::addConfig(std::unique_ptr<config::entry::ConfigEntry> entry) {
    if (!entry) throw std::invalid_argument("entry is null");
    auto const& id   = entry->id();
    auto        path = id.substr(id.find(':') + 1);
    if (std::count(path.begin(), path.end(), '.') > 1) throw std::invalid_argument("too many '.' in config key: " + id);
    if (hasConfig(id)) throw std::invalid_argument("Duplicate config key: " + id);
    if (!entry->isValidValue(entry->defaultValue())) throw std::invalid_argument("Default value of config does not pass value check: " + id);
    configEntries.emplace(id, std::move(entry));
}

void WailaClientRegistration::addConfig(Identifier const& key, std::string icon, bool defaultValue) {
    auto e = std::make_unique<config::entry::BooleanConfigEntry>(key, defaultValue);
    e->setIcon(std::move(icon));
    addConfig(std::move(e));
}

void WailaClientRegistration::addConfig(Identifier const& key, std::string icon, std::string defaultValue, std::vector<std::string> enumNames) {
    auto e = std::make_unique<config::entry::EnumConfigEntry>(key, std::move(defaultValue), std::move(enumNames));
    e->setIcon(std::move(icon));
    addConfig(std::move(e));
}

void WailaClientRegistration::addConfig(Identifier const& key, std::string icon, std::string defaultValue, std::function<bool(std::string const&)> validator) {
    auto e = std::make_unique<config::entry::StringConfigEntry>(key, std::move(defaultValue), std::move(validator));
    e->setIcon(std::move(icon));
    addConfig(std::move(e));
}

void WailaClientRegistration::addConfig(Identifier const& key, std::string icon, int defaultValue, int min, int max, bool slider) {
    auto e = std::make_unique<config::entry::IntConfigEntry>(key, defaultValue, min, max, slider);
    e->setIcon(std::move(icon));
    addConfig(std::move(e));
}

void WailaClientRegistration::addConfig(Identifier const& key, std::string icon, float defaultValue, float min, float max, bool slider) {
    auto e = std::make_unique<config::entry::FloatConfigEntry>(key, defaultValue, min, max, slider);
    e->setIcon(std::move(icon));
    addConfig(std::move(e));
}

void WailaClientRegistration::addConfigListener(Identifier const& key, std::function<void(Identifier const&)> listener) {
    auto* e = getConfigEntry(key);
    if (!e) throw std::invalid_argument("Unknown config key: " + key);
    e->addListener(std::move(listener));
}

void WailaClientRegistration::setConfigCategoryOverride(Identifier const& key, text::Component override) {
    if (isAccess(key)) throw std::invalid_argument("Cannot override option from access category");
    if (!api::config::IPluginConfig::isPrimaryKey(key)) throw std::invalid_argument("Only primary config key can be overridden");
    if (!hasConfig(key)) throw std::invalid_argument("Unknown config key: " + key);
    configCategoryOverrides[key].push_back(std::move(override));
}

void WailaClientRegistration::setConfigCategoryOverride(Identifier const& key, std::vector<text::Component> const& overrides) {
    for (auto const& o : overrides) setConfigCategoryOverride(key, o);
}

void WailaClientRegistration::setConfigIcon(Identifier const& key, std::string icon) {
    auto* e = getConfigEntry(key);
    if (!e) throw std::invalid_argument("Unknown config key: " + key);
    e->setIcon(std::move(icon));
}

void WailaClientRegistration::tryAddConfig(api::IToggleableProvider const& provider) {
    if (!provider.isRequired() && !hasConfig(provider.getUid())) addConfig(provider.getUid(), provider.getConfigIcon(), provider.enabledByDefault());
}

std::set<Identifier> WailaClientRegistration::getConfigKeys() const {
    std::set<Identifier> out;
    for (auto const& [k, _] : configEntries) out.insert(k);
    return out;
}

std::set<Identifier> WailaClientRegistration::getConfigKeys(std::string const& ns) const {
    std::set<Identifier> out;
    for (auto const& [k, _] : configEntries)
        if (k.substr(0, k.find(':')) == ns) out.insert(k);
    return out;
}

config::entry::ConfigEntry* WailaClientRegistration::getConfigEntry(Identifier const& key) {
    auto it = configEntries.find(key);
    return it == configEntries.end() ? nullptr : it->second.get();
}

void WailaClientRegistration::registerAccessorHandler(api::AccessorType type, std::unique_ptr<api::AccessorClientHandler> handler) {
    accessorHandlers[type] = std::move(handler);
}

api::AccessorClientHandler& WailaClientRegistration::getAccessorHandler(api::AccessorType type) {
    auto it = accessorHandlers.find(type);
    if (it == accessorHandlers.end()) throw std::logic_error("No accessor handler");
    return *it->second;
}

void WailaClientRegistration::setServerConfig(nlohmann::json const& cfg) {
    for (auto& [_, entry] : configEntries) entry->setSyncedValue(std::nullopt);
    if (cfg.is_object()) {
        for (auto const& [key, value] : cfg.items()) {
            auto* entry = getConfigEntry(key);
            if (!entry) continue;
            try {
                auto v = entry->convertValue(value);
                if (entry->isValidValue(v)) entry->setSyncedValue(v);
            } catch (...) {}
        }
    }
    config::ConfigStore::get().config().fixData();
}

void WailaClientRegistration::loadComplete() {
    auto& priorities = WailaCommonRegistration::instance().priorities;
    blockComponentProviders.loadComplete(priorities);
    blockIconProviders.loadComplete(priorities);
    entityComponentProviders.loadComplete(priorities);
    entityIconProviders.loadComplete(priorities);
}

}
