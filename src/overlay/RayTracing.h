// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <functional>
#include <optional>

#include "api/Accessor.h"

namespace jade::overlay {

class RayTracing {
public:
    static RayTracing INSTANCE;

    std::function<bool(Actor const&)> entityFilter = [](Actor const&) { return true; };

    void                                         fire();
    [[nodiscard]] std::optional<api::HitResult> const& getTarget() const { return mTarget; }
    [[nodiscard]] Vec3 const&                    getHitLocation() const { return mHitLocation; }

    static bool canBeTarget(Actor const& target, Actor const& viewEntity);

private:
    std::optional<api::HitResult> mTarget;
    Vec3                          mHitLocation{0, 0, 0};
};

}
