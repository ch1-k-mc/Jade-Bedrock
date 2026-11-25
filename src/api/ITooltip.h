// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "api/JadeIds.h"
#include "api/ui/Element.h"
#include "platform/layout/Layout.h"
#include "platform/text/Component.h"

namespace jade::api {

using layout::LayoutElementPtr;
using ElementLines = std::vector<std::vector<LayoutElementPtr>>;

class ITooltip {
public:
    virtual ~ITooltip() = default;

    virtual void clear()      = 0;
    virtual int  size() const = 0;
    bool         isEmpty() const { return size() == 0; }

    void add(text::Component const& component, std::optional<Identifier> tag = std::nullopt) { add(size(), component, std::move(tag)); }
    void add(int index, text::Component const& component, std::optional<Identifier> tag = std::nullopt);
    void addAll(std::vector<text::Component> const& components) {
        for (auto const& c : components) add(c);
    }
    void         add(LayoutElementPtr element) { add(size(), std::move(element)); }
    void         add(int index, std::vector<LayoutElementPtr> const& elements);
    void         add(std::vector<LayoutElementPtr> const& elements) { add(size(), elements); }
    virtual void add(int index, LayoutElementPtr element) = 0;

    void append(text::Component const& component, std::optional<Identifier> tag = std::nullopt);
    void append(LayoutElementPtr element) { append(size() - 1, std::move(element)); }
    void append(int index, std::vector<LayoutElementPtr> const& elements) {
        for (auto const& e : elements) append(index, e);
    }
    virtual void append(int index, LayoutElementPtr element) = 0;

    virtual bool                          remove(Identifier const& tag)                                                 = 0;
    virtual bool                          replace(Identifier const& tag, std::function<ElementLines(ElementLines)> fn) = 0;
    virtual bool                          replace(Identifier const& tag, text::Component const& component)              = 0;
    virtual std::vector<LayoutElementPtr> get(Identifier const& tag)                                                    = 0;
    virtual void                          setLineMargin(int index, ui::ScreenDirection side, int margin)                = 0;
    virtual void                          setLineSettings(int index, layout::LayoutSettingsOperator settings)           = 0;
    virtual std::string                   getNarration()                                                                = 0;
    virtual std::string                   getString(Identifier const& tag)                                              = 0;
    virtual ui::ElementPtr                getIcon()                                                                     = 0;
};

}
