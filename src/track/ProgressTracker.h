// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "api/JadeIds.h"

namespace jade::track {

class TrackInfo {
public:
    virtual ~TrackInfo()            = default;
    virtual void update(float pTicks) = 0;
    virtual void tick()             = 0;

    bool alive           = true;
    bool updatedThisTick = false;
    bool persistent      = false;
};

class ProgressTracker {
public:
    template <class T>
    std::shared_ptr<T> getOrCreate(Identifier const& tag, std::function<std::unique_ptr<T>()> const& supplier) {
        auto& infos = mMap[tag];
        std::shared_ptr<T> info;
        for (auto& o : infos) {
            if (!o->updatedThisTick) {
                if (auto t = std::dynamic_pointer_cast<T>(o)) {
                    info = std::move(t);
                    break;
                }
            }
        }
        if (!info) {
            info = std::shared_ptr<T>(supplier());
            infos.push_back(info);
        }
        info->updatedThisTick = true;
        return info;
    }

    template <class T>
    std::shared_ptr<T> get(Identifier const& tag) {
        auto it = mMap.find(tag);
        if (it == mMap.end()) return nullptr;
        for (auto& o : it->second)
            if (auto t = std::dynamic_pointer_cast<T>(o)) return t;
        return nullptr;
    }

    void tick() {
        for (auto it = mMap.begin(); it != mMap.end();) {
            auto& infos = it->second;
            std::erase_if(infos, [](std::shared_ptr<TrackInfo>& info) {
                if (info->updatedThisTick && info->alive) {
                    info->tick();
                    info->updatedThisTick = false;
                    return false;
                }
                return true;
            });
            it = infos.empty() ? mMap.erase(it) : std::next(it);
        }
    }

    void clear() {
        for (auto it = mMap.begin(); it != mMap.end();) {
            std::erase_if(it->second, [](std::shared_ptr<TrackInfo>& info) { return !info->persistent; });
            it = it->second.empty() ? mMap.erase(it) : std::next(it);
        }
    }

private:
    std::map<Identifier, std::vector<std::shared_ptr<TrackInfo>>> mMap;
};

}
