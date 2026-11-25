// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/PriorityStore.h"

#include <algorithm>
#include <fstream>

#include "nlohmann/json.hpp"

#include "api/IJadeProvider.h"

namespace jade::impl {

void PriorityStore::put(api::IJadeProvider const& provider) { put(provider, provider.getDefaultPriority()); }

void PriorityStore::put(api::IJadeProvider const& provider, int priority) { mPriorities[provider.getUid()] = priority; }

int PriorityStore::byValue(api::IJadeProvider const& provider) const { return byKey(provider.getUid()); }

int PriorityStore::byKey(Identifier const& id) const {
    auto it = mPriorities.find(id);
    return it == mPriorities.end() ? 0 : it->second;
}

void PriorityStore::sort(std::set<Identifier> const& extraKeys) {
    std::set<Identifier> allKeys;
    for (auto const& [k, _] : mPriorities) allKeys.insert(k);
    allKeys.insert(extraKeys.begin(), extraKeys.end());
    if (!mConfigFile.empty()) {
        nlohmann::json map = nlohmann::json::object();
        {
            std::ifstream in(mConfigFile);
            if (in) {
                auto j = nlohmann::json::parse(in, nullptr, false);
                if (j.is_object()) map = j;
            }
        }
        for (auto const& [k, v] : map.items())
            if (v.is_number_integer()) mPriorities[k] = v.get<int>();
        bool changed = false;
        for (auto const& [id, _] : mPriorities) {
            if (!map.contains(id)) {
                map[id] = nullptr;
                changed = true;
            }
        }
        if (changed) {
            std::error_code ec;
            std::filesystem::create_directories(mConfigFile.parent_path(), ec);
            std::ofstream out(mConfigFile, std::ios::binary | std::ios::trunc);
            if (out) out << map.dump(2);
        }
    }
    if (mSorting) {
        mSortedList = mSorting(*this, allKeys);
    } else {
        mSortedList.assign(allKeys.begin(), allKeys.end());
        std::stable_sort(mSortedList.begin(), mSortedList.end(), [this](auto const& a, auto const& b) { return byKey(a) < byKey(b); });
    }
}

}
