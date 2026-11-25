// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "util/CommonProxy.h"

#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components_json_legacy/ContainerComponent.h"
#include "mc/world/Container.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/BlockType.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/inventory/EnderChestContainer.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/CauldronLiquidType.h"
#include "mc/world/level/block/VanillaStates.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/CauldronBlockActor.h"
#include "mc/deps/core/math/Color.h"
#include "mc/world/effect/MobEffect.h"
#include "mc/world/effect/MobEffectInstance.h"
#include "mc/world/item/alchemy/Potion.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/ChestBlockActor.h"
#include "mc/world/level/block/actor/DispenserBlockActor.h"
#include "mc/world/level/block/actor/RandomizableBlockActorContainerBase.h"
#include "mc/world/level/block/actor/component/IVanillaMainBlockActorComponent.h"

#include "addon/universal/ItemCollector.h"
#include "addon/universal/ItemIterator.h"

namespace jade::util {

namespace {

std::uint32_t potionColor(int potionId) {
    constexpr std::uint32_t WATER = 0xFF385DC6;
    auto const&             all   = Potion::mPotionsById();
    if (potionId < 0 || potionId >= static_cast<int>(all.size()) || !all[potionId]) return WATER;
    float r = 0, g = 0, b = 0, n = 0;
    for (MobEffectInstance const& inst : all[potionId]->mEffects.get()) {
        MobEffect const* effect = MobEffect::getByName(MobEffect::getNameById(inst.mId));
        if (!effect) continue;
        mce::Color const& c = effect->mColor.get();
        float             w = static_cast<float>(inst.mAmplifier + 1);
        r += c.r * w;
        g += c.g * w;
        b += c.b * w;
        n += w;
    }
    if (n == 0) return WATER;
    return mce::Color(r / n, g / n, b / n, 1.0f).toARGB();
}

BlockActor* blockEntityOf(api::Accessor& accessor) {
    if (accessor.getAccessorType() != api::AccessorType::BLOCK) return nullptr;
    return static_cast<api::BlockAccessor&>(accessor).getBlockEntity();
}

ContainerComponent* containerComponentOf(api::Accessor& accessor) {
    if (accessor.getAccessorType() != api::AccessorType::ENTITY) return nullptr;
    Actor* entity = static_cast<api::EntityAccessor&>(accessor).getEntity();
    if (!entity) return nullptr;
    auto ref = entity->getEntityContext().tryGetComponent<ContainerComponent>();
    return ref ? ref.as_ptr() : nullptr;
}

RandomizableBlockActorContainerBase* randomizable(BlockActor* be) {
    if (!be) return nullptr;
    switch (be->getType()) {
    case BlockActorType::Chest:
    case BlockActorType::BarrelBlock:
    case BlockActorType::ShulkerBox:
        return static_cast<ChestBlockActor*>(be);
    case BlockActorType::Dispenser:
    case BlockActorType::Dropper:
        return static_cast<DispenserBlockActor*>(be);
    default:
        return nullptr;
    }
}

}

Container* CommonProxy::findContainer(api::Accessor& accessor) {
    if (BlockActor* be = blockEntityOf(accessor)) {
        if (be->getType() == BlockActorType::EnderChest) return nullptr;
        if (be->getType() == BlockActorType::Lectern || be->getType() == BlockActorType::Cauldron) return nullptr;
        auto* main = be->_getMainComponent();
        return main ? main->getContainer() : nullptr;
    }
    if (auto* comp = containerComponentOf(accessor)) {
        if (comp->mPrivate) return nullptr;
        return &comp->mContainer.get();
    }
    return nullptr;
}

bool CommonProxy::isEnderChest(api::Accessor& accessor) {
    BlockActor* be = blockEntityOf(accessor);
    return be && be->getType() == BlockActorType::EnderChest;
}

Container* CommonProxy::findEnderChest(api::Accessor& accessor) {
    if (!isEnderChest(accessor)) return nullptr;
    auto inv = accessor.getPlayer().getEnderChestContainer();
    return inv ? static_cast<Container*>(inv.as_ptr()) : nullptr;
}

bool CommonProxy::isFurnace(api::Accessor& accessor) {
    BlockActor* be = blockEntityOf(accessor);
    if (!be) return false;
    auto t = be->getType();
    return t == BlockActorType::Furnace || t == BlockActorType::BlastFurnace || t == BlockActorType::Smoker;
}

bool CommonProxy::hasPendingLoot(api::Accessor& accessor) {
    if (auto* r = randomizable(blockEntityOf(accessor))) return !r->mLootTable->empty();
    if (auto* comp = containerComponentOf(accessor)) return !comp->mLootTable->empty();
    return false;
}

bool CommonProxy::hasDefaultItemStorage(api::Accessor& accessor) {
    if (accessor.getAccessorType() == api::AccessorType::BLOCK) return findContainer(accessor) != nullptr;
    return true;
}

std::shared_ptr<addon::universal::IItemCollector> CommonProxy::createItemCollector(api::Accessor& accessor) {
    using namespace addon::universal;
    if (accessor.getAccessorType() == api::AccessorType::ENTITY) {
        Actor* e = static_cast<api::EntityAccessor&>(accessor).getEntity();
        if (!e || e->isPlayer()) return std::make_shared<EmptyItemCollector>();
    }
    if (findContainer(accessor)) {
        return std::make_shared<ItemCollector<Container>>(std::make_unique<ContainerItemIterator>(&CommonProxy::findContainer, 0));
    }
    return std::make_shared<EmptyItemCollector>();
}

bool CommonProxy::isCorrectToolForDrops(Block const& block, Player const& player) {
    if (!block.getBlockType().mRequiresCorrectToolForDrops) return true;
    ItemStack const& held = player.getSelectedItem();
    Item const*      item = held.isNull() ? nullptr : held.mItem.get();
    return item && item->canDestroySpecial(block);
}

bool CommonProxy::isCauldron(api::Accessor& accessor) {
    if (accessor.getAccessorType() != api::AccessorType::BLOCK) return false;
    auto const& type = static_cast<api::BlockAccessor&>(accessor).getBlock().getTypeName();
    return type == "minecraft:cauldron" || type == "minecraft:lava_cauldron";
}

bool CommonProxy::hasDefaultFluidStorage(api::Accessor& accessor) {
    if (accessor.getAccessorType() == api::AccessorType::BLOCK) return isCauldron(accessor);
    return true;
}

std::optional<api::view::ViewGroups<api::view::FluidView::Data>> CommonProxy::wrapFluidStorage(api::Accessor& accessor) {
    using api::view::FluidView;
    using api::view::JadeFluidObject;
    if (!isCauldron(accessor)) return std::nullopt;
    auto&        a      = static_cast<api::BlockAccessor&>(accessor);
    Block const& block  = a.getBlock();
    int          level  = block.getState<int>(VanillaStates::FillLevel()).value_or(0);
    auto         liquid = block.getState<CauldronLiquidType>(VanillaStates::CauldronLiquid()).value_or(CauldronLiquidType::Water);
    if (block.getTypeName() == "minecraft:lava_cauldron") liquid = CauldronLiquidType::Lava;
    FluidView::Data data;
    data.capacity = JadeFluidObject::bucketVolume();
    if (level > 0) {
        JadeFluidObject f;
        f.amount = JadeFluidObject::bucketVolume() * level / 6;
        switch (liquid) {
        case CauldronLiquidType::Lava:
            f.type = "minecraft:lava";
            break;
        case CauldronLiquidType::PowderSnow:
            f.type = "minecraft:powder_snow";
            break;
        default: {
            f.type = "minecraft:water";
            BlockActor* be = a.getBlockEntity();
            if (be && be->getType() == BlockActorType::Cauldron) {
                auto* c = static_cast<CauldronBlockActor*>(be);
                if (c->mPotionId >= 0) {
                    f.type       = "minecraft:potion";
                    f.potionId   = c->mPotionId;
                    f.potionType = static_cast<int>(static_cast<PotionType>(c->mPotionType));
                    f.color      = potionColor(c->mPotionId);
                } else if (c->mHasCustomColor) {
                    f.color = c->mCustomColor;
                }
            }
            break;
        }
        }
        data.fluids.push_back(std::move(f));
    }
    return api::view::ViewGroups<FluidView::Data>{api::view::ViewGroup<FluidView::Data>({std::move(data)})};
}

}
