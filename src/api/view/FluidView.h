// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/ListTag.h"

#include "api/ui/Element.h"
#include "api/view/ViewGroup.h"
#include "platform/text/Component.h"

namespace jade::api::view {

struct JadeFluidObject {
    std::string type;
    int64_t     amount     = 0;
    int         color      = -1;
    int         potionId   = -1;
    int         potionType = 0;

    static constexpr int64_t bucketVolume() { return 81000; }
    static JadeFluidObject   empty() { return {}; }

    [[nodiscard]] bool            isEmpty() const { return type.empty() || amount <= 0; }
    [[nodiscard]] bool            is(std::string const& t) const { return type == t; }
    [[nodiscard]] text::Component getDisplayName() const;
    [[nodiscard]] std::string     texture() const;
    [[nodiscard]] uint32_t        tint() const;

    void                                  encode(CompoundTag& out) const;
    static std::optional<JadeFluidObject> decode(CompoundTag const& in);
};

class FluidView {
public:
    struct Data {
        std::vector<JadeFluidObject> fluids;
        int64_t                      capacity = 0;
    };

    FluidView(ui::ElementPtr overlay, text::Component current, text::Component max)
    : overlay(std::move(overlay)),
      current(std::move(current)),
      max(std::move(max)) {}

    static text::Component            emptyFluid() { return text::Component::translatable("jade.fluid.empty"); }
    static std::optional<FluidView>   readDefault(Data const& data);

    ui::ElementPtr                 overlay;
    text::Component                current;
    text::Component                max;
    float                          ratio = 0;
    std::optional<text::Component> fluidName;
    std::optional<text::Component> overrideText;
};

template <>
struct ViewCodec<FluidView::Data> {
    static void encode(CompoundTag& out, FluidView::Data const& v) {
        ListTag list;
        for (auto const& f : v.fluids) {
            CompoundTag t;
            f.encode(t);
            list.add(std::make_unique<CompoundTag>(std::move(t)));
        }
        out["fluids"]   = std::move(list);
        out["capacity"] = static_cast<int64>(v.capacity);
    }
    static std::optional<FluidView::Data> decode(CompoundTag const& in) {
        try {
            FluidView::Data d;
            d.capacity = static_cast<int64>(in.at("capacity"));
            if (in.contains("fluids") && in.at("fluids").hold<ListTag>()) {
                for (auto const& p : in.at("fluids").get<ListTag>()) {
                    if (!p.hold<CompoundTag>()) continue;
                    if (auto f = JadeFluidObject::decode(p.get<CompoundTag>())) d.fluids.push_back(std::move(*f));
                }
            }
            return d;
        } catch (...) {
            return std::nullopt;
        }
    }
};

}
