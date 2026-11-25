// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "addon/universal/FluidStorageProvider.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/biome/biome_color_sampling/BiomeColorSampling.h"

#include "api/theme/IThemeHelper.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/item/ScopedItemRegistry.h"
#include "util/CommonProxy.h"
#include "util/ViewProxy.h"

namespace jade::addon::universal {

using impl::ui::JadeUIInternal;
using view::ClientViewGroup;
using view::FluidView;
using view::ProgressView;

std::shared_ptr<FluidStorageExtension> const FluidStorageExtension::INSTANCE = std::make_shared<FluidStorageExtension>();

namespace {

int intOf(CompoundTag const& tag, std::string const& key) {
    if (!tag.contains(key)) return 0;
    try {
        return static_cast<int>(tag.at(key));
    } catch (...) {
        return 0;
    }
}

template <class V>
void header(ITooltip& theTooltip, ClientViewGroup<V>& group, bool renderGroup) {
    if (renderGroup) group.renderHeader(theTooltip);
}

}

void FluidStorageCommon::appendServerData(Accessor& accessor) {
    auto entry = util::CommonProxy::getServerExtensionData(accessor, impl::WailaCommonRegistration::instance().fluidStorageProviders);
    if (entry) accessor.getServerData()[JadeIds::UNIVERSAL_FLUID_STORAGE] = view::encodeGroups(entry->first, entry->second);
}

bool FluidStorageCommon::shouldRequestData(Accessor& accessor) {
    if (!accessor.showDetails() && config::IWailaConfig::get().plugin().get(JadeIds::UNIVERSAL_FLUID_STORAGE_DETAILED)) return false;
    return impl::WailaCommonRegistration::instance().fluidStorageProviders.hitsAny(accessor, [](auto& p, Accessor& a) { return p.shouldRequestData(a); });
}

std::optional<view::ViewGroups<FluidView::Data>> FluidStorageExtension::getGroups(Accessor& accessor) { return util::CommonProxy::wrapFluidStorage(accessor); }

bool FluidStorageExtension::shouldRequestData(Accessor& accessor) { return util::CommonProxy::hasDefaultFluidStorage(accessor); }

std::vector<ClientViewGroup<FluidView>> FluidStorageExtension::getClientGroups(Accessor& accessor, view::ViewGroups<FluidView::Data> const& groups) {
    std::optional<int> waterColor;
    if (accessor.getAccessorType() == AccessorType::BLOCK) {
        auto&    a   = static_cast<BlockAccessor&>(accessor);
        BlockPos pos = a.getPosition();
        waterColor   = BiomeColorSampling::getWaterColor(a.getRegion().getBiome(pos), pos);
    }
    return ClientViewGroup<FluidView>::map<FluidView::Data>(groups, [&](FluidView::Data const& d) {
        FluidView::Data copy = d;
        for (auto& f : copy.fluids)
            if (f.is("minecraft:water") && f.color == -1 && waterColor) f.color = *waterColor;
        return FluidView::readDefault(copy);
    });
}

void FluidStorageCommon::append(ITooltip& tooltip, Accessor& accessor, IPluginConfig& config) {
    if (!accessor.showDetails() && config.get(JadeIds::UNIVERSAL_FLUID_STORAGE_DETAILED)) return;
    auto groups = util::mapToClientGroups<FluidView::Data, FluidView>(
        accessor,
        JadeIds::UNIVERSAL_FLUID_STORAGE,
        [](Identifier const& uid) { return impl::WailaClientRegistration::instance().getFluidStorageClient(uid); },
        tooltip
    );
    if (!groups || groups->empty()) return;
    auto& themes      = theme::IThemeHelper::get();
    bool  renderGroup = groups->size() > 1 || groups->front().shouldRenderGroup();
    ClientViewGroup<FluidView>::tooltip(tooltip, *groups, renderGroup, [&](ITooltip& theTooltip, ClientViewGroup<FluidView>& group) {
        header(theTooltip, group, renderGroup);
        auto style = config.getEnum<config::HandlerDisplayStyle>(JadeIds::UNIVERSAL_FLUID_STORAGE_STYLE);
        for (auto& view : group.views) {
            text::Component text;
            if (view.overrideText) {
                text = *view.overrideText;
            } else if (!view.fluidName) {
                text = themes.info(view.current);
            } else {
                text::Component fluidAmount;
                text::Component fluidName = themes.info(view.fluidName->stripColor());
                if (accessor.showDetails() || style != config::HandlerDisplayStyle::PROGRESS_BAR) {
                    fluidAmount = text::Component::translatable("jade.fluid.with_capacity", std::vector<text::Component>{themes.info(view.current), view.max});
                } else {
                    fluidAmount = themes.info(view.current);
                }
                std::string key = style == config::HandlerDisplayStyle::PLAIN_TEXT ? "jade.fluid.text" : "jade.fluid";
                text            = text::Component::translatable(key, std::vector<text::Component>{fluidName, fluidAmount});
            }
            switch (style) {
            case config::HandlerDisplayStyle::PLAIN_TEXT:
                theTooltip.add(text);
                break;
            case config::HandlerDisplayStyle::ICON: {
                platform::item::ScopedItemRegistry scoped;
                ItemStack                          bucket;
                bucket.reinit("minecraft:bucket", 1, 0);
                theTooltip.add(JadeUIInternal::smallItem(bucket));
                theTooltip.append(text);
                break;
            }
            case config::HandlerDisplayStyle::PROGRESS_BAR: {
                auto progressStyle = JadeUIInternal::progressStyle();
                progressStyle->canDecrease(true);
                ProgressView progressView(ProgressView::Part::of(view.ratio, view.overlay), text, progressStyle, ui::BoxStyle::nestedBox());
                theTooltip.add(JadeUIInternal::progress(std::move(progressView)));
                break;
            }
            }
        }
        if (group.extraData) {
            int extra = intOf(*group.extraData, "+");
            if (extra > 0) theTooltip.add(text::Component::translatable("jade.fluid.more_tanks", std::vector<std::string>{std::to_string(extra)}));
        }
    });
}

void ProgressCommon::appendServerData(Accessor& accessor) {
    auto entry = util::CommonProxy::getServerExtensionData(accessor, impl::WailaCommonRegistration::instance().progressProviders);
    if (entry) accessor.getServerData()[JadeIds::UNIVERSAL_PROGRESS] = view::encodeGroups(entry->first, entry->second);
}

bool ProgressCommon::shouldRequestData(Accessor& accessor) {
    return impl::WailaCommonRegistration::instance().progressProviders.hitsAny(accessor, [](auto& p, Accessor& a) { return p.shouldRequestData(a); });
}

void ProgressCommon::append(ITooltip& tooltip, Accessor& accessor, IPluginConfig&) {
    auto groups = util::mapToClientGroups<ProgressView::Data, ProgressView>(
        accessor,
        JadeIds::UNIVERSAL_PROGRESS,
        [](Identifier const& uid) { return impl::WailaClientRegistration::instance().getProgressClient(uid); },
        tooltip
    );
    if (!groups || groups->empty()) return;
    bool renderGroup = groups->size() > 1 || groups->front().shouldRenderGroup();
    ClientViewGroup<ProgressView>::tooltip(tooltip, *groups, renderGroup, [&](ITooltip& theTooltip, ClientViewGroup<ProgressView>& group) {
        header(theTooltip, group, renderGroup);
        for (auto& view : group.views) {
            if (view.text) {
                auto t = JadeUIInternal::text(*view.text);
                t->scale(0.75f);
                theTooltip.add(t);
                theTooltip.setLineMargin(-1, ui::ScreenDirection::DOWN, 0);
                view.text.reset();
            }
            theTooltip.add(JadeUIInternal::progress(view));
        }
    });
}

}
