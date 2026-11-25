// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include "api/IWailaPlugin.h"

namespace jade::addon::universal {

class UniversalPlugin : public api::IWailaPlugin {
public:
    void register_(impl::WailaCommonRegistration& registration) override;
    void registerClient(impl::WailaClientRegistration& registration) override;
};

}
