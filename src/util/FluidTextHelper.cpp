// SPDX-License-Identifier: CC-BY-NC-SA-4.0 AND MIT
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "util/FluidTextHelper.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "overlay/DisplayHelper.h"

namespace jade::util {

namespace {

char const* const SUPERSCRIPT[]{"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};
char const* const SUBSCRIPT[]{"₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉"};
char const* const FRACTION_BAR = "⁄";

std::string formatNumber(int64_t n) {
    std::string s   = std::to_string(n < 0 ? -n : n);
    std::string out;
    int         cnt = 0;
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        if (cnt > 0 && cnt % 3 == 0) out.insert(out.begin(), ',');
        out.insert(out.begin(), *it);
        ++cnt;
    }
    return n < 0 ? "-" + out : out;
}

std::string digits(int64_t n, char const* const* table) {
    if (n == 0) return "";
    std::string out;
    while (n > 0) {
        out.insert(0, table[n % 10]);
        n /= 10;
    }
    return out;
}

}

std::string FluidTextHelper::getUnicodeFraction(int64_t numerator, int64_t denominator, bool simplify) {
    if (numerator < 0 || denominator < 0) throw std::invalid_argument("Numerator and denominator must be non negative.");
    if (simplify && denominator != 0) {
        int64_t g = std::gcd(numerator, denominator);
        if (g != 0) {
            numerator /= g;
            denominator /= g;
        }
    }
    return digits(numerator, SUPERSCRIPT) + FRACTION_BAR + digits(denominator, SUBSCRIPT);
}

text::Component FluidTextHelper::getMillibuckets(int64_t integer, bool simplify) {
    int64_t mb       = integer / 81;
    int64_t leftover = integer % BUCKET;
    if (leftover == 0 || integer >= BUCKET * 100) {
        std::string s = overlay::DisplayHelper::INSTANCE.humanReadableNumber(double(mb), "B", true);
        if (s.ends_with("mB")) return makeString(s.substr(0, s.size() - 2), 0, 0, "mB");
        return makeString(s.substr(0, s.size() - 1), 0, 0, "B");
    }
    if (integer % 81 == 0) return makeString(mb, 0, 0, "mB");
    if (simplify) {
        int64_t g = std::gcd(leftover, BUCKET);
        if (g >= 1000) return makeString(mb / 1000, leftover / g, BUCKET / g, "B");
    }
    return makeString(mb, integer % 81, 81, "mB");
}

text::Component FluidTextHelper::makeString(int64_t integer, int64_t numerator, int64_t denominator, std::string const& unit) {
    return makeString(integer == 0 && denominator != 0 ? "" : formatNumber(integer), numerator, denominator, unit);
}

text::Component FluidTextHelper::makeString(std::string const& integerString, int64_t numerator, int64_t denominator, std::string const& unit) {
    std::string s;
    if (denominator == 0) s = integerString + unit;
    else if (integerString.empty()) s = getUnicodeFraction(numerator, denominator, true) + unit;
    else s = integerString + " " + getUnicodeFraction(numerator, denominator, true) + unit;
    return text::Component::literal(s);
}

}
