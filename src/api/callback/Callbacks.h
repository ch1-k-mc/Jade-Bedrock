// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "api/Accessor.h"
#include "api/ui/Element.h"
#include "api/ui/TooltipAnimation.h"

class ItemStack;

namespace jade::api::theme {
class Theme;
}

namespace jade::api::callback {

using AccessorPtr = std::shared_ptr<Accessor>;

using JadeAfterRenderCallback  = std::function<void(ui::BoxElement& root, ui::TooltipAnimation& animation, ui::GuiGraphics& graphics, Accessor& accessor)>;
using JadeBeforeRenderCallback = std::function<bool(ui::BoxElement& root, ui::TooltipAnimation& animation, ui::GuiGraphics& graphics, Accessor& accessor)>;
using JadeBeforeTooltipCollectCallback = std::function<bool(theme::Theme*& theme, Accessor& accessor)>;
using JadeItemModNameCallback          = std::function<std::string(ItemStack const& stack)>;
using JadeRayTraceCallback         = std::function<AccessorPtr(HitResult const& hitResult, AccessorPtr accessor, AccessorPtr originalAccessor)>;
using JadeTooltipCollectedCallback = std::function<void(ui::BoxElement& rootElement, Accessor& accessor)>;

template <class T>
class CallbackContainer {
public:
    void add(T callback) { add(0, std::move(callback)); }
    void add(int priority, T callback) {
        auto it = std::upper_bound(mCallbacks.begin(), mCallbacks.end(), priority, [](int p, auto const& e) { return p < e.first; });
        mCallbacks.insert(it, {priority, std::move(callback)});
    }
    template <class F>
    void call(F&& consumer) const {
        for (auto const& [_, cb] : mCallbacks) consumer(cb);
    }
    [[nodiscard]] std::vector<std::pair<int, T>> const& entries() const { return mCallbacks; }

private:
    std::vector<std::pair<int, T>> mCallbacks;
};

}
