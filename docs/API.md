# Jade API for other mods

Other LeviLamina mods can add lines, icons and server-side data to Jade's tooltip. The API is a set of C functions exported by `Jade.dll`, wrapped by the header-only class `jadeapi::Api` in [`include/jade/JadeApi.h`](../include/jade/JadeApi.h). No linking is needed. If Jade is not installed, `available()` returns false and nothing happens.

This corresponds to `IWailaPlugin`, `IBlockComponentProvider`, `IEntityComponentProvider` and `IServerDataProvider` of the original Jade.

## Setup

1. Add the include directory to your `xmake.lua`: `add_includedirs("<path to Jade>/include")`
2. Add Jade to the dependencies in your `manifest.json` so that Jade is enabled first:

   ```json
   "dependencies": [{ "name": "Jade" }]
   ```

3. Register providers in your mod's `enable()` and unregister them in `disable()`.

## Example

```cpp
#include "jade/JadeApi.h"

std::vector<uint64_t> gProviders;

void goldLine(jade_context* ctx, void*) {
    auto& jade = jadeapi::Api::get();
    int x, y, z;
    jade.position(ctx, x, y, z);
    jade.addLine(ctx, "Gold at " + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z));
}

bool MyMod::enable() {
    auto& jade = jadeapi::Api::get();
    if (!jade.available()) return true;
    jade_provider_desc d{};
    d.uid                = "my_mod:gold";          // also the config entry
    d.display_name       = "My Mod: Gold Block";   // shown in the settings screen
    d.target             = JADE_TARGET_BLOCK;
    d.targets            = "minecraft:gold_block"; // comma separated; empty for all
    d.enabled_by_default = 1;
    d.append             = goldLine;
    gProviders.push_back(jade.registerProvider(d));
    return true;
}

bool MyMod::disable() {
    for (auto id : gProviders) jadeapi::Api::get().unregisterProvider(id);
    gProviders.clear();
    return true;
}
```

## `jade_provider_desc`

| Field | Meaning |
|---|---|
| `uid` | `namespace:path`. Also the config entry. The namespaces `minecraft`, `jade`, `jade_access` and `debug` are reserved |
| `display_name` | Name in the settings screen (`NULL` for the uid) |
| `target` | `JADE_TARGET_BLOCK` or `JADE_TARGET_ENTITY` |
| `targets` | Comma-separated block or entity IDs. `NULL` or empty for all |
| `priority` | Offset from the default body position. Smaller is higher |
| `enabled_by_default` | `0` or `1` |
| `config_icon` | Item ID for the settings screen (`NULL` for none) |
| `append` | Adds lines with `addLine` (main thread) |
| `icon` | Replaces the icon with `setIconItem` (main thread) |
| `server` | Creates a string with `setData` on the server thread. Only in a local world; on other servers it is not called. Read it with `data()` in `append` or `icon` |
| `user` | Passed to the callbacks |

## Context functions

| Function | Use in | Returns |
|---|---|---|
| `target(ctx)` | all | `JADE_TARGET_BLOCK` / `JADE_TARGET_ENTITY` |
| `typeId(ctx)` | all | Block or entity ID, such as `minecraft:furnace` |
| `position(ctx, x, y, z)` | all | Block position (entities: the block they stand in) |
| `showDetails(ctx)` | all | The details key is held |
| `serverConnected(ctx)` | all | Server data is available (local world) |
| `addLine(ctx, text)` | `append` | Adds a line (UTF-8) |
| `setIconItem(ctx, id)` | `icon` | Uses the item as the icon |
| `setData(ctx, text)` | `server` | Stores a string for the client |
| `data(ctx)` | `append`, `icon` | The string from `server`, or `NULL` |

## Rules

- Registrations are applied outside a world. A provider registered while in a world appears after leaving and joining again.
- `jade_context` and the strings you get from it are valid only during the callback.
- Do not call `unregisterProvider` inside a callback: it waits for running callbacks to finish.
- After `unregisterProvider` returns, the callbacks are not called again, so your DLL can be unloaded.
