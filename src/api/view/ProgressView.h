// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "mc/deps/nbt/CompoundTag.h"

#include "api/ui/BoxStyle.h"
#include "api/ui/Element.h"
#include "api/ui/ProgressStyle.h"
#include "api/view/ViewGroup.h"
#include "platform/text/Component.h"

namespace jade::api::view {

class ProgressView {
public:
    struct Part {
        int                            id       = 0;
        float                          progress = 0;
        float                          speed    = NAN;
        float                          target   = NAN;
        ui::ElementPtr                 overlay;
        std::optional<ui::MessageType> messageType;
        int                            color = -1;

        Part() = default;
        Part(int id, float progress, float speed, float target, ui::ElementPtr overlay, std::optional<ui::MessageType> messageType, int color)
        : id(id),
          progress(progress),
          speed(speed),
          target(target),
          overlay(std::move(overlay)),
          messageType(messageType),
          color(color) {
            if (!(progress >= 0 && progress <= 1)) throw std::invalid_argument("Progress must be between 0 and 1, got: " + std::to_string(progress));
        }

        static Part of(float progress) { return of(0, progress); }
        static Part of(int id, float progress) { return of(id, progress, ui::MessageType::INFO); }
        static Part of(float progress, ui::MessageType type) { return of(0, progress, type); }
        static Part of(int id, float progress, ui::MessageType type) { return Part(id, progress, NAN, NAN, nullptr, type, -1); }
        static Part of(float progress, ui::ElementPtr overlay) { return Part(0, progress, NAN, NAN, std::move(overlay), std::nullopt, -1); }
        static Part ofColor(float progress, int color) { return Part(0, progress, NAN, NAN, nullptr, std::nullopt, color); }

        [[nodiscard]] int  themeColor() const;
        [[nodiscard]] bool targetDefined() const { return !std::isnan(target) && !std::isnan(speed); }
    };

    class PartBuilder {
    public:
        PartBuilder& id(int v) {
            mId = v;
            return *this;
        }
        PartBuilder& progress(float v) {
            mProgress = v;
            return *this;
        }
        PartBuilder& target(float speed, float target) {
            mSpeed  = speed;
            mTarget = target;
            return *this;
        }
        PartBuilder& overlay(ui::ElementPtr v) {
            mOverlay = std::move(v);
            return *this;
        }
        PartBuilder& messageType(ui::MessageType v) {
            mMessageType = v;
            return *this;
        }
        PartBuilder& color(int v) {
            mColor = v;
            return *this;
        }
        [[nodiscard]] Part build() const { return Part(mId, mProgress, mSpeed, mTarget, mOverlay, mMessageType, mColor); }

    private:
        int                             mId       = 0;
        float                           mProgress = 0;
        float                           mSpeed    = 0;
        float                           mTarget   = 0;
        ui::ElementPtr                  mOverlay;
        std::optional<ui::MessageType>  mMessageType;
        int                             mColor = -1;
    };

    struct Data {
        float           progress    = 0;
        float           speed       = NAN;
        float           target      = NAN;
        ui::MessageType messageType = ui::MessageType::INFO;
    };

    ProgressView(ui::ProgressStylePtr style, ui::BoxStyle boxStyle) : style(std::move(style)), boxStyle(std::move(boxStyle)) {
        if (!this->style) throw std::invalid_argument("style is null");
    }
    ProgressView(Part progress, std::optional<text::Component> text, ui::ProgressStylePtr style, ui::BoxStyle boxStyle)
    : ProgressView(std::vector<Part>{std::move(progress)}, std::move(text), std::move(style), std::move(boxStyle)) {}
    ProgressView(std::vector<Part> progress, std::optional<text::Component> text, ui::ProgressStylePtr style, ui::BoxStyle boxStyle)
    : ProgressView(std::move(style), std::move(boxStyle)) {
        parts      = std::move(progress);
        this->text = std::move(text);
    }

    static ProgressView read(Data const& data);

    std::vector<Part>              parts;
    ui::ProgressStylePtr           style;
    ui::BoxStyle                   boxStyle;
    std::optional<text::Component> text;
};

template <>
struct ViewCodec<ProgressView::Data> {
    static void encode(CompoundTag& out, ProgressView::Data const& v) {
        out["progress"] = v.progress;
        out["speed"]    = v.speed;
        out["target"]   = v.target;
        out["type"]     = static_cast<int>(v.messageType);
    }
    static std::optional<ProgressView::Data> decode(CompoundTag const& in) {
        try {
            ProgressView::Data d;
            d.progress    = static_cast<float>(in.at("progress"));
            d.speed       = static_cast<float>(in.at("speed"));
            d.target      = static_cast<float>(in.at("target"));
            d.messageType = static_cast<ui::MessageType>(static_cast<int>(in.at("type")));
            return d;
        } catch (...) {
            return std::nullopt;
        }
    }
};

}
