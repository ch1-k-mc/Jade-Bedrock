// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <cstdint>

#include "api/ui/Rect2f.h"

namespace jade::api::ui {

class TooltipAnimation {
public:
    Rect2f  expectedRect;
    Rect2f  rect;
    int64_t startTime = -1;
    Rect2f  startRect;
    float   scale        = 1;
    float   showHideAlpha = 0;
    float   alpha        = 0;

    void mapMousePosition(double x, double y, double& outX, double& outY) const {
        outX = (x - rect.getX()) / scale;
        outY = (y - rect.getY()) / scale;
    }
};

}
