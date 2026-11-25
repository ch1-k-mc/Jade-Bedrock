// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "mc/deps/nbt/CompoundTag.h"

#include "api/ITooltip.h"
#include "api/ui/BoxStyle.h"
#include "api/ui/Element.h"
#include "api/view/ViewGroup.h"
#include "impl/ui/Elements.h"
#include "impl/ui/JadeUIInternal.h"
#include "platform/text/Component.h"

namespace jade::api::view {

template <class T>
class ClientViewGroup {
public:
    explicit ClientViewGroup(std::vector<T> views) : views(std::move(views)) {}

    std::vector<T>                 views;
    std::optional<text::Component> title;
    ui::MessageType                messageType = ui::MessageType::NORMAL;
    float                          boxProgress = 0;
    std::optional<CompoundTag>     extraData;

    template <class In>
    static std::vector<ClientViewGroup<T>> map(
        ViewGroups<In> const&                                                   groups,
        std::function<std::optional<T>(In const&)> const&                       itemFactory,
        std::function<void(ViewGroup<In> const&, ClientViewGroup<T>&)> const&   clientGroupDecorator = nullptr
    ) {
        std::vector<ClientViewGroup<T>> out;
        for (auto const& g : groups) {
            std::vector<T> views;
            for (auto const& v : g.views)
                if (auto o = itemFactory(v)) views.push_back(std::move(*o));
            ClientViewGroup<T> group(std::move(views));
            if (g.extraData) {
                auto const& data = *g.extraData;
                if (data.contains("Progress")) {
                    try {
                        group.boxProgress = static_cast<float>(data.at("Progress"));
                    } catch (...) {}
                }
                if (data.contains("MessageType") && data.at("MessageType").template hold<StringTag>()) {
                    group.messageType = parseMessageType(data.at("MessageType").template get<StringTag>());
                }
            }
            if (clientGroupDecorator) clientGroupDecorator(g, group);
            group.extraData = g.extraData;
            out.push_back(std::move(group));
        }
        return out;
    }

    static void tooltip(
        ITooltip&                                                    tooltip,
        std::vector<ClientViewGroup<T>>&                             groups,
        bool                                                         renderGroup,
        std::function<void(ITooltip&, ClientViewGroup<T>&)> const&   consumer
    ) {
        for (auto& group : groups) {
            std::shared_ptr<ITooltip> sub = renderGroup ? impl::ui::JadeUIInternal::tooltip() : nullptr;
            ITooltip&                 the = renderGroup ? *sub : tooltip;
            consumer(the, group);
            if (renderGroup) {
                ui::BoxStyle boxStyle = ui::BoxStyle::viewGroup();
                auto         box      = impl::ui::JadeUIInternal::box(sub, boxStyle);
                box->flexGrow(1);
                tooltip.add(box);
            }
        }
    }

    [[nodiscard]] bool shouldRenderGroup() const { return title.has_value() || boxProgress > 0; }

    void renderHeader(ITooltip& tooltip) {
        if (title) {
            tooltip.add(std::make_shared<impl::ui::HorizontalLineElement>());
            auto t = impl::ui::JadeUIInternal::text(*title);
            t->scale(0.5f);
            tooltip.append(t);
            auto line = std::make_shared<impl::ui::HorizontalLineElement>();
            line->flexGrow(1);
            tooltip.append(line);
        }
    }

    static ui::MessageType parseMessageType(std::string const& s) {
        using M = ui::MessageType;
        if (s == "INFO") return M::INFO;
        if (s == "TITLE") return M::TITLE;
        if (s == "SUCCESS") return M::SUCCESS;
        if (s == "WARNING") return M::WARNING;
        if (s == "DANGER") return M::DANGER;
        if (s == "FAILURE") return M::FAILURE;
        return M::NORMAL;
    }
};

}
