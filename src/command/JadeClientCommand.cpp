// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "command/JadeClientCommand.h"

#include "gui/ConfigGui.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <vector>

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/client/ClientJoinLevelEvent.h"
#include "ll/api/event/client/ClientStartJoinLevelEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/mod/NativeMod.h"

#include "ll/api/service/Bedrock.h"

#include "mc/network/LoopbackPacketSender.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandRawText.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/Packet.h"
#include "mc/network/packet/CommandRequestPacket.h"
#include "mc/network/packet/CommandRequestPacketPayload.h"

#include "nlohmann/json.hpp"

#include "impl/WailaClientRegistration.h"
#include "impl/config/WailaConfig.h"
#include "mod/JadeClient.h"
#include "platform/MainThread.h"
#include "platform/text/Component.h"
#include "util/ClientProxy.h"
#include "util/DumpGenerator.h"

namespace jade::command {

struct JadecArgs {
    CommandRawText args;
};

namespace {

using text::Component;
using util::ClientProxy;

void success(Component const& c) { ClientProxy::displayClientMessage(c.getString()); }
void failure(Component const& c) { ClientProxy::displayClientMessage("\xC2\xA7" "c" + c.getString()); }

std::vector<std::string> split(std::string const& s) {
    std::vector<std::string> out;
    std::istringstream       in(s);
    std::string              t;
    while (in >> t) out.push_back(t);
    return out;
}

nlohmann::json parseValue(std::string const& s) {
    auto j = nlohmann::json::parse(s, nullptr, false);
    if (!j.is_discarded()) return j;
    return s;
}

int generate(std::optional<std::string> const& pattern) {
    std::optional<std::regex> regex;
    if (pattern) {
        try {
            regex.emplace(*pattern);
        } catch (std::regex_error const& e) {
            failure(Component::translatable("command.jade.generate_server_overrides.invalid_pattern", std::vector<std::string>{e.what()}));
            return 0;
        }
    }
    auto const&    all      = impl::config::ConfigStore::get().config().plugin().values();
    nlohmann::json filtered = nlohmann::json::object();
    for (auto const& [k, v] : all.items()) {
        if (!regex || std::regex_match(k, *regex)) filtered[k] = v;
    }
    if (regex && filtered.empty()) {
        failure(Component::translatable("command.jade.generate_server_overrides.no_match"));
        return 0;
    }
    auto            dir = JadeClient::configDir() / "jade";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    auto file = dir / "server-plugin-overrides.json";
    for (int i = 1; std::filesystem::exists(file); ++i) file = dir / ("server-plugin-overrides." + std::to_string(i) + ".json");
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) {
        failure(Component::literal("IOException: " + file.filename().string()));
        return 0;
    }
    out << filtered.dump(2);
    success(Component::translatable("command.jade.generate_server_overrides.success", std::vector<std::string>{file.filename().string()}));
    return 1;
}

void execute(std::vector<std::string> args) {
    auto& store = impl::config::ConfigStore::get();
    if (args.empty()) {
        success(Component::literal("/jadec handlers | config | use_profile <0-3> | pin | generate_server_overrides [pattern] | get <key> | set <key> <value> | reload | toggle"));
        return;
    }
    std::string const& sub = args[0];
    if (sub == "handlers") {
        auto          file = JadeClient::configDir().parent_path() / "jade_handlers.md";
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!out) {
            failure(Component::literal("IOException: jade_handlers.md"));
            return;
        }
        out << util::DumpGenerator::generateInfoDump();
        success(Component::translatable("command.jade.dump.success"));
    } else if (sub == "config") {
        store.invalidateConfig();
        gui::ConfigGui::open();
    } else if (sub == "use_profile" && args.size() >= 2) {
        int index = std::atoi(args[1].c_str());
        if (index < 0 || index > 3) {
            failure(Component::literal("index: 0-3"));
            return;
        }
        store.useProfile(index);
    } else if (sub == "pin") {
        failure(Component::literal("[Jade] /jadec pin is not supported in this port"));
    } else if (sub == "generate_server_overrides") {
        std::optional<std::string> pattern;
        if (args.size() >= 2) {
            std::string p;
            for (size_t i = 1; i < args.size(); ++i) p += (i > 1 ? " " : "") + args[i];
            pattern = p;
        }
        generate(pattern);
    } else if (sub == "get" && args.size() >= 2) {
        success(Component::literal(args[1] + " = " + store.getValue(args[1]).dump()));
    } else if (sub == "set" && args.size() >= 3) {
        std::string v;
        for (size_t i = 2; i < args.size(); ++i) v += (i > 2 ? " " : "") + args[i];
        std::string error;
        if (store.setValue(args[1], parseValue(v), error)) success(Component::literal(args[1] + " = " + store.getValue(args[1]).dump()));
        else failure(Component::literal(error));
    } else if (sub == "reload") {
        store.invalidateConfig();
        success(Component::literal("[Jade] Config reloaded"));
    } else if (sub == "toggle") {
        auto& general = api::config::IWailaConfig::get().general();
        general.setDisplayTooltip(!general.shouldDisplayTooltip());
        api::config::IWailaConfig::get().save();
        success(Component::literal(std::string("general.displayTooltip = ") + (general.shouldDisplayTooltip() ? "true" : "false")));
    } else {
        failure(Component::literal("/jadec: unknown argument: " + sub));
    }
}

}

bool JadeClientCommand::handle(std::string const& raw) {
    std::string command = (!raw.empty() && raw[0] == '/') ? raw.substr(1) : raw;
    auto        args    = split(command);
    if (args.empty() || args[0] != "jadec") return false;
    args.erase(args.begin());
    ll::mod::NativeMod::current()->getLogger().info("Jade: [command] {}", command);
    platform::MainThread::get().post([args = std::move(args)] { execute(args); });
    return true;
}

LL_TYPE_INSTANCE_HOOK(JadeLoopbackSendToServerHook, ll::memory::HookPriority::Normal, LoopbackPacketSender, &LoopbackPacketSender::$sendToServer, void, ::Packet& packet) {
    if (packet.getId() == MinecraftPacketIds::CommandRequest) {
        auto const& req = static_cast<::CommandRequestPacket const&>(packet);
        ll::mod::NativeMod::current()->getLogger().debug("Jade: [command] saw request: {}", req.mCommand.get());
        if (JadeClientCommand::handle(req.mCommand.get())) return;
    }
    origin(packet);
}

namespace {
bool                                gRegistered = false;
std::vector<ll::event::ListenerPtr> gListeners;

bool tryRegister() {
    if (gRegistered) return true;
    if (!ll::service::getCommandRegistry(true)) return false;
    auto& cmd = ll::command::CommandRegistrar::getInstance(true).getOrCreateCommand("jadec", "Jade (unofficial Bedrock port)", CommandPermissionLevel::Any);
    cmd.overload().execute([](CommandOrigin const&, CommandOutput&) {});
    cmd.overload<JadecArgs>().required("args").execute([](CommandOrigin const&, CommandOutput&, JadecArgs const&) {});
    gRegistered = true;
    ll::mod::NativeMod::current()->getLogger().info("Jade: [command] registered /jadec in the client command list");
    return true;
}
}

bool JadeClientCommand::install() {
    if (!tryRegister()) {
        auto& bus = ll::event::EventBus::getInstance();
        gListeners.push_back(bus.emplaceListener<ll::event::ClientStartJoinLevelEvent>([](ll::event::ClientStartJoinLevelEvent&) { tryRegister(); }));
        gListeners.push_back(bus.emplaceListener<ll::event::ClientJoinLevelEvent>([](ll::event::ClientJoinLevelEvent&) { tryRegister(); }));
    }
    return JadeLoopbackSendToServerHook::hook() == 0;
}

void JadeClientCommand::uninstall() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& l : gListeners) bus.removeListener(l);
    gListeners.clear();
    JadeLoopbackSendToServerHook::unhook();
}

}
