// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Based on Snownee/Jade (branch 26.3-fabric @3dc11bb, CC BY-NC-SA 4.0; Copyright (c) Snownee and contributors), ported to C++ and modified.
#pragma once

#include <optional>

#include "mc/deps/nbt/CompoundTag.h"

#include "api/IJadeProvider.h"

namespace jade::api {

template <class T, class D>
class StreamServerDataProvider : public IServerDataProvider<T> {
public:
    void appendServerData(CompoundTag& data, T& accessor) override {
        auto value = streamData(accessor);
        if (!value) return;
        CompoundTag out;
        encode(out, *value);
        data[this->getUid()] = std::move(out);
    }

    std::optional<D> decodeFromData(T& accessor) {
        auto& data = accessor.getServerData();
        if (!data.contains(this->getUid())) return std::nullopt;
        try {
            auto const& v = data.at(this->getUid());
            if (!v.template hold<CompoundTag>()) return std::nullopt;
            return decode(v.template get<CompoundTag>());
        } catch (...) {
            return std::nullopt;
        }
    }

    virtual std::optional<D> streamData(T& accessor)                    = 0;
    virtual void             encode(CompoundTag& out, D const& value)   = 0;
    virtual std::optional<D> decode(CompoundTag const& in)              = 0;
};

}
