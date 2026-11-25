// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/vanilla/VanillaProviders.h"

#include "mc/client/player/LocalPlayer.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/item/FallingBlockActor.h"
#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"

#include "impl/ui/JadeUIInternal.h"
#include "platform/item/ScopedItemRegistry.h"

namespace jade::addon::vanilla {

Block const* FallingBlockProvider::blockOf(Actor& entity) {
    if (entity.getTypeName() != "minecraft:falling_block") return nullptr;
    auto&        falling = static_cast<FallingBlockActor&>(entity);
    Block const* block   = nullptr;
    if (auto const& tag = falling.mFallingBlockSerId.get()) {
        if (auto b = Block::tryGetFromRegistry(*tag)) block = b.as_ptr();
    }
    if (!block) {
        if (auto b = Block::tryGetFromRegistry(static_cast<uint>(entity.getVariant()))) block = b.as_ptr();
    }
    return block && !block->isAir() ? block : nullptr;
}

ui::ElementPtr FallingBlockProvider::getIcon(EntityAccessor& accessor, IPluginConfig&, ui::ElementPtr currentIcon) {
    Actor* e = accessor.getEntity();
    if (!e) return currentIcon;
    Block const* block = blockOf(*e);
    if (!block) return currentIcon;
    platform::item::ScopedItemRegistry scoped;
    ItemInstance inst = block->asItemInstance(e->getDimensionBlockSource(), BlockPos(e->getPosition()), true);
    if (inst.isNull() || !inst.mItem.get()) return currentIcon;
    ItemStack stack;
    stack.reinit(*inst.mItem.get(), 1, inst.getAuxValue());
    return stack.isNull() ? currentIcon : impl::ui::JadeUIInternal::item(stack);
}

}
