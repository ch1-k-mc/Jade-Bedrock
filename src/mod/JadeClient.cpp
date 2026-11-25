// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "mod/JadeClient.h"

#include <vector>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/client/ClientExitLevelEvent.h"
#include "ll/api/event/render/UIRenderEvent.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/screens/ScreenView.h"

#include "addon/core/CoreProviders.h"
#include "addon/universal/UniversalPlugin.h"
#include "addon/access/AccessibilityPlugin.h"
#include "addon/debug/DebugPlugin.h"
#include "addon/harvest/HarvestToolProvider.h"
#include "addon/vanilla/VanillaProviders.h"
#include "command/JadeClientCommand.h"
#include "impl/ObjectDataCenter.h"
#include "addon/bedrock/BedrockPlugin.h"
#include "api/ExternalPlugins.h"
#include "gui/ConfigGui.h"
#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"
#include "impl/config/WailaConfig.h"
#include "impl/theme/ThemeHelper.h"
#include "overlay/OverlayRenderer.h"
#include "overlay/WailaTickHandler.h"
#include "platform/MainThread.h"
#include "platform/item/ScopedItemRegistry.h"
#include "platform/render/GuiGraphics.h"
#include "util/ClientProxy.h"
#include "util/JadeFont.h"
#include "util/JadeKeys.h"
#include "util/ModIdentification.h"

namespace jade {

namespace {

std::filesystem::path               gAssetsDir;
std::filesystem::path               gConfigDir;
std::vector<ll::event::ListenerPtr> gListeners;
int64_t                             gLastTick  = 0;
int64_t                             gLastFrame = 0;

auto& logger() { return ll::mod::NativeMod::current()->getLogger(); }

void loadPlugins() {
    impl::WailaCommonRegistration::reset();
    impl::WailaClientRegistration::reset();
    auto& common = impl::WailaCommonRegistration::instance();
    auto& client = impl::WailaClientRegistration::instance();
    std::vector<std::unique_ptr<api::IWailaPlugin>> plugins;
    plugins.push_back(std::make_unique<addon::core::CorePlugin>());
    plugins.push_back(std::make_unique<addon::universal::UniversalPlugin>());
    plugins.push_back(std::make_unique<addon::vanilla::VanillaPlugin>());
    plugins.push_back(std::make_unique<addon::debug::DebugPlugin>());
    plugins.push_back(std::make_unique<addon::access::AccessibilityPlugin>());
    plugins.push_back(std::make_unique<addon::bedrock::BedrockPlugin>());
    for (auto& p : plugins) {
        try {
            p->register_(common);
            p->registerClient(client);
        } catch (std::exception const& e) {
            logger().error("Jade: exception while registering plugins: {}", e.what());
        }
    }
    api::external::registerAll(common, client);
    common.priorities.configurable(gConfigDir / "jade" / "sort-order.json");
    common.priorities.sort(client.getConfigKeys());
    common.loadComplete();
    client.loadComplete();
}

}

std::filesystem::path const& JadeClient::configDir() { return gConfigDir; }
std::filesystem::path const& JadeClient::assetsDir() { return gAssetsDir; }

bool JadeClient::init(std::filesystem::path const& assetsDir, std::filesystem::path const& configDir) {
    gAssetsDir = assetsDir;
    gConfigDir = configDir;
    try {
        loadPlugins();
        impl::config::ConfigStore::get().load(configDir);
        impl::WailaCommonRegistration::instance().reloadOperations(configDir);
        impl::theme::ThemeHelper::INSTANCE.load(assetsDir);
        util::JadeKeys::init(configDir);
        gui::ConfigGui::install();
        if (!command::JadeClientCommand::install()) logger().warn("Jade: failed to install the /jadec hook");
    } catch (std::exception const& e) {
        logger().error("Jade: exception during initialization: {}", e.what());
        return false;
    }

    platform::MainThread::get().addTick([](std::uint64_t) {
        if (api::external::isDirty()) {
            auto client = ll::service::getClientInstance();
            if ((!client || !client->getLocalPlayer()) && api::external::consumeDirty()) {
                try {
                    loadPlugins();
                    impl::config::ConfigStore::get().invalidateConfig();
                } catch (std::exception const& e) {
                    logger().error("Jade: exception while applying providers from other mods: {}", e.what());
                }
            }
        }
        util::JadeKeys::onKeyPressed();
        int64_t now = util::ClientProxy::millis();
        if (now - gLastTick >= 50) {
            gLastTick = now;
            try {
                overlay::WailaTickHandler::instance().tickClient();
            } catch (std::exception const& e) {
                logger().error("Jade: exception in tickClient: {}", e.what());
            }
        }
    });

    auto& bus = ll::event::EventBus::getInstance();
    gListeners.push_back(bus.emplaceListener<ll::event::AfterUIRenderEvent>([](ll::event::AfterUIRenderEvent& e) {
        if (e.screenView().getScreenName() != "hud_screen") return;
        auto ci = ll::service::getClientInstance();
        if (!ci || !ci->getLocalPlayer()) return;
        util::ClientProxy::updateBossBarRect(e.screenView());
        int64_t now   = util::ClientProxy::millis();
        float   delta = gLastFrame == 0 ? 0.0f : static_cast<float>(now - gLastFrame) / 50.0f;
        gLastFrame    = now;
        platform::item::ScopedItemRegistry scoped(*ci);
        platform::render::GuiGraphics      graphics(e.uiRenderContext(), *ci);
        if (!gui::ConfigGui::isOpen()) {
            overlay::OverlayRenderer::renderOverlay478757(graphics, std::min(delta, 5.0f));
        } else {
            gui::ConfigGui::render(graphics);
            if (gui::ConfigGui::showPreview()) overlay::OverlayRenderer::renderOverlay478757(graphics, std::min(delta, 5.0f));
            gui::ConfigGui::renderTop(graphics);
        }
    }));
    gListeners.push_back(bus.emplaceListener<ll::event::ClientExitLevelEvent>([](ll::event::ClientExitLevelEvent&) {
        gui::ConfigGui::close();
        addon::bedrock::PackNames::reset();
        util::ModIdentification::invalidateCache();
        impl::ObjectDataCenter::disconnect();
        overlay::OverlayRenderer::clearLingerTooltip();
        util::JadeFont::reset();
        addon::harvest::HarvestToolProvider::INSTANCE->invalidateCache();
    }));
    logger().info("Jade: initialized ({} config entries, {} themes)", impl::WailaClientRegistration::instance().configEntries.size(),
                  impl::theme::ThemeHelper::INSTANCE.getThemes().size());
    return true;
}

void JadeClient::shutdown() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& l : gListeners) bus.removeListener(l);
    gListeners.clear();
    gui::ConfigGui::uninstall();
    util::JadeKeys::shutdown();
    command::JadeClientCommand::uninstall();
    {
        auto ci = ll::service::getClientInstance();
        if (ci) {
            platform::item::ScopedItemRegistry scoped(*ci);
            overlay::WailaTickHandler::instance().clearState();
            overlay::OverlayRenderer::clearLingerTooltip();
        }
    }
}

}
