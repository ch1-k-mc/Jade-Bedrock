// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#include "mod/Jade.h"

#include "ll/api/mod/RegisterHelper.h"

#include "mod/JadeClient.h"
#include "platform/MainThread.h"
#include "platform/render/SpriteManager.h"
#include "platform/render/TextureManager.h"
#include "platform/text/I18n.h"

namespace jade {

Jade& Jade::getInstance() {
    static Jade instance;
    return instance;
}

bool Jade::load() {
    auto assets = getSelf().getModDir() / "assets";
    platform::render::TextureManager::get().setAssetsDir(assets);
    platform::render::SpriteManager::get().setAssetsDir(assets.string());
    text::I18n::load(assets / "jade" / "lang");
    getSelf().getLogger().info("Jade: loading");
    return true;
}

bool Jade::enable() {
    if (!platform::MainThread::get().start()) {
        getSelf().getLogger().error("Jade: failed to install the frame hook");
        return false;
    }
    if (!JadeClient::init(getSelf().getModDir() / "assets", getSelf().getModDir() / "config")) return false;
    getSelf().getLogger().info("Jade: enabled");
    return true;
}

bool Jade::disable() {
    JadeClient::shutdown();
    platform::MainThread::get().stop();
    platform::render::TextureManager::get().clear();
    platform::render::SpriteManager::get().clear();
    getSelf().getLogger().info("Jade: disabled");
    return true;
}

}

LL_REGISTER_MOD(jade::Jade, jade::Jade::getInstance());
