// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/ui/JadeUIInternal.h"

#include "api/view/ProgressView.h"
#include "impl/ui/ProgressElement.h"

#include <set>

#include "impl/Tooltip.h"
#include "impl/ui/BoxElementImpl.h"
#include "impl/ui/Elements.h"
#include "util/JadeFont.h"

namespace jade::impl::ui {

Identifier const JadeUIInternal::DEFAULT_PROGRESS      = JadeIds::JADE("progress");
Identifier const JadeUIInternal::DEFAULT_PROGRESS_BASE = JadeIds::JADE("progress_base");

std::shared_ptr<api::ui::TextElement> JadeUIInternal::text(text::Component component) {
    return std::make_shared<TextElementImpl>(std::move(component));
}

api::ui::ElementPtr JadeUIInternal::item(ItemStack const& stack, float scale, std::optional<std::string> text) {
    return ItemStackElement::of(stack, scale, std::move(text));
}

api::ui::ElementPtr JadeUIInternal::smallItem(ItemStack const& stack) {
    int  lineHeight = util::JadeFont::lineHeight;
    auto e          = item(stack, 0.5f, std::string());
    auto sized      = std::static_pointer_cast<SpacerElement>(size(e, lineHeight + 1, lineHeight - 1));
    sized->offsetWrapped(0, -1);
    sized->narration("");
    return sized;
}

api::ui::ElementPtr JadeUIInternal::progressArrow(float p) {
    return progress(p, DEFAULT_PROGRESS_BASE, DEFAULT_PROGRESS, 22, 16, std::nullopt, nullptr);
}

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::progress(api::view::ProgressView view) { return std::make_shared<ProgressElement>(std::move(view)); }

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::progress(api::view::ProgressView view, int width, int height) {
    return std::make_shared<ProgressElement>(std::move(view), width, height);
}

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::progress(
    float                          p,
    Identifier const&              baseSprite,
    Identifier const&              progressSprite,
    int                            width,
    int                            height,
    std::optional<text::Component> text,
    api::ui::ProgressStylePtr      style
) {
    if (!style) {
        style = progressStyle();
        style->fitContentX(false).fitContentY(false);
    }
    return progress(
        api::view::ProgressView(api::view::ProgressView::Part::of(p, sprite(progressSprite, width, height)), std::move(text), std::move(style), api::ui::BoxStyle::sprite(baseSprite, std::nullopt, 0)),
        width,
        height
    );
}

api::ui::ProgressStylePtr JadeUIInternal::progressStyle() { return std::make_shared<api::ui::SimpleProgressStyle>(); }

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::spacer(int width, int height) { return std::make_shared<SpacerElement>(width, height); }

std::shared_ptr<api::ui::BoxElement> JadeUIInternal::box(std::shared_ptr<api::ITooltip> tooltip, api::ui::BoxStyle const& boxStyle) {
    return std::make_shared<BoxElementImpl>(std::static_pointer_cast<Tooltip>(std::move(tooltip)), boxStyle);
}

std::shared_ptr<api::ITooltip> JadeUIInternal::tooltip(api::ui::ElementPtr icon) {
    auto t = std::make_shared<Tooltip>();
    if (icon) t->setIcon(std::move(icon));
    return t;
}

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::sprite(Identifier sprite, int width, int height) {
    return std::make_shared<SpriteElement>(std::move(sprite), width, height);
}

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::horizontalTiledSprite(Identifier s, int width, int height) {
    auto e              = std::make_shared<SpriteElement>(std::move(s), width, height);
    e->tiledOrientation = api::ui::Orientation::HORIZONTAL;
    return e;
}

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::verticalTiledSprite(Identifier s, int width, int height) {
    auto e              = std::make_shared<SpriteElement>(std::move(s), width, height);
    e->tiledOrientation = api::ui::Orientation::VERTICAL;
    return e;
}

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::offset(api::ui::ElementPtr element, int x, int y) {
    auto s = std::make_shared<SpacerElement>(element->getWidth(), element->getHeight());
    s->wrapped(element);
    s->offsetWrapped(x, y);
    return s;
}

std::shared_ptr<api::ui::ResizeableElement> JadeUIInternal::size(api::ui::ElementPtr element, int width, int height) {
    auto s = std::make_shared<SpacerElement>(width, height);
    s->wrapped(element);
    return s;
}

namespace {
void visitInternal(
    layout::LayoutElementPtr const&                              e,
    std::function<void(layout::LayoutElementPtr const&)> const& consumer,
    std::set<layout::LayoutElement*>&                           seen
) {
    if (!e || !seen.insert(e.get()).second) return;
    consumer(e);
    if (dynamic_cast<api::ui::BoxElement*>(e.get())) return;
    if (auto* layout = dynamic_cast<layout::Layout*>(e.get())) {
        layout->visitChildren([&](layout::LayoutElementPtr const& c) { visitInternal(c, consumer, seen); });
    }
}
}

void JadeUIInternal::visitChildrenRecursive(layout::LayoutElementPtr const& element, std::function<void(layout::LayoutElementPtr const&)> const& consumer) {
    std::set<layout::LayoutElement*> seen;
    visitInternal(element, consumer, seen);
}

}
