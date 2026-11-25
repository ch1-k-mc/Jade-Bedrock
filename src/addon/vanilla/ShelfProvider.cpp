// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include <algorithm>
#include <cmath>

#include "mc/world/Container.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/VanillaStates.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/component/IVanillaMainBlockActorComponent.h"

#include "impl/ui/JadeUIInternal.h"
#include "util/BlockStates.h"

namespace jade::addon::vanilla {

namespace {

constexpr int NORTH = 2, SOUTH = 3, WEST = 4, EAST = 5;

struct ShelfShape {
    int columns;
    int rows;
    int facing;
};

std::optional<ShelfShape> shapeOf(Block const& block) {
    auto const& type = block.getTypeName();
    if (type == "minecraft:chiseled_bookshelf") {
        static constexpr int legacy[]{SOUTH, WEST, NORTH, EAST};
        int d = block.getState<int>(VanillaStates::Direction()).value_or(0);
        return ShelfShape{3, 2, legacy[std::clamp(d, 0, 3)]};
    }
    if (type.ends_with("_shelf")) {
        int facing = -1;
        auto const& ser = block.mSerializationId.get();
        if (ser.contains("states") && ser.at("states").hold<CompoundTag>()) {
            auto const& states = ser.at("states").get<CompoundTag>();
            if (states.contains("minecraft:cardinal_direction") && states.at("minecraft:cardinal_direction").hold<StringTag>()) {
                std::string d = states.at("minecraft:cardinal_direction").get<StringTag>();
                facing        = d == "north" ? NORTH : d == "south" ? SOUTH : d == "west" ? WEST : d == "east" ? EAST : -1;
            }
        }
        return ShelfShape{3, 1, facing};
    }
    return std::nullopt;
}

int section(float v, int n) { return std::clamp(static_cast<int>(std::floor(v * static_cast<float>(n))), 0, n - 1); }

std::optional<int> hitSlot(BlockAccessor& accessor) {
    auto shape = shapeOf(accessor.getBlock());
    if (!shape || shape->facing == -1) return std::nullopt;
    auto const& hit = accessor.getHitResult();
    if (hit.direction != shape->facing) return std::nullopt;
    BlockPos pos = accessor.getPosition();
    float    rx = hit.location.x - float(pos.x), ry = hit.location.y - float(pos.y), rz = hit.location.z - float(pos.z);
    float    x  = 0;
    switch (shape->facing) {
    case NORTH: x = 1 - rx; break;
    case SOUTH: x = rx; break;
    case WEST: x = rz; break;
    case EAST: x = 1 - rz; break;
    default: return std::nullopt;
    }
    int column = section(x, shape->columns);
    int row    = section(1 - ry, shape->rows);
    return column + row * shape->columns;
}

Container* containerOf(BlockAccessor& accessor) {
    BlockActor* be = accessor.getBlockEntity();
    if (!be || (be->getType() != BlockActorType::ChiseledBookshelf && be->getType() != BlockActorType::Shelf)) return nullptr;
    auto* main = be->_getMainComponent();
    return main ? main->getContainer() : nullptr;
}

}

bool ShelfProvider::shouldRequestData(BlockAccessor& accessor) {
    if (accessor.showDetails()) return false;
    auto slot = hitSlot(accessor);
    if (!slot) return false;
    if (accessor.getBlock().getTypeName() == "minecraft:chiseled_bookshelf") {
        int occupied = util::BlockStates::intState(accessor.getBlock(), "books_stored").value_or(0);
        if ((occupied & (1 << *slot)) == 0) return false;
    }
    return containerOf(accessor) != nullptr;
}

std::optional<view::ItemData> ShelfProvider::streamData(BlockAccessor& accessor) {
    auto slot = hitSlot(accessor);
    if (!slot) return std::nullopt;
    Container* c = containerOf(accessor);
    if (!c || *slot >= c->getContainerSize()) return std::nullopt;
    ItemStack const& item = c->getItem(*slot);
    return item.isNull() ? view::ItemData{} : view::ItemData::of(item);
}

namespace {
std::optional<view::ItemView> hitBook(BlockAccessor& accessor) {
    if (accessor.showDetails()) return std::nullopt;
    auto data = ShelfProvider::INSTANCE->decodeFromData(accessor);
    if (!data) return std::nullopt;
    return view::ItemView::fromData(*data);
}
}

ui::ElementPtr ShelfProvider::Client::getIcon(BlockAccessor& accessor, IPluginConfig&, ui::ElementPtr) {
    auto item = hitBook(accessor);
    return item ? impl::ui::JadeUIInternal::item(*item->item) : nullptr;
}

void ShelfProvider::Client::appendTooltip(ITooltip& tooltip, BlockAccessor& accessor, IPluginConfig&) {
    auto item = hitBook(accessor);
    if (!item) return;
    tooltip.remove(JadeIds::UNIVERSAL_ITEM_STORAGE);
    tooltip.add(text::Component::literal(text::stripFormatting(item->item->getHoverName())));
}

}
