// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <cmath>

namespace jade::util {

class SmoothChasingValue {
public:
    static constexpr float eps = 1.0f / 4096.0f;

    float value = 0;

    [[nodiscard]] float getTarget() const { return mTarget; }
    [[nodiscard]] float getSpeed() const { return mSpeed; }
    [[nodiscard]] bool  isMoving() const { return std::abs(getCurrentDiff()) > 1.0f / 128.0f; }

    SmoothChasingValue& set(float v) {
        value = v;
        return *this;
    }
    SmoothChasingValue& start(float v) {
        value = v;
        return target(v);
    }
    SmoothChasingValue& target(float t) {
        mTarget = t;
        return *this;
    }
    SmoothChasingValue& withSpeed(float s) {
        mSpeed = s;
        return *this;
    }
    void tick(float pTicks) {
        float diff = getCurrentDiff();
        if (std::abs(diff) < eps) {
            set(mTarget);
            return;
        }
        set(value + diff * mSpeed * pTicks);
    }

protected:
    [[nodiscard]] float getCurrentDiff() const { return mTarget - value; }

    float mSpeed  = 0.4f;
    float mTarget = 0;
};

}
