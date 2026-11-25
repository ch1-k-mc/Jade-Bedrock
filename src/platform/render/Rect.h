// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

namespace jade::platform::render {

struct Rect {
    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;

    [[nodiscard]] int  right() const { return x + width; }
    [[nodiscard]] int  bottom() const { return y + height; }
    [[nodiscard]] bool isEmpty() const { return width <= 0 || height <= 0; }
};

}
