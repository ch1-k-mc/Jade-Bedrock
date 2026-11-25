// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include <algorithm>

#include "mc/deps/nbt/ListTag.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/FurnaceBlockActor.h"

#include "impl/ui/JadeUIInternal.h"

namespace jade::addon::vanilla {

namespace {
FurnaceBlockActor* furnaceOf(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be) return nullptr;
    auto t = be->getType();
    if (t != BlockActorType::Furnace && t != BlockActorType::BlastFurnace && t != BlockActorType::Smoker) return nullptr;
    return static_cast<FurnaceBlockActor*>(be);
}
}

bool FurnaceProvider::shouldRequestData(BlockAccessor& accessor) { return furnaceOf(accessor) != nullptr; }

std::optional<FurnaceData> FurnaceProvider::streamData(BlockAccessor& accessor) {
    auto* f = furnaceOf(accessor);
    if (!f) return std::nullopt;
    FurnaceData d;
    d.progress = f->mCookingProgress;
    d.total    = f->mBurnInterval;
    for (int i = 0; i < 3; ++i) {
        ItemStack const& s = f->mItems[i].get();
        d.inventory[i]     = s.isNull() ? view::ItemData{} : view::ItemData::of(s);
    }
    return d;
}

void FurnaceProvider::encode(CompoundTag& out, FurnaceData const& value) {
    out["progress"] = value.progress;
    out["total"]    = value.total;
    ListTag items;
    for (auto const& item : value.inventory) {
        CompoundTag t;
        view::ViewCodec<view::ItemData>::encode(t, item);
        items.add(std::make_unique<CompoundTag>(std::move(t)));
    }
    out["inventory"] = std::move(items);
}

std::optional<FurnaceData> FurnaceProvider::decode(CompoundTag const& in) {
    FurnaceData d;
    d.progress = static_cast<int>(in.at("progress"));
    d.total    = static_cast<int>(in.at("total"));
    auto const& list = in.at("inventory").get<ListTag>();
    for (size_t i = 0; i < 3 && i < list.size(); ++i) {
        if (!list[i].hold<CompoundTag>()) continue;
        if (auto item = view::ViewCodec<view::ItemData>::decode(list[i].get<CompoundTag>())) d.inventory[i] = std::move(*item);
    }
    return d;
}

void FurnaceProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto data = FurnaceProvider::INSTANCE->decodeFromData(accessor);
    if (!data) return;
    std::array<std::optional<view::ItemView>, 3> views;
    bool                                         allEmpty = true;
    for (int i = 0; i < 3; ++i) {
        views[i] = view::ItemView::fromData(data->inventory[i]);
        if (views[i]) allEmpty = false;
    }
    if (allEmpty) return;
    auto itemOf = [&](int i) {
        ItemStack empty;
        auto      e = impl::ui::JadeUIInternal::item(views[i] ? *views[i]->item : empty);
        e->alignSelfCenter();
        return e;
    };
    tooltip.add(itemOf(0));
    tooltip.append(itemOf(1));
    float ratio = data->total == 0 ? 0 : std::clamp(static_cast<float>(data->progress) / static_cast<float>(data->total), 0.0f, 1.0f);
    auto  arrow = impl::ui::JadeUIInternal::progressArrow(ratio);
    arrow->alignSelfCenter();
    arrow->settings([](layout::LayoutSettings s) { return s.paddingHorizontal(3); });
    tooltip.append(arrow);
    tooltip.append(itemOf(2));
}

}
