// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "util/WailaExceptionHandler.h"

#include <mutex>
#include <set>

#include "ll/api/mod/NativeMod.h"

#include "api/IJadeProvider.h"
#include "api/ITooltip.h"
#include "util/ModIdentification.h"

namespace jade::util {

namespace {
std::mutex                            gMutex;
std::set<api::IJadeProvider const*>   gErrors;
bool                                  gNullError = false;
}

void WailaExceptionHandler::handleErr(std::exception const& e, api::IJadeProvider const* provider, api::ITooltip* tooltip) {
    bool log = false;
    {
        std::lock_guard lock(gMutex);
        if (!provider) {
            if (!gNullError) gNullError = log = true;
        } else if (gErrors.insert(provider).second) {
            log = true;
        }
    }
    if (log) {
        ll::mod::NativeMod::current()->getLogger().error("Jade: exception in provider: [{}] {}", provider ? provider->getUid() : "null", e.what());
    }
    if (tooltip) {
        std::string modid = provider ? provider->getUid().substr(0, provider->getUid().find(':')) : "jade";
        if (modid.empty() || modid == "minecraft") modid = "jade";
        auto name = ModIdentification::getModFullName(modid).value_or(modid);
        tooltip->add(text::Component::translatable("jade.error", std::vector<std::string>{name}).withColor(0xFFAA0000));
    }
}

}
