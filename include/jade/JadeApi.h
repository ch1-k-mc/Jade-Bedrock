// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Part of Jade (unofficial Bedrock port). Not in the original Jade; written for Bedrock Edition.
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <Windows.h>

extern "C" {

typedef struct jade_context jade_context;
typedef void (*jade_callback_t)(jade_context* ctx, void* user);

enum { JADE_API_VERSION = 1, JADE_TARGET_BLOCK = 0, JADE_TARGET_ENTITY = 1 };

typedef struct jade_provider_desc {
    uint32_t        struct_size;
    char const*     uid;
    char const*     display_name;
    int             target;
    char const*     targets;
    int             priority;
    int             enabled_by_default;
    char const*     config_icon;
    jade_callback_t append;
    jade_callback_t icon;
    jade_callback_t server;
    void*           user;
} jade_provider_desc;

typedef int (*jade_api_version_t)(void);
typedef uint64_t (*jade_register_provider_t)(jade_provider_desc const* desc);
typedef int (*jade_unregister_provider_t)(uint64_t id);
typedef int (*jade_ctx_target_t)(jade_context const* ctx);
typedef char const* (*jade_ctx_type_id_t)(jade_context* ctx);
typedef void (*jade_ctx_position_t)(jade_context const* ctx, int* x, int* y, int* z);
typedef int (*jade_ctx_show_details_t)(jade_context const* ctx);
typedef int (*jade_ctx_server_connected_t)(jade_context const* ctx);
typedef void (*jade_ctx_add_line_t)(jade_context* ctx, char const* utf8);
typedef void (*jade_ctx_set_icon_item_t)(jade_context* ctx, char const* item);
typedef void (*jade_ctx_set_data_t)(jade_context* ctx, char const* utf8);
typedef char const* (*jade_ctx_get_data_t)(jade_context* ctx);

#ifdef JADE_INTERNAL
uint64_t jade_register_provider(jade_provider_desc const* desc);
int      jade_unregister_provider(uint64_t id);
void     jade_ctx_add_line(jade_context* ctx, char const* utf8);
#endif
}

namespace jadeapi {

class Api {
public:
    static Api& get() {
        static Api instance;
        return instance;
    }

    [[nodiscard]] bool available() const { return mRegister && mVersion && mVersion() >= JADE_API_VERSION; }

    uint64_t registerProvider(jade_provider_desc desc) const {
        desc.struct_size = sizeof(jade_provider_desc);
        return available() ? mRegister(&desc) : 0;
    }
    bool unregisterProvider(uint64_t id) const { return mUnregister && mUnregister(id) != 0; }

    [[nodiscard]] int         target(jade_context const* c) const { return mTarget(c); }
    [[nodiscard]] char const* typeId(jade_context* c) const { return mTypeId(c); }
    void                      position(jade_context const* c, int& x, int& y, int& z) const { mPosition(c, &x, &y, &z); }
    [[nodiscard]] bool        showDetails(jade_context const* c) const { return mShowDetails(c) != 0; }
    [[nodiscard]] bool        serverConnected(jade_context const* c) const { return mServerConnected(c) != 0; }
    void                      addLine(jade_context* c, std::string const& utf8) const { mAddLine(c, utf8.c_str()); }
    void                      setIconItem(jade_context* c, std::string const& item) const { mSetIconItem(c, item.c_str()); }
    void                      setData(jade_context* c, std::string const& utf8) const { mSetData(c, utf8.c_str()); }
    [[nodiscard]] char const* data(jade_context* c) const { return mGetData(c); }

private:
    Api() {
        HMODULE m = GetModuleHandleW(L"Jade.dll");
        if (!m) return;
        auto load = [m](auto& fn, char const* name) { fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(reinterpret_cast<void*>(GetProcAddress(m, name))); };
        load(mVersion, "jade_api_version");
        load(mRegister, "jade_register_provider");
        load(mUnregister, "jade_unregister_provider");
        load(mTarget, "jade_ctx_target");
        load(mTypeId, "jade_ctx_type_id");
        load(mPosition, "jade_ctx_position");
        load(mShowDetails, "jade_ctx_show_details");
        load(mServerConnected, "jade_ctx_server_connected");
        load(mAddLine, "jade_ctx_add_line");
        load(mSetIconItem, "jade_ctx_set_icon_item");
        load(mSetData, "jade_ctx_set_data");
        load(mGetData, "jade_ctx_get_data");
    }

    jade_api_version_t          mVersion         = nullptr;
    jade_register_provider_t    mRegister        = nullptr;
    jade_unregister_provider_t  mUnregister      = nullptr;
    jade_ctx_target_t           mTarget          = nullptr;
    jade_ctx_type_id_t          mTypeId          = nullptr;
    jade_ctx_position_t         mPosition        = nullptr;
    jade_ctx_show_details_t     mShowDetails     = nullptr;
    jade_ctx_server_connected_t mServerConnected = nullptr;
    jade_ctx_add_line_t         mAddLine         = nullptr;
    jade_ctx_set_icon_item_t    mSetIconItem     = nullptr;
    jade_ctx_set_data_t         mSetData         = nullptr;
    jade_ctx_get_data_t         mGetData         = nullptr;
};

}
