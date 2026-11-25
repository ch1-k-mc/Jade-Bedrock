// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "util/ClientProxy.h"

#include <chrono>
#include <vector>

#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/GuiData.h"
#include "mc/client/gui/ScreenSizeData.h"
#include "mc/client/gui/controls/UIControl.h"
#include "mc/client/gui/controls/VisualTree.h"
#include "mc/client/gui/screens/ScreenView.h"
#include "mc/client/options/IOptionRegistry.h"

#include "api/config/IWailaConfig.h"

namespace jade::util {

namespace {
std::optional<api::ui::Rect2f> gBossBar;

bool visible(UIControl const& c) { return c.mVisible != ui::VisibilityFlag::None && c.mIsVisibleInTree; }
}

int ClientProxy::guiScaledWidth() {
    auto ci = ll::service::getClientInstance();
    if (!ci) return 0;
    return static_cast<int>(ci->getGuiData()->mScreenSizeData.get().clientUIScreenSize.get().x);
}

int ClientProxy::guiScaledHeight() {
    auto ci = ll::service::getClientInstance();
    if (!ci) return 0;
    return static_cast<int>(ci->getGuiData()->mScreenSizeData.get().clientUIScreenSize.get().y);
}

float ClientProxy::guiScale() {
    auto ci = ll::service::getClientInstance();
    return ci ? ci->getGuiData()->mGuiScale : 1.0f;
}

int64_t ClientProxy::millis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::optional<api::ui::Rect2f> ClientProxy::getBossBarRect() { return gBossBar; }

void ClientProxy::updateBossBarRect(ScreenView& view) {
    gBossBar.reset();
    VisualTree* tree = view.mVisualTree.get();
    if (!tree) return;
    std::shared_ptr<UIControl> root = tree->mRootControl;
    if (!root) return;
    std::vector<UIControl*> queue{root.get()};
    float                   x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    bool                    any = false;
    while (!queue.empty()) {
        UIControl* c = queue.back();
        queue.pop_back();
        if (!visible(*c)) continue;
        std::string const& name = c->mName.get();
        if (name == "boss_health_panel") {
            glm::vec2 pos = c->mCachedPosition;
            glm::vec2 sz  = c->mSize;
            if (sz.x > 0 && sz.y > 0 && sz.x < 600) {
                x0  = std::min(x0, pos.x);
                y0  = std::min(y0, pos.y);
                x1  = std::max(x1, pos.x + sz.x);
                y1  = std::max(y1, pos.y + sz.y);
                any = true;
            }
        }
        for (auto& child : c->mChildren.get()) queue.push_back(child.get());
    }
    if (any) gBossBar = api::ui::Rect2f(x0, y0, x1 - x0, y1 - y0);
}

std::string ClientProxy::topScreenName() {
    auto ci = ll::service::getClientInstance();
    return ci ? ci->getTopScreenName() : std::string();
}

bool ClientProxy::isScreenOpen() {
    std::string name = topScreenName();
    return !(name.starts_with("hud_") || name == "toast_screen");
}

bool ClientProxy::shouldHideWithGui() {
    if (!isScreenOpen()) return false;
    bool showBefore = !api::config::IWailaConfig::get().general().shouldHideFromGUIs();
    return !showBefore;
}

bool ClientProxy::isHudHidden() {
    auto ci = ll::service::getClientInstance();
    if (!ci) return false;
    return ci->getOptions().getHideHud();
}

void ClientProxy::displayClientMessage(std::string const& message) {
    auto ci = ll::service::getClientInstance();
    if (!ci) return;
    ci->getGuiData()->displayClientMessage(message, std::nullopt, false);
}

}
