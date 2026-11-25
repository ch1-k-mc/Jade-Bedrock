// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <string>

#include "platform/text/Component.h"

class Font;

namespace jade::util {

class JadeFont {
public:
    static constexpr int lineHeight = 9;

    static int   width(text::Component const& text);
    static int   width(std::string const& text);
    static Font* font();
    static void  reset();
};

}
