---
name: cpp-dev
description: Use BEFORE modifying any KTPReAPI C++ (extension-mode bridge, ReHLDS hookchain natives, the vtable header) — the Metamod-path-only teardown trap, the vtable ABI contract, the build-success lie, and verify-by-md5. Also use when planning a change, to know which invariants it touches.
---

# KTPReAPI Development

This is a fork of s1lentq/reapi bridging KTP-ReHLDS and KTPAMXX plugins in
**extension mode** (`REAPI_NO_METAMOD`, no Metamod — engine funcs come from
KTPAMXX's `MF_GetEngineFuncs()`/`GetGlobalVars()`) on a 24-instance production
fleet. The rules below each encode a real production finding; follow them even
when they feel paranoid.

## Hard safety rules
- **NEVER restart game servers** without the operator's explicit permission in
  the current conversation.
- Binaries deploy as `modules/reapi_ktp_i386.so.new` and swap at the 03:00 ET
  nightly restart.
- Run the `ktp-code-review` agent before staging any nontrivial change.
- Commit source before or at ship. The vtable header in this repo is mirrored
  by hand into two other repos (KTPReHLDS, KTPAMXX) — an uncommitted change
  here is a silent fork of an ABI contract three repos rely on.

## The Metamod-path-only teardown trap (most important rule in this file)
`REAPI_NO_METAMOD` is unconditionally defined and included first by
`precompiled.h`, so every `#ifndef REAPI_NO_METAMOD` block (most of `main.cpp`,
`meta_api.cpp`, `dllapi.cpp`, `h_export.cpp`) is compile-time dead on **every**
platform, including MSVC — keep it for upstream mergeability, never report it
as dead code, but never assume it still runs anywhere.

That leaves a recurring bug class: anything that used to run from
`ServerDeactivate_Post`/`Meta_Detach` in the old Metamod flow has **no**
extension-mode equivalent unless someone explicitly ported it. Confirmed
instances in this repo: `AMXX_Detach` is an empty stub, so
`ExtensionMode_Shutdown()` (hookchain unregister), entity-callback cleanup on
map change, `gmsg*` user-message-ID init, and `g_pGameRules` nulling all have
doc comments claiming a caller that doesn't exist. **When adding any new
Metamod-guarded init/teardown, verify a real extension-mode call site exists —
don't trust a comment that says one does; this repo has shipped that lie more
than once.**

`AMXX_Detach` is now a **live wiring point**, not decoration — KTPAMXX 2.7.21+
actually invokes it during `KTP_ExtensionShutdown`. Anything reapi needs to do
at shutdown (unregister its own ReHLDS hooks, flush, log) belongs there.

## Vtable ABI contract
`rehlds_api.h`'s `IRehldsHookchains` vtable is a hand-mirrored ABI contract
across **three** repos: KTPReHLDS (source of truth), KTPReAPI, and KTPAMXX. It
has already been reshaped once (a hook inserted mid-interface, more appended)
without bumping `REHLDS_API_VERSION_MINOR` — so the existing version gate
cannot detect an engine/module vtable mismatch if a stale engine and current
reapi ever pair up. Any future vtable edit: **append-only, never mid-insert**,
and if slots must move, bump the minor version in all three copies the same
day. The header also declares several KTP hookchain registry entries that
reapi itself never wires into a hook list — those exist purely to keep vtable
*order* matching what KTPAMXX/DODX also consume; don't delete an
unused-looking entry without checking all three repos first.

## The build wrapper lies about success
`build_linux.sh` prints "BUILD COMPLETE!" by checking that `build/` exists,
not that the `.so` was produced — a failed compile still leaves the **stale
previous** `reapi_ktp_i386.so` sitting in the staging folder, and the banner
says success. **After every build, confirm the staged `.so`'s mtime/md5
actually changed** before trusting the banner or shipping it.

Relatedly: the extension-mode `Plugin_info.version` string is a hardcoded
literal, independent of the (separately known-stale) appversion banner. There
are now three places the version can lie — the hardcode, the banner, the
CHANGELOG — so **only the `.so` md5 is trustworthy** for verifying what's
actually deployed.

## CI is not a safety net here
Push CI has been fully broken since extension mode landed (Dec 2025): the
Linux job moves a filename CMake stopped producing, the Windows project never
compiles `extension_mode.cpp` (permanent link failure), and even a fixed Linux
job would die on leftover upstream signing steps referencing secrets this fork
doesn't have. Treat every push as **unverified by CI** — a green run doesn't
exist to lean on. Build locally and smoke-test manually.

## Include contract
`reapi.inc` is DUAL-COPY between this repo's `extra/` and KTPAMXX's
`plugins/include/` — every native/forward signature change must be mirrored
into both, or plugins compiled against one copy silently diverge from the
runtime behavior described by the other.

## Workflow
1. **Build**: `wsl bash -c "cd '/mnt/n/Nein_/KTP Git Projects/KTPReAPI' && bash build_linux.sh"`
   (CMake → `build/`, auto-stages `reapi_ktp_i386.so` to the KTP DoD Server
   test tree). Confirm the staged `.so`'s md5 actually moved — the wrapper's
   success banner cannot be trusted (see above).
2. **Review**: `ktp-code-review` agent before staging anything nontrivial.
3. **Smoke test on the Tier-2 runner** (data server, `/opt/ktp-tier2-runner`):
   before/after comparison for any hookchain or lifecycle change (boot → map
   change → quit, exit 0). The runner's module stack must match the fleet;
   sync is deliberate, never automated.
4. **Fleet stage**: `.new` via paramiko to all 24 active instances; md5-verify
   every staged file.
5. **Post-activation verify**: 24/24 on the new md5, no leftover `.new`, zero
   new cores — check `/tmp` (`find /tmp -maxdepth 1 -name 'core.*' -mtime -1`),
   NOT the game trees (that search matches only `core.so`/`core.ini` and
   always looks clean). Verify by md5, never the console banner.

## Versioning
Bump the version (`5.29.0.NNN-ktp`) for every shipped change, write the
CHANGELOG.md entry with what/why + the `.so` md5 once built, and update the
version line in README.md. Comments: short, why-not-what, no ticket IDs, never
delete a tripwire fact while shortening.
