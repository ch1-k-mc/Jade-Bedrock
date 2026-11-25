// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
// The original file is from Modern Industrialization (MIT, Copyright (c) 2020 Azercoco & Technici4n). See licenses/Modern-Industrialization.txt.
#pragma once

#include <cstdint>
#include <string>

#include "platform/text/Component.h"

namespace jade::util {

class FluidTextHelper {
public:
    static constexpr int64_t BUCKET = 81000;

    static std::string     getUnicodeFraction(int64_t numerator, int64_t denominator, bool simplify);
    static text::Component getMillibuckets(int64_t integer, bool simplify);

private:
    static text::Component makeString(int64_t integer, int64_t numerator, int64_t denominator, std::string const& unit);
    static text::Component makeString(std::string const& integerString, int64_t numerator, int64_t denominator, std::string const& unit);
};

}
