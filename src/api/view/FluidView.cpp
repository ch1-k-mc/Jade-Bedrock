// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "api/view/FluidView.h"

#include "mc/world/item/ItemStack.h"

#include "impl/ui/FluidStackElement.h"
#include "platform/item/ScopedItemRegistry.h"
#include "util/FluidTextHelper.h"

namespace jade::api::view {

text::Component JadeFluidObject::getDisplayName() const {
    if (type == "minecraft:water") return text::Component::translatable("tile.water.name");
    if (type == "minecraft:lava") return text::Component::translatable("tile.lava.name");
    if (type == "minecraft:powder_snow") return text::Component::translatable("tile.powder_snow.name");
    if (type == "minecraft:potion") {
        platform::item::ScopedItemRegistry scoped;
        char const* item = potionType == 1 ? "minecraft:splash_potion" : potionType == 2 ? "minecraft:lingering_potion" : "minecraft:potion";
        ItemStack   stack;
        stack.reinit(item, 1, static_cast<short>(potionId < 0 ? 0 : potionId));
        if (!stack.isNull()) return text::Component::literal(text::stripFormatting(stack.getHoverName()));
    }
    return text::Component::literal(type);
}

std::string JadeFluidObject::texture() const {
    if (type == "minecraft:lava") return "game:textures/blocks/lava_still";
    if (type == "minecraft:powder_snow") return "game:textures/blocks/powder_snow";
    return "game:textures/blocks/cauldron_water";
}

uint32_t JadeFluidObject::tint() const {
    if (color != -1) return static_cast<uint32_t>(color) | 0xFF000000u;
    if (type == "minecraft:water") return 0xFF44AFF5u;
    return 0xFFFFFFFFu;
}

void JadeFluidObject::encode(CompoundTag& out) const {
    out["type"]       = type;
    out["amount"]     = static_cast<int64>(amount);
    out["color"]      = color;
    out["potionId"]   = potionId;
    out["potionType"] = potionType;
}

std::optional<JadeFluidObject> JadeFluidObject::decode(CompoundTag const& in) {
    try {
        JadeFluidObject f;
        if (in.at("type").hold<StringTag>()) f.type = in.at("type").get<StringTag>();
        f.amount     = static_cast<int64>(in.at("amount"));
        f.color      = static_cast<int>(in.at("color"));
        f.potionId   = static_cast<int>(in.at("potionId"));
        f.potionType = static_cast<int>(in.at("potionType"));
        return f;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<FluidView> FluidView::readDefault(Data const& data) {
    if (data.capacity <= 0 || data.fluids.size() > 1) return std::nullopt;
    JadeFluidObject fluid   = data.fluids.empty() ? JadeFluidObject::empty() : data.fluids.front();
    int64_t         amount  = fluid.amount;
    auto            current = util::FluidTextHelper::getMillibuckets(amount, true);
    auto            max     = util::FluidTextHelper::getMillibuckets(data.capacity, true);
    FluidView       view(std::make_shared<impl::ui::FluidStackElement>(fluid), current, max);
    if (!fluid.isEmpty()) view.fluidName = fluid.getDisplayName();
    view.ratio = static_cast<float>(static_cast<double>(amount) / static_cast<double>(data.capacity));
    if (fluid.isEmpty()) {
        view.overrideText = text::Component::translatable("jade.fluid", std::vector<text::Component>{emptyFluid(), text::Component::literal(view.max.getString())});
    }
    return view;
}

}
