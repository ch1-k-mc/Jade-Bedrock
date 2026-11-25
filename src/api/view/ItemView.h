// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mc/deps/nbt/CompoundTag.h"

#include "api/ui/Element.h"
#include "api/view/ViewGroup.h"

class ItemStack;
class ItemStackBase;

namespace jade::api::view {

struct ItemData {
    CompoundTag tag;
    int         count = 0;

    static ItemData of(ItemStackBase const& stack, int count);
    static ItemData of(ItemStackBase const& stack);
    [[nodiscard]] std::string key() const;
};

template <>
struct ViewCodec<ItemData> {
    static void encode(CompoundTag& out, ItemData const& v) {
        out["item"]  = v.tag;
        out["count"] = v.count;
    }
    static std::optional<ItemData> decode(CompoundTag const& in) {
        if (!in.contains("item") || !in.at("item").hold<CompoundTag>() || !in.contains("count")) return std::nullopt;
        ItemData v;
        v.tag   = in.at("item").get<CompoundTag>();
        v.count = static_cast<int>(in.at("count"));
        return v;
    }
};

class ItemView {
public:
    static std::optional<ItemView> fromData(ItemData const& data);
    explicit ItemView(std::shared_ptr<ItemStack> item, int count) : item(std::move(item)), count(count) {}

    std::shared_ptr<ItemStack>                 item;
    int                                        count;
    std::optional<std::string>                 amountText;
    std::optional<std::vector<ui::ElementPtr>> description;

    ItemView& setAmountText(std::string text) {
        amountText = std::move(text);
        return *this;
    }
    ItemView& setDescription(std::vector<ui::ElementPtr> d) {
        description = std::move(d);
        return *this;
    }
    [[nodiscard]] bool isEmpty() const;
};

}
