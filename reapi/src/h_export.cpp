#include "precompiled.h"

enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;

#ifndef REAPI_NO_METAMOD
// Receive engine function table from engine.
// This appears to be the _first_ DLL routine called by the engine, so we
// do some setup operations here.
// NOTE: In extension mode (REAPI_NO_METAMOD), we don't export this function
// because ReAPI runs as an AMXX module, not as a standalone DLL.
// The engine functions are obtained from AMXX/the parent extension instead.
C_DLLEXPORT void WINAPI GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals)
{
	memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;
}
#endif
