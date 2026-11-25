// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#include "util/DumpGenerator.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <map>
#include <set>
#include <typeinfo>
#include <vector>

#include "impl/WailaClientRegistration.h"
#include "impl/WailaCommonRegistration.h"

namespace jade::util {

namespace {

template <class Lookup>
void createSection(std::string& b, char const* subsection, Lookup const& lookup) {
    if (lookup.isEmpty()) return;
    auto& pr = impl::WailaCommonRegistration::instance().priorities;
    b += std::string("\n### ") + subsection;
    std::map<std::string, std::vector<api::IJadeProvider const*>> byClass;
    for (auto const& e : lookup.entries()) {
        auto& list = byClass[e.clazz.name];
        if (std::find(list.begin(), list.end(), e.provider.get()) == list.end()) list.push_back(e.provider.get());
    }
    for (auto& [name, list] : byClass) {
        std::stable_sort(list.begin(), list.end(), [&pr](auto* a, auto* c) { return pr.byValue(*a) < pr.byValue(*c); });
        b += "\n\n#### " + name;
        for (auto* p : list) b += std::format("\n* {}, {}, {}", p->getUid(), pr.byValue(*p), typeid(*p).name());
    }
    b += "\n\n";
}

}

std::string DumpGenerator::generateInfoDump() {
    std::string b      = "# Jade Handler Dump";
    auto&       client = impl::WailaClientRegistration::instance();
    auto&       common = impl::WailaCommonRegistration::instance();
    b += std::format("\nGenerated at {:%FT%TZ}\n", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    b += "\n## Block";
    createSection(b, "Icon Providers", client.blockIconProviders);
    createSection(b, "Component Providers", client.blockComponentProviders);
    createSection(b, "Data Providers (Block)", common.blockDataProviders.first);
    createSection(b, "Data Providers (BlockActor)", common.blockDataProviders.second);
    b += "\n## Entity";
    createSection(b, "Icon Providers", client.entityIconProviders);
    createSection(b, "Component Providers", client.entityComponentProviders);
    createSection(b, "Data Providers", common.entityDataProviders);
    b += "\n## Priorities";
    for (auto const& id : common.priorities.getSortedList()) b += "\n* " + id;
    return b;
}

}
