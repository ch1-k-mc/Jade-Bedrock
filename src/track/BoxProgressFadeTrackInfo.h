// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <optional>

#include "api/ui/Element.h"
#include "track/ProgressTracker.h"

namespace jade::track {

class BoxProgressFadeTrackInfo : public TrackInfo {
public:
    BoxProgressFadeTrackInfo() { persistent = true; }

    std::optional<api::ui::BoxProgress> progress;
    float                               alpha = 0;

    void touch() { updatedThisTick = true; }
    void update(float) override {}
    void tick() override {}
};

}
