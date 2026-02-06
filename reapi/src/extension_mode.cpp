// vim: set ts=4 sw=4 tw=99 noet:
//
// KTP ReAPI - Extension Mode Implementation
// Replaces Metamod hooks with ReHLDS hookchains
//
// Copyright (C) 2025 KTP
//

#include "precompiled.h"

#ifdef REAPI_NO_METAMOD

#include "extension_mode.h"

bool g_bExtensionMode = true;
int g_nExtRequestId = 0;

// Game DLL function table for extension mode (stored as void* to avoid forward declaration issues)
void* g_pExtDllFuncs = nullptr;

// Engine function table pointer - in Metamod mode this points to Metamod's hooked table
// In extension mode, we point directly to g_engfuncs
// This starts as nullptr and is set to &g_engfuncs in OnAmxxAttach/ExtensionMode_Init
enginefuncs_t* g_pengfuncsTable = nullptr;

// DLL function table pointer - not used in extension mode, but declared extern in reapi_utils.h
// Provide a stub definition to satisfy the linker
DLL_FUNCTIONS* g_pFunctionTable = nullptr;

// Wrapper functions for MDLL_ macros - these have access to the full DLL_FUNCTIONS definition
int ExtensionMode_MDLL_Spawn(edict_t* pent)
{
	if (g_pExtDllFuncs)
	{
		DLL_FUNCTIONS* funcs = (DLL_FUNCTIONS*)g_pExtDllFuncs;
		if (funcs->pfnSpawn)
			return funcs->pfnSpawn(pent);
	}
	return 0;
}

void ExtensionMode_MDLL_Touch(edict_t* pent, edict_t* other)
{
	if (g_pExtDllFuncs)
	{
		DLL_FUNCTIONS* funcs = (DLL_FUNCTIONS*)g_pExtDllFuncs;
		if (funcs->pfnTouch)
			funcs->pfnTouch(pent, other);
	}
}

// Static buffer for game DLL path
static char g_szGameDLLPath[260] = "";

// Static buffer for game directory
static char g_szGameDir[260] = "";

// Get game info in extension mode - replaces GET_GAME_INFO from Metamod
const char* ExtensionMode_GetGameInfo(ginfo_t type)
{
	switch (type)
	{
	case GINFO_DLL_FULLPATH:
	case GINFO_REALDLL_FULLPATH:
		// Return the game DLL path - construct it from game dir
		if (g_szGameDLLPath[0] == '\0')
		{
			// Get game directory from engine function
			char gameDir[260];
			g_engfuncs.pfnGetGameDir(gameDir);
			if (gameDir[0])
			{
#ifdef WIN32
				snprintf(g_szGameDLLPath, sizeof(g_szGameDLLPath), "%s/dlls/dod.dll", gameDir);
#else
				snprintf(g_szGameDLLPath, sizeof(g_szGameDLLPath), "%s/dlls/dod_i386.so", gameDir);
#endif
			}
		}
		return g_szGameDLLPath;

	case GINFO_GAMEDIR:
		// Get game directory from engine function
		if (g_szGameDir[0] == '\0')
		{
			g_engfuncs.pfnGetGameDir(g_szGameDir);
		}
		return g_szGameDir;

	case GINFO_NAME:
	case GINFO_DESC:
		return "dod";

	case GINFO_DLL_FILENAME:
#ifdef WIN32
		return "dod.dll";
#else
		return "dod_i386.so";
#endif

	default:
		return "";
	}
}

// Forward declarations for hook callbacks with proper signatures
// ReHLDS hookchains require a chain parameter as first argument
void ExtHook_SV_ActivateServer(IRehldsHook_SV_ActivateServer* chain, int runPhysics);
void ExtHook_ED_Free(IRehldsHook_ED_Free* chain, edict_t* ed);

bool ExtensionMode_Init()
{
	// Verify ReHLDS is available - it's required for extension mode
	if (!api_cfg.hasReHLDS())
	{
		UTIL_ServerPrint("[ReAPI] ERROR: Extension mode requires ReHLDS!\n");
		return false;
	}

	// Set engine function table pointer to g_engfuncs
	// In Metamod mode, this would point to Metamod's hooked function table
	// In extension mode, we use the engine functions directly
	g_pengfuncsTable = &g_engfuncs;

	// Get game DLL function table from ReHLDS
	if (g_RehldsFuncs)
	{
		g_pExtDllFuncs = g_RehldsFuncs->GetEntityInterface();
		if (!g_pExtDllFuncs)
		{
			UTIL_ServerPrint("[ReAPI] WARNING: Could not get DLL_FUNCTIONS from ReHLDS\n");
		}
	}

	UTIL_ServerPrint("[ReAPI] Extension mode initialized (no Metamod)\n");

	// Register our ReHLDS hooks
	ExtensionMode_RegisterHooks();

	return true;
}

void ExtensionMode_Shutdown()
{
	UTIL_ServerPrint("[ReAPI] Extension mode shutting down\n");

	// Unregister hooks
	ExtensionMode_UnregisterHooks();

	// Clear hook managers
	g_hookManager.Clear();
	g_messageHookManager.Clear();
	g_queryFileManager.Clear();
}

void ExtensionMode_RegisterHooks()
{
	// Register SV_ActivateServer hook (replaces ServerActivate_Post)
	if (g_RehldsHookchains)
	{
		g_RehldsHookchains->SV_ActivateServer()->registerHook(&ExtHook_SV_ActivateServer);
		g_RehldsHookchains->ED_Free()->registerHook(&ExtHook_ED_Free);

		UTIL_ServerPrint("[ReAPI] Registered ReHLDS extension hooks\n");
	}
}

void ExtensionMode_UnregisterHooks()
{
	if (g_RehldsHookchains)
	{
		g_RehldsHookchains->SV_ActivateServer()->unregisterHook(&ExtHook_SV_ActivateServer);
		g_RehldsHookchains->ED_Free()->unregisterHook(&ExtHook_ED_Free);
	}
}

// Reference to user message IDs defined in main.cpp
extern int gmsgSendAudio, gmsgStatusIcon, gmsgArmorType, gmsgItemStatus, gmsgBarTime, gmsgBarTime2;

// Lookup user message ID by name
// In extension mode, we return 0 - callers should use AMXX's get_user_msgid native
int ExtensionMode_GetUserMsgID(void* plid, const char* msgname, int* size)
{
	if (size)
		*size = -1;  // Unknown size

	return 0;
}

// Lookup user message name by ID
const char* ExtensionMode_GetUserMsgName(void* plid, int msgid, int* size)
{
	if (size)
		*size = -1;  // Unknown size

	return nullptr;
}

// Hook callback: Called when server activates (replaces ServerActivate_Post)
void ExtHook_SV_ActivateServer(IRehldsHook_SV_ActivateServer* chain, int runPhysics)
{
	// Call the next hook in the chain first
	chain->callNext(runPhysics);

	// Get edict pointer
	g_pEdicts = g_engfuncs.pfnPEntityOfEntIndex(0);

	// Get mapname from gpGlobals
	if (gpGlobals)
	{
		const char* mapName = STRING(gpGlobals->mapname);
		if (mapName && mapName[0])
		{
			strncpy(g_szMapName, mapName, sizeof(g_szMapName) - 1);
			g_szMapName[sizeof(g_szMapName) - 1] = '\0';
		}
	}

	// Initialize ReGameDLL move pointer if available
	if (api_cfg.hasReGameDLL())
	{
		g_pMove = g_ReGameApi->GetPlayerMove();
	}

	// Note: User message IDs (gmsgSendAudio, etc.) are set by the game DLL
	// In extension mode, we rely on AMXX's get_user_msgid() native for lookups
	// The ReAPI natives that use these will get them on-demand if needed
}

// Hook callback: Called when entity is freed (replaces OnFreeEntPrivateData)
void ExtHook_ED_Free(IRehldsHook_ED_Free* chain, edict_t* pEdict)
{
	// Process our callback first
	if (pEdict)
	{
		CBaseEntity *pEntity = getPrivate<CBaseEntity>(pEdict);
		if (pEntity)
		{
			EntityCallbackDispatcher().DeleteExistingCallbacks(pEntity);
		}
	}

	// Call the next hook in the chain
	chain->callNext(pEdict);
}

#endif // REAPI_NO_METAMOD
