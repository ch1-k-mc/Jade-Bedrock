// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/Tooltip.h"

#include <algorithm>

#include "impl/ui/JadeUIInternal.h"

namespace jade::api {

void ITooltip::add(int index, text::Component const& component, std::optional<Identifier> tag) {
    auto e = impl::ui::JadeUIInternal::text(component);
    e->tag(std::move(tag));
    add(index, std::static_pointer_cast<layout::LayoutElement>(e));
}

void ITooltip::add(int index, std::vector<LayoutElementPtr> const& elements) {
    bool first = true;
    for (auto const& e : elements) {
        if (first) add(index, e);
        else append(index, e);
        first = false;
    }
}

void ITooltip::append(text::Component const& component, std::optional<Identifier> tag) {
    auto e = impl::ui::JadeUIInternal::text(component);
    e->tag(std::move(tag));
    append(std::static_pointer_cast<layout::LayoutElement>(e));
}

}

namespace jade::impl {

namespace {
std::optional<Identifier> tagOf(layout::LayoutElementPtr const& e) {
    if (auto* t = dynamic_cast<api::ui::Element*>(e.get())) return t->getTag();
    return std::nullopt;
}
}

void Tooltip::clear() {
    lines.clear();
    isDirty = true;
}

void Tooltip::append(int index, layout::LayoutElementPtr element) {
    if (auto* t = dynamic_cast<api::ui::Element*>(element.get()); t && !t->getTag()) t->tag(ui::JadeUIInternal::contextUid());
    if (isEmpty() || index == size()) {
        add(element);
    } else {
        lines.at(static_cast<size_t>(index)).elements.push_back(std::move(element));
    }
    isDirty = true;
}

void Tooltip::add(int index, layout::LayoutElementPtr element) {
    lines.insert(lines.begin() + index, Line{});
    append(index, std::move(element));
}

std::vector<layout::LayoutElementPtr> Tooltip::get(Identifier const& tag) {
    std::vector<layout::LayoutElementPtr> out;
    for (auto const& line : lines)
        for (auto const& e : line.elements)
            if (tagOf(e) == tag) out.push_back(e);
    return out;
}

bool Tooltip::remove(Identifier const& tag) { return removeInternal(tag, true, nullptr); }

bool Tooltip::removeInternal(Identifier const& tag, bool removeFirstLineIfEmpty, api::ElementLines* collector) {
    bool removed = false;
    for (auto it = lines.begin(); it != lines.end();) {
        std::vector<layout::LayoutElementPtr> collected;
        auto&                                 els = it->elements;
        auto                                  end = std::remove_if(els.begin(), els.end(), [&](layout::LayoutElementPtr const& e) {
            if (tagOf(e) == tag) {
                if (collector) collected.push_back(e);
                return true;
            }
            return false;
        });
        bool any = end != els.end();
        els.erase(end, els.end());
        if (any) {
            bool erase = els.empty() && (removed || removeFirstLineIfEmpty);
            removed    = true;
            if (collector && !collected.empty()) collector->push_back(collected);
            if (erase) {
                it = lines.erase(it);
                continue;
            }
        }
        ++it;
    }
    return isDirty = removed;
}

bool Tooltip::replace(Identifier const& tag, text::Component const& component) {
    return isDirty = replace(tag, [&component](api::ElementLines) {
               return api::ElementLines{{std::static_pointer_cast<layout::LayoutElement>(ui::JadeUIInternal::text(component))}};
           });
}

bool Tooltip::replace(Identifier const& tag, std::function<api::ElementLines(api::ElementLines)> fn) {
    int firstX = -1, firstY = -1;
    for (int y = 0; y < static_cast<int>(lines.size()) && firstX == -1; ++y) {
        auto const& line = lines[y];
        for (int x = 0; x < static_cast<int>(line.elements.size()); ++x) {
            if (tagOf(line.elements[x]) == tag) {
                firstX = x;
                firstY = y;
                break;
            }
        }
    }
    if (firstX == -1) return false;
    api::ElementLines elements;
    removeInternal(tag, false, &elements);
    elements = fn(std::move(elements));
    for (auto const& list : elements)
        for (auto const& e : list)
            if (auto* t = dynamic_cast<api::ui::Element*>(e.get()); t && !t->getTag()) t->tag(tag);
    for (size_t i = 0; i < elements.size(); ++i) {
        auto const& list = elements[i];
        if (i == 0) {
            auto& line = lines[static_cast<size_t>(firstY)];
            line.elements.insert(line.elements.begin() + firstX, list.begin(), list.end());
        } else {
            add(firstY + static_cast<int>(i), list);
        }
    }
    return true;
}

void Tooltip::setLineMargin(int index, api::ui::ScreenDirection side, int margin) {
    if (index < 0) index += size();
    auto& line = lines.at(static_cast<size_t>(index));
    if (side == api::ui::ScreenDirection::UP) line.marginTop = margin;
    else if (side == api::ui::ScreenDirection::DOWN) line.marginBottom = margin;
    isDirty = true;
}

void Tooltip::setLineSettings(int index, layout::LayoutSettingsOperator settings) {
    if (index < 0) index += size();
    lines.at(static_cast<size_t>(index)).settings = std::move(settings);
    isDirty                                       = true;
}

std::string Tooltip::getNarration() {
    std::string sb;
    for (auto const& line : lines) {
        bool hasSupplier = false;
        for (auto const& e : line.elements) {
            if (auto* el = dynamic_cast<api::ui::Element*>(e.get())) {
                auto n = el->cachedNarration();
                if (!n.empty()) sb += n + ". ";
                hasSupplier = true;
            }
        }
        if (hasSupplier && !sb.empty()) sb.push_back('\n');
    }
    if (!sb.empty() && sb.back() == '\n') sb.pop_back();
    return sb;
}

std::string Tooltip::getString(Identifier const& tag) {
    for (auto const& e : get(tag))
        if (auto* t = dynamic_cast<api::ui::TextElement*>(e.get())) return t->getString();
    return "";
}

void Tooltip::setIcon(api::ui::ElementPtr i) {
    icon    = std::move(i);
    isDirty = true;
}

}
