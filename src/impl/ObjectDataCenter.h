// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "api/Accessor.h"

namespace jade::api {
class IJadeProvider;
}

namespace jade::impl {

class ObjectDataCenter {
public:
    static inline int     rateLimiter     = 250;
    static inline int64_t timeLastUpdate  = 0;
    static inline bool    serverConnected = false;

    static void set(api::Accessor& accessor);
    static void requestServerData() { timeLastUpdate = 0; }
    static bool isTimeElapsed(int64_t time);
    static void resetTimer();
    static void disconnect();

    static void requestBlockData(api::BlockAccessor& accessor, std::vector<std::shared_ptr<api::IJadeProvider>> const& providers);
    static void requestEntityData(api::EntityAccessor& accessor, std::vector<std::shared_ptr<api::IJadeProvider>> const& providers);

private:
    static inline void const* sLastObject = nullptr;
};

}
