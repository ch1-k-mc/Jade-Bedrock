// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "impl/ui/BoxElementImpl.h"

#include <climits>
#include <stdexcept>

#include "api/view/ProgressView.h"
#include "overlay/OverlayRenderer.h"
#include "overlay/WailaTickHandler.h"
#include "track/BoxProgressFadeTrackInfo.h"
#include "track/ProgressTrackInfo.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "api/config/IWailaConfig.h"
#include "api/theme/IThemeHelper.h"
#include "api/ui/Orientation.h"
#include "impl/ui/JadeUIInternal.h"
#include "overlay/DisplayHelper.h"
#include "platform/render/GuiGraphics.h"
#include "util/ClientProxy.h"

namespace jade::impl::ui {

using namespace api::ui;
using api::config::IWailaConfig;
using gui::JadeLinearLayout;
using gui::LinearAlign;

BoxElementImpl::BoxElementImpl(std::shared_ptr<Tooltip> tooltip, BoxStyle style) : mTooltip(std::move(tooltip)), mStyle(std::move(style)) {
    mIcon = mTooltip->getIcon();
    updateSize();
}

void BoxElementImpl::updateSize() {
    auto linearLayout = JadeLinearLayout::vertical();
    linearLayout->alignItems(LinearAlign::STRETCH);
    for (auto const& line : mTooltip->lines) {
        auto lineLayout = JadeLinearLayout::horizontal();
        for (auto const& element : line.elements) {
            if (auto* r = dynamic_cast<ResizeableElement*>(element.get())) r->updateSize();
            lineLayout->addChild(element, lineLayout->newChildLayoutSettings(element), [](JadeLinearLayout::ChildContainer& container) {
                if (auto* e = dynamic_cast<Element*>(container.child.get()); e && e->getAlignSelf()) container.alignSelf = e->getAlignSelf();
                if (auto* r = dynamic_cast<gui::ResizeableLayout*>(container.child.get())) container.flexGrow = r->getFlexGrow();
            });
        }
        auto lineSettings = linearLayout->newChildLayoutSettings(lineLayout);
        if (line.settings) lineSettings = line.settings(lineSettings);
        int top = line.marginTop, bottom = line.marginBottom;
        linearLayout->addChild(lineLayout, lineSettings, [top, bottom](JadeLinearLayout::ChildContainer& container) {
            container.headMargin = top;
            container.tailMargin = bottom;
        });
    }
    mTooltip->isDirty = false;

    layout::LayoutElementPtr content = linearLayout;
    if (mIcon) {
        auto iconLayout = JadeLinearLayout::horizontal();
        iconLayout->alignItems(LinearAlign::START).spacing(3);
        auto iconMode = IWailaConfig::get().overlay().getIconMode();
        if (iconMode == api::config::IconMode::CENTERED) {
            iconLayout->alignItems(LinearAlign::CENTER);
        } else if (iconMode == api::config::IconMode::TOP && mIcon->getHeight() > linearLayout->getHeight()) {
            iconLayout->alignItems(LinearAlign::CENTER);
        }
        iconLayout->addChild(mIcon);
        iconLayout->addChild(linearLayout);
        content = iconLayout;
    }

    layout = std::make_shared<gui::LayoutWithPadding>(
        content,
        mStyle.padding(ScreenDirection::LEFT),
        mStyle.padding(ScreenDirection::UP),
        mStyle.padding(ScreenDirection::RIGHT),
        mStyle.padding(ScreenDirection::DOWN)
    );
    layout->arrangeElements();
    width  = layout->getWidth();
    height = layout->getHeight();

    mRenderables.clear();
    JadeUIInternal::visitChildrenRecursive(layout, [this](layout::LayoutElementPtr const& e) {
        if (auto* r = dynamic_cast<Element*>(e.get())) mRenderables.push_back(r);
    });
}

void BoxElementImpl::setX(int x) {
    Element::setX(x);
    layout->setX(x);
}

void BoxElementImpl::setY(int y) {
    Element::setY(y);
    layout->setY(y);
}

namespace {

int64_t nowMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

template <class Getter, class Setter>
void chase(TooltipAnimation& animation, Getter getter, Setter setter, float progress) {
    if (IWailaConfig::get().overlay().getAnimation()) {
        float source = getter(animation.rect);
        float target = getter(animation.expectedRect);
        float diff   = target - source;
        if (diff == 0) return;
        if (progress >= 1) {
            animation.startTime = -1;
            setter(target);
            return;
        }
        float startValue = getter(animation.startRect);
        float deltaValue = target - startValue;
        setter(startValue + progress * deltaValue);
    } else {
        setter(getter(animation.expectedRect));
    }
}

}

void BoxElementImpl::render(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks) {
    if (mTooltip->isEmpty()) return;

    float alpha = overlay::DisplayHelper::INSTANCE.backgroundOpacity();
    bool  root  = getTag() == JadeIds::ROOT;
    if (root) alpha *= IWailaConfig::get().overlay().getAlpha();
    if (alpha > 0) mStyle.render(graphics, *this, float(getX()), float(getY()), float(getWidth()), float(getHeight()), alpha);

    graphics.enableScissor(float(getX()), float(getY()), float(getX() + getWidth()), float(getY() + getHeight()));
    for (auto* r : mRenderables) r->render(graphics, mouseX, mouseY, partialTicks);
    graphics.disableScissor();

    if (mPendingProgress) drawBoxProgressBar(graphics, *mPendingProgress);

    if (root && mTooltip->sneakyDetails) api::theme::IThemeHelper::get().theme().sneakyDetails.render(graphics, partialTicks, *this);
}

void BoxElementImpl::renderDebug(GuiGraphics& graphics, int mouseX, int mouseY, float partialTicks, RenderDebugContext const& context) {
    Element::renderDebug(graphics, mouseX, mouseY, partialTicks, context);
    if (!context.renderChildren) return;
    JadeUIInternal::visitChildrenRecursive(layout, [&](layout::LayoutElementPtr const& e) {
        if (auto* el = dynamic_cast<Element*>(e.get())) {
            el->renderDebug(graphics, mouseX, mouseY, partialTicks, context);
        } else if (dynamic_cast<layout::Layout*>(e.get())) {
            overlay::DisplayHelper::INSTANCE.drawBorder(graphics, Rect2f::of(*e), 1, 0x8800FF00, true);
        }
    });
}

namespace {

constexpr int STATIC_PROGRESS_PRIORITY = INT_MAX;

class StaticBoxProgressProvider : public api::ui::IBoxProgressProvider {
public:
    StaticBoxProgressProvider(float progress, MessageType type) : mProgress(progress), mType(type) {}
    std::optional<api::ui::BoxProgress> getProgress(api::ui::BoxElement& box, api::Accessor&, float) override {
        auto const& tag = box.getTag();
        if (!tag) return api::ui::BoxProgress{mProgress, mType};
        std::vector<api::view::ProgressView::Part> parts{api::view::ProgressView::Part::of(0, std::clamp(mProgress, 0.0f, 1.0f))};
        if (!mTrack) {
            int w  = box.getWidth();
            mTrack = overlay::WailaTickHandler::instance().progressTracker.getOrCreate<track::ProgressTrackInfo>(*tag, [&] {
                return std::make_unique<track::ProgressTrackInfo>(parts, false, w);
            });
        }
        mTrack->setProgress(parts);
        mTrack->update(overlay::OverlayRenderer::realtimeDeltaTicks);
        return api::ui::BoxProgress{mTrack->getSmoothProgress(parts.front()), mType};
    }

private:
    float                     mProgress;
    MessageType               mType;
    std::shared_ptr<track::ProgressTrackInfo> mTrack;
};

}

void BoxElementImpl::setBoxProgress(MessageType type, float progress) {
    std::erase_if(mProviders, [](PrioritizedProvider const& e) { return e.priority == STATIC_PROGRESS_PRIORITY; });
    mProviders.push_back({STATIC_PROGRESS_PRIORITY, std::make_shared<StaticBoxProgressProvider>(progress, type), true});
    std::stable_sort(mProviders.begin(), mProviders.end(), [](auto const& a, auto const& b) { return a.priority < b.priority; });
}

void BoxElementImpl::clearBoxProgress() {
    std::erase_if(mProviders, [](PrioritizedProvider const& e) { return e.isStatic; });
}

void BoxElementImpl::addProgressProvider(int priority, std::shared_ptr<api::ui::IBoxProgressProvider> provider) {
    if (!provider) throw std::invalid_argument("provider is null");
    mProviders.push_back({priority, std::move(provider), false});
    std::stable_sort(mProviders.begin(), mProviders.end(), [](auto const& a, auto const& b) { return a.priority < b.priority; });
}

void BoxElementImpl::beforeRender(api::Accessor* accessor, float partialTicks) {
    if (accessor) computeBoxProgress(*accessor, partialTicks);
    for (auto* r : mRenderables)
        if (auto* box = dynamic_cast<BoxElementImpl*>(r)) box->beforeRender(accessor, partialTicks);
}

void BoxElementImpl::computeBoxProgress(api::Accessor& accessor, float partialTicks) {
    std::optional<api::ui::BoxProgress> current;
    for (auto const& entry : mProviders) {
        if (auto progress = entry.provider->getProgress(*this, accessor, partialTicks)) {
            current = progress;
            break;
        }
    }
    if (current) {
        mFadeProgress    = current;
        mFadeAlpha       = current->alpha;
        mPendingProgress = current;
        if (auto track = getFadeTrack(true)) {
            track->progress = mFadeProgress;
            track->alpha    = mFadeAlpha;
            track->touch();
        }
        return;
    }
    auto track = getFadeTrack(false);
    if (track && !mFadeProgress) {
        mFadeProgress = track->progress;
        mFadeAlpha    = track->alpha;
    }
    if (!mFadeProgress) {
        mPendingProgress.reset();
        return;
    }
    mFadeAlpha -= overlay::OverlayRenderer::realtimeDeltaTicks * 0.1f;
    if (mFadeAlpha <= 0) {
        mFadeAlpha = 0;
        mFadeProgress.reset();
        mPendingProgress.reset();
    } else {
        mPendingProgress = api::ui::BoxProgress{mFadeProgress->progress, mFadeProgress->type, mFadeProgress->color, mFadeAlpha};
    }
    if (track) {
        track->progress = mFadeProgress;
        track->alpha    = mFadeAlpha;
        if (mFadeProgress) track->touch();
    }
}

std::shared_ptr<track::BoxProgressFadeTrackInfo> BoxElementImpl::getFadeTrack(bool create) {
    if (mFadeTrack) return mFadeTrack;
    auto const& tag = getTag();
    if (!tag) return nullptr;
    auto& tracker = overlay::WailaTickHandler::instance().progressTracker;
    mFadeTrack    = tracker.get<track::BoxProgressFadeTrackInfo>(*tag);
    if (!mFadeTrack && create) {
        mFadeTrack = tracker.getOrCreate<track::BoxProgressFadeTrackInfo>(*tag, [] { return std::make_unique<track::BoxProgressFadeTrackInfo>(); });
    }
    return mFadeTrack;
}

void BoxElementImpl::drawBoxProgressBar(GuiGraphics& graphics, api::ui::BoxProgress const& progress) {
    uint32_t baseColor = progress.color ? *progress.color : mStyle.boxProgressColors.get(progress.type);
    float    boxAlpha  = overlay::DisplayHelper::INSTANCE.backgroundOpacity();
    if (getTag() == JadeIds::ROOT) boxAlpha *= IWailaConfig::get().overlay().getAlpha();
    uint32_t color   = api::config::applyAlpha(baseColor, progress.alpha * boxAlpha);
    float    x       = float(getX());
    float    top     = float(getY() + getHeight());
    float    w       = float(getWidth());
    float    offset0 = mStyle.boxProgressOffset(ScreenDirection::UP);
    float    offset1 = mStyle.boxProgressOffset(ScreenDirection::RIGHT);
    float    offset2 = mStyle.boxProgressOffset(ScreenDirection::DOWN);
    float    offset3 = mStyle.boxProgressOffset(ScreenDirection::LEFT);
    w += offset1 - offset3;
    overlay::DisplayHelper::fill(
        graphics,
        x + offset3,
        top - 1 + offset0,
        x + offset3 + w * std::clamp(progress.progress, 0.0f, 1.0f),
        top + offset2,
        color
    );
}

void BoxElementImpl::updateExpectedRect(TooltipAnimation& animation) {
    auto&       overlay       = IWailaConfig::get().overlay();
    auto&       accessibility = IWailaConfig::get().accessibility();
    float const screenW       = float(util::ClientProxy::guiScaledWidth());
    float const screenH       = float(util::ClientProxy::guiScaledHeight());
    float       x             = screenW * accessibility.tryFlip(overlay.getOverlayPosX());
    float       y             = screenH * (1.0f - overlay.getOverlayPosY());
    float       w             = float(layout->getWidth());
    float       h             = float(layout->getHeight());

    animation.scale       = overlay.getOverlayScale();
    float thresholdHeight = screenH * overlay.getAutoScaleThreshold();
    if (!JadeUIInternal::isPinned() && layout->getHeight() * animation.scale > thresholdHeight) {
        animation.scale = std::max(animation.scale * 0.5f, thresholdHeight / float(layout->getHeight()));
    }

    Rect2f& expected = animation.expectedRect;
    expected.setWidth(float(int(w * animation.scale)));
    expected.setHeight(float(int(h * animation.scale)));
    expected.setX(float(int(x - expected.getWidth() * accessibility.tryFlip(overlay.getAnchorX()))));
    expected.setY(float(int(y - expected.getHeight() * overlay.getAnchorY())));

    if (IWailaConfig::get().general().getBossBarOverlapMode() == api::config::BossBarOverlapMode::PUSH_DOWN) {
        auto bossBarRect = util::ClientProxy::getBossBarRect();
        if (bossBarRect && bossBarRect->intersects(expected)) expected.setY(bossBarRect->getY() + bossBarRect->getHeight());
    }
}

void BoxElementImpl::updateRect(TooltipAnimation& animation) {
    Rect2f&       src    = animation.rect;
    Rect2f const& target = animation.expectedRect;
    if (src.getWidth() == 0) {
        src.setX(target.getX());
        src.setY(target.getY());
        src.setWidth(target.getWidth());
        src.setHeight(target.getHeight());
        animation.alpha = animation.showHideAlpha;
    } else {
        constexpr int64_t durationMillis = 75;
        int64_t           deltaTime      = nowMillis() - animation.startTime;
        float             progress       = float(deltaTime) / float(durationMillis);
        animation.alpha                  = std::min(animation.showHideAlpha, std::max(progress, 0.55f));
        chase(animation, [](Rect2f const& r) { return r.getX(); }, [&](float v) { src.setX(v); }, progress);
        chase(animation, [](Rect2f const& r) { return r.getY(); }, [&](float v) { src.setY(v); }, progress);
        chase(
            animation,
            [](Rect2f const& r) { return r.getWidth(); },
            [&](float v) {
                src.setWidth(v);
                width = int(v / animation.scale);
            },
            progress
        );
        chase(
            animation,
            [](Rect2f const& r) { return r.getHeight(); },
            [&](float v) {
                src.setHeight(v);
                height = int(v / animation.scale);
            },
            progress
        );
    }
}

std::string BoxElementImpl::getNarration() {
    if (mTooltip->isEmpty()) return "";
    return mTooltip->getNarration();
}

void BoxElementImpl::setFreeSpace(int w, int h) {
    layout->setFreeSpace(w, h);
    width  = layout->getWidth();
    height = layout->getHeight();
}

}
