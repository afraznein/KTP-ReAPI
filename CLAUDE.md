# KTPReAPI - Claude Code Context

**REQUIRED: Before modifying any C++ or include files in this repo, invoke the `cpp-dev` skill** (`.claude/skills/cpp-dev/SKILL.md`). It carries the Metamod-path-only teardown trap, the vtable ABI contract, and the build/verify workflow; do not edit source without it loaded.

## Build Command
To build this project, use:
```bash
wsl bash -c "cd '/mnt/n/Nein_/KTP Git Projects/KTPReAPI' && bash build_linux.sh"
```

This will:
1. Build using CMake
2. Output to `build/`
3. Auto-stage to `N:\Nein_\KTP Git Projects\KTP DoD Server\serverfiles\dod\addons\ktpamx\modules\`

## Project Structure
- `build_linux.sh` - WSL build script
- `build.sh` - Core CMake build script
- `reapi/` - Main source directory
- `build/` - Build output

## Purpose
ReAPI fork modified for extension mode operation. Provides engine hooks and forwards to AMXX plugins without requiring Metamod. Key features:
- Server pause control (`rh_set_server_pause`)
- Paused HUD updates (`RH_SV_UpdatePausedHUD` hook)
- RCON command audit (`RH_SV_Rcon` hook)
- Map change interception (`RH_PF_changelevel_I`, `RH_Host_Changelevel_f` hooks)
- Engine function access via `MF_GetEngineFuncs()` instead of Metamod

## KTP-ReHLDS Custom Hooks
| Hook | Purpose |
|------|---------|
| `RH_SV_UpdatePausedHUD` | Real-time HUD updates during pause |
| `RH_SV_Rcon` | RCON command audit logging |
| `RH_PF_changelevel_I` | Intercept game DLL pfnChangeLevel calls |
| `RH_Host_Changelevel_f` | Intercept console changelevel commands |

## Build Output
| File | Destination |
|------|-------------|
| `reapi_ktp_i386.so` | `addons/ktpamx/modules/` |

## Dependencies
- CMake
- GCC with 32-bit support

## Server Deployment

Deploy compiled module to production servers using Python/Paramiko.

**Server Credentials:** never in this repo (it is PUBLIC). The table that used to
live here listed the pre-2026-05-31 rotated values — that credential is dead and
authenticates nowhere, but publishing host/user/password rows is the exact habit
that caused the earlier leak. Current credentials, the real IPs behind the
`<ATL_BM_GAME_IP>`-style placeholders, and the full paramiko SSH documentation live
in the private root context: `N:\Nein_\KTP Git Projects\CLAUDE.md` § Server
Credentials.

**Remote Path:** `~/dod-{port}/serverfiles/dod/addons/ktpamx/modules/reapi_ktp_i386.so`

## Related Projects
- `N:\Nein_\KTP Git Projects\KTPAMXX` - AMX Mod X fork (provides module API)
- `N:\Nein_\KTP Git Projects\KTPReHLDS` - ReHLDS fork (engine layer)
- `N:\Nein_\KTP Git Projects\KTPMatchHandler` - Primary consumer (pause system)
- `N:\Nein_\KTP Git Projects\KTP DoD Server` - Test server with staged modules
