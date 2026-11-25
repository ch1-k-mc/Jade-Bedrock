// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <cstdint>
#include <functional>

namespace jade::platform {

class MainThread {
public:
    static MainThread& get();

    bool start();
    void stop();

    using Tick = std::function<void(std::uint64_t frame)>;
    void addTick(Tick tick);

    void post(std::function<void()> fn);

    [[nodiscard]] std::uint64_t frames() const;
    [[nodiscard]] bool          isMainThread() const;

    void onFrame();
};

}
