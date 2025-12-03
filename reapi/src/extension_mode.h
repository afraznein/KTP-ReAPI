// vim: set ts=4 sw=4 tw=99 noet:
//
// KTP ReAPI - Extension Mode Support
// Allows ReAPI to run without Metamod by using ReHLDS hookchains directly
//
// Copyright (C) 2025 KTP
//

#pragma once

// Define this to compile ReAPI without Metamod support
// When defined, ReAPI will:
// - Use ReHLDS hookchains instead of Metamod DLL hooks
// - Get engine/gamedll interfaces directly from ReHLDS
// - Not export Meta_Query/Meta_Attach/Meta_Detach
#define REAPI_NO_METAMOD

#ifdef REAPI_NO_METAMOD

// Stub out Metamod macros that are used throughout the codebase
#define SET_META_RESULT(x)
#define RETURN_META(x) return
#define RETURN_META_VALUE(x, val) return val
#define META_RESULT_STATUS META_RES
#define MRES_IGNORED 0
#define MRES_HANDLED 1
#define MRES_OVERRIDE 2
#define MRES_SUPERCEDE 3

// Stub PLID - not needed without Metamod
#define PLID nullptr

// Stub MAKE_REQUESTID - generate a simple unique ID without Metamod
// In extension mode, we use a simple counter since we don't have Metamod's request ID system
extern int g_nExtRequestId;
#define MAKE_REQUESTID(plid) (++g_nExtRequestId)

// Stub GET_GAME_INFO - get game DLL path from ReHLDS instead of Metamod
// In extension mode, we get this from ReHLDS's server data
#define GET_GAME_INFO(plid, type) ExtensionMode_GetGameInfo(type)

// Game info types (matching Metamod's GINFO_ values)
enum ginfo_t {
	GINFO_NAME = 0,
	GINFO_DESC,
	GINFO_GAMEDIR,
	GINFO_DLL_FULLPATH,
	GINFO_DLL_FILENAME,
	GINFO_REALDLL_FULLPATH
};

// Get game info in extension mode
const char* ExtensionMode_GetGameInfo(ginfo_t type);

// GET_USER_MSG_ID - lookup user message ID by name (stub in extension mode)
// In extension mode, this needs to be provided via AMXX's get_user_msgid native
// or obtained from the engine's message registration
int ExtensionMode_GetUserMsgID(void* plid, const char* msgname, int* size);
#define GET_USER_MSG_ID(plid, msgname, size) ExtensionMode_GetUserMsgID(plid, msgname, size)

// GET_USER_MSG_NAME - lookup user message name by ID (stub in extension mode)
const char* ExtensionMode_GetUserMsgName(void* plid, int msgid, int* size);
#define GET_USER_MSG_NAME(plid, msgid, size) ExtensionMode_GetUserMsgName(plid, msgid, size)

// Game DLL function access - get from ReHLDS in extension mode
// We use void* here because DLL_FUNCTIONS is defined later via typedef struct
extern void* g_pExtDllFuncs;

// Forward declare edict_t (defined in progdefs.h via typedef struct)
struct edict_s;
typedef struct edict_s edict_t;

// MDLL_Spawn and MDLL_Touch are implemented as functions in extension_mode.cpp
// because we need the full DLL_FUNCTIONS definition to access the function pointers
int ExtensionMode_MDLL_Spawn(edict_t* pent);
void ExtensionMode_MDLL_Touch(edict_t* pent, edict_t* other);

#define MDLL_Spawn(pent)      ExtensionMode_MDLL_Spawn(pent)
#define MDLL_Touch(pent, other) ExtensionMode_MDLL_Touch(pent, other)

// g_pFunctionTable stub - not used in extension mode
// In extension mode, code that uses g_pFunctionTable should be guarded with #ifndef REAPI_NO_METAMOD

// Plugin info stub for extension mode - used for logging and AMXX module info
struct plugin_info_stub_t {
	const char* name;
	const char* author;
	const char* version;
	int reload;
	const char* logtag;
};
extern plugin_info_stub_t Plugin_info;

// Global flag to track extension mode (always true when compiled without Metamod)
extern bool g_bExtensionMode;

// Initialize ReAPI in extension mode (called from AMXX_Attach)
bool ExtensionMode_Init();

// Cleanup ReAPI extension mode (called from AMXX_Detach)
void ExtensionMode_Shutdown();

// Register ReHLDS hooks that replace Metamod DLL hooks
void ExtensionMode_RegisterHooks();

// Unregister ReHLDS hooks
void ExtensionMode_UnregisterHooks();

#endif // REAPI_NO_METAMOD
