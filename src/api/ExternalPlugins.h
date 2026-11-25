// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <optional>
#include <cstdint>
#include <string>

namespace jade::impl {
class WailaCommonRegistration;
class WailaClientRegistration;
}

namespace jade::api::external {

void registerAll(impl::WailaCommonRegistration& common, impl::WailaClientRegistration& client);
[[nodiscard]] bool consumeDirty();
[[nodiscard]] bool isDirty();
[[nodiscard]] std::optional<std::string> displayName(std::string const& uid);
[[nodiscard]] uint64_t idOf(std::string const& uid);

}
