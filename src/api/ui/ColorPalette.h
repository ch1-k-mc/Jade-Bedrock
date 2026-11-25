// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <cstdint>

#include "nlohmann/json.hpp"

#include "api/ui/Element.h"

namespace jade::api::ui {

struct ColorPalette {
    uint32_t normal  = 0xFFA0A0A0;
    uint32_t info    = 0xFFFFFFFF;
    uint32_t title   = 0xFFFFFFFF;
    uint32_t success = 0xFF55FF55;
    uint32_t warning = 0xFFFFC107;
    uint32_t danger  = 0xFFFF5555;
    uint32_t failure = 0xFFAA0000;

    static ColorPalette const DEFAULT;

    [[nodiscard]] uint32_t get(MessageType type) const {
        switch (type) {
        case MessageType::INFO:
            return info;
        case MessageType::TITLE:
            return title;
        case MessageType::SUCCESS:
            return success;
        case MessageType::WARNING:
            return warning;
        case MessageType::DANGER:
            return danger;
        case MessageType::FAILURE:
            return failure;
        default:
            return normal;
        }
    }

    static ColorPalette fromJson(nlohmann::json const& j);
};

inline ColorPalette const ColorPalette::DEFAULT{};

}
