// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <vector>

#include "api/ITooltip.h"

namespace jade::impl {

class Tooltip : public api::ITooltip {
public:
    struct Line {
        std::vector<layout::LayoutElementPtr> elements;
        int                                   marginTop    = 0;
        int                                   marginBottom = 2;
        layout::LayoutSettingsOperator        settings;
    };

    void clear() override;
    int  size() const override { return static_cast<int>(lines.size()); }
    using ITooltip::add;
    using ITooltip::append;
    void                                  add(int index, layout::LayoutElementPtr element) override;
    void                                  append(int index, layout::LayoutElementPtr element) override;
    bool                                  remove(Identifier const& tag) override;
    bool                                  replace(Identifier const& tag, std::function<api::ElementLines(api::ElementLines)> fn) override;
    bool                                  replace(Identifier const& tag, text::Component const& component) override;
    std::vector<layout::LayoutElementPtr> get(Identifier const& tag) override;
    void                                  setLineMargin(int index, api::ui::ScreenDirection side, int margin) override;
    void                                  setLineSettings(int index, layout::LayoutSettingsOperator settings) override;
    std::string                           getNarration() override;
    std::string                           getString(Identifier const& tag) override;
    api::ui::ElementPtr                   getIcon() override { return icon; }
    void                                  setIcon(api::ui::ElementPtr icon);

    std::vector<Line>   lines;
    bool                sneakyDetails = false;
    api::ui::ElementPtr icon;
    bool                isDirty = false;

private:
    bool removeInternal(Identifier const& tag, bool removeFirstLineIfEmpty, api::ElementLines* collector);
};

}
