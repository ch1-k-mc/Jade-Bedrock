// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "api/ITooltip.h"
#include "api/ui/BoxStyle.h"
#include "api/ui/Element.h"
#include "api/ui/ProgressStyle.h"
#include "platform/text/Component.h"

class ItemStack;

namespace jade::api::view {
class ProgressView;
}

namespace jade::impl::ui {

class JadeUIInternal {
public:
    static Identifier const DEFAULT_PROGRESS;
    static Identifier const DEFAULT_PROGRESS_BASE;

    static bool isEmptyElement(api::ui::ElementPtr const& element) { return element == nullptr; }

    static std::shared_ptr<api::ui::TextElement>       text(text::Component component);
    static api::ui::ElementPtr                         item(ItemStack const& stack, float scale = 1, std::optional<std::string> text = std::nullopt);
    static api::ui::ElementPtr                         smallItem(ItemStack const& stack);
    static std::shared_ptr<api::ui::ResizeableElement> spacer(int width, int height);
    static api::ui::ElementPtr                         progressArrow(float progress);
    static std::shared_ptr<api::ui::ResizeableElement> progress(api::view::ProgressView view);
    static std::shared_ptr<api::ui::ResizeableElement> progress(api::view::ProgressView view, int width, int height);
    static std::shared_ptr<api::ui::ResizeableElement> progress(
        float                          progress,
        Identifier const&              baseSprite,
        Identifier const&              progressSprite,
        int                            width,
        int                            height,
        std::optional<text::Component> text,
        api::ui::ProgressStylePtr      style
    );
    static api::ui::ProgressStylePtr                   progressStyle();
    static std::shared_ptr<api::ui::BoxElement>        box(std::shared_ptr<api::ITooltip> tooltip, api::ui::BoxStyle const& boxStyle);
    static std::shared_ptr<api::ITooltip>              tooltip(api::ui::ElementPtr icon = nullptr);
    static std::shared_ptr<api::ui::ResizeableElement> sprite(Identifier sprite, int width, int height);
    static std::shared_ptr<api::ui::ResizeableElement> horizontalTiledSprite(Identifier sprite, int width, int height);
    static std::shared_ptr<api::ui::ResizeableElement> verticalTiledSprite(Identifier sprite, int width, int height);
    static std::shared_ptr<api::ui::ResizeableElement> offset(api::ui::ElementPtr element, int x, int y);
    static std::shared_ptr<api::ui::ResizeableElement> size(api::ui::ElementPtr element, int width, int height);

    static std::optional<Identifier> const& contextUid() { return sContextUid; }
    static void                             setContextUid(std::optional<Identifier> uid) { sContextUid = std::move(uid); }

    static void visitChildrenRecursive(layout::LayoutElementPtr const& element, std::function<void(layout::LayoutElementPtr const&)> const& consumer);

    static bool isPinned() { return false; }

private:
    static inline std::optional<Identifier> sContextUid;
};

}
