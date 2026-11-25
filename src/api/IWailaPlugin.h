// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

namespace jade::impl {
class WailaCommonRegistration;
class WailaClientRegistration;
}

namespace jade::api {

class IWailaPlugin {
public:
    virtual ~IWailaPlugin() = default;
    virtual void register_(impl::WailaCommonRegistration&) {}
    virtual void registerClient(impl::WailaClientRegistration&) {}
};

}
