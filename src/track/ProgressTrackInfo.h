// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>

#include "api/view/ProgressView.h"
#include "track/ProgressTracker.h"
#include "util/SmoothChasingValue.h"

namespace jade::track {

class ProgressTrackInfo : public TrackInfo {
public:
    using Part = api::view::ProgressView::Part;

    ProgressTrackInfo(std::vector<Part> const& parts, bool canDecrease, int width) : mCanDecrease(canDecrease), mWidth(width) { setProgress(parts); }

    [[nodiscard]] int getWidth() const { return mWidth; }

    void update(float pTicks) override {
        for (auto& [_, child] : mChildren) child.update(pTicks);
    }

    void setExpectedWidth(int expectedWidth) {
        mExpectedWidth = expectedWidth;
        if (expectedWidth > mWidth) {
            mWidth                  = expectedWidth;
            mTicksSinceWidthChanged = 0;
        }
    }

    void setProgress(std::vector<Part> const& parts) {
        std::set<int> seen;
        for (auto const& part : parts) {
            auto it = mChildren.find(part.id);
            if (it == mChildren.end()) {
                mChildren.emplace(part.id, Child(part, mCanDecrease));
            } else {
                it->second.part = part;
                it->second.setProgress(part.progress);
            }
            seen.insert(part.id);
        }
        for (auto& [id, child] : mChildren)
            if (!seen.contains(id)) child.setProgress(0);
    }

    [[nodiscard]] float getSmoothProgress(Part const& part) const {
        auto it = mChildren.find(part.id);
        return it == mChildren.end() ? part.progress : it->second.smoothProgress.value;
    }

    void tick() override {
        if (mExpectedWidth < mWidth && ++mTicksSinceWidthChanged > 10) {
            mWidth                  = mExpectedWidth;
            mTicksSinceWidthChanged = 0;
        }
    }

private:
    struct Child {
        Part                     part;
        bool                     canDecrease;
        util::SmoothChasingValue smoothProgress;
        float                    ticksSinceValueChanged = 0;

        Child(Part p, bool canDecrease) : part(std::move(p)), canDecrease(canDecrease) { smoothProgress.start(part.progress); }

        void setProgress(float progress) {
            if (smoothProgress.getTarget() == progress) return;
            ticksSinceValueChanged = 0;
            if (canDecrease || progress > smoothProgress.getTarget()) smoothProgress.target(progress);
            else smoothProgress.start(progress);
        }

        void update(float pTicks) {
            float progress = part.progress;
            if (progress != smoothProgress.getTarget() && ticksSinceValueChanged > 0) {
                if (ticksSinceValueChanged > 10) {
                    smoothProgress.withSpeed(0.4f);
                } else if (canDecrease || progress > smoothProgress.getTarget()) {
                    float spd = std::abs(progress - smoothProgress.getTarget()) / ticksSinceValueChanged;
                    spd       = std::max(0.1f, 4.0f * spd);
                    smoothProgress.withSpeed(spd);
                }
                ticksSinceValueChanged = pTicks;
            } else {
                ticksSinceValueChanged += pTicks;
            }
            if (!canDecrease && progress < smoothProgress.getTarget()) {
                if (smoothProgress.isMoving() && !part.targetDefined()) {
                    smoothProgress.withSpeed(std::max(0.5f, smoothProgress.getSpeed()));
                    if (smoothProgress.getTarget() > 0.9f) smoothProgress.target(1);
                } else {
                    smoothProgress.start(progress);
                }
            } else {
                smoothProgress.target(progress);
            }
            if (part.targetDefined()) {
                float value = smoothProgress.value + pTicks * part.speed;
                if ((part.speed > 0 && value >= part.target) || (part.speed < 0 && value <= part.target)) value = part.target;
                smoothProgress.set(value);
            } else {
                smoothProgress.tick(pTicks);
            }
        }
    };

    std::map<int, Child> mChildren;
    bool                 mCanDecrease;
    int                  mWidth;
    int                  mTicksSinceWidthChanged = 0;
    int                  mExpectedWidth          = 0;
};

}
