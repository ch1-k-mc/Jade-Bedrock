// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/universal/ItemStorageProvider.h"

#include <algorithm>
#include <map>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"

#include "addon/universal/ItemCollector.h"
#include "api/ui/Element.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"
#include "impl/ui/Elements.h"
#include "impl/ui/JadeUIInternal.h"
#include "overlay/DisplayHelper.h"
#include "util/ClientProxy.h"
#include "util/CommonProxy.h"
#include "util/JadeFont.h"
#include "util/WailaExceptionHandler.h"

namespace jade::addon::universal {

using impl::ui::JadeUIInternal;
using overlay::DisplayHelper;
using view::ClientViewGroup;
using view::ItemData;
using view::ItemView;
using view::ViewGroups;

namespace {

struct CacheEntry {
    std::shared_ptr<IItemCollector> collector;
    int64_t                         lastAccess;
};
std::map<void const*, CacheEntry> gTargetCache;

std::shared_ptr<IItemCollector> targetCollector(Accessor& accessor) {
    int64_t now = util::ClientProxy::millis();
    for (auto it = gTargetCache.begin(); it != gTargetCache.end();) {
        if (now - it->second.lastAccess > 60000) it = gTargetCache.erase(it);
        else ++it;
    }
    void const* target = accessor.getTarget();
    auto        it     = gTargetCache.find(target);
    if (it == gTargetCache.end()) it = gTargetCache.emplace(target, CacheEntry{util::CommonProxy::createItemCollector(accessor), now}).first;
    it->second.lastAccess = now;
    return it->second.collector;
}

bool isArmorStand(Accessor& accessor) {
    if (accessor.getAccessorType() != AccessorType::ENTITY) return false;
    Actor* e = static_cast<EntityAccessor&>(accessor).getEntity();
    return e && e->getTypeName() == "minecraft:armor_stand";
}

bool boolOf(CompoundTag const& tag, std::string const& key) {
    if (!tag.contains(key)) return false;
    try {
        return static_cast<int>(tag.at(key)) != 0;
    } catch (...) {
        return false;
    }
}

}

std::shared_ptr<ItemStorageExtension> const ItemStorageExtension::INSTANCE = std::make_shared<ItemStorageExtension>();

void ItemStorageCommon::putData(Accessor& accessor) {
    CompoundTag& tag   = accessor.getServerData();
    auto         entry = util::CommonProxy::getServerExtensionData(accessor, impl::WailaCommonRegistration::instance().itemStorageProviders);
    if (entry) {
        for (auto& group : entry->second)
            if (group.views.size() > ItemCollector<Container>::MAX_SIZE) group.views.resize(ItemCollector<Container>::MAX_SIZE);
        tag[JadeIds::UNIVERSAL_ITEM_STORAGE] = view::encodeGroups(entry->first, entry->second);
        return;
    }
    if (util::CommonProxy::hasPendingLoot(accessor)) tag["Loot"] = true;
}

void ItemStorageCommon::appendServerData(Accessor& accessor) {
    if (util::CommonProxy::isFurnace(accessor)) return;
    putData(accessor);
}

bool ItemStorageCommon::shouldRequestData(Accessor& accessor) {
    if (util::CommonProxy::isFurnace(accessor)) return false;
    auto& plugin = config::IWailaConfig::get().plugin();
    int   amount = plugin.getInt(accessor.showDetails() ? JadeIds::UNIVERSAL_ITEM_STORAGE_DETAILED_AMOUNT : JadeIds::UNIVERSAL_ITEM_STORAGE_NORMAL_AMOUNT);
    if (amount == 0) return false;
    if (plugin.get(JadeIds::UNIVERSAL_ITEM_STORAGE_SORT)) accessor.getServerData()["SortItems"] = true;
    return impl::WailaCommonRegistration::instance().itemStorageProviders.hitsAny(accessor, [](auto& p, Accessor& a) { return p.shouldRequestData(a); });
}

std::optional<ViewGroups<ItemData>> ItemStorageExtension::getGroups(Accessor& accessor) {
    void const* target = accessor.getTarget();
    if (!target) return std::nullopt;
    if (util::CommonProxy::hasPendingLoot(accessor)) return std::nullopt;
    if (isArmorStand(accessor)) return std::nullopt;
    if (Container* ender = util::CommonProxy::findEnderChest(accessor)) {
        ItemCollector<Container> collector(std::make_unique<ContainerItemIterator>([ender](Accessor&) { return ender; }, 0));
        return collector.update(accessor);
    }
    try {
        return targetCollector(accessor)->update(accessor);
    } catch (std::exception const& e) {
        util::WailaExceptionHandler::handleErr(e, this);
        return std::nullopt;
    }
}

bool ItemStorageExtension::shouldRequestData(Accessor& accessor) {
    if (util::CommonProxy::isEnderChest(accessor) || util::CommonProxy::findContainer(accessor)) return true;
    return util::CommonProxy::hasDefaultItemStorage(accessor);
}

std::vector<ClientViewGroup<ItemView>> ItemStorageExtension::getClientGroups(Accessor&, ViewGroups<ItemData> const& groups) {
    return ClientViewGroup<ItemView>::map<ItemData>(groups, [](ItemData const& d) { return ItemView::fromData(d); });
}

void ItemStorageCommon::append(ITooltip& tooltip, Accessor& accessor, IPluginConfig& config) {
    CompoundTag const& data = accessor.getServerData();
    if (!data.contains(JadeIds::UNIVERSAL_ITEM_STORAGE)) {
        if (boolOf(data, "Loot")) tooltip.add(text::Component::translatable("jade.loot_not_generated"));
        else if (boolOf(data, "Locked")) tooltip.add(text::Component::translatable("jade.locked"));
        return;
    }
    std::vector<ClientViewGroup<ItemView>> groups;
    {
        auto const& v = data.at(JadeIds::UNIVERSAL_ITEM_STORAGE);
        if (!v.hold<CompoundTag>()) return;
        auto entry = view::decodeGroups<ItemData>(v.get<CompoundTag>());
        if (!entry) return;
        auto* provider = impl::WailaClientRegistration::instance().getItemStorageClient(entry->first);
        if (!provider) return;
        try {
            groups = provider->getClientGroups(accessor, entry->second);
        } catch (std::exception const& e) {
            util::WailaExceptionHandler::handleErr(e, provider, &tooltip);
            return;
        }
    }
    if (groups.empty()) return;

    bool showName    = true;
    int  amountWidth = 0;
    {
        int showNameAmount = config.getInt(JadeIds::UNIVERSAL_ITEM_STORAGE_SHOW_NAME_AMOUNT);
        int totalSize      = 0;
        for (auto const& group : groups) {
            for (auto const& view : group.views) {
                if (view.amountText) showName = false;
                if (!view.isEmpty()) {
                    ++totalSize;
                    if (totalSize == showNameAmount) showName = false;
                }
                if (showName) {
                    std::string s = DisplayHelper::INSTANCE.humanReadableNumber(view.count, "", false, false);
                    amountWidth   = std::max(amountWidth, util::JadeFont::width(s));
                }
            }
        }
    }

    bool renderGroup = groups.size() > 1 || groups.front().shouldRenderGroup();
    ClientViewGroup<ItemView>::tooltip(tooltip, groups, renderGroup, [&](ITooltip& theTooltip, ClientViewGroup<ItemView>& group) {
        if (renderGroup) {
            theTooltip.add(std::make_shared<impl::ui::HorizontalLineElement>());
            if (group.title) {
                auto t = JadeUIInternal::text(*group.title);
                t->scale(0.5f);
                theTooltip.append(t);
                auto line = std::make_shared<impl::ui::HorizontalLineElement>();
                line->flexGrow(1);
                theTooltip.append(line);
            }
        }
        if (group.views.empty() && group.extraData && group.extraData->contains("Collecting")) {
            float progress = -1;
            try {
                progress = static_cast<float>(group.extraData->at("Collecting"));
            } catch (...) {}
            if (progress >= 0 && progress < 1) {
                auto component = text::Component::translatable("jade.collectingItems");
                if (progress != 0) component.append(" " + std::to_string(static_cast<int>(progress * 100)) + "%");
                theTooltip.add(component);
            }
        }
        int drawnCount = 0;
        int realSize   = config.getInt(accessor.showDetails() ? JadeIds::UNIVERSAL_ITEM_STORAGE_DETAILED_AMOUNT : JadeIds::UNIVERSAL_ITEM_STORAGE_NORMAL_AMOUNT);
        realSize       = std::min(static_cast<int>(group.views.size()), realSize);
        int itemsPerLine = config.getInt(JadeIds::UNIVERSAL_ITEM_STORAGE_ITEMS_PER_LINE);
        std::vector<layout::LayoutElementPtr> elements;
        for (int i = 0; i < realSize; i++) {
            ItemView& itemView = group.views[static_cast<size_t>(i)];
            if (itemView.isEmpty()) continue;
            ItemStack const& stack = *itemView.item;
            if (i > 0 && (showName || drawnCount >= itemsPerLine)) {
                theTooltip.add(elements);
                theTooltip.setLineMargin(-1, ui::ScreenDirection::DOWN, 0);
                elements.clear();
                drawnCount = 0;
            }
            if (showName) {
                if (itemView.description) {
                    elements.push_back(JadeUIInternal::smallItem(stack));
                    for (auto const& d : *itemView.description) elements.push_back(d);
                } else {
                    auto smallEl = JadeUIInternal::smallItem(stack);
                    smallEl->refreshNarration();
                    elements.push_back(smallEl);
                    std::string s     = DisplayHelper::INSTANCE.humanReadableNumber(itemView.count, "", false, false);
                    int         width = util::JadeFont::width(s);
                    if (width < amountWidth) elements.push_back(JadeUIInternal::spacer(amountWidth - width, 0));
                    std::string name = text::stripFormatting(stack.getHoverName());
                    auto        t    = JadeUIInternal::text(text::Component::literal(s).append("× ").append(name));
                    t->narration("");
                    elements.push_back(t);
                }
            } else if (itemView.amountText) {
                elements.push_back(JadeUIInternal::item(stack, 1, *itemView.amountText));
            } else if (itemView.count != static_cast<int>(stack.mCount)) {
                elements.push_back(JadeUIInternal::item(stack, 1, DisplayHelper::INSTANCE.humanReadableNumber(itemView.count, "", false, false)));
            } else {
                elements.push_back(JadeUIInternal::item(stack));
            }
            drawnCount += 1;
        }
        if (!elements.empty()) theTooltip.add(elements);
    });
}

template <class T>
void ItemStorageProviderClient<T>::appendTooltip(ITooltip& tooltip, T& accessor, IPluginConfig& config) {
    if (util::CommonProxy::isFurnace(accessor)) return;
    ItemStorageCommon::append(tooltip, accessor, config);
}

template class ItemStorageProviderClient<BlockAccessor>;
template class ItemStorageProviderClient<EntityAccessor>;

}
