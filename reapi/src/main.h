#pragma once

extern char g_szMapName[32];
extern playermove_t* g_pMove;
extern int gmsgSendAudio;
extern int gmsgStatusIcon;
extern int gmsgArmorType;
extern int gmsgItemStatus;
extern int gmsgBarTime;
extern int gmsgBarTime2;

void OnAmxxAttach();

#ifndef REAPI_NO_METAMOD
// Metamod-specific functions
bool OnMetaAttach();
void OnMetaDetach();
void OnFreeEntPrivateData(edict_t *pEdict);
void ServerActivate_Post(edict_t *pEdictList, int edictCount, int clientMax);
void ServerDeactivate_Post();
int DispatchSpawn(edict_t* pEntity);
void ResetGlobalState();
void KeyValue(edict_t *pentKeyvalue, KeyValueData *pkvd);
#endif

// Used by both modes (ReGameDLL hookchain)
CGameRules *InstallGameRules(IReGameHook_InstallGameRules *chain);
