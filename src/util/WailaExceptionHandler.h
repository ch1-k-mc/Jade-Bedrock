// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <exception>

namespace jade::api {
class IJadeProvider;
class ITooltip;
}

namespace jade::util {

class WailaExceptionHandler {
public:
    static void handleErr(std::exception const& e, api::IJadeProvider const* provider, api::ITooltip* tooltip = nullptr);
};

}
