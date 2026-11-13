#pragma once

#include "LuaFL/include/lua.h"

namespace Lua_Callbacks
{

struct CallbackEntryT
{
    char const* pcName;
    lua_CFunction pfnCallback;
};

void DoPeriodicLuaGC(bool);
int GetPeriodicLuaGCCount();
void SetPeriodicLuaGCCount(int iCount);

int ScriptCB_DoFile(lua_State* L);
int ScriptCB_GetPlatform(lua_State* L);
int ScriptCB_GetLanguage(lua_State* L);
int ScriptCB_FormatTimeString(lua_State* L);
int ScriptCB_GetVKeyboardCharacter(lua_State* L);
int ScriptCB_IsLegalGamespyString(lua_State* L);
int ScriptCB_tounicode(lua_State* L);
int ScriptCB_UnicodeStrCat(lua_State* L);
int ScriptCB_ununicode(lua_State* L);
int ScriptCB_getlocalizestr(lua_State* L);
int ScriptCB_usprintf(lua_State* L);
int ScriptCB_ustrupper(lua_State* L);
int ScriptCB_srand(lua_State* L);
int ScriptCB_random(lua_State* L);
int ScriptCB_GetDateStr(lua_State* L);
int ScriptCB_GetShellActive(lua_State* L);
int ScriptCB_SetAutoAcquireControllers(lua_State* L);
int ScriptCB_ReadAllControllers(lua_State* L);
int ScriptCB_UnbindController(lua_State* L);
int ScriptCB_GetPrimaryController(lua_State* L);
int ScriptCB_GetSecondaryController(lua_State* L);
int ScriptCB_IsControllerConnected(lua_State* L);
int ScriptCB_IsControllerBound(lua_State* L);
int ScriptCB_SwapPrimaryController(lua_State* L);
int ScriptCB_SetHotController(lua_State* L);
int ScriptCB_RestorePrimaryController(lua_State* L);
int ScriptCB_SetIgnoreControllerRemoval(lua_State* L);
int ScriptCB_GetNumControllers(lua_State* L);
int ScriptCB_GetMaxControllers(lua_State* L);
int ScriptCB_GetAutoNetIsDedicated(lua_State* L);
int ScriptCB_GetAutoNetGameName(lua_State* L);
int ScriptCB_GetAutoGetPlayerCount(lua_State* L);
int ScriptCB_SetupAutoNetMissions(lua_State* L);
int ScriptCB_SetupAutoNetIsLan(lua_State* L);
int ScriptCB_GetAutoNetScript(lua_State* L);
int ScriptCB_ReadLeftstick(lua_State* L);
int ScriptCB_SetQuitPlayer(lua_State* L);
int ScriptCB_GetQuitPlayer(lua_State* L);
int ScriptCB_GetMaxMissionQueue(lua_State* L);
int ScriptCB_SetMissionNames(lua_State* L);
int ScriptCB_SetTeamNames(lua_State* L);
int ScriptCB_EnterMission(lua_State* L);
int ScriptCB_LaunchDemo(lua_State* L);
int ScriptCB_Reboot(lua_State* L);
int ScriptCB_GetPausingViewport(lua_State* L);
int ScriptCB_GetPausingJoystick(lua_State* L);
int ScriptCB_PausingIsPrimary(lua_State* L);
int ScriptCB_GetBuildStr(lua_State* L);
int ScriptCB_UpdateScreen(lua_State* L);
int ScriptCB_UnlockableState(lua_State* L);
int ScriptCB_UnlockUnlockable(lua_State* L);
int ScriptCB_ShouldShowLegal(lua_State* L);
int ScriptCB_PauseDemoTimer(lua_State* L);
int ScriptCB_ShouldShowDemoPostscreen(lua_State* L);
int ScriptCB_ShouldCheckDNAS(lua_State* L);
int ScriptCB_DoneCheckDNAS(lua_State* L);
int ScriptCB_HasSeenDNASEULA(lua_State* L);
int ScriptCB_SetHasSeenDNASEULA(lua_State* L);
int ScriptCB_ShouldCheckPatch(lua_State* L);
int ScriptCB_DoneCheckPatch(lua_State* L);
int ScriptCB_IndexMultipageText(lua_State* L);
int ScriptCB_ShowMultipageText(lua_State* L);
int ScriptCB_SetDCMap(lua_State* L);
int ScriptCB_HideDownloadableContent(lua_State* L);
int ScriptCB_RestartMission(lua_State* L);
int ScriptCB_Unpause(lua_State* L);
int ScriptCB_SkipToPlayerList(lua_State* L);
int ScriptCB_ResetSkipToPlayerList(lua_State* L);
int ScriptCB_CheckPlayerListDone(lua_State* L);
int ScriptCB_GetMissionTime(lua_State* L);
int ScriptCB_Freecamera(lua_State* L);
int ScriptCB_IsGameOver(lua_State* L);
int ScriptCB_QuitToShell(lua_State* L);
int ScriptCB_QuitToWindows(lua_State* L);
int ScriptCB_MinimizeWindow(lua_State* L);
int ScriptCB_QuitFromStats(lua_State* L);
int ScriptCB_GetTeamName(lua_State* L);
int ScriptCB_GetSPProgress(lua_State* L);
int ScriptCB_SetSPProgress(lua_State* L);
int ScriptCB_GetLastBattleVictory(lua_State* L);
int ScriptCB_SetLastBattleVictoryValid(lua_State* L);
int ScriptCB_MetagameSetBonus(lua_State* L);
int ScriptCB_MetagameSetSide(lua_State* L);
int ScriptCB_DisableFlyerShadows(lua_State* L);
int DrawBeamBetween(lua_State* L);
int DrawParticleAt(lua_State* L);
int ScriptCB_EnableScene(lua_State* L);
int ScriptCB_PostLoadHack(lua_State* L);
int BeginScreenTransition(lua_State* L);

int ScriptCB_SetVictoryMovie(lua_State* L);
int ScriptCB_SetDefeatMovie(lua_State* L);

void Init();

}

#include "RedEngineFL/Application/Renderable.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Graphics/RedColor.h"


