# KTP-ReAPI

**Version 5.29.0.365-ktp** | ReAPI fork for KTP competitive Day of Defeat infrastructure

A specialized fork of [ReAPI](https://github.com/rehlds/ReAPI) modified to run as a **ReHLDS extension module without Metamod**. Operates directly through ReHLDS hookchains and exposes custom KTP-ReHLDS engine hooks to AMXX plugins.

---

## Purpose

ReAPI bridges the Half-Life engine (ReHLDS) and AMX Mod X plugins. KTP-ReAPI extends this bridge in two ways:

1. **Extension Mode** - Runs without Metamod by using ReHLDS hookchains directly
2. **Custom KTP Hooks** - Exposes KTP-ReHLDS hooks unavailable in standard ReAPI

**Why this fork exists:**
- Wall penetration breaks when using ReHLDS + Metamod together
- KTP bypasses this by loading KTPAMXX as a ReHLDS extension directly
- Standard ReAPI requires Metamod and doesn't know about KTP-ReHLDS custom hooks
- KTP-ReAPI enables real-time HUD updates during pause, RCON auditing, and map change interception

---

## Extension Mode

Extension mode is always on in this fork — `REAPI_NO_METAMOD` is defined unconditionally in `reapi/src/extension_mode.h`. There is no build option that turns it off, and the shipped `.so` cannot load under Metamod.

| Feature | Standard ReAPI | KTP-ReAPI (Extension Mode) |
|---------|---------------|---------------------------|
| Metamod | Required | Not needed |
| Loading | Metamod plugin | AMXX module |
| Engine access | Via Metamod | Via KTPAMXX/ReHLDS |
| DLL hooks | Metamod DLL API | ReHLDS hookchains |
| Dependencies | Metamod + ReHLDS | KTP-ReHLDS + KTPAMXX only |

**Metamod hooks replaced with ReHLDS equivalents:**
- `ServerActivate_Post` -> `SV_ActivateServer` hookchain
- `OnFreeEntPrivateData` -> `ED_Free` hookchain

**Engine functions obtained from KTPAMXX:**
```cpp
enginefuncs_t* pEngFuncs = g_amxxapi.GetEngineFuncs();
globalvars_t* pGlobals = g_amxxapi.GetGlobalVars();
```

### Requirements

- **KTPAMXX** (v2.6.10+) - Modified AMX Mod X with `GetEngineFuncs()`/`GetGlobalVars()` exports. 2.6.10 is the floor because it is the first version that calls `modules_callPluginsUnloading()` before `plugin_init`; on 2.6.9 this module exports `AMXX_PluginsUnloading()` but nothing invokes it, so hookchains accumulate across map changes.
- **KTP-ReHLDS** (v3.22+) - Modified ReHLDS with hookchain support
- **NOT compatible** with standard AMX Mod X or Metamod

---

## Architecture

```
+-----------------------------------------------------------+
|                 KTPAMXX (AMX Mod X Fork)                  |
|  - Loads reapi module                                     |
|  - Provides GetEngineFuncs() / GetGlobalVars() to modules |
+----------------------------+------------------------------+
                             | AMXX Module Interface
                             v
+-----------------------------------------------------------+
|              KTP-ReAPI (Extension Mode)                    |
|  - Defines REAPI_NO_METAMOD                               |
|  - Stubs Metamod macros                                   |
|  - Registers ReHLDS hooks (SV_ActivateServer, ED_Free)    |
+----------------------------+------------------------------+
                             | ReHLDS API (hookchains)
                             v
+-----------------------------------------------------------+
|                 KTP-ReHLDS (Engine Fork)                   |
|  - Provides rehlds_api.h interface                        |
|  - Exposes hookchains for server events                   |
|  - Custom: SV_UpdatePausedHUD(), SV_Rcon(), etc.          |
+-----------------------------------------------------------+
```

---

## Custom KTP Hooks

These hooks require KTP-ReHLDS and are not available in standard ReAPI. Plugins should guard usage with `#if defined` checks.

### `RH_SV_UpdatePausedHUD`

Called **every frame** while the server is paused. Enables real-time HUD updates (countdown timers, pause status) even with `pausable 0`.

- **Type**: `IVoidHookChain<>` (no parameters, void return)
- **Frequency**: Every `host_frametime` during pause (~60-1000 Hz)
- **Consumer**: KTPMatchHandler - displays live MM:SS pause countdown

### `RH_SV_Rcon`

Called when any RCON command is received. Provides command text, source IP, and authentication status.

- **Type**: `IVoidHookChain<const char *, const char *, bool>`
- **Parameters**: `command`, `from_ip`, `is_valid`
- **Consumer**: KTPAdminAudit - RCON audit logging with Discord notifications

### `RH_PF_changelevel_I`

Called when the game DLL triggers a map change via `pfnChangeLevel` (timelimit, round end, etc.). Fires before the map actually changes. Return `HC_SUPERCEDE` to block.

- **Type**: `IVoidHookChain<const char *, const char *>`
- **Parameters**: `map` (target map), `landmark` (startspot)
- **Consumer**: KTPMatchHandler - match state persistence across map changes

### `RH_Host_Changelevel_f`

Called when `server_cmd("changelevel ...")` or console changelevel executes. Fires before the map changes. Return `HC_SUPERCEDE` to block.

- **Type**: `IVoidHookChain<const char *, const char *>`
- **Parameters**: `map` (target map), `startspot` (usually empty)
- **Consumer**: KTPMatchHandler - match state finalization before map change

### Plugin Usage Example

```pawn
#include <amxmodx>
#include <reapi>

public plugin_init() {
    register_plugin("Example", "1.0", "Author");

    #if defined RH_Host_Changelevel_f
        new HookChain:h = RegisterHookChain(RH_Host_Changelevel_f, "OnChangelevel", .post = false);
        if (h == INVALID_HOOKCHAIN) {
            log_amx("[Match] changelevel hook did not register");
        }
    #endif
}

#if defined RH_Host_Changelevel_f
public OnChangelevel(const map[], const startspot[]) {
    log_amx("[Match] Map changing to %s", map);
    return HC_CONTINUE; // or HC_SUPERCEDE to block
}
#endif
```

**Registration failure is checkable, not fatal (5.29.0.365+).** `RegisterHookChain`
logs the reason and returns `INVALID_HOOKCHAIN` (0) instead of aborting the calling
public, so a plugin that ignores the return value now runs on with a hook that was
never installed. Check it.

**Hookchain registrations are cleared on every map change.** In extension mode
`AMXX_PluginsUnloading()` and `ExtensionMode_Shutdown()` clear the hook, message,
and query-file managers, so every registration is dropped per map. Register in
`plugin_init`; never cache a hook handle across a map change — it is dead on the
other side. This is a real behavioral difference from Metamod-mode ReAPI.

---

## Comparison: Standard vs KTP

| Feature | Standard ReAPI | KTP-ReAPI |
|---------|---------------|-----------|
| All standard ReHLDS hooks | Yes | Yes |
| `RH_SV_UpdatePausedHUD` | No | Yes |
| `RH_SV_Rcon` | No | Yes |
| `RH_PF_changelevel_I` | No | Yes |
| `RH_Host_Changelevel_f` | No | Yes |
| Standard ReAPI plugin API | Yes | Yes |
| Requires KTP-ReHLDS | No | Always — this build has no Metamod path |

---

## Build Instructions

### Linux (WSL)

```bash
wsl bash -c "cd '/mnt/n/Nein_/KTP Git Projects/KTPReAPI' && bash build_linux.sh"
```

Output: `build/reapi/reapi_ktp_i386.so`

The build script auto-stages to `KTP DoD Server/serverfiles/dod/addons/ktpamx/modules/`.

**Requirements:**
- cmake >= 3.10
- GCC >= 4.9.2 with 32-bit multilib (`sudo apt-get install gcc-multilib g++-multilib`)

**Compiler options:**
```
./build.sh --compiler=[icc|gcc|clang] --jobs=[N]
```

### Windows

1. Open `msvc/reapi.sln` in Visual Studio 2022
2. Select `Release` / `Win32`
3. Build -> Rebuild Solution

Output: `msvc/Release/reapi_amxx.dll`

---

## Installation

### KTP Servers (Extension Mode)

The module is deployed as part of the KTPAMXX module stack:

```
dod/addons/ktpamx/modules/reapi_ktp_i386.so
```

Loaded automatically by KTPAMXX via `modules.ini`:
```ini
reapi
```
(The bare name is what the deployed `modules.ini` carries; KTPAMXX appends `_ktp_i386.so`. Either form resolves to the same file.)

Include files for plugin compilation are maintained here in `reapi/extra/amxmodx/scripting/include/` and mirrored into `KTPAMXX/plugins/include/` on release.

### Server Configuration

```
// Engine pause disabled - KTP-ReAPI/KTPMatchHandler handle pausing
pausable 0
```

---

## Changes from Upstream

### New Files
| File | Purpose |
|------|---------|
| `reapi/src/extension_mode.h` | `REAPI_NO_METAMOD` define, Metamod macro stubs |
| `reapi/src/extension_mode.cpp` | Extension mode init, ReHLDS hook registration |
| `build_linux.sh` | Linux/WSL build script with auto-staging |

### Modified Files
| File | Changes |
|------|---------|
| `reapi/src/main.cpp` | Extension mode init in `OnAmxxAttach()`, engine funcs from KTPAMXX |
| `reapi/src/main.h` | Guard Metamod functions with `#ifndef REAPI_NO_METAMOD` |
| `reapi/src/meta_api.cpp` | Guard entire file |
| `reapi/src/dllapi.cpp` | Guard entire file |
| `reapi/src/h_export.cpp` | Guard `GiveFnptrsToDll` export |
| `reapi/src/amxxmodule.cpp/h` | Add `PFN_GET_ENGINE_FUNCS`/`PFN_GET_GLOBAL_VARS` |
| `reapi/src/hook_list.h/cpp` | Add 4 KTP hook entries |
| `reapi/src/hook_callback.h/cpp` | Add KTP hook callback handlers |
| `reapi/CMakeLists.txt` | Add `extension_mode.cpp` to sources |

### Build Changes
- Upgraded to Visual Studio 2022 (v143 toolset) from v140_xp
- Uses KTP-ReHLDS headers instead of standard ReHLDS
- Added WSL build support

---

## Version Information

- **Based on**: upstream ReAPI 5.29.0 (exact merge-base unrecorded)
- **KTP Fork**: 5.29.0.365-ktp
- **Platform**: Visual Studio 2022 (v143) / GCC with 32-bit multilib
- **Compatible with**: KTPAMXX 2.6.10+, KTP-ReHLDS 3.22+

The `appversion.h` banner still reads `5.29.0.360-dev+m` and is *not* a deploy
check — verify by the md5 of `reapi_ktp_i386.so`.

See [CHANGELOG.md](CHANGELOG.md) for full version history.

---

## Related Projects

**KTP Stack:**
- [KTP-ReHLDS](https://github.com/afraznein/KTP-ReHLDS) - Custom game engine with pause/hook support
- [KTPAMXX](https://github.com/afraznein/KTPAMXX) - AMX Mod X fork (scripting platform)
- [KTPMatchHandler](https://github.com/afraznein/KTPMatchHandler) - Match management plugin (primary consumer)
- [KTPAdminAudit](https://github.com/afraznein/KTPAdminAudit) - RCON audit logging plugin

**Upstream:**
- [ReAPI](https://github.com/rehlds/ReAPI) - Original project
- [ReHLDS](https://github.com/dreamstalker/rehlds) - Reverse-engineered HLDS

---

## License

GPL v3 - Same as upstream ReAPI. See [LICENSE](LICENSE).
