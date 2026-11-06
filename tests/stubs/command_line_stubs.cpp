#include <string.h>

#include "Battlefront2/Source/CommandLine.h"
#include "Battlefront2/Source/FLInputManager.h"
#include "Battlefront2/Source/GameLoop.h"
#include "Battlefront2/Source/GameSoundEngine.h"
#include "Battlefront2/Source/HUDManager.h"
#include "Battlefront2/Source/LuaCallbacks_Options.h"
#include "Battlefront2/Source/NetGame.h"
#include "Battlefront2/Source/NetSetup.h"
#include "Battlefront2/Source/ShellLoop.h"
#include "Battlefront2/Source/pcCrash.h"

bool mSafeMode;
bool pcWindowed;
bool pcEnableLogging;
bool pcPostInstall;
bool pcForceFixedFunction;
int pcWidth;
int pcHeight;
int journalMode;
bool dragEnabled;
bool netAutoAim;
bool netHiRes;
bool gNoPregame;
bool netWaitLate;
bool netHeroesEnabled;
bool netFrameLock;
bool gNoAIRespawn;
bool netAutoAssignTeamsOriginalSetting;
bool gCentreSpeakerEnhancement;
bool netIsOneWorld;
bool netAutoAssignTeams;
bool netTeamDamage;
bool netShowNames;
int dragLoss;
int dragLatency;
int dragJitter;
int dragBandwidth;
int netHeroRuleRespawnVal;
int netVoiceMode;
int netThrottle;
int gAutoNetPlayers;
int gSpawnInvicibility;
int netHeroRuleTeam;
int netHeroRuleUnlock;
int netHeroRuleUnlockVal;
int netHeroRulePlayer;
int netHeroRuleRespawn;
int netUpdateSize;
int netPlayerLimit;
int netHostedPlayerLimit;
int netNumBots;
int netDedicatedPlayerLimit;
int autoNetPlayers;
int netFPS;
float gGameSlice;
bool autoNetEnabled;
bool autoNetLan;
bool autoNetRandomize;
bool gDisableLCD;
bool gMemoryPoolErrors;
bool g_bNoRender;
bool g_bNoSound;
bool g_bNoWait;
float g_fFixedRate;
char autoNetScript[32];
char autoNetSession[64];
int netDifficulty;
unsigned int netCrashAddr;
unsigned int netPasswordHash;

namespace AIDifficulty
{
bool gAutoBalanceEnabled;
}

namespace GameSoundEngine
{
int gSamplerate;
int gMaxVoices;
float gSoftMixerBufferSizeMs;

void DisableStartupMusic()
{
}
}

namespace GameLoop
{
bool _bFirstPersonEnabled;
char mMissionScript[64];

void SetNextMission(char const* pcName)
{
    strncpy(mMissionScript, pcName, sizeof(mMissionScript) - 1);
    mMissionScript[sizeof(mMissionScript) - 1] = 0;
}
}

namespace HUD
{
bool gScreenGroupEditEnabled;
}

namespace Lua_Callbacks
{
eGAME_RULES gCurRules;
int gASSNumBots[4];
int gCONNumBots[4];
int gCTFNumBots[4];
int gELINumBots[4];
int gSpaceAssaultScoreLimit[5];
int gCONMaxTimeLimit[5];
bool gHeroesEnabled[4];
int gHeroRuleUnlock[4];
int gHeroRuleUnlockVal[4];
int gHeroRuleTeam[4];
int gHeroRulePlayer[4];
int gHeroRuleRespawn[4];
int gHeroRuleRespawnVal[4];
}

namespace RedLocalizeDB
{
void ForceLanguageID(int)
{
}
}

ShellLoop::eLEGALSCREENS_SHOWTYPE ShellLoop::sLegalShowtype;

extern "C" __declspec(align(16)) char gStubFLInputManager[sizeof(FLInputManager)];
__declspec(align(16)) char gStubFLInputManager[sizeof(FLInputManager)];
#pragma comment(linker, "/alternatename:?s_instance@FLInputManager@@2V1@A=_gStubFLInputManager")

void AddAutoNetMission(char const*, int, int)
{
}

void SetNetAwardsEnabled(int)
{
}

unsigned int NetSetup::CalcPassword(char const*)
{
    return 0;
}

void NetGame::SetMaxPreGameTime(int)
{
}

void pcCrash::SetCrashTest(char const*)
{
}

void pcCrash::SetHangTimeout(int)
{
}

void pcCrash::SetLogPath(char const*)
{
}
