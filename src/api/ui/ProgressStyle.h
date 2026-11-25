// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>
#include <stdexcept>

#include "api/JadeIds.h"
#include "api/ui/Element.h"

namespace jade::api::ui {

class ProgressStyle {
public:
    virtual ~ProgressStyle() = default;

    virtual ProgressStyle& direction(ScreenDirection d) {
        mDirection = d;
        return *this;
    }
    [[nodiscard]] ScreenDirection direction() const { return mDirection; }
    ProgressStyle&                fitContentX(bool v) {
        mFitContentX = v;
        return *this;
    }
    [[nodiscard]] bool fitContentX() const { return mFitContentX; }
    ProgressStyle&     fitContentY(bool v) {
        mFitContentY = v;
        return *this;
    }
    [[nodiscard]] bool fitContentY() const { return mFitContentY; }
    ProgressStyle&     foreground(std::optional<Identifier> f) {
        mForeground = std::move(f);
        return *this;
    }
    [[nodiscard]] std::optional<Identifier> const& foreground() const { return mForeground; }
    ProgressStyle&                                 canDecrease(bool v) {
        mCanDecrease = v;
        return *this;
    }
    [[nodiscard]] bool canDecrease() const { return mCanDecrease; }

    [[nodiscard]] static bool isHorizontal(ScreenDirection d) { return d == ScreenDirection::LEFT || d == ScreenDirection::RIGHT; }

protected:
    bool                      mFitContentX = true;
    bool                      mFitContentY = true;
    ScreenDirection           mDirection   = ScreenDirection::RIGHT;
    std::optional<Identifier> mForeground;
    bool                      mCanDecrease = false;
};

class SimpleProgressStyle : public ProgressStyle {
public:
    bool     autoTextColor = true;
    int      color         = 0;
    bool     vertical      = false;

    ProgressStyle& direction(ScreenDirection d) override {
        if (d != ScreenDirection::UP && d != ScreenDirection::RIGHT) throw std::invalid_argument("Only UP and RIGHT are supported");
        ProgressStyle::direction(d);
        vertical = d == ScreenDirection::UP;
        return *this;
    }
};

using ProgressStylePtr = std::shared_ptr<ProgressStyle>;

}
