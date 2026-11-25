// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <string>

namespace jade::command {

class JadeClientCommand {
public:
    static bool install();
    static void uninstall();
    static bool handle(std::string const& command);
};

}
