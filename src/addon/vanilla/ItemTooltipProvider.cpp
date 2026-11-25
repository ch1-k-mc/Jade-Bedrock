// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include <sstream>

#include "mc/safety/RedactableString.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemStack.h"

#include "platform/item/ScopedItemRegistry.h"
#include "util/JadeFont.h"
#include "util/ModIdentification.h"
#include "util/WailaExceptionHandler.h"

namespace jade::addon::vanilla {

namespace {

void popUtf8(std::string& s) {
    if (s.empty()) return;
    size_t i = s.size() - 1;
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) --i;
    s.erase(i);
}

}

void ItemTooltipProvider::appendTooltip(ITooltip& tooltip, EntityAccessor& accessor, IPluginConfig&) {
    Actor* e = accessor.getEntity();
    if (!e || !e->hasCategory(ActorCategory::Item)) return;
    platform::item::ScopedItemRegistry scoped;
    ItemStack const&                   stack = static_cast<ItemActor*>(e)->mItem.get();
    Item const*                        item  = stack.isNull() ? nullptr : stack.mItem.get();
    if (!item) return;
    Bedrock::Safety::RedactableString hovertext{std::string()};
    try {
        item->appendFormattedHovertext(stack, accessor.getLevel(), hovertext, false);
    } catch (std::exception const& ex) {
        util::WailaExceptionHandler::handleErr(ex, this, &tooltip);
        return;
    }
    std::vector<std::string> lines;
    std::stringstream        ss(hovertext.mUnredactedString);
    for (std::string line; std::getline(ss, line);) lines.push_back(line);
    if (lines.size() < 2) return;
    lines.erase(lines.begin());
    std::string modName  = util::ModIdentification::getModName(stack);
    int const   maxWidth = 250;
    for (auto& raw : lines) {
        std::string line = text::stripColorCodes(raw);
        if (line.empty() || text::stripFormatting(line) == modName) continue;
        if (util::JadeFont::width(line) > maxWidth) {
            while (!line.empty() && util::JadeFont::width(line) > maxWidth - 5) popUtf8(line);
            line += "..";
        }
        tooltip.add(text::Component::literal(line));
    }
}

}
