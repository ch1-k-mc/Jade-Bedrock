// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "api/config/IWailaConfig.h"

namespace jade::impl::config {

namespace entry {
class ConfigEntry;
}

using api::config::IWailaConfig;

class ClientPluginConfig : public api::config::IPluginConfig {
public:
    bool                  get(Identifier const& key) override;
    int                   getInt(Identifier const& key) override;
    float                 getFloat(Identifier const& key) override;
    std::string           getString(Identifier const& key) override;
    bool                  set(Identifier const& key, nlohmann::json value) override;
    nlohmann::json const& values() const override { return mValues; }

    void ensureEntry(entry::ConfigEntry const& entry);
    void fromJson(nlohmann::json const& j);

private:
    nlohmann::json const& merged(Identifier const& key);

    nlohmann::json mValues       = nlohmann::json::object();
    nlohmann::json mMergedValues = nlohmann::json::object();
};

class WailaConfig : public IWailaConfig {
public:
    General&       general() override { return mGeneral; }
    Overlay&       overlay() override { return mOverlay; }
    Formatting&    formatting() override { return mFormatting; }
    Accessibility& accessibility() override { return mAccessibility; }
    ClientPluginConfig& plugin() override { return mPlugin; }
    void           save() override;
    void           invalidate() override;
    std::string    getName() override { return mName; }
    void           setName(std::string name) override { mName = name.substr(0, 100); }

    void fixData();

    virtual void           fromJson(nlohmann::json const& j);
    virtual nlohmann::json toJson() const;

protected:
    std::string        mName;
    General            mGeneral;
    Overlay            mOverlay;
    Formatting         mFormatting;
    Accessibility      mAccessibility;
    ClientPluginConfig mPlugin;
};

struct History {
    bool             previewOverlay         = true;
    bool             hintOverlayToggle      = true;
    bool             hintNarratorToggle     = true;
    bool             accessibilityModMemory = false;
    int              themesHash             = 0;
    std::vector<int> usersHash;

    void           checkNewUser(int hash);
    void           fromJson(nlohmann::json const& j);
    nlohmann::json toJson() const;
};

class RootConfig : public WailaConfig {
public:
    bool    isEnableProfiles() const { return enableProfiles; }
    void    setEnableProfiles(bool v) { enableProfiles = v; }
    bool    enableProfiles = false;
    int     profileIndex   = 0;
    History history;

    void           fromJson(nlohmann::json const& j) override;
    nlohmann::json toJson() const override;
};

class ConfigStore {
public:
    static ConfigStore& get();

    void load(std::filesystem::path const& configDir);

    WailaConfig& config();
    RootConfig&  rootConfig() { return *mRoot; }
    History&     history() { return mRoot->history; }
    void         saveConfig();
    void         invalidateConfig();
    void         resetConfig();
    void         useProfile(int index);
    void         saveProfile(int index);
    void         renameProfile(int index, std::string const& name);
    WailaConfig& profile(int index);

    [[nodiscard]] std::filesystem::path fileOf(int index) const;

    nlohmann::json getValue(std::string const& key);
    bool           setValue(std::string const& key, nlohmann::json const& value, std::string& error);

private:
    WailaConfig& holder();
    void         saveIndex(int index);
    void         loadIndex(int index);

    std::filesystem::path                     mDir;
    std::unique_ptr<RootConfig>               mRoot = std::make_unique<RootConfig>();
    std::vector<std::unique_ptr<WailaConfig>> mProfiles;
};

}
