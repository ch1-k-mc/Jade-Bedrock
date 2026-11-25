// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include "track/ProgressTracker.h"

namespace jade::track {

class HealthTrackInfo : public TrackInfo {
public:
    HealthTrackInfo(float health, float absorption) : mHealth(health), mLastHealth(health), mAbsorption(absorption), mLastAbsorption(absorption) {}

    void update(float) override {}
    void tick() override {
        mTicksSinceHurt++;
        if (mHealth != mLastHealth && mTicksSinceHurt >= 5) mLastHealth = mHealth;
        if (mAbsorption != mLastAbsorption && mTicksSinceHurt >= 5) mLastAbsorption = mAbsorption;
    }

    [[nodiscard]] float getLastHealth() const { return mLastHealth; }
    [[nodiscard]] float getLastAbsorption() const { return mLastAbsorption; }
    [[nodiscard]] bool  isBlinking() const { return mTicksSinceHurt < 5; }

    void setHealth(float health, float absorption) {
        if (health < mHealth) mTicksSinceHurt = 0;
        else if (health > mHealth) mLastHealth = health;
        mHealth = health;
        if (absorption < mAbsorption) mTicksSinceHurt = 0;
        else if (absorption > mAbsorption) mLastAbsorption = absorption;
        mAbsorption = absorption;
    }

private:
    float mHealth;
    float mLastHealth;
    float mAbsorption;
    float mLastAbsorption;
    int   mTicksSinceHurt = 1000;
};

}
