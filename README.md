# KTP-ReAPI

**Custom AMX Mod X module bridging KTP-ReHLDS engine hooks to plugin developers**

A specialized fork of [ReAPI](https://github.com/rehlds/ReAPI) that exposes custom KTP-ReHLDS engine hooks to AMX ModX plugins, enabling advanced competitive features like real-time pause HUD updates, enhanced match control, and deeper engine integration.

---

## 🎯 Purpose

ReAPI is a bridge between the Half-Life engine (ReHLDS) and AMX ModX plugins. **KTP-ReAPI extends this bridge** to expose custom KTP-ReHLDS hooks that aren't available in standard ReAPI.

**Why this fork exists:**
- ✅ Standard ReAPI doesn't know about KTP-ReHLDS custom hooks
- ✅ KTP-ReHLDS adds engine-level pause features (pause without `pausable 1`)
- ✅ KTP-ReAPI exposes these features to AMX plugins via new hook constants
- ✅ Enables real-time HUD updates during pause (impossible with standard ReAPI)

---

## 🏗️ Architecture Position

KTP-ReAPI sits between the engine and plugins in the KTP stack:

```
┌─────────────────────────────────────────────────┐
│  Layer 4: KTPMatchHandler (AMX Plugin)          │
│  Uses: RegisterHookChain(RH_SV_UpdatePausedHUD) │
└─────────────────────────────────────────────────┘
                     ↓ Calls
┌─────────────────────────────────────────────────┐
│  Layer 3: KTPCvarChecker (AMX Plugin)           │
│  Uses: Standard ReAPI + KTP hooks               │
└─────────────────────────────────────────────────┘
                     ↓ Uses
┌─────────────────────────────────────────────────┐
│  Layer 2: KTP-ReAPI (AMX Module) ← YOU ARE HERE │
│  Exposes: RH_SV_UpdatePausedHUD hook constant   │
│  Bridges: KTP-ReHLDS hooks → AMX plugins        │
└─────────────────────────────────────────────────┘
                     ↓ Calls
┌─────────────────────────────────────────────────┐
│  Layer 1: KTP-ReHLDS (Game Engine)              │
│  Provides: SV_UpdatePausedHUD() engine function │
│  Features: Pause without pausable, HUD updates  │
└─────────────────────────────────────────────────┘
```

---

## 🔌 Custom KTP Hooks

### `RH_SV_UpdatePausedHUD`

**The critical hook that makes real-time pause HUD possible.**

#### What it does:
- Called **every frame** by KTP-ReHLDS while the server is paused
- Allows plugins to update HUD elements with live countdown timers
- Works even with `pausable 0` (engine pause disabled)
- Enables MM:SS pause timer display for all players

#### Technical Details:
- **Type**: `IVoidHookChain<>` (no parameters, void return)
- **Frequency**: Called every `host_frametime` during pause (~60-100 Hz)
- **Requirements**: KTP-ReHLDS 3.14+ running on server
- **Fallback**: Gracefully ignored if KTP-ReHLDS not present

#### Plugin Usage Example:

```pawn
// In your plugin includes
#include <amxmodx>
#include <reapi>

// Check if KTP-ReAPI hook is available
#if defined RH_SV_UpdatePausedHUD
    #define KTP_REAPI_AVAILABLE
#endif

public plugin_init() {
    register_plugin("My Match Plugin", "1.0", "Author");

    #if defined KTP_REAPI_AVAILABLE
        // Register the hook
        RegisterHookChain(RH_SV_UpdatePausedHUD, "OnPausedHUDUpdate", .post = false);
        server_print("[KTP] Using KTP-ReAPI pause HUD updates");
    #else
        server_print("[WARNING] KTP-ReAPI not available, pause HUD disabled");
    #endif
}

#if defined KTP_REAPI_AVAILABLE
public OnPausedHUDUpdate() {
    // Called EVERY FRAME during pause
    // Update HUD for all players with current pause time remaining

    new iTimeRemaining = g_iPauseTimeRemaining; // Your timer variable
    new iMinutes = iTimeRemaining / 60;
    new iSeconds = iTimeRemaining % 60;

    set_hudmessage(255, 255, 0, -1.0, 0.35, 0, 0.0, 0.1, 0.0, 0.0, -1);
    show_hudmessage(0, "⏸ PAUSED ⏸^n%02d:%02d", iMinutes, iSeconds);

    return HC_CONTINUE;
}
#endif
```

#### How KTPMatchHandler Uses It:

```pawn
// Real implementation from KTPMatchHandler v0.4.0
public OnPausedHUDUpdate() {
    if (!g_bIsPaused) {
        return HC_CONTINUE; // Safety check
    }

    // Calculate time remaining
    new iElapsed = get_systime() - g_iPauseStartTime;
    new iRemaining = g_iPauseDuration - iElapsed;

    if (iRemaining < 0) iRemaining = 0;

    new iMinutes = iRemaining / 60;
    new iSeconds = iRemaining % 60;

    // Update HUD for all players
    set_hudmessage(255, 255, 0, -1.0, 0.35, 0, 0.0, 0.1, 0.0, 0.0, -1);
    show_hudmessage(0, "⏸ PAUSED ⏸^n%02d:%02d remaining^nType /resume to unpause",
        iMinutes, iSeconds);

    return HC_CONTINUE;
}
```

---

## 🆚 Comparison: Standard ReAPI vs KTP-ReAPI

| Feature | Standard ReAPI | KTP-ReAPI |
|---------|---------------|-----------|
| **ReHLDS Hooks** | ✅ All standard hooks | ✅ All standard hooks |
| **ReGameDLL Hooks** | ✅ Full support | ✅ Full support |
| **RH_SV_UpdatePausedHUD** | ❌ Not available | ✅ Available |
| **Future KTP Hooks** | ❌ Not available | ✅ As added to KTP-ReHLDS |
| **Backward Compatibility** | ✅ N/A | ✅ Full compatibility |
| **Requires KTP-ReHLDS** | ❌ No | ⚠️ Only for KTP hooks |
| **Works with Standard ReHLDS** | ✅ Yes | ✅ Yes (KTP hooks ignored) |

---

## 💡 Real-World Impact

### Before KTP-ReAPI:
```
Player pauses match with /pause

Server pauses (pausable 1 or rh_set_server_pause)
Players see: "⏸ PAUSED ⏸" message
              [Never updates, frozen forever]

Players have no idea how much time remains
Admins must manually announce time via rcon say
```

### After KTP-ReAPI:
```
Player pauses match with /pause

KTP-ReHLDS pauses engine (pausable 0 still set!)
KTP-ReHLDS calls SV_UpdatePausedHUD() every frame
KTP-ReAPI forwards to OnPausedHUDUpdate() hook
KTPMatchHandler updates HUD every frame:

Players see: "⏸ PAUSED ⏸"
             "04:37 remaining"
             "Type /resume to unpause"
             [Updates every frame in real-time!]

Players have live countdown
Experience is professional and polished
```

---

## 🎮 Integration with KTP Ecosystem

This module is the **critical middleware** for:

### **KTPMatchHandler** (Primary Consumer)
- Uses `RH_SV_UpdatePausedHUD` for real-time pause HUD
- Displays MM:SS countdown during tactical/technical pauses
- Updates unpause countdown (5...4...3...2...1)
- Shows pause warnings (30s, 10s remaining)

### **KTPCvarChecker** (Potential Consumer)
- Could use for displaying cvar check status during pause
- Future: Show "Checking cvars..." progress during technical pause

### **KTP-ReHLDS** (Engine Layer)
- Provides the `SV_UpdatePausedHUD()` engine function
- KTP-ReAPI exposes it to plugins via hook constant
- Maintains `pausable 0` while allowing selective pause

---

## 📦 Changes from Upstream ReAPI

### Code Changes

**New Hook Constant:**
```pawn
// reapi/extra/amxmodx/scripting/include/reapi_engine_const.inc
enum RehldsHook
{
    // ... existing hooks ...

    /*
    * Description:  Called during pause to allow HUD updates (KTP-ReHLDS custom hook)
    * Params:       ()
    * @note         This is a KTP-ReHLDS specific hook, not available in standard ReHLDS
    */
    RH_SV_UpdatePausedHUD,
};
```

**Hook Registration:**
```cpp
// reapi/src/hook_list.cpp
hook_t hooklist_engine[] = {
    // ... existing hooks ...
    H(SV_UpdatePausedHUD, void, (void), (), ()),
};
```

**Detailed File Changes:**
- **reapi_engine_const.inc**: Added `RH_SV_UpdatePausedHUD` enum constant (line ~380)
- **hook_list.h**: Added `RH_SV_UpdatePausedHUD` to `EngineHook` enum
- **hook_list.cpp**: Registered hook in `hooklist_engine[]` table with callback
- **hook_callback.h**: Added `void SV_UpdatePausedHUD()` function declaration
- **hook_callback.cpp**: Implemented hook callback handler forwarding to plugins
- **rehlds_api.h**: Updated with KTP-ReHLDS headers containing custom engine hooks

### Build Changes
- **Upgraded to Visual Studio 2022 (v143 toolset)**
  - Updated from `v140_xp` (VS2015) to `v143` (VS2022)
  - Improved C++17 standard support
  - Better optimization and debugging
- **Uses KTP-ReHLDS headers** instead of standard ReHLDS headers
  - Located in `reapi/public/rehlds/`
  - Contains KTP-specific hook definitions
  - Forward compatible with future KTP hooks

### Version Information
- **Based on**: ReAPI 5.26+ (upstream)
- **KTP Fork Version**: 1.0
- **Platform Toolset**: Visual Studio 2022 (v143)
- **Compatible with**: ReHLDS 3.x, KTP-ReHLDS 3.14+

---

## 🚀 Quick Start

### For Plugin Developers

**1. Download KTP-ReAPI include files:**
```
reapi/extra/amxmodx/scripting/include/
├── reapi.inc
├── reapi_engine.inc
├── reapi_engine_const.inc  ← Contains RH_SV_UpdatePausedHUD
├── reapi_gamedll.inc
└── ... other includes
```

**2. Include in your plugin:**
```pawn
#include <amxmodx>
#include <reapi>  // Automatically includes reapi_engine_const.inc

// Optional: Check if hook is available
#if defined RH_SV_UpdatePausedHUD
    #pragma semicolon 1
    new bool:g_bKTPReAPIAvailable = true;
#else
    new bool:g_bKTPReAPIAvailable = false;
#endif
```

**3. Register the hook:**
```pawn
public plugin_init() {
    register_plugin("My Plugin", "1.0", "Author");

    #if defined RH_SV_UpdatePausedHUD
        RegisterHookChain(RH_SV_UpdatePausedHUD, "OnPausedHUDUpdate", .post = false);
        server_print("[MyPlugin] KTP-ReAPI pause HUD support enabled");
    #endif
}

#if defined RH_SV_UpdatePausedHUD
public OnPausedHUDUpdate() {
    // Your HUD update code here
    return HC_CONTINUE;
}
#endif
```

**4. Compile with KTP-ReAPI includes:**
```bash
amxxpc YourPlugin.sma -i"path/to/ktp-reapi/include"
```

---

## 🔧 Build Instructions

### Checking requirements

#### Windows
<pre>
Visual Studio 2022 (v143 toolset)
KTP-ReHLDS headers (included in this repository)
</pre>

#### Linux
<pre>
git >= 1.8.5
cmake >= 3.10
GCC >= 4.9.2 (Optional)
ICC >= 15.0.1 20141023 (Optional)
LLVM (Clang) >= 6.0 (Optional)
</pre>

### Building

#### Windows
Use `Visual Studio 2022` to build:
1. Open `msvc/reapi.sln`
2. Select configuration: `Release`
3. Select platform: `Win32`
4. Build → Rebuild Solution

Output: `msvc/Release/reapi_amxx.dll`

#### Linux

* Optional options using `build.sh --compiler=[gcc] --jobs=[N] -D[option]=[ON or OFF]` (without square brackets)

<pre>
-c=|--compiler=[icc|gcc|clang]  - Select preferred C/C++ compiler to build
-j=|--jobs=[N]                  - Specifies the number of jobs (commands) to run simultaneously (For faster building)

<sub>Definitions (-D)</sub>
DEBUG                           - Enables debugging mode
USE_STATIC_LIBSTDC              - Enables static linking library libstdc++
</pre>

* ICC          <pre>./build.sh --compiler=intel</pre>
* LLVM (Clang) <pre>./build.sh --compiler=clang</pre>
* GCC          <pre>./build.sh --compiler=gcc</pre>

##### Checking build environment (Debian / Ubuntu)

<details>
<summary>Click to expand</summary>

<ul>
<li>
Installing required packages
<pre>
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib
sudo apt-get install -y build-essential
sudo apt-get install -y libc6-dev libc6-dev-i386
</pre>
</li>

<li>
Select the preferred C/C++ Compiler installation
<pre>
1) sudo apt-get install -y gcc g++
2) sudo apt-get install -y clang
</pre>
</li>
</ul>

</details>

---

## 📦 Installation & Deployment

### For Server Administrators

**Prerequisites:**
- AMX Mod X 1.9+ installed
- KTP-ReHLDS 3.14+ running (or standard ReHLDS 3.x for basic functionality)
- ReGameDLL (if using game hooks)

**Step 1: Build or Download Module**

Option A - Download prebuilt (recommended):
```bash
# Download from KTP-ReAPI releases
# https://github.com/afraznein/KTP-ReAPI/releases
```

Option B - Build from source (see Build Instructions above):
```bash
# Windows: Use Visual Studio 2022
# Linux: Use build.sh
```

**Step 2: Install Module**

```bash
# Windows
copy reapi_amxx.dll "<game>/addons/amxmodx/modules/"

# Linux
cp reapi_amxx_i386.so "<game>/addons/amxmodx/modules/"
```

**Step 3: Enable in modules.ini**

Edit `<game>/addons/amxmodx/configs/modules.ini`:
```ini
; Add this line:
reapi_amxx.dll     ; Windows
; OR
reapi_amxx_i386.so ; Linux
```

**Step 4: Install Include Files (for plugin compilation)**

```bash
# Copy include files for plugins to use
cp -r reapi/extra/amxmodx/scripting/include/* \
      "<amxmodx>/scripting/include/"
```

**Step 5: Verify Installation**

```bash
# Start server and check AMX Mod X console
# Should see: "[REAPI] Module loaded successfully"

# In server console, check plugins:
amxx list

# Should show any plugins using ReAPI hooks
```

**Step 6: Install KTP-ReHLDS (for KTP hooks)**

KTP-specific hooks (`RH_SV_UpdatePausedHUD`) only work with KTP-ReHLDS:
```bash
# Download and install KTP-ReHLDS
# https://github.com/afraznein/KTP-ReHLDS

# Replace your hlds_linux/hlds.exe with KTP-ReHLDS build
# Set server.cfg: pausable 0  (let ReAPI handle pausing)
```

---

## 🎯 Server Configuration

### Recommended server.cfg Settings

```
// Engine pause disabled - let ReAPI handle it
pausable 0

// Match handler settings (if using KTPMatchHandler)
ktp_pause_duration "300"        // 5-minute pause base time
ktp_pause_extension "120"       // 2-minute extensions
ktp_pause_max_extensions "2"    // Max 2 extensions (9 min total)
```

### Plugin Load Order

Ensure proper load order in `plugins.ini`:
```ini
; Load ReAPI-dependent modules first
reapi_amxx.dll

; Then load plugins that use ReAPI
KTPCvarChecker.amxx    ; Uses standard ReAPI
KTPMatchHandler.amxx   ; Uses RH_SV_UpdatePausedHUD
```

---

## ✅ Compatibility Matrix

| Component | KTP-ReHLDS | Standard ReHLDS | KTP Hooks Work? |
|-----------|-----------|----------------|-----------------|
| **KTP-ReAPI + KTP-ReHLDS** | ✅ Required | ❌ N/A | ✅ Yes |
| **KTP-ReAPI + Standard ReHLDS** | ❌ N/A | ✅ Works | ⚠️ No (graceful fallback) |
| **Standard ReAPI + KTP-ReHLDS** | ✅ Works | ❌ N/A | ❌ No (hooks not exposed) |
| **Standard ReAPI + Standard ReHLDS** | ❌ N/A | ✅ Works | ❌ N/A |

**Key Points:**
- ✅ **Backward compatible**: KTP-ReAPI works with standard ReHLDS (KTP hooks ignored)
- ✅ **Forward compatible**: Standard plugins work with KTP-ReAPI
- ✅ **Graceful degradation**: Plugins check `#if defined RH_SV_UpdatePausedHUD` before using
- ⚠️ **KTP-ReHLDS required**: For `RH_SV_UpdatePausedHUD` to actually be called

---

## 📋 Version History

### KTP-ReAPI v1.0 (2025-11-16)
- ✨ Initial KTP fork based on ReAPI 5.26+
- ✨ Added `RH_SV_UpdatePausedHUD` hook support
- 🔧 Upgraded to Visual Studio 2022 (v143 toolset)
- 🔧 Integrated KTP-ReHLDS 3.14 headers
- 📚 Comprehensive documentation
- ✅ Backward compatibility with standard ReAPI plugins

### Upstream ReAPI 5.26+ (Base)
- All standard ReHLDS/ReGameDLL hooks
- Full AMX ModX module support
- Cross-platform (Windows/Linux)

---

## 🔗 Related KTP Projects

### **KTP Competitive Infrastructure Stack:**

**🔧 Engine Layer:**
- **[KTP-ReHLDS](https://github.com/afraznein/KTP-ReHLDS)** - Custom ReHLDS fork
  - Selective pause system (works with `pausable 0`)
  - Real-time HUD updates during pause
  - Chat during pause support (WIP)
  - Provides `SV_UpdatePausedHUD()` engine function

**🔌 Module Layer:**
- **[KTP-ReAPI](https://github.com/afraznein/KTP-ReAPI)** - This project
  - Exposes KTP-ReHLDS hooks to AMX plugins
  - Bridges engine to plugin layer
  - Backward compatible with standard ReAPI

**🎮 Plugin Layer:**
- **[KTP Match Handler](https://github.com/afraznein/KTPMatchHandler)** - Match management
  - Pause system with real-time countdown
  - Discord integration via relay
  - Match workflow (ready-up, live, pause, etc.)

- **[KTP Cvar Checker](https://github.com/afraznein/KTPCvarChecker)** - Anti-cheat
  - Real-time client cvar enforcement
  - FTP upload of screenshots
  - Player compliance tracking

**🌐 Supporting Services:**
- **[KTP Discord Relay](https://github.com/afraznein/DiscordRelay)** - HTTP relay for Discord
- **[KTP HLTV Kicker](https://github.com/afraznein/KTPHLTVKicker)** - HLTV spectator management

### **Upstream Projects:**
- **[ReAPI (Upstream)](https://github.com/rehlds/ReAPI)** - Original ReAPI project
- **[ReHLDS](https://github.com/dreamstalker/rehlds)** - Reverse-engineered HLDS
- **[ReGameDLL](https://github.com/s1lentq/ReGameDLL_CS)** - Game logic module

---

## 🙏 Acknowledgments

**KTP Fork:**
- **Nein_** ([@afraznein](https://github.com/afraznein)) - KTP-ReAPI fork maintainer
- **KTP Community** - Testing, feedback, competitive insights

**Upstream ReAPI:**
- **s1lentq** - Original ReAPI development and architecture
- **dreamstalker** - Original ReHLDS project foundation
- **ReAPI Contributors** - Module framework and hook system
- **ReHLDS Team** - Engine enhancements and API design

---

## 📝 License

**GPL v3** - Same as upstream ReAPI

This fork maintains GPL v3 licensing from upstream ReAPI project.

See [LICENSE](LICENSE) file for full text.

---

## 🤝 Contributing

### For KTP-Specific Features

This is a specialized fork for **KTP competitive infrastructure**.

**KTP contributions welcome:**
- New KTP-ReHLDS hook integrations
- Documentation improvements
- Bug fixes for KTP-specific features
- Build system enhancements

**Submit issues/PRs at:**
- https://github.com/afraznein/KTP-ReAPI

### For General ReAPI Features

For **general ReAPI issues** (not KTP-specific), please see:
- **[Upstream ReAPI](https://github.com/rehlds/ReAPI)**

We periodically sync with upstream to incorporate improvements.

---

## 💬 Support

**For KTP-ReAPI help:**
- Open an issue: https://github.com/afraznein/KTP-ReAPI/issues
- Check KTP Match Handler docs: https://github.com/afraznein/KTPMatchHandler
- Review KTP-ReHLDS docs: https://github.com/afraznein/KTP-ReHLDS

**For general ReAPI questions:**
- Upstream docs: https://github.com/rehlds/ReAPI
- ReHLDS community forums

---

## 📚 Additional Resources

**KTP Documentation:**
- [KTP Match Handler - Discord Guide](https://github.com/afraznein/KTPMatchHandler/blob/main/DISCORD_GUIDE.md) - Complete KTP stack overview
- [KTP-ReHLDS README](https://github.com/afraznein/KTP-ReHLDS/blob/main/README.md) - Engine modifications
- [KTP Match Handler README](https://github.com/afraznein/KTPMatchHandler/blob/main/README.md) - Plugin usage

**Upstream Documentation:**
- [ReAPI Documentation](https://github.com/rehlds/ReAPI/wiki)
- [ReHLDS API Reference](https://github.com/dreamstalker/rehlds/wiki)
- [AMX Mod X Scripting](https://www.amxmodx.org/api/)

---

**KTP-ReAPI** - The bridge that makes real-time competitive features possible. 🔌
