// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <vector>

#include "api/JadeIds.h"

namespace jade::api {
class IJadeProvider;
}

namespace jade::impl {

class PriorityStore {
public:
    using SortingFunction = std::function<std::vector<Identifier>(PriorityStore&, std::set<Identifier> const&)>;

    void setSortingFunction(SortingFunction f) { mSorting = std::move(f); }
    void configurable(std::filesystem::path configFile) { mConfigFile = std::move(configFile); }

    void put(api::IJadeProvider const& provider);
    void put(api::IJadeProvider const& provider, int priority);
    void putUnsafe(Identifier const& key, int priority) { mPriorities[key] = priority; }

    void sort(std::set<Identifier> const& extraKeys);

    [[nodiscard]] int                            byValue(api::IJadeProvider const& provider) const;
    [[nodiscard]] int                            byKey(Identifier const& id) const;
    [[nodiscard]] std::vector<Identifier> const& getSortedList() const { return mSortedList; }

private:
    std::map<Identifier, int> mPriorities;
    std::filesystem::path     mConfigFile;
    std::vector<Identifier>   mSortedList;
    SortingFunction           mSorting;
};

}
