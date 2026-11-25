// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "api/ui/BoxStyle.h"
#include "api/ui/Element.h"
#include "api/ui/TooltipAnimation.h"
#include "gui/JadeLinearLayout.h"
#include "impl/Tooltip.h"

namespace jade::api {
class Accessor;
}

namespace jade::track {
class BoxProgressFadeTrackInfo;
}

namespace jade::impl::ui {

class BoxElementImpl : public api::ui::BoxElement {
public:
    BoxElementImpl(std::shared_ptr<Tooltip> tooltip, api::ui::BoxStyle style);

    void updateSize() override;
    void setX(int x) override;
    void setY(int y) override;
    void render(api::ui::GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) override;
    void renderDebug(api::ui::GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks, RenderDebugContext const& context) override;

    api::ITooltip&      getTooltip() override { return *mTooltip; }
    Tooltip&            tooltip() { return *mTooltip; }
    void                setBoxProgress(api::ui::MessageType type, float progress) override;
    void                clearBoxProgress() override;
    void                addProgressProvider(int priority, std::shared_ptr<api::ui::IBoxProgressProvider> provider) override;
    api::ui::ElementPtr getIcon() override { return mIcon; }
    void                setIcon(api::ui::ElementPtr icon) override { mIcon = std::move(icon); }
    api::ui::BoxStyle&  getStyle() override { return mStyle; }
    std::string         getNarration() override;
    void                setFreeSpace(int width, int height) override;

    void beforeRender(api::Accessor* accessor, float partialTicks);
    void updateExpectedRect(api::ui::TooltipAnimation& animation);
    void updateRect(api::ui::TooltipAnimation& animation);

    std::shared_ptr<gui::LayoutWithPadding> layout;

public:
    [[nodiscard]] std::optional<api::ui::BoxProgress> const& pendingProgress() const { return mPendingProgress; }

private:
    void computeBoxProgress(api::Accessor& accessor, float partialTicks);
    std::shared_ptr<track::BoxProgressFadeTrackInfo> getFadeTrack(bool create);
    void drawBoxProgressBar(api::ui::GuiGraphics& graphics, api::ui::BoxProgress const& progress);

    struct PrioritizedProvider {
        int                                            priority;
        std::shared_ptr<api::ui::IBoxProgressProvider> provider;
        bool                                           isStatic;
    };

    std::shared_ptr<Tooltip>             mTooltip;
    api::ui::BoxStyle                    mStyle;
    std::vector<api::ui::Element*>       mRenderables;
    api::ui::ElementPtr                  mIcon;
    std::vector<PrioritizedProvider>     mProviders;
    std::optional<api::ui::BoxProgress>  mPendingProgress;
    std::optional<api::ui::BoxProgress>  mFadeProgress;
    float                                mFadeAlpha = 0;
    std::shared_ptr<track::BoxProgressFadeTrackInfo> mFadeTrack;
};

}
