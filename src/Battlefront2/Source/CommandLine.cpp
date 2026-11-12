#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GameMovie.h"
#include "RedEngineFL/Graphics/PC/pcDisplaySettings.h"
#include "pcCrash.h"
#include "RedEngineFL/Util/RedLog.h"
#include "RedEngineFL/Util/RedWarning.h"

#include "CommandLine.h"
#include "LocalLimits.h"
#include "FLInputManager.h"
#include "GameLoop.h"
#include "GameSoundEngine.h"
#include "GameState.h"
#include "HUDManager.h"
#include "LuaCallbacks_Options.h"
#include "NetSetup.h"
#include "NetGame.h"
#include "NetDrag.h"
#include "ShellLoop.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblUnicode.h"

#include "RedEngineFL/input/pc/pcRedInputManager.h"
#include "RedEngineFL/Interface/RedFont.h"
#include "RedEngineFL/Util/RedLocalizeDB.h"
#include "PebbleFL/Common/PblMath.h"

// GLOBAL: SWBF2 0x01e3073c
// gGamespyAutoName
char gGamespyAutoName[32];
// GLOBAL: SWBF2 0x01e30760
char gGamespyAutoConnectAddr[64];
// GLOBAL: SWBF2 0x01e307a0
char gGamespyAutoPassStr[128];
// GLOBAL: SWBF2 0x01e30820
// gGamespyAutoEmail
char gGamespyAutoEmail[64];
// GLOBAL: SWBF2 0x01e30860
// gGamespyAutoPPass
char gGamespyAutoPPass[32];
// GLOBAL: SWBF2 0x01e30880
unsigned int gGamespyAutoPass;
// GLOBAL: SWBF2 0x01e30736
bool gBootLagger;
bool gShowLoadSpam;
// GLOBAL: SWBF2 0x01e3075c
bool gLogFinalScore;
bool gExitOnGameOver;
bool gNoVictory;
int gDigestTrace;
bool gFactoryLog;
bool gNoFocusPause;
bool gAllowUploads = LocalLimits::kUploadsDefault;
bool gSeedSet;
unsigned int gSeed;
static bool sbNoVictoryNetLogged;
static unsigned int suTestOnlyIgnored;
static char const* const sapcTestOnlyArgs[] = { "perturbheap", "repeatmission", "shellfirst", "exitonmovieclose" };
// GLOBAL: SWBF2 0x007e6310
int gBootLaggerPing = 300;

// GLOBAL: SWBF2 0x007e69c9

bool sLoadNestedLevels = true;

// GLOBAL: SWBF2 0x007e666d
bool GameMovie::sUseMovies = true;

// GLOBAL: SWBF2 0x01e57a10
char Destination[16];

namespace PlayerPrefs
{
    // GLOBAL: SWBF2 0x007e9030
    int gDefaultDifficulty = 2;
}
namespace PlayerPrefs
{
    // GLOBAL: SWBF2 0x007e9034
    int gDefaultTeamNum = 1;
}

extern bool pcWindowed;
extern bool pcEnableLogging;
extern bool pcPostInstall;
extern bool pcForceFixedFunction;
extern int pcWidth;
extern int pcHeight;

extern int journalMode;

extern bool dragEnabled;
extern bool netAutoAim;
extern bool netHiRes;
extern bool gNoPregame;
extern bool netWaitLate;
extern bool netHeroesEnabled;
extern bool gShowAllSessions;
extern bool netFrameLock;
extern bool gAIOptionEnabled;
extern bool gNoAIRespawn;
extern bool gSoundWarningEnabled;
extern bool gProfileOptionEnabled;
extern bool netAutoAssignTeamsOriginalSetting;
namespace AIDifficulty
{
    extern bool gAutoBalanceEnabled;
}
extern bool gCentreSpeakerEnhancement;

extern bool netIsOneWorld;

extern bool netAutoAssignTeams;
extern bool gUseValues;

extern bool netTeamDamage;
extern bool netShowNames;
extern int dragLoss;
extern int dragLatency;
extern int dragJitter;
extern int netHeroRuleRespawnVal;
extern int netVoiceMode;
extern int netThrottle;
extern int gAutoNetPlayers;
extern int gSpawnInvicibility;
namespace GameSoundEngine
{
    extern int gSamplerate;
}
namespace GameSoundEngine
{
    extern int gMaxVoices;
}
namespace GameSoundEngine
{
    extern float gSoftMixerBufferSizeMs;
}

extern int dragBandwidth;
extern int netHeroRuleTeam;
extern int netHeroRuleUnlock;
extern int netHeroRuleUnlockVal;
extern int netHeroRulePlayer;
extern int netHeroRuleRespawn;
extern int netUpdateSize;
extern int netPlayerLimit;
extern int netHostedPlayerLimit;
extern int netNumBots;
extern int netDedicatedPlayerLimit;
extern int autoNetPlayers;
extern int netFPS;
extern float gGameSlice;

void AddAutoNetMission(char const* pcArg, int iA, int iB);
namespace PlayerProfile
{
    void SetSafeMode();
}
void SetNetAwardsEnabled(int);




static bool NextCommandToken(char const* pcLine, int* piPos, int* piStart, int* piLen, bool* pbQuoted)
{
    int i = *piPos;
    while (pcLine[i] == ' ' || pcLine[i] == ',')
    {
        ++i;
    }
    if (pcLine[i] == '\0')
    {
        *piPos = i;
        return false;
    }
    int iStart = i;
    int iLen = 0;
    const bool bQuoted = pcLine[i] == '"';
    if (bQuoted)
    {
        ++i;
        iStart = i;
        while ((pcLine[i] != '\0' && pcLine[i] != '"') || (pcLine[i] == '"' && pcLine[i - 1] == '\\'))
        {
            ++i;
        }
        iLen = i - iStart;
        if (pcLine[i] == '"')
        {
            ++i;
        }
    }
    else
    {
        while (pcLine[i] != '\0' && pcLine[i] != ' ' && pcLine[i] != ',')
        {
            ++i;
        }
        iLen = i - iStart;
    }
    if (pcLine[i] != '\0')
    {
        ++i;
    }
    *piPos = i;
    *piStart = iStart;
    *piLen = iLen;
    *pbQuoted = bQuoted;
    return true;
}

int CommandLineRepeatCount(char const* pcVal, int iLen)
{
    if (iLen <= 0)
    {
        return -1;
    }
    int iCount = 0;
    for (int i = 0; i < iLen; ++i)
    {
        if (pcVal[i] < '0' || pcVal[i] > '9')
        {
            return -1;
        }
        if (iCount < 17)
        {
            iCount = iCount * 10 + (pcVal[i] - '0');
        }
    }
    return iCount < 1 ? 1 : iCount > 16 ? 16 : iCount;
}

bool FindNextCommandLineArg(char const* pcLine, char const* pcName, int* piPos, char const** ppcVal, int* piLen,
                            bool bKeepSwitch)
{
    const int iNameLen = (int)strlen(pcName);
    int iPos = *piPos;
    int iStart = 0;
    int iLen = 0;
    bool bQuoted = false;
    *ppcVal = 0;
    *piLen = 0;
    while (NextCommandToken(pcLine, &iPos, &iStart, &iLen, &bQuoted))
    {
        const char c = pcLine[iStart];
        if ((c == '/' || c == '-' || c == '+') && iLen == iNameLen &&
            _strnicmp(pcLine + iStart + 1, pcName + 1, iNameLen - 1) == 0)
        {
            const int iBeforeValue = iPos;
            int iValStart = iPos;
            int iValLen = 0;
            bool bValQuoted = false;
            NextCommandToken(pcLine, &iPos, &iValStart, &iValLen, &bValQuoted);
            *ppcVal = pcLine + iValStart;
            *piLen = iValLen;
            *piPos = bKeepSwitch && iValLen > 0 && pcLine[iValStart] == '-' ? iBeforeValue : iPos;
            return true;
        }
    }
    return false;
}

bool FindCommandLineArg(char const* pcLine, char const* pcName, char const** ppcVal, int* piLen)
{
    int iPos = 0;
    return FindNextCommandLineArg(pcLine, pcName, &iPos, ppcVal, piLen);
}

// FUNCTION: SWBF2 0x004795a0
void ParseCommandLine(char const* pcAppName, char const* pcCmdLine)
{

    char* apcArgv[256];
    memset(apcArgv, 0, sizeof(apcArgv));
    char szCmdLine[0x800] = "";
    char szArgv0[0x100] = "";

    (void)pcAppName;
    strlcat(szArgv0, "", sizeof(szArgv0));
    strlcat(szCmdLine, pcCmdLine, sizeof(szCmdLine));

    int iArgc = 1;
    int iPos = 0;
    int iStart = 0;
    int iLen = 0;
    bool bQuoted = false;

    while (iArgc < 256 && NextCommandToken(szCmdLine, &iPos, &iStart, &iLen, &bQuoted))
    {
        apcArgv[iArgc++] = &szCmdLine[iStart];

        for (int j = iStart; bQuoted && j < iStart + iLen; ++j)
        {
            if (szCmdLine[j] == '"' && szCmdLine[j - 1] == '\\')
            {
                szCmdLine[j - 1] = '_';
                szCmdLine[j] = '_';
            }
        }

        szCmdLine[iStart + iLen] = '\0';
    }

    apcArgv[0] = szArgv0;

    ParseCommandLine(iArgc, apcArgv);
}

bool NoVictoryActive()
{
    if (!gNoVictory)
    {
        return false;
    }
    if (!NetGame::IsEnabled())
    {
        return true;
    }
    if (!sbNoVictoryNetLogged)
    {
        sbNoVictoryNetLogged = true;
        LOG(LogCore, Warning, "-novictory is ignored in a network game");
    }
    return false;
}

bool TestOnlyArgIgnored(int i, char const** ppcName)
{
    if (i < 0 || i >= (int)(sizeof(sapcTestOnlyArgs) / sizeof(sapcTestOnlyArgs[0])))
    {
        return false;
    }
    *ppcName = (suTestOnlyIgnored & (1u << i)) != 0 ? sapcTestOnlyArgs[i] : 0;
    return true;
}

static void NoteTestOnlyArg(int i)
{
#ifdef OPENSWBF2_TEST
    (void)i;
#else
    suTestOnlyIgnored |= 1u << i;
#endif
}

// FUNCTION: SWBF2 0x00479740
void ParseCommandLine(int iArgc, char** const apcArgv)
{

    char acBuf[0x80];
    memset(acBuf, 0, sizeof(acBuf));
    _snprintf(acBuf, 0x7F, "%i Arguments Found\n", iArgc - 1);

    HUD::gScreenGroupEditEnabled = false;
    gNoVictory = false;
    sbNoVictoryNetLogged = false;
    suTestOnlyIgnored = 0;
    bool bNamed = false;

    for (int i = 1; i < iArgc; ++i)
    {
        _snprintf(acBuf, 0x7F, "Arg %i: %s\n", i, apcArgv[i]);

        char* pcArg = apcArgv[i];
        if (pcArg[0] != '/' && pcArg[0] != '-' && pcArg[0] != '+')
        {

            if (autoNetEnabled)
            {
                int iA = 0;
                int iB = 0;
                if (++i < iArgc)
                {
                    iA = atoi(apcArgv[i]);
                    if (iA)
                    {
                        iB = atoi(apcArgv[++i]);
                    }
                    else
                    {
                        --i;
                    }
                }
                AddAutoNetMission(pcArg, iA, iB);
            }
            else
            {
                GameLoop::SetNextMission(pcArg);
                FLInputManager::Instance()->SetAutoAcquire(true);
            }
            continue;
        }

        if (_stricmp(apcArgv[i] + 1, "DecompCONMinutes") == 0)
        {
            if (i + 1 < iArgc)
            {
                const char* value = apcArgv[++i];
                const int slot = (int)Lua_Callbacks::gCurRules;
                if (strcmp(value, "5") == 0 && slot >= 0 && slot <= 4)
                {
                    Lua_Callbacks::gCONMaxTimeLimit[slot] = 5;
                }
            }
            continue;
        }

        switch (PblHash(apcArgv[i] + 1))
        {
        case 0x02049853: /* "jitter" */
            dragEnabled = true;
            dragJitter = atoi(apcArgv[++i]);
            break;
        case 0x03D17781: /* "adminpw" */
            strncpy(Destination, apcArgv[++i], 16);
            Destination[15] = '\0';
            break;
        case 0x0451542B: /* "safemode" */
            PlayerProfile::SetSafeMode();
            GameMovie::sUseMovies = false;
            break;
        case 0x0481D555: /* "showlegal" */
        case 0xA5CD594E: /* "showintro" */
            ShellLoop::SetLegalShowtype(ShellLoop::eLEGALSCREENS_SHOW);
            break;
        case 0x0652936B: /* "audiomixbuffer" */
            sscanf(apcArgv[++i], "%f", &GameSoundEngine::gSoftMixerBufferSizeMs);
            if (GameSoundEngine::gSoftMixerBufferSizeMs < 20.0f)
            {
                GameSoundEngine::gSoftMixerBufferSizeMs = 20.0f;
            }
            break;
        case 0x09C3E0FF: /* "logfinalscore" */
            gLogFinalScore = true;
            break;
        case 0x09CFDCF0: /* "difficulty" */
            netDifficulty = atoi(apcArgv[++i]);
            break;
        case 0x0BF7CB59: /* "win" */
            pcWindowed = true;
            break;
        case 0x0D3D814F: /* "noaim" */
            netAutoAim = false;
            break;
        case 0x124E8FBB: /* "netHiRes" */
            netHiRes = true;
            break;
        case 0x1503F968: /* "netUpdateSize" */
            netUpdateSize = atoi(apcArgv[++i]);
            break;
        case 0x1D215C8F: /* "resolution" */
            pcWidth = atoi(apcArgv[++i]);
            pcHeight = atoi(apcArgv[++i]);
            break;
        case 0x1DEBAE35:
        case 0xA563005A: /* "hrrespawnvalue" */
        {
            int iValue = atoi(apcArgv[++i]);
            netHeroRuleRespawnVal = iValue;
            Lua_Callbacks::gHeroRuleRespawnVal[0] = iValue;
            Lua_Callbacks::gHeroRuleRespawnVal[1] = iValue;
            break;
        }
        case 0x202DB217: /* "heroruleunlock" */
        case 0xCD0D5437: /* "hrunlock" */
        {
            int iValue = atoi(apcArgv[++i]);
            netHeroRuleUnlock = iValue;
            Lua_Callbacks::gHeroRuleUnlock[0] = iValue;
            Lua_Callbacks::gHeroRuleUnlock[1] = iValue;
            break;
        }
        case 0x204F0D0D: /* "nojournal" */
            break;
        case 0x2264B8ED: /* "oneworld" */
            netIsOneWorld = true;
            break;
        case 0x26D94B4E: /* "multiworld" */
            netIsOneWorld = false;
            break;
        case 0x2E325FB4:
            SetNetAwardsEnabled(0);
            break;
        case 0x31694A02: /* "nostartupmusic" */
            GameSoundEngine::DisableStartupMusic();
            break;
        case 0x331843BC: /* "randomize" */
            autoNetRandomize = true;
            break;
        case 0x351FC388: /* "PoolErrors" */
            gMemoryPoolErrors = true;
            break;
        case 0x364B5F18: /* "password" */
            strlcpy(gGamespyAutoPassStr, apcArgv[++i], sizeof(gGamespyAutoPassStr));
            if (gGamespyAutoPassStr[0])
            {
                unsigned int uiHash = NetSetup::CalcPassword(gGamespyAutoPassStr);
                gGamespyAutoPass = uiHash;
                netPasswordHash = uiHash;
            }
            bNamed = true;
            break;
        case 0x371E360D: /* "pregametime" */
        {
            int iSeconds = atoi(apcArgv[++i]);
            NetGame::SetMaxPreGameTime(iSeconds);
            if (iSeconds <= 0)
            {
                gNoPregame = true;
            }
            break;
        }
        case 0x378F32DD: /* "throttle" */
            netThrottle = atoi(apcArgv[++i]);
            break;
        case 0x3A00D703: /* "playerlimit" */
        {
            int iLimit = atoi(apcArgv[++i]);
            netPlayerLimit = iLimit;
            netHostedPlayerLimit = iLimit;
            netDedicatedPlayerLimit = iLimit;
            break;
        }
        case 0x3A224D98: /* "spawn" */
            gSpawnInvicibility = atoi(apcArgv[++i]);
            break;
        case 0x3C3F5C38: /* "audionoenhance" */
            gCentreSpeakerEnhancement = false;
            break;
        case 0x3EC8DA95: /* "herorulerespawn" */
        case 0xD0CFBC35: /* "hrrespawn" */
        {
            int iValue = atoi(apcArgv[++i]);
            netHeroRuleRespawn = iValue;
            Lua_Callbacks::gHeroRuleRespawn[0] = iValue;
            Lua_Callbacks::gHeroRuleRespawn[1] = iValue;
            break;
        }
        case 0x3F0ECDEB:
        case 0x77E5E744: /* "nointro" */
        case 0xBBB3B003:
            ShellLoop::SetLegalShowtype(ShellLoop::eLEGALSCREENS_SKIP);
            break;
        case 0x42EACCDD: /* "ShowAllServers" */
        case 0x4EAE0006: /* "ShowAllSessions" */
            gShowAllSessions = true;
            break;
        case 0x4424F79F: /* "ai" */
            gAIOptionEnabled = true;
            break;
        case 0x4674CAEE: /* "profile" */
            gProfileOptionEnabled = true;
            break;
        case 0x4BE2EE33: /* "nowaitlate" */
            netWaitLate = false;
            break;
        case 0x4F0C4904: /* "gamename" */
            strlcpy(autoNetSession, apcArgv[++i], sizeof(autoNetSession));
            break;
        case 0x56608010: /* "lan" */
            autoNetLan = true;
            break;
        case 0x593058CC: /* "record" */
            journalMode = 1;
            GameMovie::sUseMovies = false;
            break;
        case 0x5D62BEB3: /* "bootlagger" */
        {
            gBootLagger = true;
            int iPing = atoi(apcArgv[++i]);
            if (iPing > gBootLaggerPing)
            {
                gBootLaggerPing = iPing;
            }
            break;
        }
        case 0x6363D774: /* "firstperson" */
            GameLoop::EnableFirstPerson(true);
            break;
        case 0x68F63ADB: /* "autoaim" */
            netAutoAim = true;
            break;
        case 0x69F6C31E: /* "hrteam" */
        case 0xF546DAFE: /* "heroruleteam" */
        {
            int iValue = atoi(apcArgv[++i]);
            netHeroRuleTeam = iValue;
            Lua_Callbacks::gHeroRuleTeam[0] = iValue;
            Lua_Callbacks::gHeroRuleTeam[1] = iValue;
            break;
        }
        case 0x70EA3B55: /* "lang" */
            RedLocalizeDB::ForceLanguageID(static_cast<eGAMELANGUAGE>(atoi(apcArgv[++i])));
            break;
        case 0x718E7AD4: /* "nodrag" */
            dragEnabled = false;
            break;
        case 0x75E8450E: /* "norender" */
            g_bNoRender = true;
            GameMovie::sUseMovies = false;
            break;
        case 0x7AC8BB2B: /* "fixedrate" */
            g_fFixedRate = (float)atof(apcArgv[++i]);
            break;
        case 0x81BA46B8: /* "noai" */
            gNoAIRespawn = true;
            break;
        case 0x8463E1A0: /* "install" */
            pcPostInstall = true;
            break;
        case 0x8A8753C7: /* "email" */
            strlcpy(gGamespyAutoEmail, apcArgv[++i], sizeof(gGamespyAutoEmail));
            bNamed = true;
            break;
        case 0x8CEAC9BD: /* "audiomaxchannels" */
            sscanf(apcArgv[++i], "%d", &GameSoundEngine::gMaxVoices);
            GameSoundEngine::gMaxVoices = Clamp(GameSoundEngine::gMaxVoices, 8, 32);
            break;
        case 0x8D39BDE6: /* "name" */
            strlcpy(gGamespyAutoName, apcArgv[++i], sizeof(gGamespyAutoName));
            bNamed = true;
            break;
        case 0x8DE05C06: /* "configdir" */
        case 0xE91814A3: /* "recorddir" */
            ++i;
            break;
        case 0x90B89FFA: /* "netplayers" */
            gAutoNetPlayers = atoi(apcArgv[++i]);
            break;
        case 0x91739772: /* "bandwidth" */
            dragEnabled = true;
            dragBandwidth = atoi(apcArgv[++i]);
            break;
        case 0x98BF5C17: /* "gameslice" */
            gGameSlice = (float)atof(apcArgv[++i]);
            break;
        case 0x99317EA6: /* "sideselect" */
            netAutoAssignTeams = false;
            netAutoAssignTeamsOriginalSetting = false;
            break;
        case 0x996359B2: /* "DisableAIAutoBalance" */
            AIDifficulty::gAutoBalanceEnabled = false;
            break;
        case 0x99B7123E: /* "hrplayer" */
        case 0xECD7701E: /* "heroruleplayer" */
        {
            int iValue = atoi(apcArgv[++i]);
            netHeroRulePlayer = iValue;
            Lua_Callbacks::gHeroRulePlayer[0] = iValue;
            Lua_Callbacks::gHeroRulePlayer[1] = iValue;
            break;
        }
        case 0xA2FD7D0C: /* "team" */
            PlayerPrefs::gDefaultTeamNum = atoi(apcArgv[++i]);
            break;
        case 0xA8C4C105: /* "heroes" */
        {
            bool bHeroes = atoi(apcArgv[++i]) != 0;
            netHeroesEnabled = bHeroes;
            Lua_Callbacks::gHeroesEnabled[0] = bHeroes;
            Lua_Callbacks::gHeroesEnabled[1] = bHeroes;
            break;
        }
        case 0xAAE0CCF9: /* "connect" */
            strlcpy(gGamespyAutoConnectAddr, apcArgv[++i], sizeof(gGamespyAutoConnectAddr));
            bNamed = true;
            break;
        case 0xAC16C90C: /* "heroruleunlockvalue" */
        case 0xB418A0AC: /* "hrunlockvalue" */
        {
            int iValue = atoi(apcArgv[++i]);
            netHeroRuleUnlockVal = iValue;
            Lua_Callbacks::gHeroRuleUnlockVal[0] = iValue;
            Lua_Callbacks::gHeroRuleUnlockVal[1] = iValue;
            break;
        }
        case 0xB08CEC40: /* "notvalues" */
            gUseValues = false;
            break;
        case 0xB22236AE: /* "tps" */
            netFPS = atoi(apcArgv[++i]);
            if (!netFPS)
            {
                __debugbreak();
            }
            break;
        case 0xBA0ECD59: /* "crashed" */
            netCrashAddr = atoi(apcArgv[++i]);
            break;
        case 0xBC494AFF: /* "nowait" */
            g_bNoWait = true;
            break;
        case 0xC01E4E25:
            gDisableLCD = true;
            break;
        case 0xC11F80F6: /* "nonames" */
            netShowNames = false;
            break;
        case 0xC16650C8: /* "soundwarning" */
            gSoundWarningEnabled = true;
            break;
        case 0xC23937B9: /* "autonet" */
            autoNetEnabled = true;
            strlcpy(autoNetScript, apcArgv[++i], sizeof(autoNetScript));
            break;
        case 0xC53B7844: /* "loadspam" */
            gShowLoadSpam = true;
            break;
        case 0xC707B50F: /* "playercount" */
            autoNetPlayers = atoi(apcArgv[++i]);
            break;
        case 0xCA15D036: /* "skill" */
            PlayerPrefs::gDefaultDifficulty = Clamp(atoi(apcArgv[++i]), 1, 3);
            break;
        case 0xCD154944: /* "noframelock" */
            netFrameLock = false;
            break;
        case 0xCF8A43EC: /* "playback" */
            GameMovie::sUseMovies = false;
            journalMode = 2;
            break;
        case 0xD0BD1109: /* "audiosamplerate" */
        {

            static const int aiRates[9] = { 8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000, 192000 };
            sscanf(apcArgv[++i], "%d", &GameSoundEngine::gSamplerate);
            unsigned int uiBest = 0xFFFFFFFF;
            int iBest = 0;
            for (int j = 0; j < 9; ++j)
            {
                unsigned int uiDiff = abs(aiRates[j] - GameSoundEngine::gSamplerate);
                if (uiDiff < uiBest)
                {
                    uiBest = uiDiff;
                    iBest = j;
                }
            }
            GameSoundEngine::gSamplerate = aiRates[iBest];
            break;
        }
        case 0xD43981B7: /* "NoPoolErrors" */
            gMemoryPoolErrors = false;
            break;
        case 0xD5C0B6F5: /* "spaceassaultscorelimit" */
            Lua_Callbacks::gSpaceAssaultScoreLimit[0] = Lua_Callbacks::gSpaceAssaultScoreLimit[1] = atoi(apcArgv[++i]);
            break;
        case 0xDABA84C3: /* "nomovies" */
            GameMovie::sUseMovies = false;
            break;
        case 0xDADFA856: /* "voicemode" */
            netVoiceMode = atoi(apcArgv[++i]);
            break;
        case 0xE0EBCA12: /* "ppass" */
            strlcpy(gGamespyAutoPPass, apcArgv[++i], sizeof(gGamespyAutoPPass));
            bNamed = true;
            break;
        case 0xE181E077: /* "bots" */
            netNumBots =
                Lua_Callbacks::gASSNumBots[0] =
                Lua_Callbacks::gASSNumBots[1] =
                Lua_Callbacks::gASSNumBots[2] =
                Lua_Callbacks::gASSNumBots[3] =
                Lua_Callbacks::gELINumBots[0] =
                Lua_Callbacks::gELINumBots[1] =
                Lua_Callbacks::gELINumBots[2] =
                Lua_Callbacks::gELINumBots[3] =
                Lua_Callbacks::gCTFNumBots[0] =
                Lua_Callbacks::gCTFNumBots[1] =
                Lua_Callbacks::gCTFNumBots[2] =
                Lua_Callbacks::gCTFNumBots[3] =
                Lua_Callbacks::gCONNumBots[0] =
                Lua_Callbacks::gCONNumBots[1] =
                Lua_Callbacks::gCONNumBots[2] =
                Lua_Callbacks::gCONNumBots[3] = atoi(apcArgv[++i]);
            break;
        case 0xE3D8AB51: /* "framelock" */
            netFrameLock = true;
            break;
        case 0xE51023BA: /* "noteamdamage" */
            netTeamDamage = false;
            break;
        case 0xE7347B1A: /* "loss" */
            dragEnabled = true;
            dragLoss = atoi(apcArgv[++i]);
            break;
        case 0xF2FB8359: /* "latency" */
            dragEnabled = true;
            dragLatency = atoi(apcArgv[++i]);
            break;
        case 0xF4553EF1: /* "nosound" */
            g_bNoSound = true;
            break;
        case 0xF5DD1FBD: /* "fixedfunction" */
            pcForceFixedFunction = true;
            break;
        case 0xF6976B61: /* "nopregame" */
            gNoPregame = true;
            break;
        }
    }

    if (bNamed)
    {
        ShellLoop::SetLegalShowtype(ShellLoop::GetLegalShowtype() == ShellLoop::eLEGALSCREENS_SKIP
                                        ? ShellLoop::eLEGALSCREENS_SKIP
                                        : ShellLoop::eLEGALSCREENS_SHOWFAST);
    }
}



// GLOBAL: SWBF2 0x01e2fdbb

bool gAIOptionEnabled = false;

// GLOBAL: SWBF2 0x01e56281

bool gSoundWarningEnabled = false;

// GLOBAL: SWBF2 0x01e65033

bool gShowAllSessions = false;

// GLOBAL: SWBF2 0x01eaefc5

bool gProfileOptionEnabled = false;

// GLOBAL: SWBF2 0x007e802e

bool gUseValues = true;
