// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "util/JadeFont.h"

#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/Font.h"
#include "mc/client/gui/FontHandle.h"

namespace jade::util {

namespace {
Font* gFont = nullptr;
}

Font* JadeFont::font() {
    if (gFont) return gFont;
    auto ci = ll::service::getClientInstance();
    if (!ci) return nullptr;
    FontHandle handle = ci->getFontHandle();
    gFont             = handle.mDefaultFont.get();
    return gFont;
}

void JadeFont::reset() { gFont = nullptr; }

int JadeFont::width(std::string const& text) {
    if (text.empty()) return 0;
    Font* f = font();
    return f ? f->getLineLength(text, 1.0f, false) : static_cast<int>(text.size()) * 6;
}

int JadeFont::width(text::Component const& text) {
    int w = 0;
    for (auto const& span : text.spans()) {
        if (span.text.empty()) continue;
        w += width(span.style.formatCodes() + span.text);
    }
    return w;
}

}
