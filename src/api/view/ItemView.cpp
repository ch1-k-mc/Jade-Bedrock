// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "api/view/ItemView.h"

#include <algorithm>

#include "mc/world/item/ItemStack.h"
#include "mc/world/item/SaveContext.h"
#include "mc/world/item/SaveContextFactory.h"

#include "impl/ui/Elements.h"
#include "platform/item/ScopedItemRegistry.h"

namespace jade::api::view {

ItemData ItemData::of(ItemStackBase const& stack, int count) {
    ItemData d;
    auto     ctx = SaveContextFactory::createCloneSaveContext();
    if (auto tag = stack.save(*ctx)) d.tag = std::move(*tag);
    d.tag.erase("Count");
    d.count = count;
    return d;
}

ItemData ItemData::of(ItemStackBase const& stack) { return of(stack, static_cast<int>(stack.mCount)); }

std::string ItemData::key() const { return tag.toSnbt(SnbtFormat::Minimize, 0); }

std::optional<ItemView> ItemView::fromData(ItemData const& data) {
    if (data.count <= 0) return std::nullopt;
    platform::item::ScopedItemRegistry scoped;
    CompoundTag                        tag = data.tag;
    tag["Count"] = static_cast<schar>(std::clamp(data.count, 1, 64));
    ItemStack stack = ItemStack::fromTag(tag);
    if (stack.isNull()) return std::nullopt;
    return ItemView(impl::ui::makeSharedItem(stack), data.count);
}

bool ItemView::isEmpty() const { return !item || item->isNull() || count <= 0; }

}
