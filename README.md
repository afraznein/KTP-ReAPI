# KTP-ReAPI

**Custom fork of [ReAPI](https://github.com/rehlds/ReAPI) with KTP-ReHLDS hook support**

AMX Mod X module, using API regamedll & rehlds with custom KTP-ReHLDS extensions

---

## 🎯 KTP Modifications

This fork adds support for **KTP-ReHLDS custom hooks** to enable advanced competitive match features:

### Added Hooks

#### `RH_SV_UpdatePausedHUD`
- **Purpose**: Enables real-time HUD updates during pause
- **Type**: `IVoidHookChain<>` (no parameters)
- **Use Case**: Allows plugins to update pause timer HUD, remaining time, and match state in real-time during selective pause
- **Requirements**: KTP-ReHLDS 3.14+

### Integration with KTP Ecosystem

This module is designed to work with:
- **KTP-ReHLDS**: Custom ReHLDS fork with selective pause system
- **KTP Match Handler**: Competitive match management plugin
- **KTP Cvar Checker**: Real-time cvar enforcement

---

## 📦 Changes from Upstream ReAPI

### Code Changes
- **reapi_engine_const.inc**: Added `RH_SV_UpdatePausedHUD` enum constant
- **hook_list.h**: Added `RH_SV_UpdatePausedHUD` to engine hook enum
- **hook_list.cpp**: Registered hook in `hooklist_engine` table
- **hook_callback.h**: Added function declaration for `SV_UpdatePausedHUD`
- **hook_callback.cpp**: Implemented hook callback handler
- **rehlds_api.h**: Updated with KTP-ReHLDS headers containing custom hooks

### Build Changes
- **Upgraded to Visual Studio 2022 (v143 toolset)**
- Updated from `v140_xp` to `v143` platform toolset
- Uses KTP-ReHLDS headers instead of standard ReHLDS headers

---

## Build instructions

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

## 🚀 Deployment

### Installation

1. **Build or download** `reapi_amxx.dll` (Windows) or `reapi_amxx_i386.so` (Linux)
2. **Copy to**: `<game>/addons/amxmodx/modules/`
3. **Enable in** `<game>/addons/amxmodx/configs/modules.ini`:
   ```ini
   reapi_amxx.dll     ; Windows
   reapi_amxx_i386.so ; Linux
   ```
4. **Restart server**

### Requirements for KTP Hooks

To use KTP-specific hooks, you must be running:
- **KTP-ReHLDS 3.14+** (standard ReHLDS will not have the custom hooks)
- Plugins compiled with KTP-ReAPI include files

### Compatibility

- ✅ **Backward compatible** with standard ReHLDS/ReAPI plugins
- ✅ KTP hooks are **optional** - plugins will gracefully skip if not available
- ✅ Works with **ReGameDLL** and all standard ReAPI features

---

## 📋 Version History

### KTP-ReAPI v1.0 (2025-11-16)
- Initial KTP fork based on ReAPI 5.26+
- Added `RH_SV_UpdatePausedHUD` hook support
- Upgraded to VS 2022 toolset
- Integrated KTP-ReHLDS headers

---

## 🔗 Related Projects

- **[KTP-ReHLDS](https://github.com/afraznein/KTP-ReHLDS)** - Custom ReHLDS fork with selective pause
- **[KTP Match Handler](https://github.com/afraznein/KTPMatchHandler)** - Competitive match management
- **[Upstream ReAPI](https://github.com/rehlds/ReAPI)** - Original ReAPI project
- **[ReHLDS](https://github.com/dreamstalker/rehlds)** - Reverse-engineered HLDS

---

## 📝 Credits

**KTP Fork Maintainer**: Nein_
**Original ReAPI**: s1lentq and ReAPI contributors
**License**: GPL v3

---

## 🤝 Contributing

This is a specialized fork for KTP competitive infrastructure. For general ReAPI issues, please see the [upstream repository](https://github.com/rehlds/ReAPI).

For KTP-specific features or bugs, feel free to open issues or submit pull requests.
