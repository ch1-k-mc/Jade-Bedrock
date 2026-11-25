// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>

#include "api/AccessorClientHandler.h"
#include "track/ProgressTracker.h"

class CompoundTag;

namespace jade::impl::ui {
class BoxElementImpl;
}

namespace jade::overlay {

class WailaTickHandler {
public:
    static WailaTickHandler& instance();

    struct State {
        std::shared_ptr<api::Accessor> accessor;
        api::AccessorClientHandler*    handler = nullptr;
        std::shared_ptr<CompoundTag>   data;

        static State   create(std::optional<State> const& prev, std::shared_ptr<api::Accessor> accessor, api::AccessorClientHandler& handler,
                              std::shared_ptr<CompoundTag> data);
        api::ui::ElementPtr getIcon() const;
        bool                verifyData(CompoundTag const* data) const;
    };

    void clearState();
    void tickClient();

    void         setData(std::shared_ptr<CompoundTag> tag);
    CompoundTag* getData() const { return state ? state->data.get() : nullptr; }

    track::ProgressTracker                    progressTracker;
    std::shared_ptr<impl::ui::BoxElementImpl> rootElement;
    std::optional<State>                      state;

private:
    std::shared_ptr<api::Accessor> createEmpty(api::HitResult const& hit);
};

}
