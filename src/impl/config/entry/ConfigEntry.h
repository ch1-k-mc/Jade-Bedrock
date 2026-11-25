// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "api/JadeIds.h"

namespace jade::impl::config::entry {

class ConfigEntry {
public:
    ConfigEntry(Identifier id, nlohmann::json defaultValue) : mId(std::move(id)), mDefault(std::move(defaultValue)) {}
    virtual ~ConfigEntry() = default;

    [[nodiscard]] Identifier const&     id() const { return mId; }
    [[nodiscard]] nlohmann::json const& defaultValue() const { return mDefault; }
    [[nodiscard]] nlohmann::json const& syncedValue() const { return *mSynced; }
    [[nodiscard]] bool                  isSynced() const { return mSynced.has_value(); }
    void                                setSyncedValue(std::optional<nlohmann::json> v) { mSynced = std::move(v); }

    void                             setIcon(std::string itemName) { mIcon = std::move(itemName); }
    [[nodiscard]] std::string const& icon() const { return mIcon; }

    virtual nlohmann::json convertValue(nlohmann::json const& value) const { return value; }
    virtual bool           isValidValue(nlohmann::json const& value) const = 0;
    [[nodiscard]] virtual char const* kind() const = 0;

    void addListener(std::function<void(Identifier const&)> listener) { mListeners.push_back(std::move(listener)); }
    void notifyChange() const {
        for (auto const& l : mListeners) l(mId);
    }

protected:
    Identifier                                        mId;
    nlohmann::json                                    mDefault;
    std::optional<nlohmann::json>                     mSynced;
    std::string                                       mIcon;
    std::vector<std::function<void(Identifier const&)>> mListeners;
};

class BooleanConfigEntry : public ConfigEntry {
public:
    BooleanConfigEntry(Identifier id, bool def) : ConfigEntry(std::move(id), def) {}
    bool        isValidValue(nlohmann::json const& v) const override { return v.is_boolean(); }
    char const* kind() const override { return "boolean"; }
};

class IntConfigEntry : public ConfigEntry {
public:
    IntConfigEntry(Identifier id, int def, int min, int max, bool slider) : ConfigEntry(std::move(id), def), mMin(min), mMax(max), mSlider(slider) {}
    nlohmann::json convertValue(nlohmann::json const& v) const override {
        if (!v.is_number()) throw std::invalid_argument("not a number");
        return static_cast<int>(v.get<double>());
    }
    bool        isValidValue(nlohmann::json const& v) const override { return v.is_number() && v.get<int>() >= mMin && v.get<int>() <= mMax; }
    char const* kind() const override { return "int"; }
    int         min() const { return mMin; }
    int         max() const { return mMax; }
    bool        slider() const { return mSlider; }

private:
    int  mMin, mMax;
    bool mSlider;
};

class FloatConfigEntry : public ConfigEntry {
public:
    FloatConfigEntry(Identifier id, float def, float min, float max, bool slider)
    : ConfigEntry(std::move(id), def),
      mMin(min),
      mMax(max),
      mSlider(slider) {}
    nlohmann::json convertValue(nlohmann::json const& v) const override {
        if (!v.is_number()) throw std::invalid_argument("not a number");
        return v.get<float>();
    }
    bool        isValidValue(nlohmann::json const& v) const override { return v.is_number() && v.get<float>() >= mMin && v.get<float>() <= mMax; }
    char const* kind() const override { return "float"; }
    float       min() const { return mMin; }
    float       max() const { return mMax; }

private:
    float mMin, mMax;
    bool  mSlider;
};

class EnumConfigEntry : public ConfigEntry {
public:
    EnumConfigEntry(Identifier id, std::string def, std::vector<std::string> names) : ConfigEntry(std::move(id), def), mNames(std::move(names)) {}
    nlohmann::json convertValue(nlohmann::json const& v) const override {
        if (v.is_boolean()) return v.get<bool>() ? "TRUE" : "FALSE";
        return v;
    }
    bool isValidValue(nlohmann::json const& v) const override {
        return v.is_string() && std::find(mNames.begin(), mNames.end(), v.get<std::string>()) != mNames.end();
    }
    char const*                     kind() const override { return "enum"; }
    std::vector<std::string> const& names() const { return mNames; }

private:
    std::vector<std::string> mNames;
};

class StringConfigEntry : public ConfigEntry {
public:
    StringConfigEntry(Identifier id, std::string def, std::function<bool(std::string const&)> validator)
    : ConfigEntry(std::move(id), def),
      mValidator(std::move(validator)) {}
    bool isValidValue(nlohmann::json const& v) const override {
        return v.is_string() && (!mValidator || mValidator(v.get<std::string>()));
    }
    char const* kind() const override { return "string"; }

private:
    std::function<bool(std::string const&)> mValidator;
};

}
