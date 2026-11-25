// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include <algorithm>

#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/BrewingStandBlockActor.h"
#include "mc/world/level/block/actor/CampfireBlockActor.h"
#include "mc/world/level/block/actor/LecternBlockActor.h"

#include "api/theme/IThemeHelper.h"
#include "impl/ui/JadeUIInternal.h"
#include "overlay/DisplayHelper.h"
#include "platform/item/ScopedItemRegistry.h"

namespace jade::addon::vanilla {

using impl::ui::JadeUIInternal;
using text::Component;

namespace {

template <class T>
T* blockEntityOf(BlockAccessor& accessor, BlockActorType type) {
    BlockActor* be = accessor.getBlockEntity();
    return be && be->getType() == type ? static_cast<T*>(be) : nullptr;
}

api::ui::ElementPtr smallItemOf(char const* name) {
    platform::item::ScopedItemRegistry scoped;
    ItemStack                          stack;
    stack.reinit(name, 1, 0);
    return JadeUIInternal::smallItem(stack);
}

}

bool BrewingStandProvider::shouldRequestData(BlockAccessor& accessor) {
    return blockEntityOf<BrewingStandBlockActor>(accessor, BlockActorType::BrewingStand) != nullptr;
}

std::optional<BrewingStandData> BrewingStandProvider::streamData(BlockAccessor& accessor) {
    auto* b = blockEntityOf<BrewingStandBlockActor>(accessor, BlockActorType::BrewingStand);
    if (!b) return std::nullopt;
    return BrewingStandData{b->mFuelAmount, b->mBrewTime};
}

std::optional<BrewingStandData> BrewingStandProvider::decode(CompoundTag const& in) {
    return BrewingStandData{static_cast<int>(in.at("fuel")), static_cast<int>(in.at("time"))};
}

void BrewingStandProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto data = BrewingStandProvider::INSTANCE->decodeFromData(accessor);
    if (!data) return;
    auto& t      = theme::IThemeHelper::get();
    auto  powder = smallItemOf("minecraft:blaze_powder");
    powder->narration("");
    tooltip.add(powder);
    tooltip.append(JadeUIInternal::text(t.info(double(data->fuel))));
    if (data->time > 0) {
        tooltip.append(JadeUIInternal::spacer(5, 0));
        auto clock = smallItemOf("minecraft:clock");
        clock->narration("");
        tooltip.append(clock);
        tooltip.append(t.seconds(data->time, accessor.tickRate()));
    }
}

bool LecternProvider::shouldRequestData(BlockAccessor& accessor) {
    return blockEntityOf<LecternBlockActor>(accessor, BlockActorType::Lectern) != nullptr;
}

std::optional<view::ItemData> LecternProvider::streamData(BlockAccessor& accessor) {
    auto* l = blockEntityOf<LecternBlockActor>(accessor, BlockActorType::Lectern);
    if (!l) return std::nullopt;
    ItemStack const& book = l->mBook.get();
    return book.isNull() ? view::ItemData{} : view::ItemData::of(book);
}

void LecternProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto data = LecternProvider::INSTANCE->decodeFromData(accessor);
    if (!data) return;
    auto view = view::ItemView::fromData(*data);
    if (!view) return;
    tooltip.add(JadeUIInternal::smallItem(*view->item));
    tooltip.append(JadeUIInternal::text(Component::literal(text::stripFormatting(view->item->getHoverName()))));
}

namespace {
constexpr int CAMPFIRE_COOK_TIME = 600;
}

std::optional<view::ViewGroups<view::ItemData>> CampfireProvider::getGroups(Accessor& accessor) {
    if (accessor.getAccessorType() != AccessorType::BLOCK) return std::nullopt;
    auto* c = blockEntityOf<CampfireBlockActor>(static_cast<BlockAccessor&>(accessor), BlockActorType::Campfire);
    if (!c) return std::nullopt;
    std::vector<view::ItemData> list;
    for (int i = 0; i < 4; i++) {
        ItemInstance const& item = c->mCookingItem[i].get();
        if (item.isNull()) continue;
        view::ItemData d          = view::ItemData::of(item);
        d.tag["jade:cooking"]     = std::max(0, CAMPFIRE_COOK_TIME - c->mCookingTime[i]);
        list.push_back(std::move(d));
    }
    return view::ViewGroups<view::ItemData>{view::ViewGroup<view::ItemData>(std::move(list))};
}

std::vector<view::ClientViewGroup<view::ItemView>> CampfireProvider::getClientGroups(Accessor& accessor, view::ViewGroups<view::ItemData> const& groups) {
    float tickRate = accessor.tickRate();
    return view::ClientViewGroup<view::ItemView>::map<view::ItemData>(groups, [tickRate](view::ItemData const& d) -> std::optional<view::ItemView> {
        if (!d.tag.contains("jade:cooking")) return std::nullopt;
        int            time = static_cast<int>(d.tag.at("jade:cooking"));
        view::ItemData copy = d;
        copy.tag.erase("jade:cooking");
        auto v = view::ItemView::fromData(copy);
        if (!v) return std::nullopt;
        v->setAmountText(theme::IThemeHelper::get().seconds(time, tickRate).getString());
        return v;
    });
}

float EnchantmentPowerProvider::getPower(BlockSource& region, BlockPos const& pos) {
    return region.getBlock(pos).getTypeName() == "minecraft:bookshelf" ? 1.0f : 0.0f;
}

void EnchantmentPowerProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    float power = getPower(accessor.getRegion(), accessor.getPosition());
    if (power > 0) {
        tooltip.add(Component::translatable("jade.ench_power", std::vector<Component>{theme::IThemeHelper::get().info(overlay::DisplayHelper::formatNumber(power))}));
    }
}

void TotalEnchantmentPowerProvider::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    BlockSource& region = accessor.getRegion();
    BlockPos     pos    = accessor.getPosition();
    float        power  = 0;
    for (int y = 0; y <= 1; ++y) {
        for (int x = -2; x <= 2; ++x) {
            for (int z = -2; z <= 2; ++z) {
                if (std::abs(x) != 2 && std::abs(z) != 2) continue;
                BlockPos shelf{pos.x + x, pos.y + y, pos.z + z};
                BlockPos between{pos.x + x / 2, pos.y + y, pos.z + z / 2};
                if (EnchantmentPowerProvider::getPower(region, shelf) > 0 && region.getBlock(between).isAir()) {
                    power += EnchantmentPowerProvider::getPower(region, shelf);
                }
            }
        }
    }
    if (power > 0) {
        tooltip.add(Component::translatable("jade.ench_power", std::vector<Component>{theme::IThemeHelper::get().info(overlay::DisplayHelper::formatNumber(power))}));
    }
}

}
