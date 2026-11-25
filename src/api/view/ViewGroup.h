// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/ListTag.h"

#include "api/JadeIds.h"

namespace jade::api::view {

template <class T>
class ViewGroup {
public:
    ViewGroup() = default;
    explicit ViewGroup(std::vector<T> views) : views(std::move(views)) {}
    ViewGroup(std::vector<T> views, std::optional<std::string> id, std::optional<CompoundTag> extraData)
    : views(std::move(views)),
      id(std::move(id)),
      extraData(std::move(extraData)) {}

    std::vector<T>             views;
    std::optional<std::string> id;
    std::optional<CompoundTag> extraData;

    CompoundTag& getExtraData() {
        if (!extraData) extraData.emplace();
        return *extraData;
    }
    void setProgress(float progress) { getExtraData()["Progress"] = progress; }
};

template <class T>
struct ViewCodec;

template <class T>
using ViewGroups = std::vector<ViewGroup<T>>;

template <class T>
CompoundTag encodeGroups(Identifier const& uid, ViewGroups<T> const& groups) {
    CompoundTag out;
    out["id"] = uid;
    ListTag list;
    for (auto const& g : groups) {
        CompoundTag gt;
        ListTag     views;
        for (auto const& v : g.views) {
            CompoundTag vt;
            ViewCodec<T>::encode(vt, v);
            views.add(std::make_unique<CompoundTag>(std::move(vt)));
        }
        gt["views"] = std::move(views);
        if (g.id) gt["gid"] = *g.id;
        if (g.extraData) gt["extra"] = *g.extraData;
        list.add(std::make_unique<CompoundTag>(std::move(gt)));
    }
    out["groups"] = std::move(list);
    return out;
}

template <class T>
std::optional<std::pair<Identifier, ViewGroups<T>>> decodeGroups(CompoundTag const& in) {
    try {
        if (!in.contains("id") || !in.contains("groups")) return std::nullopt;
        auto const& idv = in.at("id");
        if (!idv.template hold<StringTag>()) return std::nullopt;
        Identifier  uid = idv.template get<StringTag>();
        auto const& gv  = in.at("groups");
        if (!gv.template hold<ListTag>()) return std::nullopt;
        ViewGroups<T> groups;
        for (auto const& gp : gv.template get<ListTag>()) {
            if (!gp.template hold<CompoundTag>()) continue;
            auto const&  gt = gp.template get<CompoundTag>();
            ViewGroup<T> g;
            if (gt.contains("views") && gt.at("views").template hold<ListTag>()) {
                for (auto const& vp : gt.at("views").template get<ListTag>()) {
                    if (!vp.template hold<CompoundTag>()) continue;
                    if (auto v = ViewCodec<T>::decode(vp.template get<CompoundTag>())) g.views.push_back(std::move(*v));
                }
            }
            if (gt.contains("gid") && gt.at("gid").template hold<StringTag>()) g.id = std::string(gt.at("gid").template get<StringTag>());
            if (gt.contains("extra") && gt.at("extra").template hold<CompoundTag>()) g.extraData = gt.at("extra").template get<CompoundTag>();
            groups.push_back(std::move(g));
        }
        return std::make_pair(std::move(uid), std::move(groups));
    } catch (...) {
        return std::nullopt;
    }
}

}
