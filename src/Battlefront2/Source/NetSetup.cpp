#include "GameVoiceChatNode.h"
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <winsock2.h>

#include "NetSetup.h"
#include "pcNetSocket.h"
#include "NetMatch.h"
#include "GameLoop.h"
#include "ShellLoop.h"
#include "LuaCallbacks_Options.h"
#include "PebbleFL/Common/PblJournal.h"
#include "PebbleFL/Common/PblFile.h"
#include "PebbleFL/Common/PblRandom.h"
#include "PebbleFL/Common/PblTime.h"
#include "PebbleFL/Common/PblMath.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Util/RedLocalizeDB.h"
#include "FLGuiManager.h"
#include "FLInputManager.h"
#include "GameState.h"
#include "LuaHelper.h"
#include "PebbleFL/Common/PblValues.h"
#include "MissionPlayList.h"
#include "NetComm.h"
#include "NetGame.h"
#include "NetPacket.h"
#include "NetWire.h"
#include "LocalLimits.h"
#include "Character.h"
#include "GameVoiceChat.h"
#include "GameVoiceChatTransport.h"
#include "Battlefront2/Platform/Pc/pcSpawnDisplay.h"
#include "PlayerProfile.h"
#include "LuaCallbacks_Options.h"
#include "Steam/utf8.h"
#include "Steam/galaxy_api.h"
#include "Team.h"
#include "DownloadableContent.h"
#include <math.h>

extern void sub_42E960();

extern unsigned __int64 netCreatedLobbyId;
extern int netLoginFailureReason;
const char* GetNextMissionForLobby();
const char* GetEncodedLobbyPassword();
galaxy::api::IMatchmaking* GetGalaxyMatchmaking();

namespace gf
{
namespace utils
{
const char* ToString(bool b);
const char* ToString(int i);
const char* ToString(unsigned int u);
}
}

// LIBRARY: SWBF2 0x00411c30
// strchr

// GLOBAL: SWBF2 0x01ead478
// autoNetEnabled
bool autoNetEnabled;
// GLOBAL: SWBF2 0x01ead479
// autoNetLan
bool autoNetLan;
// GLOBAL: SWBF2 0x01ead47a
bool autoNetRandomize;
// GLOBAL: SWBF2 0x01ead47b
bool g_bNoRender;
// GLOBAL: SWBF2 0x01ead47c
unsigned int netCrashAddr;
// GLOBAL: SWBF2 0x01ead480
// autoNetScript
char autoNetScript[32];
// GLOBAL: SWBF2 0x01ead4a0
// g_fFixedRate
float g_fFixedRate;
// GLOBAL: SWBF2 0x01ead4a4
unsigned int netPasswordHash;

// GLOBAL: SWBF2 0x01ead474
int NetSetup::mPasswordHash;

// GLOBAL: SWBF2 0x01ead4b8
unsigned char netPasswordSha512[64];

// GLOBAL: SWBF2 0x01ead4f8
unsigned char netEncodedPassword[512];
// GLOBAL: SWBF2 0x01ead4a8
int netDifficulty;
// GLOBAL: SWBF2 0x01ead4ac
// g_bNoWait
bool g_bNoWait;
// GLOBAL: SWBF2 0x01ead4ad
bool g_bNoSound;

 

// GLOBAL: SWBF2 0x007e8e90
int autoNetPlayers = 2;
// GLOBAL: SWBF2 0x007e8e94
int netFPS = 20;

// GLOBAL: SWBF2 0x007e8e98
char autoNetSession[64] = "LY";

// GLOBAL: SWBF2 0x007e8ed8
bool useProfile = true;

// GLOBAL: SWBF2 0x01fa88fc
NetScript netScript;

// GLOBAL: SWBF2 0x01ead924
unsigned int autoNetMissionCount;
// GLOBAL: SWBF2 0x01ead930
stAutoNetMission autoNetMissions[256];

// SYNTHETIC: SWBF2 0x004059a0
// `dynamic initializer for 'netRandom''

// GLOBAL: SWBF2 0x01ef24b0
// netRandom
PblRandom netRandom;

extern float gGameSlice;
extern bool gSeedSet;
extern unsigned int gSeed;

// FUNCTION: SWBF2 0x005db5d0
void NetSetup::Create(bool bMission)
{
    unsigned int uiSeed = gSeedSet ? gSeed : (unsigned int)PblTime::GetTickCount();
    PblJournal::Record(uiSeed, PJT_FULL);
    netRandom.SetSeed(uiSeed);

    PblJournal::Record(netFPS, PJT_FULL);
    PblJournal::Record(gGameSlice, PJT_FULL);
    PblJournal::Record(autoNetEnabled, PJT_FULL);

    if (autoNetEnabled)
    {
        PblJournal::Record(autoNetLan, PJT_FULL);
        JournalString(autoNetScript, sizeof(autoNetScript));
        JournalString(autoNetSession, sizeof(autoNetSession));
        PblJournal::Record(autoNetPlayers, PJT_FULL);
        useProfile = false;
    }

    if (autoNetEnabled)
    {
        if (strcmp(autoNetScript, "host") == 0)
        {
            netScript = NS_HOST;
        }
        else if (strcmp(autoNetScript, "client") == 0)
        {
            netScript = NS_CLIENT;
        }
        else if (strcmp(autoNetScript, "dedicated") == 0)
        {
            netScript = NS_DEDICATED;
        }
        else
        {
            netScript = NS_UNKNOWN;
        }

        if (netScript == NS_HOST || netScript == NS_DEDICATED)
        {
            PblJournal::Record(autoNetMissionCount, PJT_FULL);
            for (unsigned int i = 0; i < autoNetMissionCount; i++)
            {
                JournalString(autoNetMissions[i].name, sizeof(autoNetMissions[i].name));
            }
        }

        if (bMission)
        {
            autoNetEnabled = false;
            autoNetScript[0] = '\0';
        }
    }
}

class NetPacket;

// GLOBAL: SWBF2 0x007e8ed9
bool useMatchService = true;
// GLOBAL: SWBF2 0x007e8eda
bool gCableConnected = true;

enum GameMode
{
    GM_LOBBY = 0,
    GM_GAME = 1
};

// GLOBAL: SWBF2 0x007e8ee0
static GameMode setupMode = GM_GAME;

// GLOBAL: SWBF2 0x01ead4ae
bool netGameError;
// GLOBAL: SWBF2 0x01ead4af
bool netLoginError;
// GLOBAL: SWBF2 0x01ead4b5
bool netWasDedicated;
// GLOBAL: SWBF2 0x01ead4b7
bool netInMultiplayer;
// GLOBAL: SWBF2 0x01ead904
char netLoginNames[1][32];

// GLOBAL: SWBF2 0x01ead8f8
unsigned int intraSessionPlayerId[2];
// GLOBAL: SWBF2 0x01ead92a
bool gOverrideCable;
// GLOBAL: SWBF2 0x01ead92b
bool netWasHost;
// GLOBAL: SWBF2 0x01ead92c
bool isInSession;
// GLOBAL: SWBF2 0x01ead92d
bool netWasClient;
// GLOBAL: SWBF2 0x01ead92f
bool netInGame;

// FUNCTION: SWBF2 0x005e7210
unsigned int NetSetup::CalcPassword(char const* password)
{
    return password ? CalcCRC(password, strlen(password), 0) : 0;
}

// FUNCTION: SWBF2 0x005e7270
void NetSetup::SetPassword(char const* password)
{
    if (netInGame && netWasClient)
    {
        return;
    }
    NetSetup::mPasswordHash = CalcPassword(password);
    memset(netPasswordSha512, 0, sizeof(netPasswordSha512));
    memset(netEncodedPassword, 0, sizeof(netEncodedPassword));
    if (password)
    {
        Sha512Hash(reinterpret_cast<const unsigned char*>(password), strlen(password), netPasswordSha512, 0);
        unsigned int length = 0;
        Base64Encode(netEncodedPassword, sizeof(netEncodedPassword), &length, netPasswordSha512, sizeof(netPasswordSha512));
    }
    if (netPasswordHash)
    {
        NetSetup::mPasswordHash = netPasswordHash;
    }
}

// GLOBAL: SWBF2 0x01ead470
int gNetSetup_errState;

// GLOBAL: SWBF2 0x01eaed34
NetLoadGame* loadGame;
// GLOBAL: SWBF2 0x01eaed38
NetJoinGame* joinGame;
// GLOBAL: SWBF2 0x01eaed3c
NetHostGame* hostGame;
// GLOBAL: SWBF2 0x01eaed40
NetFindGame* findGame;
// GLOBAL: SWBF2 0x01eaed44
NetPostLoadHostGame* postLoadHostGame;
// GLOBAL: SWBF2 0x01eaed48
NetPostLoadJoinGame* postLoadJoinGame;

// GLOBAL: SWBF2 0x01ead6f8
IntraSessionPlayer intraSessionPlayers[64];

// SYNTHETIC: SWBF2 0x00405990
// `dynamic initializer for 'netPrevSessionId''

// GLOBAL: SWBF2 0x01ef1fa0
// netPrevSessionId
NetSessionId netPrevSessionId;
// SYNTHETIC: SWBF2 0x00405940
// `dynamic initializer for 'clientAddrs''

// GLOBAL: SWBF2 0x01ef1fa8
// clientAddrs
NetAddr clientAddrs[64];

// SYNTHETIC: SWBF2 0x00405980
// `dynamic initializer for 'netSessionId''

// GLOBAL: SWBF2 0x01ef24a8
// netSessionId
NetSessionId netSessionId;

// SYNTHETIC: SWBF2 0x00405960
// `dynamic initializer for 'hostAddrs''

// GLOBAL: SWBF2 0x01ef2510
// hostAddrs
NetAddr hostAddrs[3];
// SYNTHETIC: SWBF2 0x00405930
// `dynamic initializer for 'myAddr''

// GLOBAL: SWBF2 0x01ef2558
// myAddr
NetAddr myAddr;

// GLOBAL: SWBF2 0x01fa88f0
int hostPlayer;

// GLOBAL: SWBF2 0x01fa88f4
bool isLoginError;
// GLOBAL: SWBF2 0x01fa8900
unsigned __int64 playerChangedMask[64];
// GLOBAL: SWBF2 0x01fa8b00
unsigned int playerChangeTurn[64];
// GLOBAL: SWBF2 0x01fa8c08
unsigned __int64 playerChangedSendMask;

extern int netCurMaxPlayers;
void ReadDestMask(NetPktGroup& group, unsigned __int64& mask);
void ReadString(NetPktGroup& group, char* text, unsigned int capacity);

// FUNCTION: SWBF2 0x005e39b0
void NetSetup::RemoveGamePlayer(int iPlayer)
{
    if (NetGame::IsHost() && !NetGame::IsClient())
    {
        playerChangedSendMask &= ~(1ULL << iPlayer);
        playerChangeTurn[iPlayer] = 0;
        playerChangedMask[iPlayer] = 0;
        for (int i = 0; i < netCurMaxPlayers; ++i)
        {
            playerChangedMask[i] &= ~(1ULL << iPlayer);
        }
        NetMatch::ChangeRegisterGame();
    }
}


// FUNCTION: SWBF2 0x005e37f0
void NetSetup::PlayerDataChanged(int iChangedPlayer)
{
    if (NetGame::IsHost() && !NetGame::IsClient())
    {
        for (int i = 0; i < netCurMaxPlayers; ++i)
        {
            if (NetComm::IsConnected(i) && NetGame::GetJoystickIndex(i) < 0)
            {
                playerChangedSendMask |= 1ULL << i;
                playerChangeTurn[i] = NetGame::GetTurnNumber();
                playerChangedMask[i] |= 1ULL << iChangedPlayer;
            }
        }
        NetMatch::ChangeRegisterGame();
    }
}


// FUNCTION: SWBF2 0x005e38d0
void NetSetup::AddGamePlayer(int iChangedPlayer)
{
    if (NetGame::IsHost() && !NetGame::IsClient())
    {
        for (int i = 0; i < netCurMaxPlayers; ++i)
        {
            if (NetComm::IsConnected(i) && NetGame::GetJoystickIndex(i) < 0)
            {
                playerChangedSendMask |= 1ULL << i;
                playerChangeTurn[i] = NetGame::GetTurnNumber();
                playerChangedMask[i] |= 1ULL << iChangedPlayer;
            }
        }
        NetMatch::ChangeRegisterGame();
    }
}


// FUNCTION: SWBF2 0x005e3cd0
void NetSetup::ReadGameUpdate(NetPktGroup& group)
{
    if (!NetGame::IsHost())
    {
        NetGame::ReadPreGame(group);
        bool hasPlayers;
        group.Read(hasPlayers);
        if (hasPlayers)
        {
            unsigned __int64 changedMask;
            ReadDestMask(group, changedMask);
            for (int i = 0; i < netCurMaxPlayers; ++i)
            {
                if (changedMask & (1ULL << i))
                {
                    bool teamOne;
                    group.Read(teamOne);
                    NetGame::SetStartTeam(i, teamOne ? 1 : 2);
                    Character* character = Character::FindPlayer(i);
                    if (character)
                    {
                        character->SelectTeam(NetGame::GetStartTeam(i));
                    }
                    int joystick = NetGame::GetJoystickIndex(i);
                    if (joystick >= 0)
                    {
                        if (SpawnDisplay::Instance(joystick)->IsWaitingForTeam())
                        {
                            SpawnDisplay::Instance(joystick)->SelectTeam(NetGame::GetStartTeam(i));
                            SpawnDisplay::Instance(joystick)->SetWaitingForTeam(false);
                        }
                    }
                    if (i == NetComm::GetLocalIndex(0))
                    {
                        continue;
                    }
                    char name[32];
                    ReadString(group, name, sizeof(name));
                    NetGameP::SetPlayerName(i, name);
                    unsigned __int64 id;
                    group.ReadBytes((char*)&id, sizeof(id));
                    bool isGuest;
                    group.Read(isGuest);
                    NetAddr address;
                    if (isGuest)
                    {
                        unsigned int guestOf;
                        group.ReadBits(guestOf, 6);
                        NetGameP::SetGuestOf(i, guestOf);
                    }
                    else
                    {
                        ReadAddr(group, address);
                    }
                    unsigned __int64 oldId = NetGame::GetPlayerGSID(i);
                    if (oldId != id)
                    {
                        NetGame::SetPlayerGSID(i, id);
                        NetGame::ClearPlayerCDKey(i);
                    }
                    if (isGuest)
                    {
                        NetCommP::SetArch(i, NetComm::GetArch(NetGame::GetGuestOf(i)));
                        clientAddrs[i] = clientAddrs[NetGame::GetGuestOf(i)];
                    }
                    else
                    {
                        unsigned int bits;
                        group.ReadBits(bits, 2);
                        NetCommP::SetArch(i, (NetArch)bits);
                        clientAddrs[i] = address;
                    }
                }
            }
        }
    }
}

// GLOBAL: SWBF2 0x01fa8c10
char netGameType[32];

extern char netGameName[64];

namespace GameLoop
{
extern bool sQuitRequested;
}
namespace GameLoop
{
extern unsigned int sQuitTurn;
}
extern int netCurMaxPlayers;

// FUNCTION: SWBF2 0x005db330
char const* NetSetup::GetAutoNetScript()
{
    return autoNetScript;
}

// FUNCTION: SWBF2 0x005db340
bool NetSetup::IsAutoNetLan()
{
    return autoNetLan;
}

// FUNCTION: SWBF2 0x005db350
char const* NetSetup::GetAutoNetName()
{
    return autoNetSession;
}

// FUNCTION: SWBF2 0x005db360
int NetSetup::GetAutoNetPlayerCount()
{
    return autoNetPlayers;
}

// FUNCTION: SWBF2 0x005db370
static char* GetFactionName(char faction, int team)
{
    static struct
    {
        char faction;
        char* name;
        char enemy;
    } factions[4] = {
        { 'a', "common.sides.all.name", 'i' },
        { 'i', "common.sides.imp.name", 'a' },
        { 'r', "common.sides.rep.name", 'c' },
        { 'c', "common.sides.cis.name", 'r' },
    };

    char side = faction;
    if (team != 1)
    {
        unsigned int i;
        for (i = 0; i < 4; i++)
        {
            if (factions[i].faction == faction)
            {
                side = factions[i].enemy;
                break;
            }
        }
        if (i == 4)
        {
            return 0;
        }
    }

    for (unsigned int i = 0; i < 4; i++)
    {
        if (factions[i].faction == side)
        {
            return factions[i].name;
        }
    }
    return 0;
}

char* NetSetup_GetFactionName(char faction, int team)
{
    return GetFactionName(faction, team);
}

// FUNCTION: SWBF2 0x005db410
void NetSetup::SetupAutoNetMissions()
{
    MissionPlayList::ClearEntries();
    for (unsigned int i = 0; i < autoNetMissionCount; i++)
    {
        char faction = autoNetMissions[i].name[4];
        char* pcTeam1 = GetFactionName(faction, 1);
        char* pcTeam2 = GetFactionName(faction, 2);
        MissionPlayList::AddEntry(autoNetMissions[i].name, 1, pcTeam1, pcTeam2,
                                  autoNetMissions[i].tickets[0], autoNetMissions[i].tickets[1]);
    }
    MissionPlayList::SetLoopForever(true);
    if (autoNetRandomize)
    {
        MissionPlayList::RandomizeOrder();
    }
    MissionPlayList::SelectNextEntry();
}

// FUNCTION: SWBF2 0x005db4e0
void JRR_to_Errorbox(JoinRejectReason eReason)
{
    char const* pcError = 0;
    int iSeverity = 6;

    switch (eReason)
    {
    case JRR_FULL: pcError = "ifs.mp.joinerrors.full"; break;
    case JRR_PASSWORD: pcError = "ifs.mp.joinerrors.password"; break;
    case JRR_GAME: pcError = "ifs.mp.joinerrors.game"; break;
    case JRR_VERSION: pcError = "ifs.mp.joinerrors.version"; break;
    case JRR_BOOTED: pcError = "ifs.mp.joinerrors.booted"; break;
    case JRR_CONNECTION_LOST: pcError = "ifs.mp.joinerrors.connectlost"; break;
    case JRR_MISSING_DC:
        pcError = "ifs.mp.joinerrors.missingdc";
        iSeverity = 12;
        break;
    case JRR_BAD_CDKEY: pcError = "ifs.mp.joinerrors.cdkey"; break;
    case JRR_BAN: pcError = "ifs.mp.joinerrors.banned"; break;
    case 11: pcError = "ifs.mp.joinerrors.doesnotexist"; break;
    case 12: pcError = "ifs.mp.joinerrors.cantconnect"; break;
    default: break;
    }

    if (pcError)
    {
        NetMatch::SetGameError(pcError, iSeverity);
    }
}

// FUNCTION: SWBF2 0x005db850
void NetSetup::BeginShell()
{
}

void NetSetup::EndShell()
{
}

// FUNCTION: SWBF2 0x005db860
void NetSetup::SetAsHost()
{
    netEnabled = true;
    NetComm::SetHost(true);
}

// FUNCTION: SWBF2 0x005db880
void NetSetup::SetAsClient()
{
    netEnabled = true;
    NetComm::SetHost(false);
}

// FUNCTION: SWBF2 0x005db8a0
void NetSetup::SetAsDedicated(bool bDedicated)
{
    netIsDedicated = bDedicated;
}

// FUNCTION: SWBF2 0x005db8b0
void NetSetup::SetGameName(char const* pcName)
{
    LocalLimits::CopyLobbyString(netGameName, pcName, LocalLimits::kLobbyNameSize);
}

// FUNCTION: SWBF2 0x005db8d0
void NetSetup::SetLoginName(int iViewport, char const* pcName)
{
    if (iViewport < 0 || iViewport >= (int)(sizeof(netLoginNames) / sizeof(netLoginNames[0])))
    {
        return;
    }
    char const* pcPlatformName = NetMatch::GetLoginOverrideName(iViewport);
    memset(netLoginNames[iViewport], 0, sizeof(netLoginNames[iViewport]));
    if (pcPlatformName)
    {
        LocalLimits::CopyLobbyString(netLoginNames[iViewport], pcPlatformName, LocalLimits::kLobbyTagSize);
    }
    else
    {
        LocalLimits::CopyLobbyString(netLoginNames[iViewport], pcName, LocalLimits::kLobbyTagSize);
    }
}

// FUNCTION: SWBF2 0x005db950
bool NetSetup::InNetGame()
{
    return netInGame;
}

// FUNCTION: SWBF2 0x005db960
bool NetSetup::InMultiplayer()
{
    return netInMultiplayer;
}

// FUNCTION: SWBF2 0x005db970
bool NetSetup::SetInNetGame(bool bInGame)
{
    if (!bInGame)
    {
        netInMultiplayer = false;
    }
    netInGame = bInGame;
    return netInGame;
}

// FUNCTION: SWBF2 0x005db9a0
bool NetSetup::WasHost()
{
    return netWasHost;
}

// FUNCTION: SWBF2 0x005db9b0
bool NetSetup::WasDedicated()
{
    return netWasDedicated;
}

// FUNCTION: SWBF2 0x005db9c0
bool NetSetup::WasClient()
{
    return netWasClient;
}

// FUNCTION: SWBF2 0x005db9d0
bool NetSetup::IsInSession()
{
    return isInSession;
}

// FUNCTION: SWBF2 0x005db9e0
void NetSetup::RequestQuit(unsigned int uiReason)
{
    FLInputManager::Instance()->StopAllRumble();
    if (!GameLoop::sQuitRequested)
    {
        GameLoop::sQuitTurn = uiReason;
        GameLoop::sQuitRequested = true;
    }
    if (NetGame::IsEnabled())
    {
        netInGame = false;
        netWasHost = false;
        netWasClient = false;
    }
}

// FUNCTION: SWBF2 0x005dba30
bool NetSetup::IsLoginError()
{
    return netLoginError;
}

// FUNCTION: SWBF2 0x005dba40
bool NetSetup::IsGameError()
{
    return netGameError;
}

// FUNCTION: SWBF2 0x005dba50
bool NetSetup::IsNetCableConnected()
{
    if (gOverrideCable)
    {
        return gCableConnected;
    }
    return true;
}

// FUNCTION: SWBF2 0x005dba70
void NetSetup::GetLoginError(char const*& pcError, int& iSeverity)
{
    NetMatch::GetLoginError(pcError, iSeverity);
}

// FUNCTION: SWBF2 0x005dba80
void NetSetup::GetGameError(char const*& pcError, int& iSeverity)
{
    NetMatch::GetGameError(pcError, iSeverity);
}

// FUNCTION: SWBF2 0x005dba90
void NetSetup::SetErrorMode(bool bMode)
{
    isLoginError = bMode;
}

// FUNCTION: SWBF2 0x005dbaa0
bool NetSetup::IsErrorDismissed()
{
    char const* pcError;
    int iSeverity;
    GetLoginError(pcError, iSeverity);
    if (!pcError)
    {
        GetGameError(pcError, iSeverity);
    }

    GuiManager* pGui = GuiManager::Instance();
    bool bDismissed = false;
    switch (gNetSetup_errState)
    {
    case 0:
    {
        const unsigned short* pLocalizedStr = RedLocalizeDB::Find(PblHash(pcError), false);
        pGui->OpenError((GuiManager::eErrorSeverity)iSeverity, pLocalizedStr, 0, 0);
        gNetSetup_errState = 1;
        break;
    }
    case 2:
        bDismissed = true;
        break;
    }
    return bDismissed;
}

// FUNCTION: SWBF2 0x005dbb40
void NetSetup::HandleError(bool bArg)
{
    if (isLoginError)
    {
        NetMatch::HandleLoginError(bArg);
        return;
    }

    unsigned int uiState = GameState::GetState();
    if (uiState != 0x11E1FC01)
    {
        if (postLoadHostGame)
        {
            postLoadHostGame->Cancel();
        }
        else if (postLoadJoinGame)
        {
            postLoadJoinGame->Cancel();
        }
        GameState::SetState(0x11E1FC01);
    }
    NetMatch::ClearGameError();
}

// FUNCTION: SWBF2 0x005dbbc0
void NetSetup::EnableOnlineService(bool bEnable)
{
    useMatchService = bEnable;
}

// FUNCTION: SWBF2 0x005dbbd0
char const* NetSetup::GetOnlineService()
{
    return "Galaxy";
}

// FUNCTION: SWBF2 0x005dbbe0
char* NetSetup::GetIPAddr()
{
    return NetMatch::GetIPAddr();
}

// FUNCTION: SWBF2 0x005dbbf0
void NetSetup::GetPrevSessionId(NetSessionId* pId)
{
    if (pId)
    {
        *pId = netPrevSessionId;
    }
}

// FUNCTION: SWBF2 0x005dbc10
void NetSetup::StartLogin(int iViewport, bool bArg2, char const* pcArg3, char const* pcArg4,
                          char const* pcArg5, bool bArg6)
{
    NetMatch::StartLogin(iViewport, bArg2, pcArg3, pcArg4, pcArg5, bArg6);
}

// FUNCTION: SWBF2 0x005dbc40
bool NetSetup::IsLoginDone(int iViewport)
{
    return NetMatch::UpdateLogin(iViewport);
}

// FUNCTION: SWBF2 0x005dbc50
bool NetSetup::IsLoginSuccessful(int iViewport, bool bArg)
{
    return NetMatch::IsLoginSuccessful(iViewport, bArg);
}

// FUNCTION: SWBF2 0x005dbc60
void NetSetup::CancelLogin(int iViewport, bool bArg)
{
    NetMatch::CancelLogin(iViewport, bArg);
}

// FUNCTION: SWBF2 0x005dbc70
static void FindAddress()
{
    myAddr = NetAddr();
    NetMatch::StartFindAddress();
    while (!NetMatch::UpdateFindAddress())
        ;
    NetMatch::EndFindAddress();
}

// FUNCTION: SWBF2 0x005dbcb0
NetHostGame* NetSetup::HostGame()
{
    return new CNetHostGame;
}

// FUNCTION: SWBF2 0x005db240
ShellUpdate::ShellUpdate()
{
    postLoad = false;
    hostLaunched = false;
    connectMask = 0;
    moveNum = 0;
    turnNumber = 0;
    connectingMask = 0;
    quitMask = 0;
    hostLoadDone = false;
    patchFile.Init();
}

// FUNCTION: SWBF2 0x005db2c0
void ShellPlayer::Init()
{
    joinAck = false;
    hostRequest = 0;
    moveNum = 0;
    updateNum = 1;
    ackedUpdate = 0;
    changedUpdate = 0;
    playerId = 0;
    quitRequested = false;
    patchFile.Init(0, 0);
}

// FUNCTION: SWBF2 0x005dbd20
CNetHostGame::CNetHostGame()
{
    unsigned int i;
    FindAddress();

    PlayerProfile::tProfile* pProfile = PlayerProfile::GetCurProfile();
    if (pProfile)
    {
        PblValues::Record<bool>(pProfile->mGameOptions.mCockpitCam, (PblValuesType)0);
    }

    netLoginError = false;
    netGameError = false;
    NetComm::AdvanceTime(true);
    setupMode = GM_LOBBY;

    changeCount = 0;
    timer = 0.0f;
    playerCount = 0;
    launchReady = false;
    launchTimer = 0.0f;
    hostFailed = false;
    bootMask = 0;
    needSendMask = 0;

    NetCommP::GetConnected(64) = true;
    NetCommP::GetLastRecv(64) = NetComm::GetTime();
    NetGameP::GetHostIndex() = 0;
    *NetGameP::GetHostMode(0) = 0;

    if (NetGame::IsDedicated())
    {
        hostPlayer = -1;
        for (unsigned int joyId = 0; joyId < 2; joyId++)
        {
            NetCommP::GetLocalIndex(joyId) = -1;
        }
        NetComm::SetPing(64, 33);
    }
    else
    {
        FLInputManager* pInput = FLInputManager::Instance();
        int aiJoystick[1];
        unsigned int uiCount = 0;

        if (autoNetEnabled)
        {
            aiJoystick[uiCount++] = 0;
        }
        else
        {
            for (i = 0; i < 1; i++)
            {
                bool bActive = pInput->mActiveControllers[i];
                if (bActive)
                {
                    aiJoystick[uiCount++] = i;
                }
            }
        }
        for (i = uiCount; i < 1; i++)
        {
            aiJoystick[i] = -1;
        }

        for (i = 0; i < 2; i++)
        {
            int iJoystick = -1;
            if (i < 1)
            {
                iJoystick = aiJoystick[i];
            }

            if (iJoystick >= 0)
            {
                hostPlayer = 0;
                NetCommP::GetConnected(i) = true;
                NetCommP::GetLastRecv(i) = NetComm::GetTime();
                NetCommP::GetLocalIndex(i) = i;
                NetCommP::GetPipe(i) = PIPE_LAN;

                char acName[64];
                PblUnicode::ustrncnv(acName, PlayerProfile::GetProfile(iJoystick)->GetName(), 64);
                NetGameP::SetPlayerName(i, acName);
                if (i == 0)
                {
                    *NetGameP::GetHostIndex(0) = 0;
                }
                *NetGameP::GetClientOfIndex(i) = 0;
                if (i == 0)
                {
                    NetGameP::SetGuestOf(i, -1);
                }
                else
                {
                    NetGameP::SetGuestOf(i, 0);
                }
                if (i > 0)
                {
                    UpdateCurMaxPlayers((unsigned __int64)1 << i, true);
                }
            }
            else
            {
                NetCommP::GetLocalIndex(i) = -1;
            }
        }
        changeCount++;
    }

    shellState.canceled = false;
    for (i = 0; i < 64; i++)
    {
        shellState.players[i].Init();
        clientAddrs[i] = NetAddr();
        bootTime[i] = 0.0f;
    }

    char* gameType = "duel";
    strlcpy(netGameType, gameType, sizeof(netGameType));
    NetMatch::StartRegisterGame(useMatchService);
    isInSession = true;
    if (!NetSetup::InNetGame())
    {
        netPrevSessionId.SetNULL();
    }
    NetMatch::GetSessionId(netSessionId);

    for (i = 0; i < 64; i++)
    {
        playerChangeTurn[i] = 0;
        playerChangedMask[i] = 0;
    }
    playerChangedSendMask = 0;

    int iTurnRate = netFPS;
    if (useProfile)
    {
        PlayerProfile::tProfile* pCur = PlayerProfile::GetCurProfile();
        if (pCur)
        {
            iTurnRate = pCur->mOnlineOptions.mu8TurnsPerSecond;
        }
    }
    if (iTurnRate < 10)
    {
        iTurnRate = netFPS;
    }
    NetGameP::SetTurnRate(iTurnRate, 1);

    shellUpdate.patchFile.Init();
    shellUpdate.patchFile.done = true;
}

// FUNCTION: SWBF2 0x005dc240
CNetHostGame::~CNetHostGame()
{
    int i;
    if (!shellState.canceled)
    {
        for (i = 0; i < netCurMaxPlayers; i++)
        {
            if (NetComm::IsConnected(i))
            {
                intraSessionPlayers[i].playerId = shellState.players[i].playerId;
            }
        }
    }
    else
    {
        netInGame = false;
        isInSession = false;
        for (i = 0; i < netCurMaxPlayers; i++)
        {
            if (NetComm::IsConnected(i))
            {
                NetComm::Disconnect(i);
            }
        }
        if (NetComm::IsConnected(64))
        {
            NetComm::Disconnect(64);
        }
        NetMatch::EndRegisterGame();
        for (i = 0; i < 64; i++)
        {
            NetGame::SetPlayerGSID(i, 0);
            NetGame::ClearPlayerCDKey(i);
            NetGameP::SetPlayerName(i, "");
        }
        NetGameP::GetHostIndex() = -1;
        netIsDedicated = false;
        NetQOS::CancelGame();
    }
}

// FUNCTION: SWBF2 0x005dc3b0
void CNetHostGame::Cancel()
{
    if (shellState.canceled)
    {
        return;
    }

    shellState.canceled = true;
    unsigned __int64 uMask = 0;
    for (int i = 0; i < netCurMaxPlayers; i++)
    {
        if (NetComm::IsConnected(i) && shellState.players[i].joinAck)
        {
            uMask |= (unsigned __int64)1 << i;
        }
    }
    needSendMask = uMask;
}

// FUNCTION: SWBF2 0x005dc450
bool CNetHostGame::IsCancelled()
{
    if (!shellState.canceled)
    {
        return false;
    }

    bool bCancelled = true;
    for (int i = 0; i < netCurMaxPlayers; i++)
    {
        if (!NetComm::IsConnected(i))
        {
            continue;
        }
        if (NetGame::GetJoystickIndex(i) >= 0)
        {
            continue;
        }
        if (shellState.players[i].quitRequested)
        {
            continue;
        }
        bCancelled = false;
        break;
    }
    return bCancelled;
}

// FUNCTION: SWBF2 0x005dc4d0
bool BanIP(int iPlayer, NetPacket* pPacket)
{
    return false;
}

// FUNCTION: SWBF2 0x005e75f0
void SendStateDst::Init()
{
    done = false;
    gotSome = false;
    baseBlock = 0;
    hasBlocks = 0;
    fd = -1;
}

// FUNCTION: SWBF2 0x005e7460
void SendStateSrc::Init(int iCount, char** ppcFiles)
{
    fileCount = iCount;
    fileNum = 0;
    if (iCount == 0)
    {
        files = 0;
    }
    else
    {
        int bufferSize = iCount * sizeof(char*);
        int i;
        for (i = 0; i < iCount; ++i)
        {
            bufferSize += strlen(ppcFiles[i]) + 1;
        }
        files = (char*)RedAllocFromHeap(__RedCurrHeap, bufferSize, 0);
        if (files)
        {
            char** fileNames = (char**)files;
            char* fileNamePtr = (char*)(fileNames + iCount);
            for (i = 0; i < iCount; ++i)
            {
                fileNames[i] = fileNamePtr;
                strlcpy(fileNames[i], ppcFiles[i], bufferSize - (fileNamePtr - files));
                fileNamePtr += strlen(fileNamePtr) + 1;
            }
            baseBlock = 0;
            hasBlocks = 0;
            curFile = 0;
            blockCycle = 0;
        }
    }
}


// GLOBAL: SWBF2 0x007e8edc
int autoNetLast = 1;

// GLOBAL: SWBF2 0x01ead4b0
int autoNetCounter;

// GLOBAL: SWBF2 0x01ead4b6
bool autoNetFinishedShell;

// GLOBAL: SWBF2 0x01ead928
bool autoNetTimeInit;
// GLOBAL: SWBF2 0x01ead92e
bool NetSetup::mNoticeNoCable;

// GLOBAL: SWBF2 0x01fa88f8
int autoNetRepeat;

extern PblTimeBase::Stamp autoNetTimeStamp;
extern int netMinPlayersToAutoLaunchDedicated;
extern int netNumBots;

namespace StatsManager
{
void Update();
}

void AddAutoNetMission(char const* pcArg, int iA, int iB);

// FUNCTION: SWBF2 0x005e4030
void AddAutoNetMission(char const* pcArg, int iA, int iB)
{
    if (autoNetMissionCount >= 256)
    {
        return;
    }
    int i = autoNetMissionCount;
    char* mission = autoNetMissions[autoNetMissionCount++].name;
    strlcpy(mission, pcArg, sizeof(autoNetMissions[i].name));
    autoNetMissions[i].tickets[0] = iA;
    autoNetMissions[i].tickets[1] = iB;
}

// FUNCTION: SWBF2 0x005e3fb0
void NetSetupP::Update()
{
    PblValues::Record('Z', PVT_FULL);
    NetMatch::UpdateLogin(-1);
    PblValues::Record('Z', PVT_FULL);
    NetMatch::UpdatePresence(false);
    PblValues::Record('Z', PVT_FULL);
    NetMatch::UpdateRegisterGame();
    PblValues::Record('Z', PVT_FULL);
    StatsManager::Update();
    PblValues::Record('Z', PVT_FULL);
}

// FUNCTION: SWBF2 0x005e41e0
void NetSetupP::UpdateScreen()
{
    netRandom.GetInt();

    bool bInShell = NetGame::InShell();
    if (bInShell)
    {
        NetComm::AdvanceTime(true);
        Update();
    }

    if (NetSetup::mNoticeNoCable && !NetSetup::IsNetCableConnected())
    {
        if (bInShell)
        {
            NetMatch::SetNoCableError();
        }
        else
        {
            NetSetup::RequestQuit(NetGame::GetTurnNumber());
            GuiManager::eErrorSeverity Severity = GuiManager::ERROR_LOGOFF_CABLE;
            if (!NetComm::IsLAN())
            {
                ShellLoop::SetNetCableErrorOnEnter(Severity, "common.mp.errors.nocable");
            }
            else
            {
                ShellLoop::SetNetCableErrorOnEnter(Severity, 0);
            }
        }
    }

    if (!autoNetEnabled)
    {
        return;
    }

    if (!autoNetFinishedShell && !bInShell)
    {
        autoNetFinishedShell = true;
        autoNetTimeInit = false;
    }

    char const* pcScreen = GuiManager::Instance()->GetCurrentScreen();
    if (strcmp(pcScreen, "ifs_mp_autonet") == 0)
    {
        netMinPlayersToAutoLaunchDedicated = autoNetPlayers;
        MissionPlayList::ClearEntries();
        for (unsigned int i = 0; i < autoNetMissionCount; i++)
        {
            MissionPlayList::AddEntry(autoNetMissions[i].name, autoNetMissions[i].name[4], 0, 0,
                                      autoNetMissions[i].tickets[0], autoNetMissions[i].tickets[1]);
        }
        MissionPlayList::SelectNextEntry();
        autoNetRepeat = -1;
    }

    if (autoNetFinishedShell)
    {
        return;
    }

    if (!autoNetTimeInit)
    {
        autoNetTimeInit = true;
        autoNetTimeStamp.Mark(GameState::GetSystemTimeBase());
    }
    if (autoNetTimeStamp.GetElapsed() < 0.1f)
    {
        return;
    }
    autoNetTimeStamp.Mark(GameState::GetSystemTimeBase());

    if (GuiManager::Instance()->IsPopupOpen())
    {
        char const* pcPopup = GuiManager::Instance()->GetCurrentFocus();
        if (strcmp(pcPopup, "Popup_LobbyOpts") == 0)
        {
            if (++autoNetCounter == 10)
            {
                LuaHelper::CallTableProcWThis(pcPopup, "Input_Accept");
                autoNetCounter = 0;
            }
        }
        else if (strcmp(pcScreen, "ifs_mpgs_login") == 0)
        {
            if (++autoNetCounter == 10)
            {
                LuaHelper::CallTableProcWThis(pcPopup, "Input_GeneralRight");
            }
            else if (autoNetCounter == 20)
            {
                LuaHelper::CallTableProcWThis(pcPopup, "Input_Accept");
                autoNetCounter = 0;
            }
        }
        return;
    }

    autoNetCounter++;
    if (strcmp(pcScreen, "ifs_legal") == 0)
    {
        autoNetCounter = 0;
    }
    else if (strcmp(pcScreen, "ifs_start") == 0)
    {
        if (autoNetCounter > 10)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Start");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_login") == 0)
    {
        autoNetLast = 1;
        if (autoNetCounter == 30)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_main") == 0)
    {
        if (autoNetCounter == 10)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralDown");
        }
        else if (autoNetCounter == 20)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_mpxl_login") == 0)
    {
        if (autoNetCounter == 10)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_mp") == 0)
    {
        if (autoNetCounter == 10)
        {
            if (!autoNetLan)
            {
                LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralDown");
            }
            if (netScript == NS_DEDICATED)
            {
                netMinPlayersToAutoLaunchDedicated = autoNetPlayers;
            }
        }
        else if (autoNetCounter == 20)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_mp_main") == 0)
    {
        autoNetLast = 2;
        if (autoNetCounter == 10)
        {
            if (netScript == NS_DEDICATED || netScript == NS_HOST)
            {
                LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralDown");
            }
        }
        else if (autoNetCounter == 20)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_vkeyboard") == 0)
    {
        if (autoNetCounter == 20)
        {
            const int kSTR_LEN = 64;
            unsigned short usName[kSTR_LEN];
            PblUnicode::ustrncnv(usName, "<NULL>", kSTR_LEN);
            if (autoNetLast == 1)
            {
                PblUnicode::ustrncnv(usName, "AUTONET", kSTR_LEN);
            }
            else if (autoNetLast == 2)
            {
                PblUnicode::ustrncnv(usName, autoNetSession, kSTR_LEN);
            }
            LuaHelper::CallTableProcWThis(pcScreen, "AcceptString", usName);
        }
        else if (autoNetCounter == 40)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_mp_createopts") == 0 || strcmp(pcScreen, "ifs_mp_createoptsds") == 0)
    {
        if (autoNetCounter == 100)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
        else if (autoNetCounter == 10)
        {
            netNumBots = 32;
        }
    }
    else if (strcmp(pcScreen, "ifs_missionselect") == 0)
    {
        if (autoNetCounter == 10)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
        }
        else if (autoNetCounter == 20)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
        }
        else if (autoNetCounter == 30)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralLeft");
        }
        else if (autoNetCounter == 40)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralUp");
        }
        else if (autoNetCounter == 50)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_mp_maptype") == 0)
    {
        if (autoNetCounter == 10)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralDown");
        }
        else if (autoNetCounter == 20)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_mp_gameopts") == 0)
    {
        if (autoNetCounter == 10)
        {
            if (!autoNetMissionCount)
            {
                AddAutoNetMission("hot1i", 250, 250);
            }
            MissionPlayList::ClearEntries();
            for (unsigned int i = 0; i < autoNetMissionCount; i++)
            {
                MissionPlayList::AddEntry(autoNetMissions[i].name, autoNetMissions[i].name[4], 0, 0,
                                          autoNetMissions[i].tickets[0], autoNetMissions[i].tickets[1]);
            }
            MissionPlayList::SelectNextEntry();
            autoNetRepeat = -1;
        }
        else if (autoNetCounter == 20)
        {
            if (netScript == NS_DEDICATED)
            {
                LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralDown");
                if (autoNetRepeat == -1)
                {
                    autoNetRepeat = 7;
                }
                if (autoNetRepeat-- > 0)
                {
                    autoNetCounter = 11;
                }
            }
        }
        else if (autoNetCounter == 30)
        {
            if (netScript == NS_DEDICATED)
            {
                LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralRight");
            }
        }
        else if (autoNetCounter == 50)
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_mp_lobby") == 0 || strcmp(pcScreen, "ifs_lobby") == 0)
    {
        if (netScript == NS_HOST && autoNetCounter > 10 && hostGame &&
            hostGame->GetPlayerCount() >= autoNetPlayers && NetQOS::IsDone())
        {
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
            autoNetCounter = 0;
        }
    }
    else if (strcmp(pcScreen, "ifs_mp_lobbyds") == 0)
    {
        if (autoNetCounter == 10)
        {
            netMinPlayersToAutoLaunchDedicated = autoNetPlayers;
        }
    }
    else if (strcmp(pcScreen, "ifs_mp_sessionlist") == 0)
    {
        if (autoNetCounter > 500)
        {
            autoNetCounter = 90;
            LuaHelper::CallTableProcWThis(pcScreen, "Input_Back");
        }
        if (autoNetCounter % 10 == 0)
        {

            int iTop = LuaHelper::GetTop();
            (void)iTop;

            LuaHelper::GetGlobal("mpsessionlist_listbox_layout");
            LuaHelper::PushString("SelectedIdx");
            LuaHelper::RawGetFromTable(-2);
            int iSelected = LuaHelper::GetInt(-1);
            LuaHelper::Pop(2);

            LuaHelper::GetGlobal("mpsessionlist_listbox_contents");
            int iCount = LuaHelper::TableGetN(-1);
            LuaHelper::Pop(1);

            bool bFound = false;
            if (iSelected <= iCount)
            {
                LuaHelper::GetGlobal("mpsessionlist_listbox_contents");
                LuaHelper::TableGetByPosition(-1, iSelected);
                LuaHelper::PushString("namestr");
                LuaHelper::RawGetFromTable(-2);
                char const* pcName = LuaHelper::GetString(-1);
                bFound = _stricmp(pcName, autoNetSession) == 0;
                LuaHelper::Pop(3);
            }

            if (bFound)
            {
                LuaHelper::CallTableProcWThis(pcScreen, "Input_Accept");
                autoNetCounter = 0;
            }
            else if (iSelected == iCount)
            {
                while (iSelected > 1)
                {
                    LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralUp");
                    --iSelected;
                }
            }
            else
            {
                LuaHelper::CallTableProcWThis(pcScreen, "Input_GeneralDown");
            }
        }
    }
}

// FUNCTION: SWBF2 0x005e3f70
void NetSetupP::Recv(NetPacket* pPacket, const void* pSource)
{
    NetMatch::ReceiveRegisterPacket(pPacket->mData + pPacket->mCommIndex, pPacket->mMaxIndex - pPacket->mCommIndex, pSource);
}

// FUNCTION: SWBF2 0x0052e130
bool __fastcall IsGalaxyIdValid(const unsigned __int64* pThis)
{
    return *pThis != 0;
}

extern unsigned __int64 netEnteredLobbyId;
extern unsigned __int64 netCreatedLobbyId;

// FUNCTION: SWBF2 0x005d5dc0
bool HasEnteredLobby()
{
    return netEnteredLobbyId != 0;
}

// FUNCTION: SWBF2 0x005e7430
bool SelectNextHostMode(int)
{
    return false;
}

// FUNCTION: SWBF2 0x005e7440
bool SelectNextHost(int)
{
    return false;
}

// FUNCTION: SWBF2 0x005dd170
int CNetHostGame::GetChangeCount()
{
    return changeCount;
}

// FUNCTION: SWBF2 0x005dd190
int CNetHostGame::GetPlayerCount()
{
    return playerCount;
}

// FUNCTION: SWBF2 0x005dd1b0
void CNetHostGame::ForceWriteShellMove()
{
    timer = NetComm::GetTime() - 0.001f;
}

// FUNCTION: SWBF2 0x005dd1d0
void CNetHostGame::ForceWriteShellUpdate()
{
    unsigned __int64 mask = 0;
    for (int i = 0; i < netCurMaxPlayers; ++i)
    {
        if (NetComm::IsConnected(i))
        {
            mask |= 1ULL << i;
        }
    }
    needSendMask = mask;
}

// FUNCTION: SWBF2 0x005dd250
int CNetHostGame::GetPlayerIndex(int iIndex)
{
    for (int i = 0; i < netCurMaxPlayers; ++i)
    {
        if (NetComm::IsConnected(i) && shellState.players[i].joinAck && iIndex-- == 0)
        {
            return i;
        }
    }
    return -1;
}

// GLOBAL: SWBF2 0x01faab58
char hostPlayerNameBuffer[64];

// FUNCTION: SWBF2 0x005dd2c0
const char* CNetHostGame::GetPlayerName(int iPlayer)
{
    if (iPlayer < 0)
    {
        return 0;
    }
    const char* name = NetGame::GetPlayerName(iPlayer);
    strlcpy(hostPlayerNameBuffer, name, sizeof(hostPlayerNameBuffer));
    return hostPlayerNameBuffer;
}

// FUNCTION: SWBF2 0x005dd300
int CNetHostGame::GetStartTeam(int iPlayer)
{
    if (iPlayer < 0)
    {
        return 0;
    }
    return NetGame::GetStartTeam(iPlayer);
}

// FUNCTION: SWBF2 0x005dd500
bool CNetHostGame::IsFailed()
{
    return hostFailed;
}

extern int netPlayerLimit;
extern int netLobbyState;
extern char const* netLoginErrorMessage;
extern int netLoginErrorLevel;
extern int netNoCableCount;
extern char const* gameErrorMsg;
extern int gameErrorLvl;

// FUNCTION: SWBF2 0x005e7200
int NetSetup::GetMaxPlayers()
{
    return netPlayerLimit;
}

extern int netLocalIndex[2];
extern int netDirectPort;

// FUNCTION: SWBF2 0x005e71b0
int NetSetup::GetNumPlayers()
{
    int count = 0;
    for (int iDest = 0; iDest < 64; ++iDest)
    {
        if (NetComm::IsConnected(iDest))
        {
            ++count;
        }
    }
    return count;
}

// FUNCTION: SWBF2 0x005e6fb0
void WriteJoinReject(NetPktGroup& group, JoinRejectReason eReason)
{
    group.WriteBits(eReason, 4);
}


// FUNCTION: SWBF2 0x00591400
NetSessionId& NetSessionId::operator=(const NetSessionId& other)
{
    id = other.id;
    return *this;
}

// FUNCTION: SWBF2 0x005e71a0
char* NetSetup::GetGameType()
{
    return netGameType;
}

// FUNCTION: SWBF2 0x005e7360
void NetSetup::GetSpecialJoinAddress(NetSessionId* pOut)
{
    if (pOut)
    {
        *pOut = mSpecialJoinAddress;
    }
}

// FUNCTION: SWBF2 0x005e7380
void NetSetup::SetSpecialJoinValid(bool bValid)
{
    mSpecialJoinValid = bValid;
}

// FUNCTION: SWBF2 0x005e73a0
NetAddr& NetSetupP::GetMyAddr()
{
    return myAddr;
}

// FUNCTION: SWBF2 0x005e7450
int NetGetHostPlayer()
{
    return hostPlayer;
}

extern unsigned char sStatsMatchData[0x40];

namespace StatsManager
{
void BeginMatch();
}

// FUNCTION: SWBF2 0x005e6fd0
void ReadJoinReject(NetPktGroup& group, JoinRejectReason& eReason)
{
    unsigned int reason;
    group.ReadBits(reason, 4);
    eReason = (JoinRejectReason)reason;
    JRR_to_Errorbox(eReason);
}

// FUNCTION: SWBF2 0x005d4ab0
hostent* getlocalhost()
{
    char name[256];
    memset(name, 0, sizeof(name));
    gethostname(name, sizeof(name));
    return gethostbyname(name);
}

extern unsigned int netDirectSocket;

// FUNCTION: SWBF2 0x005e7630
void SendStateDst::Cancel()
{
    if (fd >= 0)
    {
        _close(fd);
        fd = -1;
        remove("sendfile.dat");
    }
}

extern int savedHeroRuleUnlock, savedHeroRuleUnlockVal, savedHeroRuleTeam;
extern int savedHeroRulePlayer, savedHeroRuleRespawn, savedHeroRuleRespawnVal, savedPlayerLimit;
extern int savedDedicatedAutoLaunchMinPlayers, savedHostVoteThreshold, savedPreGameTimeMax;
extern bool savedTeamDamage, savedAutoAim, savedShowNames, savedAutoAssignTeams;
extern bool savedAutoAssignTeamsOriginal, savedAwardsEnabled;
extern int netHeroRuleUnlock, netHeroRuleUnlockVal, netHeroRuleTeam;
extern int netHeroRulePlayer, netHeroRuleRespawn, netHeroRuleRespawnVal, hostVoteThreshold;
extern bool netTeamDamage, netAutoAim, netShowNames, netAutoAssignTeams;
extern bool netAutoAssignTeamsOriginalSetting, netAwardsEnabled;
extern int netMinPlayersToAutoLaunchDedicated, netPreGameTimeMax;

// FUNCTION: SWBF2 0x005d32c0
char SaveSessionRuleDefaults()
{
    savedHeroRuleUnlock = netHeroRuleUnlock;
    savedHeroRuleUnlockVal = netHeroRuleUnlockVal;
    savedHeroRuleTeam = netHeroRuleTeam;
    savedHeroRulePlayer = netHeroRulePlayer;
    savedHeroRuleRespawn = netHeroRuleRespawn;
    savedHeroRuleRespawnVal = netHeroRuleRespawnVal;
    savedPlayerLimit = netPlayerLimit;
    savedTeamDamage = netTeamDamage;
    savedAutoAim = netAutoAim;
    savedShowNames = netShowNames;
    savedAutoAssignTeams = netAutoAssignTeams;
    savedAutoAssignTeamsOriginal = netAutoAssignTeamsOriginalSetting;
    savedDedicatedAutoLaunchMinPlayers = netMinPlayersToAutoLaunchDedicated;
    savedHostVoteThreshold = hostVoteThreshold;
    savedPreGameTimeMax = netPreGameTimeMax;
    savedAwardsEnabled = netAwardsEnabled;
    return savedAwardsEnabled;
}

extern unsigned __int64 netLocalGSID;

namespace gf
{
bool isInitialized();
}






// FUNCTION: SWBF2 0x005e7bc0
void WriteFilesAck(NetPktGroup& group, SendStateDst& state)
{
    group.Write(state.done);
    if (!state.done)
    {
        group.WriteBits(state.baseBlock, 24);
        group.WriteBits(state.hasBlocks, 32);
        unsigned int blocks = state.hasBlocks;
        int offset = 0;
        while (blocks)
        {
            blocks >>= 1;
            ++offset;
        }
    }
}


// FUNCTION: SWBF2 0x005e7c30
void ReadFilesAck(NetPktGroup& group, SendStateSrc& state)
{
    bool clientDone;
    group.Read(clientDone);
    if (!clientDone)
    {
        unsigned int clientHasBlocks;
        unsigned int clientBaseBlock;
        group.ReadBits(clientBaseBlock, 24);
        group.ReadBits(clientHasBlocks, 32);
        while (state.baseBlock < (int)clientBaseBlock)
        {
            state.hasBlocks >>= 1;
            ++state.baseBlock;
            if (--state.blockCycle < 0)
            {
                state.blockCycle = 0;
            }
        }
        int offset = 0;
        while (clientHasBlocks)
        {
            if (clientHasBlocks & 1)
            {
                int block = clientBaseBlock + offset;
                int blockCycle = block - state.baseBlock;
                state.hasBlocks |= 1u << blockCycle;
                while (state.hasBlocks & 1)
                {
                    state.hasBlocks >>= 1;
                    ++state.baseBlock;
                    if (--state.blockCycle < 0)
                    {
                        state.blockCycle = 0;
                    }
                }
            }
            clientHasBlocks >>= 1;
            ++offset;
        }
    }
}


void WriteDestMask(NetPktGroup& group, unsigned __int64 mask);
namespace Lua_Callbacks
{
extern int gHuntShowScoreLimit;
}

// FUNCTION: SWBF2 0x005e7000
void WriteLoad(NetPktGroup& group, int, unsigned __int64 firstMask,
               unsigned __int64 secondMask)
{
    NetWire::WritePlayerCount(group, netCurMaxPlayers);
    WriteDestMask(group, firstMask);
    WriteDestMask(group, secondMask);
    unsigned int hostTurnNumber = NetGame::GetTurnNumber();
    group.WriteBits(hostTurnNumber, 32);
    group.Write(Lua_Callbacks::gHuntScoreLimit[Lua_Callbacks::gCurRules]);
    group.Write(Lua_Callbacks::gHuntShowScoreLimit);
    group.Write(Lua_Callbacks::gSpaceAssaultScoreLimit[Lua_Callbacks::gCurRules]);
}

// FUNCTION: SWBF2 0x005e70a0
void ReadLoad(NetPktGroup& group, bool& bReady)
{
    unsigned int maxPlayers;
    group.ReadBits(maxPlayers, 7);
    netCurMaxPlayers = NetWire::ClampPlayerCount(maxPlayers);
    unsigned __int64 firstMask, secondMask;
    ReadDestMask(group, firstMask);
    ReadDestMask(group, secondMask);
    if (!(firstMask & ~secondMask))
    {
        bReady = true;
    }
    unsigned int turn;
    group.ReadBits(turn, 32);
    group.Read(Lua_Callbacks::gHuntScoreLimit[Lua_Callbacks::gCurRules]);
    group.Read(Lua_Callbacks::gHuntShowScoreLimit);
    group.Read(Lua_Callbacks::gSpaceAssaultScoreLimit[Lua_Callbacks::gCurRules]);
    if (bReady)
    {
        unsigned int lagTurns = 4;
        NetGameP::SetClientTurnNumber(turn + lagTurns);
    }
}


extern NetPktGroup inQueue;

// FUNCTION: SWBF2 0x005e54a0
void SetupSleep() {}

// FUNCTION: SWBF2 0x005e09f0
int CNetJoinGame::GetChangeCount()
{
    return changeCount;
}

// FUNCTION: SWBF2 0x005e0a10
int CNetJoinGame::GetPlayerCount()
{
    int count = 0;
    for (int i = 0; i < netCurMaxPlayers; ++i)
    {
        if (shellUpdate.connectMask & (1ULL << i))
        {
            ++count;
        }
    }
    return count;
}

// FUNCTION: SWBF2 0x005e0a80
int CNetJoinGame::GetPlayerIndex(int iIndex)
{
    for (int i = 0; i < netCurMaxPlayers; ++i)
    {
        if ((shellUpdate.connectMask & (1ULL << i)) && !iIndex--)
        {
            return i;
        }
    }
    return -1;
}

// FUNCTION: SWBF2 0x005e0b40
char const* CNetJoinGame::GetEra()
{
    return gameEra;
}

// FUNCTION: SWBF2 0x005e0b60
bool CNetJoinGame::IsFailed()
{
    return joinFailed;
}

// FUNCTION: SWBF2 0x005e0940
bool CNetJoinGame::IsLaunchReady()
{
    return hostLaunched && shellUpdate.patchFile.done;
}

// FUNCTION: SWBF2 0x005e0980
bool CNetJoinGame::IsHostPlaying()
{
    return shellUpdate.hostLoadDone | (shellUpdate.turnNumber > 0);
}

// FUNCTION: SWBF2 0x005e09c0
void CNetJoinGame::Launch()
{
    if (clientLaunched)
    {
        return;
    }
    clientLaunched = true;
    timer = NetComm::GetTime();
}

// FUNCTION: SWBF2 0x005e0b80
bool CNetJoinGame::IsConnected()
{
    return connected && NetComm::GetLocalIndex(0) >= 0;
}

// FUNCTION: SWBF2 0x005e0920
void CNetJoinGame::ForceWriteShellMove()
{
    timer = NetComm::GetTime() - 0.001f;
}

// FUNCTION: SWBF2 0x005e0df0
bool CNetLaunchGame::IsDone()
{
    return launchDone;
}

// FUNCTION: SWBF2 0x005e1d30
bool CNetLoadGame::HostKnowsILoaded()
{
    return hostKnowsILoaded;
}

// FUNCTION: SWBF2 0x005e22d0
bool CNetLoadGame::IsDone()
{
    return loadDone;
}

// FUNCTION: SWBF2 0x005ddc40
int CNetFindGame::GetSelectedSession()
{
    return mSelectedSession;
}

// FUNCTION: SWBF2 0x005ddc60
void CNetFindGame::Cancel()
{
    canceled = true;
}

// FUNCTION: SWBF2 0x005def30
FindType CNetFindGame::GetFindType()
{
    return findType;
}

// FUNCTION: SWBF2 0x005df440
void CNetFindGame::SetDirty(bool bDirty)
{
    mDirty = bDirty;
}

// FUNCTION: SWBF2 0x005df460
bool CNetFindGame::GetDirty()
{
    return mDirty;
}

// FUNCTION: SWBF2 0x005df6c0
void CNetFindGame::SetFavorite(bool) {}

// FUNCTION: SWBF2 0x005df4b0
void CNetFindGame::SetNumVisible(int iNum)
{
    mNumVisible = iNum;
}

// FUNCTION: SWBF2 0x005df4d0
int CNetFindGame::GetNumVisible()
{
    return mNumVisible;
}

// FUNCTION: SWBF2 0x005df4f0
int CNetFindGame::GetPercentDone()
{
    return mPercentDone;
}

// FUNCTION: SWBF2 0x005df080
int CNetFindGame::GetGameCount()
{
    int count = 0;
    PblList<NetGameDesc>::Itor itor = gameList.GetFirst();
    while (itor)
    {
        if (itor->mValid)
        {
            ++count;
        }
        ++itor;
    }
    return count;
}

// FUNCTION: SWBF2 0x005df0e0
void CNetFindGame::SelectFirst()
{
    if (gameList.GetCount() > 0)
    {
        curGame = gameList.GetFirst();
        while (true)
        {
            if (curGame == gameList.End())
            {
                break;
            }
            if (curGame->mValid)
            {
                break;
            }
            ++curGame;
        }
    }
    else
    {
        curGame = gameList.End();
    }
}

// FUNCTION: SWBF2 0x005df190
void CNetFindGame::Select(int iIndex)
{
    if (gameList.GetCount() > 0)
    {
        while (iIndex)
        {
            if (iIndex > 0)
            {
                while (true)
                {
                    ++curGame;
                    if (!curGame)
                    {
                        curGame = gameList.GetFirst();
                    }
                    if (curGame->mValid)
                    {
                        break;
                    }
                }
                --iIndex;
            }
            else
            {
                while (true)
                {
                    --curGame;
                    if (!curGame)
                    {
                        curGame = gameList.GetLast();
                    }
                    if (curGame->mValid)
                    {
                        break;
                    }
                }
                ++iIndex;
            }
        }
    }
    else
    {
        curGame = gameList.End();
    }
}

// FUNCTION: SWBF2 0x005df2c0
void CNetFindGame::SelectSortPos(int iPos)
{
    if (gameList.GetCount() < 1)
    {
        return;
    }
    while (true)
    {
        NetGameDesc* game = curGame;
        if (game->mValid && game->mFindData.mSortOrder == iPos)
        {
            break;
        }
        ++curGame;
        if (!curGame)
        {
            break;
        }
    }
}

// FUNCTION: SWBF2 0x005df420
char* CNetFindGame::GetName()
{
    return curGame->mFindData.mName;
}

// FUNCTION: SWBF2 0x005df480
void CNetFindGame::SetSortPosition(unsigned short usPos)
{
    curGame->mFindData.mSortOrder = usPos;
}

// FUNCTION: SWBF2 0x005df550
int CNetFindGame::GetMaxPlayers()
{
    return curGame->mFindData.mMaxPlayers;
}

// FUNCTION: SWBF2 0x005df570
int CNetFindGame::GetMaxBots()
{
    return curGame->mFindRules.mNumAIUnits;
}

// FUNCTION: SWBF2 0x005df680
bool CNetFindGame::GetFavorite()
{
    if (!curGame)
    {
        return false;
    }
    return curGame->mFindData.mbIsFavorite;
}

// FUNCTION: SWBF2 0x005df6d0
bool CNetFindGame::GetFiltered()
{
    if (curGame)
    {
        return curGame->mFindData.mbFilteredOut;
    }
    return true;
}

// FUNCTION: SWBF2 0x005df710
void CNetFindGame::SetFiltered(bool bFiltered)
{
    if (curGame)
    {
        curGame->mFindData.mbFilteredOut = bFiltered;
    }
}

// FUNCTION: SWBF2 0x005df750
bool CNetFindGame::GetRules(FindGameRules& rules)
{
    NetGameDesc* game = curGame._pNode->_pObject;
    rules = game->mFindRules;
    return game->mRulesAreValid;
}

// FUNCTION: SWBF2 0x005df510
char* CNetFindGame::GetDescriptorExtension()
{
    if (curGame)
    {
        return curGame->mFindData.mGalaxyLobbyPasswordData;
    }
    return 0;
}

// FUNCTION: SWBF2 0x005df340
bool CNetFindGame::IsMetaGame()
{
    NetGameDesc* desc = curGame._pNode->_pObject;
    return _strnicmp(desc->mFindData.mMapType, "metagame", 8) == 0;
}

// FUNCTION: SWBF2 0x005e07d0
void CNetJoinGame::Cancel()
{
    if (shellMove.quitRequested)
    {
        return;
    }
    cancelTimer = NetComm::GetTime() + 5.0f;
    shellMove.quitRequested = true;
    timer = 0.0f;
    for (int i = 0; i < netCurMaxPlayers; ++i)
    {
        if (NetComm::IsConnected(i))
        {
            NetComm::Disconnect(i);
        }
    }
}

// FUNCTION: SWBF2 0x005e0860
bool CNetJoinGame::IsCancelled()
{
    bool doneCancel = false;
    if (connecting)
    {
        doneCancel = true;
    }
    else if (NetComm::GetLocalIndex(0) < 0)
    {
        doneCancel = true;
        if (NetComm::IsConnected(64))
        {
            NetComm::Disconnect(64);
        }
    }
    else
    {
        float now = NetComm::GetTime();
        doneCancel = (shellUpdate.quitMask & (1ULL << NetComm::GetLocalIndex(0))) != 0 ||
                     now > cancelTimer;
    }
    return doneCancel;
}

// FUNCTION: SWBF2 0x005e0b00
char const* CNetJoinGame::GetPlayerName(int iPlayer)
{
    return NetGame::GetPlayerName(iPlayer);
}

// FUNCTION: SWBF2 0x005e0b20
int CNetJoinGame::GetStartTeam(int iPlayer)
{
    return NetGame::GetStartTeam(iPlayer);
}

// FUNCTION: SWBF2 0x005e0bc0
void CNetJoinGame::NextHost(int)
{
    ++hostRequest;
}

// FUNCTION: SWBF2 0x005e2620
void CNetPostLoadHostGame::Cancel()
{
    canceled = true;
}

// FUNCTION: SWBF2 0x005e3280
void CNetPostLoadHostGame::BootPlayer(int) {}

// FUNCTION: SWBF2 0x005e3700
void CNetPostLoadJoinGame::Cancel()
{
    canceled = true;
}

// FUNCTION: SWBF2 0x005df390
const char* CNetFindGame::GetEra()
{
    NetGameDesc& desc = *curGame;
    if (_strnicmp(desc.mFindData.mMapType, "metagame", 8) == 0)
    {
        return &desc.mFindData.mMapType[8];
    }
    else
    {
        return "";
    }
}

void ReadString(NetPktGroup& group, char* text, unsigned int capacity);

// FUNCTION: SWBF2 0x005e7a00
void ReadFiles(NetPktGroup& group, SendStateDst& state)
{
    bool done;
    group.Read(done);
    if (done)
    {
        if (!state.done && state.gotSome)
        {
            _close(state.fd);
            state.fd = -1;
            char fileName[48];
            _snprintf(fileName, sizeof(fileName), "dl_%s", state.fName);
            rename("sendfile.dat", fileName);
        }
        state.done = true;
        return;
    }
    else
    {
        state.gotSome = true;
        if (state.fd < 0)
        {
            state.fd = _open("sendfile.dat", 0x8301, 0x1ff);
        }
        unsigned int hostBaseBlock;
        group.ReadBits(hostBaseBlock, 24);
        while (true)
        {
            unsigned int hostBlockCycle = 0;
            group.ReadBits(hostBlockCycle, 8);
            if (hostBlockCycle-- == 0)
            {
                break;
            }
            int block = hostBaseBlock + hostBlockCycle;
            if (block == 0)
            {
                ReadString(group, state.fName, sizeof(state.fName));
            }
            int clientBlockCycle = block - state.baseBlock;
            state.hasBlocks |= 1u << clientBlockCycle;
            while (state.hasBlocks & 1)
            {
                ++state.baseBlock;
                state.hasBlocks >>= 1;
            }
            char buffer[2048];
            group.ReadBytes(buffer, sizeof(buffer));
            _lseek(state.fd, (hostBaseBlock + hostBlockCycle) * 2048, SEEK_SET);
            _write(state.fd, buffer, sizeof(buffer));
        }
    }
}


void WriteString(NetPktGroup& group, const char* text, unsigned int capacity);

// FUNCTION: SWBF2 0x005e7670
void WriteFiles(NetPktGroup& group, SendStateSrc& state)
{
    if (!state.files)
    {
        group.Write(true);
        return;
    }
    if (state.curFile && state.baseBlock >= state.blockCount)
    {
        RedFreeToHeap(__RedCurrHeap, state.curFile, 0);
        state.curFile = 0;
        bool moreFiles = ++state.fileNum < state.fileCount;
        if (!moreFiles)
        {
            RedFreeToHeap(__RedCurrHeap, state.files, 0);
            state.files = 0;
            group.Write(false);
            return;
        }
    }
    group.Write(false);
    if (!state.curFile)
    {
        char** fileNames = (char**)state.files;
        PblFile file;
        if (file.Open(fileNames[state.fileNum], false, false, true))
        {
            int size = file.GetSize();
            state.blockCount = ((size + 2047) & ~2047) / 2048;
            state.curFile = (char*)RedAllocFromHeap(__RedCurrHeap, state.blockCount * 2048, 0);
            if (state.curFile)
            {
                for (int i = 0; i < state.blockCount; ++i)
                {
                    int count = size;
                    if (count > 2048)
                    {
                        count = 2048;
                    }
                    file.ReadData(state.curFile + i * 2048, count);
                    size -= count;
                }
            }
            file.Close();
        }
        else
        {
            RedFreeToHeap(__RedCurrHeap, state.files, 0);
            state.files = 0;
            return;
        }
    }
    group.WriteBits(state.baseBlock, 24);
    for (int i = 0; i < 8; ++i)
    {
        bool found = false;
        while (true)
        {
            int block = state.baseBlock + state.blockCycle;
            if (block >= state.blockCount)
            {
                state.blockCycle = 0;
            }
            if (state.hasBlocks & (1u << state.blockCycle))
            {
                if (++state.blockCycle >= 32)
                {
                    state.blockCycle = 0;
                }
                continue;
            }
            found = true;
            break;
        }
        if (!found)
        {
            break;
        }
        int block = state.baseBlock + state.blockCycle;
        group.WriteBits(state.blockCycle + 1, 8);
        if (block == 0)
        {
            char** fileNames = (char**)state.files;
            WriteString(group, fileNames[state.fileNum], 32);
        }
        group.WriteBytes(state.curFile + block * 2048, 2048);
        if (++state.blockCycle >= 32)
        {
            state.blockCycle = 0;
        }
    }
    group.WriteBits(0, 8);
}


// FUNCTION: SWBF2 0x005b1a30
unsigned int* __fastcall ClearGalaxyIdWords(unsigned int* pWords)
{
    pWords[0] = 0;
    pWords[1] = 0;
    return pWords;
}

// FUNCTION: SWBF2 0x005df5a0
bool CNetFindGame::GetInfo(char*& name, char*& mapName, int& numPlayers,
                           int& maxPlayers, bool& isMetagame, float& ping,
                           int& serverType, int& sortOrder, bool& isLocked, bool& versionMatches,
                           char*& gameVersion, bool& isFavorite)
{
    NetGameDesc* desc = curGame._pNode->_pObject;
    name = desc->mFindData.mName;
    mapName = desc->mFindData.mMapName;
    numPlayers = desc->mFindData.mNumPlayers;
    maxPlayers = desc->mFindData.mMaxPlayers;
    gameVersion = desc->mGameVer;
    isMetagame = IsMetaGame();
    ping = desc->mFindData.mPingTime;
    serverType = desc->mFindData.mServerType;
    sortOrder = desc->mFindData.mSortOrder;
    isLocked = desc->mFindData.mLocked;
    versionMatches = desc->mFindData.mbVerMatch;
    isFavorite = desc->mFindData.mbIsFavorite;
    return desc->mFindData.mServerType > 0;
}

// FUNCTION: SWBF2 0x005dedf0
bool CNetFindGame::IsReady()
{
    switch (findType)
    {
    case FT_QUICK:
    {
        int count = GetGameCount();
        if (count > 0)
        {
            SelectFirst();
            int i;
            for (i = 0; i < count; ++i)
            {
                char *name, *mission, *version;
                int numPlayers, maxPlayers, arg7, arg8;
                bool password, arg9, arg10, arg12;
                float ping;
                findGame->GetInfo(name, mission, numPlayers, maxPlayers, password,
                                  ping, arg7, arg8, arg9, arg10, version, arg12);
                if (numPlayers < maxPlayers)
                {
                    break;
                }
                findGame->Select(1);
            }
            if (i < count)
            {
                return true;
            }
        }
        return false;
    }
    case FT_PREVIOUS:
        if (NetComm::GetTime() - findStartTime < 2.0f)
        {
            return false;
        }

    case FT_FIXED:
    case FT_CURRENT:
        if (GetGameCount() > 0)
        {
            SelectFirst();
            return true;
        }
        return false;
    default:
        return true;
    }
}

// SYNTHETIC: SWBF2 0x005e1ce0
// CNetLoadGame::`scalar deleting destructor'

// FUNCTION: SWBF2 0x005e1d10
CNetLoadGame::~CNetLoadGame() {}


extern int updatePercent;

// FUNCTION: SWBF2 0x005dd470
bool CNetHostGame::IsLaunchReady()
{
    if (!launchReady)
    {
        return false;
    }
    if (!NetQOS::IsDone())
    {
        return false;
    }
    bool ready = true;
    for (int i = 0; i < netCurMaxPlayers; ++i)
    {
        if (!NetComm::IsConnected(i))
        {
            continue;
        }
        if (shellState.players[i].joinAck)
        {
            continue;
        }
        ready = false;
    }
    if (!ready)
    {
        return false;
    }
    return true;
}

// FUNCTION: SWBF2 0x005ddc80
void CNetFindGame::Pause()
{
    NetMatch::PauseFindGames();
}

// FUNCTION: SWBF2 0x005df3f0
bool CNetFindGame::GetPlayerData(int player, FindGamePlayer& data)
{
    return NetMatch::GetPlayerDataByAddress(curGame->mAddr, player, data);
}

// GLOBAL: SWBF2 0x01e6503c
int netMatchOperation;

extern int netLanRegisterState;
extern bool isLAN;
extern unsigned int netBroadcastSendSocket;
extern unsigned int netBroadcastRecvSocket;

extern bool netLobbyRequestPending;
extern bool netLeavingLobby;
extern unsigned __int64 netCreatedLobbyId;
extern bool gGalaxyInitialized;
galaxy::api::IMatchmaking* GetGalaxyMatchmaking();
void ResetLanRegistration();
namespace gf
{
bool checkError();
}

// FUNCTION: SWBF2 0x005d6770
void ResetLanRegistration()
{
    netLobbyState = 0;
    netLobbyRequestPending = false;
    netLeavingLobby = false;
    netCreatedLobbyId = 0;
    netEnteredLobbyId = 0;
    NetSocket::OpenBroadcastSockets();
}

// GLOBAL: SWBF2 0x01faab3c
char netLocalIPText[18];

// FUNCTION: SWBF2 0x005dd520
NetGameDesc::NetGameDesc(const NetAddr& addr)
{
    mRulesAreValid = false;
    mValid = false;
    mHasSessionData = false;
    mAddr = addr;
    mStamp = NetComm::GetTime();
    mFindData.mDidServerPing = false;
    mFindData.mQuerySentTime = -1.0f;
    mFindData.mResponseRecvTime = -1.0f;
    mFindData.mPingTime = 9.999f;
    mFindData.mConnected = false;
    mFindData.mConnecting = false;
    mFindData.mConnectTimeout = 0.0f;
    mFindData.mSortOrder = 0xffff;
    mFindData.mServerType = 0;
    mFindData.mLocked = false;
    mFindRules.mAimAssist = true;
    mFindRules.mAutoTeams = true;
    mFindRules.mHeroes = false;
    mFindRules.mNumAIUnits = 16;
    mFindRules.mNumReinforcements1 = 200;
    mFindRules.mNumReinforcements2 = 200;
    mFindData.mQueryId = netRandom.GetInt();
    mFindData.mQuerySeqNo = 0;
    mFindData.mbVerMatch = true;
    mFindData.mbIsFavorite = false;
}

// FUNCTION: SWBF2 0x005dd6b0
bool NetGameDesc::operator==(const NetGameDesc& rOther)
{
    if (mAddr == rOther.mAddr)
    {
        return true;
    }
    return false;
}

// GLOBAL: SWBF2 0x01ee7d48
unsigned __int64 netFindSessionId;

// GLOBAL: SWBF2 0x01fa88d4
float netNextLanQueryTime;

// GLOBAL: SWBF2 0x01fa88d8
int netLanQueryCount;

// GLOBAL: SWBF2 0x01fa88dc
unsigned int netLanQuerySequence;

extern int netConnectState;

// FUNCTION: SWBF2 0x005def50
bool CNetFindGame::IsFinished()
{
    if (mWaitingForQueries && NetComm::GetTime() > mQueryFinishTime)
    {
        return true;
    }
    if (!mWaitingForQueries && NetMatch::IsDoneFindGames())
    {
        bool allFinished = true;
        PblList<NetGameDesc>::Itor itor = gameList.GetFirst();
        while (itor)
        {
            NetGameDesc* game = itor;
            ++itor;
            if (!game->mValid)
            {
                continue;
            }
            if (game->mHasSessionData)
            {
                continue;
            }
            if (game->mFindData.mQuerySeqNo >= 8)
            {
                continue;
            }
            allFinished = false;
            break;
        }
        if (allFinished)
        {
            mWaitingForQueries = true;
            mQueryFinishTime = NetComm::GetTime() + 3.0f;
        }
    }
    return false;
}

// FUNCTION: SWBF2 0x005df030
void CNetFindGame::Refresh()
{
    mWaitingForQueries = false;
    mQueryFinishTime = 0.0f;
    NetMatch::EndFindGames(false);
    NetMatch::StartFindGames(useMatchService, findType, 0, &findSessionId);
}

// FUNCTION: SWBF2 0x005dec90
void CNetFindGame::RemoveOld(float age)
{
    float now = NetComm::GetTime();
    PblList<NetGameDesc>::Itor itor = gameList.GetFirst();
    while (itor)
    {
        NetGameDesc* game = itor;
        ++itor;
        if (now - age > game->mStamp)
        {
            if (game->mValid && (game->mFindData.mConnecting || game->mFindData.mConnected))
            {
                NetMatch::EndConnect(game->mAddr, game->mKey, 64);
            }
            gameList.Remove(&game->mNode);
            mDirty = true;
            delete game;
        }
    }
}

// SYNTHETIC: SWBF2 0x005dda90
// CNetFindGame::`scalar deleting destructor'

// FUNCTION: SWBF2 0x005ddac0
CNetFindGame::~CNetFindGame()
{
    RemoveOld(-1.0f);
    NetMatch::EndFindGames(canceled);
    findGame = 0;
    netInGame = false;
}

// GLOBAL: SWBF2 0x01eaed30
float gSelectionTimer;

// FUNCTION: SWBF2 0x005ddb40
void CNetFindGame::SetSelectedSession(int session)
{
    if (session > gameList.GetCount() - 1)
    {
        return;
    }
    if (session != mSelectedSession)
    {
        mSelectedSession = session;
        gSelectionTimer = NetComm::GetTime();
    }
    int validIndex = 0;
    PblList<NetGameDesc>::Itor itor = gameList.GetFirst();
    itor = gameList.Begin();
    while (itor)
    {
        NetGameDesc& game = *itor;
        ++itor;
        if (!game.mValid)
        {
            continue;
        }
        if (mSelectedSession != validIndex++)
        {
            continue;
        }
        if (!game.mFindData.mDidServerPing)
        {
            game.mFindData.mDidServerPing = NetMatch::PingServerByAddress(game.mAddr);
        }
    }
    PblValues::Record('c', PVT_FULL);
}

void __fastcall HandleFindPacket(NetPacket* packet);
void __fastcall HandleFoundPacket(NetPacket* packet, const void* source);

// FUNCTION: SWBF2 0x005e3fa0
void NetSetupP::RecvLan(NetPacket* packet, const void* source)
{
    NetMatch::ReceiveLanRegisterPacket(packet, source);
}

class NetAdmin { public: static void RemovePlayer(int iPlayer); };

// SYNTHETIC: SWBF2 0x005e24a0
// CNetPostLoadHostGame::`scalar deleting destructor'

// FUNCTION: SWBF2 0x005e24d0
CNetPostLoadHostGame::~CNetPostLoadHostGame()
{
    if (canceled)
    {
        int i;
        for (i = 0; i < netCurMaxPlayers; ++i)
        {
            if (NetComm::IsConnected(i))
            {
                NetComm::Disconnect(i);
            }
        }
        if (NetComm::IsConnected(64))
        {
            NetComm::Disconnect(64);
        }
        netCurMaxPlayers = 1;
        netPrevSessionId = netSessionId;
        NetMatch::EndRegisterGame();
        isInSession = false;
        GameVoiceChat::Reset();
        for (i = 0; i < 64; ++i)
        {
            NetGame::SetPlayerGSID(i, 0);
            NetAdmin::RemovePlayer(i);
            NetGame::ClearPlayerCDKey(i);
            NetGameP::SetPlayerName(i, "");
        }
        NetGameP::GetHostIndex() = -1;
        netIsDedicated = false;
    }
    else
    {
        netEnabled = true;
    }
}

// SYNTHETIC: SWBF2 0x005dfb10
// CNetJoinGame::`scalar deleting destructor'

// FUNCTION: SWBF2 0x005dfb40
CNetJoinGame::~CNetJoinGame()
{
    if (shellMove.quitRequested)
    {
        if (cFind)
        {
            delete cFind;
        }
        netInGame = false;
        isInSession = false;
        NetMatch::EndJoinGame();
        for (int i = 0; i < 64; ++i)
        {
            NetGame::SetPlayerGSID(i, 0);
            NetGame::ClearPlayerCDKey(i);
            NetGameP::SetPlayerName(i, "");
        }
        NetQOS::CancelGame();
        shellUpdate.patchFile.Cancel();
    }
}

// FUNCTION: SWBF2 0x005dceb0
void CNetHostGame::DropPlayer(int iPlayer)
{
    NetGameP::SetPlayerName(iPlayer, "");
    NetGame::SetPlayerGSID(iPlayer, 0);
    NetGame::ClearPlayerCDKey(iPlayer);
    if (NetGame::IsHost())
    {
        GameVoiceChat::GetTransport()->DisconnectNode((unsigned char)iPlayer);
    }
    int hostIndex = *NetGameP::GetHostIndex(iPlayer);
    if (hostIndex >= 0)
    {
        *NetGameP::GetHostMode(hostIndex) = 3;
    }
    NetComm::Disconnect(iPlayer);
    bootMask &= ~(1ULL << iPlayer);
    shellState.players[iPlayer].Init();
    NetGameP::SetKey(iPlayer, 0);
}

// GLOBAL: SWBF2 0x01faa720
bool netHandlingFindPacket;

void WriteString(NetPktGroup& pGroup, const char* text, unsigned int capacity);

// FUNCTION: SWBF2 0x005e73b0
void NetSetupP::AddGame(const NetAddr& address)
{
    static_cast<CNetFindGame*>(findGame)->AddGame(address);
}

// FUNCTION: SWBF2 0x005e73f0
void NetSetupP::UpdateGame(const NetAddr& address, const NetKey* key, const NetSessionId* session,
                           const char* version, const FindGameData& data, const FindGameRules* rules)
{
    if (findGame)
    {
        ((CNetFindGame*)findGame)->UpdateGame(address, key, session, version, data, rules);
    }
}

// FUNCTION: SWBF2 0x005e0bf0
NetLaunchGame* NetSetup::LaunchGame(bool host)
{
    return new CNetLaunchGame(host);
}

// FUNCTION: SWBF2 0x005e1ba0
NetLoadGame* NetSetup::LoadGame(bool host)
{
    return new CNetLoadGame(host);
}

// FUNCTION: SWBF2 0x005e0c60
CNetLaunchGame::CNetLaunchGame(bool host)
{
    launchHost = host;
    launchDone = false;
    launchAcked = false;
    NetComm::AdvanceTime(true);
    const float now = NetComm::GetTime();
    for (unsigned int i = 0; i < 64; ++i)
    {
        launchAcks[i] = false;
        sendTimes[i] = now - 1.0f;
    }
    startTime = now;
    timer = 0.0f;
    if (NetComm::GetLocalIndex(0) >= 0)
    {
        NetPktGroup group;
        group.BeginWrite();
        group.WriteBits(0, 1);
        group.EndWrite();
        NetComm::Send(64, PKT_LAUNCH_ACK, group.pktPtr);
    }
    netInMultiplayer = true;
    netInGame = true;
    netWasHost = NetComm::IsHost();
    netWasClient = !netWasHost;
    netWasDedicated = NetGame::IsDedicated();
}

// FUNCTION: SWBF2 0x005e1c10
CNetLoadGame::CNetLoadGame(bool host)
{
    loadHost = host;
    loadDone = false;
    hostKnowsILoaded = false;
    timer = 0.0f;
    ackedMask = 0;
    loadAcked = false;
    NetComm::AdvanceTime(false);
    for (unsigned int i = 0; i < 64; ++i)
    {
        sendTimes[i] = 0.0f;
    }
}

// FUNCTION: SWBF2 0x005e22f0
NetPostLoadHostGame* NetSetup::PostLoadHostGame()
{
    return new CNetPostLoadHostGame;
}

// FUNCTION: SWBF2 0x005e3770
NetPostLoadJoinGame* NetSetup::PostLoadJoinGame()
{
    return new CNetPostLoadJoinGame;
}

// FUNCTION: SWBF2 0x005e2360
CNetPostLoadHostGame::CNetPostLoadHostGame()
{
    canceled = false;
    setupMode = GM_GAME;
    needSendMask = 0;
    loadingMask = 0;
    NetMatch::ChangeRegisterGame();
    StatsManager::BeginMatch();
    timer = 0.0f;
    shellState.canceled = false;
    for (int i = 0; i < 64; ++i)
    {
        shellState.players[i].Init();
        if (NetGame::IsPlaying(i, false))
        {
            shellState.players[i].joinAck = true;
        }
        else if (NetComm::IsConnected(i))
        {
            loadingMask |= 1ULL << i;
        }
    }
}

// FUNCTION: SWBF2 0x005dd950
CNetFindGame::CNetFindGame(FindType type, const char*, NetSessionId* id)
{
    mDirty = true;
    mPercentDone = 0;
    mSelectedSession = -1;
    findType = type;
    if (NetComm::IsLAN())
    {
        FindAddress();
    }
    canceled = false;
    netLoginError = false;
    netGameError = false;
    SetupSleep();
    NetComm::AdvanceTime(true);
    prevFindStartTime = 0.0f;
    findStartTime = NetComm::GetTime();
    if (id)
    {
        findSessionId = *id;
    }
    NetMatch::StartFindGames(useMatchService, findType, 0, &findSessionId);
    mWaitingForQueries = false;
    mQueryFinishTime = 0.0f;
}

// FUNCTION: SWBF2 0x005dd8d0
NetFindGame* NetSetup::FindGame(FindType type, const char* name, NetSessionId* id)
{
    return new CNetFindGame(type, name, id);
}

// GLOBAL: SWBF2 0x01fa8c00
NetKey hostKey;

extern int netPlayerLimit;

// FUNCTION: SWBF2 0x005df890
CNetJoinGame::CNetJoinGame(NetFindGame* find)
{
    SaveSessionRuleDefaults();
    NetGame::ClearAllCDKeyInfo();
    cFind = (CNetFindGame*)find;
    if (cFind->gameList.GetCount() < 1)
    {
        return;
    }

    cFind->GetHostAddr(hostAddr, hostKey);
    connecting = true;
    hostPlayer = -1;
    NetMatch::StartJoinGame(hostAddr, hostKey);
    connected = NetMatch::StartConnect(hostAddr, hostKey, 64);
    strlcpy(gameName, cFind->GetName(), sizeof(gameName));
    NetSetup::SetGameName(gameName);
    GameLoop::SetMetagameRulesOn(cFind->IsMetaGame());
    strlcpy(gameEra, cFind->GetEra(), sizeof(gameEra));
    hostLaunched = false;
    tryCount = 0;
    clientLaunched = false;
    changeCount = 0;
    timer = 0.0f;
    hostRequest = 0;
    moveNum = 0;
    ackedMoveNum = 0;
    joinFailed = false;
    rejectReason = 0;
    cancelTimer = 0.0f;
    for (unsigned int i = 0; i < 2; ++i)
    {
        NetCommP::GetLocalIndex(i) = -1;
    }
    isInSession = true;
    netPlayerLimit = cFind->GetMaxPlayers();
    netNumBots = cFind->GetMaxBots();
}

// FUNCTION: SWBF2 0x005df820
NetJoinGame* NetSetup::JoinGame(NetFindGame* find)
{
    return new CNetJoinGame(find);
}

// FUNCTION: SWBF2 0x005ded60
void CNetFindGame::RemoveGameByAddress(const NetAddr* address)
{
    NetGameDesc probe(*address);
    PblList<NetGameDesc>::Itor itor = gameList.GetFirst();
    while (itor)
    {
        NetGameDesc* game = itor;
        if (*game == probe)
        {
            gameList.Remove(&game->mNode);
            mDirty = true;
            delete game;
        }
    }
}

bool PumpGalaxyMatchmaking();

// FUNCTION: SWBF2 0x005dd330
void CNetHostGame::BootPlayer(int player)
{
    char name[64];
    strlcpy(name, GetPlayerName(player), sizeof(name));
    char* suffix = strchr(name, '|');
    if (suffix)
    {
        *suffix = 0;
    }
    int index = 0;
    for (; index < netCurMaxPlayers; ++index)
    {
        if (!NetComm::IsConnected(index))
        {
            continue;
        }
        if (strcmp(name, NetGame::GetPlayerName(index)) == 0)
        {
            break;
        }
    }
    if (index == netCurMaxPlayers)
    {
        return;
    }
    bootMask |= 1ULL << index;
    bootTime[index] = NetComm::GetTime() + 5.0f;
}

// SYNTHETIC: SWBF2 0x005e3740
// CNetPostLoadJoinGame::`scalar deleting destructor'

// FUNCTION: SWBF2 0x005e3470
CNetPostLoadJoinGame::~CNetPostLoadJoinGame()
{
    int i;
    if (canceled)
    {
        netPrevSessionId = netSessionId;
        NetMatch::EndJoinGame();
        isInSession = false;
        GameVoiceChat::Reset();
        for (i = 0; i < netCurMaxPlayers; ++i)
        {
            if (NetComm::IsConnected(i))
            {
                NetComm::Disconnect(i);
            }
        }
        netCurMaxPlayers = 1;
        if (NetComm::IsConnected(64))
        {
            NetComm::Disconnect(64);
        }
        for (i = 0; i < 64; ++i)
        {
            NetGame::SetPlayerGSID(i, 0);
            NetGame::ClearPlayerCDKey(i);
            NetGameP::SetPlayerName(i, "");
        }
        return;
    }

    netEnabled = true;
    NetComm::AdvanceTime(true);
    float now = NetComm::GetTime();
    float finishTime = now + 0.5f;
    while (true)
    {
        SetupSleep();
        NetComm::AdvanceTime(true);
        now = NetComm::GetTime();
        if (now > nextStats)
        {
            nextStats = now + 0.2f;
            NetPktGroup outgoing;
            outgoing.BeginWrite();
            outgoing.Write(false);
            outgoing.EndWrite();
            NetComm::Send(64, 19, &outgoing);
        }
        if (now > finishTime)
        {
            break;
        }
        NetComm::Update();
        while (true)
        {
            NetPacket* packet = NetComm::Recv(PKT_GAME_UPDATE, PKT_NONE);
            if (!packet)
            {
                break;
            }
            if (packet)
            {
                packet->Free();
            }
            finishTime = now + 3.0f;
        }
        while (true)
        {
            NetPktGroup incoming;
            if (!NetComm::Recv(incoming, PKT_STATS_UPDATE))
            {
                break;
            }
            incoming.BeginRead();
            if (incoming.IsBad())
            {
                incoming.Free();
                continue;
            }
            bool accepted;
            incoming.Read(accepted);
            if (accepted)
            {
                finishTime = now + 3.0f;
            }
            incoming.Free();
        }
    }
}

// FUNCTION: SWBF2 0x005e3a90
void NetSetup::WriteGameUpdate(NetPktGroup& group)
{
    int player = NetGame::GetCurDst();
    if (NetGame::GetJoystickIndex(player) < 0)
    {
        NetGame::WritePreGame(group);
        if (!(playerChangedSendMask & (1ULL << player)))
        {
            group.Write(false);
        }
        else if (NetGame::GetAckedUpdateTurn(player) > playerChangeTurn[player])
        {
            playerChangedSendMask &= ~(1ULL << player);
            playerChangedMask[player] = 0;
            group.Write(false);
        }
        else
        {
            group.Write(true);
            WriteDestMask(group, playerChangedMask[player]);
            for (int i = 0; i < netCurMaxPlayers; ++i)
            {
                if (playerChangedMask[player] & (1ULL << i))
                {
                    group.Write(NetGame::GetStartTeam(i) == 1);
                    if (i != player)
                    {
                        WriteString(group, NetGame::GetPlayerName(i), 32);
                        unsigned __int64 id = NetGame::GetPlayerGSID(i);
                        group.WriteBytes((const char*)&id, sizeof(id));
                        bool isGuest = NetGame::GetGuestOf(i) >= 0;
                        group.Write(isGuest);
                        if (isGuest)
                        {
                            group.WriteBits(NetGame::GetGuestOf(i), 6);
                        }
                        else
                        {
                            WriteAddr(group, clientAddrs[i]);
                            group.WriteBits(NetComm::GetArch(i), 2);
                        }
                    }
                }
            }
        }
    }
}


// FUNCTION: SWBF2 0x005e40b0
void NetSetupP::NextHost(int direction)
{
    int top = LuaHelper::GetTop();
    (void)top;
    LuaHelper::GetGlobal("lobby_listbox_layout");
    LuaHelper::PushString("SelectedIdx");
    LuaHelper::RawGetFromTable(-2);
    if (LuaHelper::IsNumber(-1))
    {
        int selected = LuaHelper::GetInt(-1);
        LuaHelper::Pop(2);
        if (NetComm::IsHost())
        {
            int player = 0;
            for (; player < netCurMaxPlayers; ++player)
            {
                if (NetComm::IsConnected(player))
                {
                    if (selected == 1)
                    {
                        break;
                    }
                    --selected;
                }
            }
            bool changed = false;
            if (direction > 0)
            {
                changed = SelectNextHostMode(player);
            }
            else if (direction < 0)
            {
                changed = SelectNextHost(player);
            }
            if (changed && hostGame)
            {
                ++((CNetHostGame*)hostGame)->changeCount;
            }
        }
        else
        {
            ((CNetJoinGame*)joinGame)->NextHost(direction);
        }
    }
    else
    {
        LuaHelper::Pop(2);
    }
}

extern int gDebugCount;

static const char* GetLoginNameRow(int iViewport)
{
    if (iViewport < 0 || iViewport >= (int)(sizeof(netLoginNames) / sizeof(netLoginNames[0])))
    {
        return "";
    }
    return netLoginNames[iViewport];
}

// FUNCTION: SWBF2 0x005e5bb0
void WriteShellMove(NetPktGroup& group, ShellUpdate& update, ShellMove& move)
{
    int viewport = FLInputManager::Instance()->GetPrimaryPort();
    WriteString(group, GetLoginNameRow(viewport), 32);
    if (move.launchGuestHost)
    {
        ++gDebugCount;
    }
    group.Write(move.launchGuestHost);
    group.Write(move.isTeamOne);
    group.Write(move.quitRequested);
    char mission[64];
    strlcpy(mission, GameLoop::GetNextMission(), sizeof(mission));
    WriteString(group, netGameName, 64);
    WriteString(group, mission, 64);
    group.WriteBits(move.hostRequest, 16);
    group.WriteBits(move.moveNum, 24);
    for (int local = 0; local < 2; ++local)
    {
        if (NetComm::GetLocalIndex(local) < 0)
        {
            break;
        }
        group.Write(true);
        group.WriteBits(NetComm::GetLocalIndex(local), 6);
        if (local == 0)
        {
            unsigned __int64 id = NetMatch::GetLocalGSID();
            group.WriteBytes((const char*)&id, sizeof(id));
        }
    }
    group.Write(false);
    WriteFilesAck(group, update.patchFile);
}

// FUNCTION: SWBF2 0x005e5d10
bool ReadShellMove(NetPktGroup& group, int player, ShellMove& move, ShellState& state)
{
    bool changed = false;
    char name[32];
    ReadString(group, name, sizeof(name));
    if (player != NetComm::GetLocalIndex(0))
    {
        if (strcmp(NetGame::GetPlayerName(player), name))
        {
            changed = true;
            NetGameP::SetPlayerName(player, name);
        }
    }
    bool guestHost, teamOne;
    group.Read(guestHost);
    group.Read(teamOne);
    if (player != NetComm::GetLocalIndex(0))
    {
        if ((NetGame::GetStartTeam(player) == 1) != teamOne)
        {
            changed = true;
            NetGame::SetStartTeam(player, teamOne ? 1 : 2);
        }
    }
    bool oldQuit = state.players[player].quitRequested;
    group.Read(move.quitRequested);
    if (move.quitRequested && !oldQuit)
    {
        changed = true;
        state.players[player].quitRequested = true;
    }
    char gameName[64], missionName[64];
    ReadString(group, gameName, sizeof(gameName));
    ReadString(group, missionName, sizeof(missionName));
    group.ReadBits(move.hostRequest, 16);
    group.ReadBits(move.moveNum, 24);
    for (int local = 0;; ++local)
    {
        bool present;
        group.Read(present);
        if (!present)
        {
            break;
        }
        unsigned int bits;
        group.ReadBits(bits, 6);
        int index = bits;
        if (index == player || NetGame::GetGuestOf(index) == player)
        {
            ShellPlayer& shellPlayer = state.players[index];
            if (!shellPlayer.joinAck)
            {
                shellPlayer.joinAck = true;
                changed = true;
            }
        }
        if (local == 0)
        {
            unsigned __int64 id;
            group.ReadBytes((char*)&id, sizeof(id));
            if (player != NetComm::GetLocalIndex(0))
            {
                unsigned __int64 oldId = NetGame::GetPlayerGSID(player);
                if (id != oldId)
                {
                    changed = true;
                    NetGame::SetPlayerGSID(player, id);
                    NetGame::ClearPlayerCDKey(player);
                }
            }
        }
    }
    ReadFilesAck(group, state.players[player].patchFile);
    return changed;
}

// GLOBAL: SWBF2 0x007e8e52
bool netReplicaOk = true;
// GLOBAL: SWBF2 0x007e8e53
bool netRelayOk = true;
extern bool g_SpectatorEnabled[1];

// FUNCTION: SWBF2 0x005e54b0
void WriteJoin(NetPktGroup& group, unsigned int guestCount)
{
    group.WriteBits(guestCount, 3);
    NetArch arch = NET_ARCH_PC;
    if (netScript == NS_CLIENT && g_bNoRender)
    {
        arch = NET_ARCH_FAKE;
    }
    group.WriteBits(arch, 2);

    FLInputManager* input = FLInputManager::Instance();
    int playerIndexes[1];
    unsigned int count = 0;
    unsigned int viewport;
    unsigned int guest;
    for (viewport = 0; viewport < _countof(playerIndexes); ++viewport)
    {
        bool active = input->mActiveControllers[viewport];
        if (active)
        {
            playerIndexes[count++] = viewport;
        }
    }
    for (viewport = count; viewport < _countof(playerIndexes); ++viewport)
    {
        playerIndexes[viewport] = -1;
    }
    int joystick = playerIndexes[0];
    WriteString(group, GetLoginNameRow(joystick), 32);

    unsigned __int64 id = NetMatch::GetLocalGSID();
    group.WriteBytes((const char*)&id, sizeof(id));
    count = 1;
    for (guest = 0; guest < guestCount; ++guest)
    {
        joystick = playerIndexes[count];
        char converted[66];
        PblUnicode::ustrncnv(converted, PlayerProfile::GetProfile((unsigned int)joystick)->GetName(), _countof(converted));
        WriteString(group, converted, _countof(converted));
        ++count;
    }
    group.WriteBits(intraSessionPlayerId[0], 32);
    for (guest = 0; guest < guestCount; ++guest)
    {
        group.WriteBits(intraSessionPlayerId[guest + 1], 32);
    }
    group.Write(netCrashAddr != 0);
    if (netCrashAddr)
    {
        group.WriteBits(netCrashAddr, 32);
    }
    group.Write(netReplicaOk);
    group.Write(netRelayOk);
    WriteAddr(group, myAddr);
    WriteAddr(group, hostAddrs[0]);
    group.Write(g_SpectatorEnabled[0]);
}

bool PumpGalaxyMatchmaking();

// FUNCTION: SWBF2 0x005e3720
void CNetPostLoadJoinGame::Update()
{
    NetMatch::UpdateLogin(-1);
    NetMatch::UpdatePresence(false);
    NetMatch::UpdateJoinGame();
}

// FUNCTION: SWBF2 0x005df790
void CNetFindGame::GetHostAddr(NetAddr& address, NetKey& key) const
{
    NetGameDesc* game = curGame;
    address = game->mAddr;
    key = game->mKey;
    game->mFindData.mQuerySeqNo = 69;
    if (game->mFindData.mConnecting || game->mFindData.mConnected)
    {
        NetMatch::EndConnect(game->mAddr, game->mKey, 64);
        game->mFindData.mConnecting = false;
        game->mFindData.mConnected = false;
    }
}

// FUNCTION: SWBF2 0x005dd730
void NetGameDesc::Update(const NetAddr& address, const NetKey* key, const NetSessionId* session,
                         const char* version, const FindGameData& data, const FindGameRules* rules)
{
    mValid = true;
    mAddr = address;
    mFindData.mNumPlayers = data.mNumPlayers;
    mFindData.mMaxPlayers = data.mMaxPlayers;
    strlcpy(mFindData.mName, data.mName, sizeof(mFindData.mName));
    strlcpy(mFindData.mMapType, data.mMapType, sizeof(mFindData.mMapType));
    if (data.mMapName[0])
    {
        strlcpy(mFindData.mMapName, data.mMapName, sizeof(mFindData.mMapName));
        mFindData.mLocked = data.mLocked;
        mFindData.mServerType = data.mServerType;
        mFindData.mbVerMatch = data.mbVerMatch;
        mFindData.mPingTime = data.mPingTime;
    }
    mFindData.mbIsFavorite = false;
    memcpy(mFindData.mGalaxyLobbyPasswordData, data.mGalaxyLobbyPasswordData, sizeof(mFindData.mGalaxyLobbyPasswordData));
    if (key)
    {
        mKey = *key;
    }
    if (session)
    {
        mPrevId = *session;
    }
    if (version)
    {
        strlcpy(mGameVer, version, sizeof(mGameVer));
        mGameVer[15] = 0;
    }
    mStamp = NetComm::GetTime();
    if (rules)
    {
        mRulesAreValid = true;
        mFindRules = *rules;
    }
}

// FUNCTION: SWBF2 0x005dea00
void CNetFindGame::AddGame(const NetAddr& address)
{
    if (gameList.GetCount() >= 2000)
    {
        return;
    }
    NetGameDesc probe(address);
    PblList<NetGameDesc>::Itor itor = gameList.GetFirst();
    while (itor)
    {
        NetGameDesc* game = itor;
        if (*game == probe)
        {
            return;
        }
        ++itor;
    }
    NetGameDesc* game = new NetGameDesc(address);
    gameList.AddLast(&game->mNode, game);
    mDirty = true;
}

// FUNCTION: SWBF2 0x005deb60
void CNetFindGame::UpdateGame(const NetAddr& address, const NetKey* key, const NetSessionId* session,
                               const char* version, const FindGameData& data, const FindGameRules* rules)
{
    PblValues::Record('u', PVT_FULL);
    NetGameDesc probe(address);
    PblList<NetGameDesc>::Itor itor = gameList.GetFirst();
    while (itor)
    {
        NetGameDesc* game = itor;
        if (*game == probe)
        {
            mDirty = true;
            game->Update(address, key, session, version, data, rules);
            if (findType == FT_PREVIOUS || findType == FT_FIXED)
            {
                game->mHasSessionData = true;
            }
            if (GetGameCount() == 1)
            {
                curGame = itor;
                NetSocketPeerRecord endpoint;
                curGame->mAddr.GetAddr(endpoint, &curGame->mKey);
                int ip, port;
                NetSocket::GetAddr(&endpoint, ip, port);
            }
            break;
        }
        ++itor;
    }
}

extern bool netAutoAssignTeams;
extern bool netAutoAssignTeamsOriginalSetting;
extern bool netHeroesEnabled;
extern int netHeroRuleUnlock, netHeroRuleUnlockVal, netHeroRulePlayer, netHeroRuleRespawn, netHeroRuleRespawnVal;
extern int gSpawnInvicibility;
extern bool netAutoAim, netShowNames, netTeamDamage;
extern int netCurMaxHosts;
extern bool dragEnabled;
extern int dragLatency, dragBandwidth, dragLoss, dragJitter;
extern bool netIsDedicated;
extern bool netIsHostDedicated;
extern NetKey hostKey;

// GLOBAL: SWBF2 0x01ead4b4
bool netJoinUpdatePending;

void WriteDestMask(NetPktGroup& group, unsigned __int64 mask);
void UpdateCurMaxHosts(unsigned int uMask, bool bIncludeCurrent);
void WriteFiles(NetPktGroup& group, SendStateSrc& state);
void ReadFiles(NetPktGroup& group, SendStateDst& state);

// FUNCTION: SWBF2 0x005e0e10
void WriteLaunch(NetPktGroup& group, bool bAcked, int player, bool bPlaying)
{
    group.Write(netAutoAssignTeams);
    group.Write(netAutoAssignTeamsOriginalSetting);
    group.Write(bAcked);

    unsigned __int64 playerMask = 0;
    int i;
    for (i = 0; i < netCurMaxPlayers; ++i)
    {
        bool present;
        if (bPlaying)
        {
            present = NetGame::IsPlaying(i, false);
        }
        else
        {
            present = NetComm::IsConnected(i);
        }
        if (present)
        {
            playerMask |= 1ULL << i;
        }
    }
    for (int slot = 0; slot < 2; ++slot)
    {
        int guest = NetGame::GetGuestIndex(player, slot);
        if (guest >= 0)
        {
            playerMask |= 1ULL << guest;
        }
    }
    WriteDestMask(group, playerMask);

    char mission[64];
    strlcpy(mission, GameLoop::GetNextMission(), sizeof(mission));
    mission[63] = 0;
    WriteString(group, mission, 64);
    WriteSessionId(group, netSessionId);

    group.Write(netHeroesEnabled);
    if (netHeroesEnabled)
    {
        group.WriteBits(netHeroRuleUnlock, 3);
        group.WriteBits(netHeroRuleUnlockVal, 9);
        group.WriteBits(netHeroRulePlayer, 2);
        group.WriteBits(netHeroRuleRespawn, 2);
        group.WriteBits(netHeroRuleRespawnVal, 9);
    }
    group.WriteBits(hostVoteThreshold, 8);
    group.Write(netTeamDamage);
    group.Write(netAutoAim);
    group.WriteBits(gSpawnInvicibility, 8);
    group.Write(netShowNames);

    int hostIndex = *NetGameP::GetHostIndex(player);
    if (hostIndex >= 0)
    {
        if (*NetGameP::GetHostMode(hostIndex))
        {
            group.Write(true);
            group.WriteBits(hostIndex, 2);
            NetCommP::Connect(hostIndex + 64, NetCommP::GetAddr(player));

            unsigned __int64 clientMask = 0;
            for (i = 0; i < netCurMaxPlayers; ++i)
            {
                if (*NetGameP::GetClientOfIndex(i) == hostIndex)
                {
                    clientMask |= 1ULL << i;
                }
            }
            WriteDestMask(group, clientMask);
            for (i = 0; i < netCurMaxPlayers; ++i)
            {
                unsigned __int64 bit = clientMask & (1ULL << i);
                if (bit && i != player)
                {
                    WriteAddr(group, clientAddrs[i]);
                }
            }

            int hostType = *NetGameP::GetHostMode(player);
            group.Write(hostType == 2);
            unsigned int hostMask = 0;
            for (i = 0; i < netCurMaxHosts; ++i)
            {
                if (*NetGameP::GetHostMode(i) == hostType)
                {
                    hostMask |= 1 << i;
                }
            }
            group.WriteBits(hostMask, 3);
            for (i = 0; i < netCurMaxHosts; ++i)
            {
                if ((hostMask & (1 << i)) && i != hostIndex)
                {
                    WriteAddr(group, hostAddrs[i]);
                }
            }
        }
    }
    else
    {
        group.Write(false);
        int clientOf = *NetGameP::GetClientOfIndex(player);
        group.WriteBits(clientOf, 2);
        if (clientOf > 0)
        {
            WriteAddr(group, hostAddrs[clientOf]);
        }
    }

    for (i = 0; i < 2; ++i)
    {
        unsigned int maxReinforcements = Team::sTeamMaxReinforcementsOverride[i];
        group.WriteBits(maxReinforcements, 32);
    }
    NetGame::WriteSpectator(group);
}

// FUNCTION: SWBF2 0x005e1290
void ReadLaunch(NetPktGroup& group, bool& bAcked)
{
    bool flagA, flagB;
    group.Read(flagA);
    group.Read(flagB);
    if (!NetGame::IsHost())
    {
        netAutoAssignTeams = flagA;
        netAutoAssignTeamsOriginalSetting = flagB;
    }
    group.Read(bAcked);

    unsigned __int64 playerMask;
    ReadDestMask(group, playerMask);
    UpdateCurMaxPlayers(playerMask, false);
    NetGameP::SetPlaying(playerMask, 0);

    char mission[64];
    ReadString(group, mission, 64);
    NetSessionId session;
    ReadSessionId(group, session);
    if (!NetComm::IsHost())
    {
        GameLoop::SetNextMission(mission);
        netSessionId = session;
    }

    group.Read(netHeroesEnabled);
    if (netHeroesEnabled)
    {
        group.ReadBits((unsigned int&)netHeroRuleUnlock, 3);
        group.ReadBits((unsigned int&)netHeroRuleUnlockVal, 9);
        group.ReadBits((unsigned int&)netHeroRulePlayer, 2);
        group.ReadBits((unsigned int&)netHeroRuleRespawn, 2);
        group.ReadBits((unsigned int&)netHeroRuleRespawnVal, 9);
    }
    group.ReadBits((unsigned int&)hostVoteThreshold, 8);
    group.Read(netTeamDamage);
    group.Read(netAutoAim);
    group.ReadBits((unsigned int&)gSpawnInvicibility, 8);
    group.Read(netShowNames);

    unsigned int value;
    int i;
    if (!NetComm::IsHost())
    {
        bool isHostClient;
        group.Read(isHostClient);
        if (isHostClient)
        {
            group.ReadBits(value, 2);
            int hostIndex = value;
            NetGameP::GetHostIndex() = hostIndex;

            NetSocketPeerRecord address;
            NetSocket::SetAddr(address, 0, 0);
            galaxy::api::GalaxyID noPeer(0);
            address.mGalaxyId = noPeer;
            NetCommP::Connect(hostIndex + 64, &address);

            unsigned __int64 clientMask;
            ReadDestMask(group, clientMask);
            for (i = 0; i < netCurMaxPlayers; ++i)
            {
                if (clientMask & (1ULL << i))
                {
                    *NetGameP::GetClientOfIndex(i) = hostIndex;
                    if (i == NetComm::GetLocalIndex(0))
                    {
                        NetSocket::SetAddr(address, 0, 0);
                        galaxy::api::GalaxyID self(0);
                        address.mGalaxyId = self;
                    }
                    else
                    {
                        NetAddr peer;
                        ReadAddr(group, peer);
                        peer.GetAddr(address, 0);
                        if (NetComm::IsLAN() && !peer.hasPriv)
                        {
                            continue;
                        }
                    }
                    NetCommP::Connect(i, &address);
                }
                else
                {
                    *NetGameP::GetClientOfIndex(i) = -1;
                }
            }

            bool dedicated;
            group.Read(dedicated);
            int hostType = dedicated ? 2 : 1;
            *NetGameP::GetHostMode(hostIndex) = hostType;
            unsigned int hostMask;
            group.ReadBits(hostMask, 3);
            UpdateCurMaxHosts(hostMask, false);
            for (i = 0; i < netCurMaxHosts; ++i)
            {
                if (hostMask & (1 << i))
                {
                    *NetGameP::GetHostMode(i) = hostType;
                    if (i != hostIndex)
                    {
                        NetAddr peer;
                        ReadAddr(group, peer);
                        peer.GetAddr(address, 0);
                        if (NetComm::IsLAN() && !peer.hasPriv)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        NetSocket::SetAddr(address, 0, 0);
                        galaxy::api::GalaxyID self(0);
                        address.mGalaxyId = self;
                    }
                    NetCommP::Connect(i + 64, &address);
                }
                else
                {
                    *NetGameP::GetHostMode(i) = 3;
                }
            }
        }
        else
        {
            group.ReadBits(value, 2);
            int clientOf = value == 3 ? -1 : (int)value;
            *NetGameP::GetClientOfIndex(NetComm::GetLocalIndex(0)) = clientOf;
            if (LocalLimits::IsValidClientOf(clientOf))
            {
                UpdateCurMaxHosts(1 << clientOf, true);
                NetAddr host;
                ReadAddr(group, host);
                NetSocketPeerRecord hostAddress;
                host.GetAddr(hostAddress, 0);
                if (!NetComm::IsLAN() || host.hasPriv)
                {
                    NetCommP::Connect(clientOf + 64, &hostAddress);
                }
            }
        }
    }

    for (i = 0; i < 2; ++i)
    {
        group.ReadBits(value, 32);
        int maxReinforcements = value;
        Team::sTeamMaxReinforcementsOverride[i] = maxReinforcements;
    }
    NetGame::ReadSpectator(group);
}

// FUNCTION: SWBF2 0x005e1770
void CNetLaunchGame::Update()
{
    int i;
    SetupSleep();
    NetComm::AdvanceTime(true);
    float now = NetComm::GetTime();
    NetComm::Update();

    while (true)
    {
        NetPacket* stale = NetComm::RecvRange(PKT_FIND, PKT_SHELL_UPDATE);
        if (!stale)
        {
            break;
        }
        stale->Free();
    }
    if (launchHost)
    {
        while (true)
        {
            NetPacket* loadAck = NetComm::Recv(PKT_LOAD_ACK, PKT_NONE);
            if (!loadAck)
            {
                break;
            }
            loadAck->Free();
        }
    }

    if (launchHost)
    {
        for (int send = 0; send < 8; ++send)
        {
            float oldest = now + 1.0f;
            int player = -1;
            for (i = 0; i < netCurMaxPlayers; ++i)
            {
                if (!NetComm::IsConnected(i))
                {
                    continue;
                }
                if (NetGame::GetGuestOf(i) >= 0)
                {
                    continue;
                }
                if (sendTimes[i] > now - 0.2f)
                {
                    continue;
                }
                if (oldest > sendTimes[i])
                {
                    oldest = sendTimes[i];
                    player = i;
                }
            }
            if (player == -1)
            {
                break;
            }
            NetPktGroup group;
            group.BeginWrite();
            WriteLaunch(group, launchAcks[player], player, false);
            group.EndWrite();
            NetComm::Send(player, PKT_LAUNCH, group.pktPtr);
            sendTimes[player] = now;
        }
    }

    bool done;
    while (true)
    {
        NetPacket* launch = NetComm::Recv(PKT_LAUNCH, PKT_NONE);
        if (!launch)
        {
            break;
        }
        NetPktGroup group;
        group.Set(launch);
        group.BeginRead();
        if (!launchHost && !launchDone)
        {
            ReadLaunch(group, done);
            if (done)
            {
                launchDone = true;
            }
        }
        group.EndRead();
        launch->Free();
        group.BeginWrite();
        group.WriteBits(0, 1);
        group.EndWrite();
        NetComm::Send(64, PKT_LAUNCH_ACK, group.pktPtr);
    }

    if (launchHost)
    {
        while (true)
        {
            NetPacket* ack = NetComm::Recv(PKT_LAUNCH_ACK, PKT_NONE);
            if (!ack)
            {
                break;
            }
            int player = GetSrc(ack);
            if (!launchAcks[player])
            {
                launchAcks[player] = true;
                for (unsigned int slot = 0; slot < 1; ++slot)
                {
                    int guest = NetGame::GetMyGuest(player, slot);
                    if (guest >= 0)
                    {
                        launchAcks[guest] = true;
                    }
                }
            }
            ack->Free();
        }
        if (!launchAcked)
        {
            bool allAcked = true;
            for (i = 0; i < netCurMaxPlayers; ++i)
            {
                if (NetComm::IsConnected(i) && !launchAcks[i])
                {
                    allAcked = false;
                    break;
                }
            }
            if (allAcked || now > startTime + 15.0f)
            {
                launchAcked = true;
                ackedTimer = now + 0.5f;
            }
        }
        else if (now > ackedTimer)
        {
            launchDone = true;
        }
    }
    else
    {
        if (now > startTime + 5.0f)
        {
            launchDone = true;
        }
        while (true)
        {
            NetPacket* update = NetComm::Recv(PKT_GAME_UPDATE, PKT_NONE);
            if (!update)
            {
                break;
            }
            launchDone = true;
            update->Free();
        }
    }
}

// FUNCTION: SWBF2 0x005e5700
void ReadJoin(NetPktGroup& group, int player, unsigned int guestCount, ShellState& state)
{
    unsigned int value;
    group.ReadBits(value, 2);
    int arch = value;
    NetCommP::SetArch(player, (NetArch)arch);

    char name[32];
    ReadString(group, name, sizeof(name));
    NetGameP::SetPlayerName(player, name);
    unsigned __int64 id;
    group.ReadBytes((char*)&id, sizeof(id));
    NetGame::SetPlayerGSID(player, id);
    NetGame::ClearPlayerCDKey(player);

    unsigned int i;
    for (i = 0; i < guestCount; ++i)
    {
        char guestName[32];
        ReadString(group, guestName, sizeof(guestName));
        NetGameP::SetPlayerName(NetGame::GetMyGuest(player, i), guestName);
    }

    unsigned int playerId;
    int j;
    group.ReadBits(playerId, 32);
    state.players[player].playerId = netRandom.GetInt();
    if (IsGalaxyIdValid(&netPrevSessionId.id))
    {
        if (playerId != 0)
        {
            for (j = 0; j < 64; ++j)
            {
                if (intraSessionPlayers[j].playerId == playerId)
                {
                    state.players[player].playerId = playerId;
                    break;
                }
            }
        }
    }
    NetGameP::SetKey(player, state.players[player].playerId);
    for (i = 0; i < guestCount; ++i)
    {
        int guest = NetGame::GetMyGuest(player, i);
        group.ReadBits(playerId, 32);
        state.players[guest].playerId = netRandom.GetInt();
        if (IsGalaxyIdValid(&netPrevSessionId.id))
        {
            if (playerId != 0)
            {
                for (j = 0; j < netCurMaxPlayers; ++j)
                {
                    if (intraSessionPlayers[j].playerId == playerId)
                    {
                        state.players[guest].playerId = playerId;
                        break;
                    }
                }
            }
        }
    }

    bool crashed;
    group.Read(crashed);
    if (crashed)
    {
        unsigned int crashAddr;
        group.ReadBits(crashAddr, 32);
        FILE* file = fopen("crashes.txt", "ta");
        if (file)
        {
            fprintf(file, "player %s had crash at 0x%x\n", NetGame::GetPlayerName(player), crashAddr);
            fclose(file);
        }
    }

    bool replica, relay;
    group.Read(replica);
    group.Read(relay);
    NetAddr clientAddr;
    ReadAddr(group, clientAddr);
    NetAddr seenHostAddr;
    ReadAddr(group, seenHostAddr);
    bool spectator = false;
    group.Read(spectator);
    if (spectator)
    {
        int none = 0;
        NetGame::SetSpectator(player, none);
        int team = NetGame::GetStartTeam(none);
        NetGame::SetStartTeam(player, team);
    }
    NetMatch::HostGetAddr(seenHostAddr, clientAddr, player);
    if (NetGame::GetJoystickIndex(player) < 0)
    {
        clientAddrs[player] = clientAddr;
        if (IsSameNet(clientAddrs[player], seenHostAddr))
        {
            NetCommP::GetPipe(player) = PIPE_LAN;
        }
        else
        {
            NetCommP::GetPipe(player) = PIPE_INTERNET;
        }
    }

    if (NetGame::InShell())
    {
        NetGameP::SetPlayerCanHost(player, replica, relay);
        *NetGameP::GetClientOfIndex(player) = 0;
        if (!replica && !relay)
        {
            *NetGameP::GetClientOfIndex(player) = 0;
        }
    }
    else
    {
        NetGameP::SetPlayerCanHost(player, 0, 0);
        *NetGameP::GetClientOfIndex(player) = 0;
    }
    for (unsigned int k = 0; k < 1; ++k)
    {
        if (NetGame::GetMyGuest(player, k) >= 0)
        {
            int guest = NetGame::GetMyGuest(player, k);
            NetGameP::SetPlayerCanHost(guest, 0, 0);
            *NetGameP::GetClientOfIndex(guest) = *NetGameP::GetClientOfIndex(player);
            NetCommP::GetPipe(guest) = NetComm::GetPipe(player);
        }
    }
}

// FUNCTION: SWBF2 0x005e5fc0
void WriteShellUpdate(NetPktGroup& group, int player, ShellUpdate& update, ShellState& state)
{
    group.WriteBits(player, 6);
    unsigned int guestCount = 0;
    unsigned int i;
    for (i = 0; i < 1; ++i)
    {
        if (NetGame::GetMyGuest(player, i) >= 0)
        {
            ++guestCount;
        }
    }
    group.WriteBits(guestCount, 3);
    for (i = 0; i < guestCount; ++i)
    {
        group.WriteBits(NetGame::GetMyGuest(player, i), 6);
    }
    WriteAddr(group, clientAddrs[player]);

    group.Write(dragEnabled);
    if (dragEnabled)
    {
        group.WriteBits(dragLatency, 12);
        group.WriteBits(dragBandwidth, 12);
        group.WriteBits(dragLoss, 7);
        group.WriteBits(dragJitter, 7);
    }
    group.Write(netIsDedicated);
    group.WriteBits(NetComm::GetMyArch(), 2);
    int localIndex = NetComm::GetLocalIndex(0);
    group.Write(localIndex >= 0);
    if (localIndex >= 0)
    {
        group.WriteBits(localIndex, 6);
    }

    ShellPlayer& self = state.players[player];
    bool booted = update.bootMask & (1ULL << player);
    group.Write(booted);
    bool quitKnown = state.canceled || state.players[player].quitRequested;
    group.Write(quitKnown);
    if (quitKnown)
    {
        bool quit = state.players[player].quitRequested;
        group.Write(quit);
    }

    int ping = NetComm::GetPing(64);
    group.WriteBits(ping, 10);
    int up, down;
    NetComm::GetBandwidth(64, up, down);
    down = up;
    group.WriteBits(up / 1024, 12);
    group.WriteBits(down / 1024, 12);

    int j;
    for (j = 0; j < 64; ++j)
    {
        if (!NetComm::IsConnected(j))
        {
            group.Write(false);
            continue;
        }
        group.Write(true);
        ShellPlayer& other = state.players[j];
        bool guest = NetGame::GetGuestOf(j) >= 0;
        group.Write(guest);
        if (guest)
        {
            group.WriteBits(NetGame::GetGuestOf(j), 6);
        }
        else
        {
            ping = NetComm::GetPing(j);
            group.WriteBits(ping, 10);
            NetComm::GetBandwidth(j, up, down);
            group.WriteBits(up / 1024, 12);
            group.WriteBits(down / 1024, 12);
        }
        if (self.ackedUpdate <= other.changedUpdate)
        {
            group.Write(true);
            char name[32];
            strlcpy(name, NetGame::GetPlayerName(j), sizeof(name));
            WriteString(group, name, 32);
            group.Write(netAutoAssignTeams);
            group.Write(netAutoAssignTeamsOriginalSetting);
            bool teamOne = NetGame::GetStartTeam(j) == 1;
            group.Write(teamOne);
            unsigned __int64 id = NetGame::GetPlayerGSID(j);
            group.WriteBytes((const char*)&id, sizeof(id));
            if (!guest)
            {
                WriteAddr(group, clientAddrs[j]);
            }
        }
        else
        {
            group.Write(false);
        }
    }

    group.Write(update.postLoad && !loadGame);
    unsigned int turn = 0;
    if (update.postLoad)
    {
        turn = NetGame::GetTurnNumber();
    }
    group.WriteBits(turn, 21);
    group.WriteBits(state.players[player].playerId, 32);
    for (i = 0; i < guestCount; ++i)
    {
        group.WriteBits(state.players[NetGame::GetMyGuest(player, i)].playerId, 32);
    }
    if (player != NetGame::GetLocalIndex(0))
    {
        NetGameP::SetKey(64, intraSessionPlayerId[0]);
    }

    unsigned __int64 playingMask = 0;
    if (update.postLoad)
    {
        for (j = 0; j < netCurMaxPlayers; ++j)
        {
            if (NetGame::IsPlaying(j, false))
            {
                playingMask |= 1ULL << j;
            }
        }
    }
    WriteDestMask(group, playingMask);
    WriteString(group, GameLoop::GetNextMission(), 64);

    unsigned int team1, team2;
    MissionPlayList::GetCurrentTeamNames(team1, team2);
    group.WriteBits(team1, 32);
    group.WriteBits(team2, 32);
    unsigned int mission = MissionPlayList::GetFirstMission();
    while (mission)
    {
        if (DownloadableContent::IsMissionDownloaded(mission))
        {
            group.WriteBits(mission, 32);
        }
        mission = MissionPlayList::GetNextMission();
    }
    group.WriteBits(mission, 32);
    group.WriteBits(update.moveNum, 32);

    int turnRate = 20;
    unsigned int turnScale = 1;
    turnRate = netFPS;
    if (useProfile)
    {
        PlayerProfile::tProfile* profile = PlayerProfile::GetCurProfile();
        if (profile)
        {
            turnRate = profile->mOnlineOptions.mu8TurnsPerSecond;
        }
    }
    if (turnRate < 10)
    {
        turnRate = netFPS;
    }
    NetWire::WriteTurnRate(group, turnRate, turnScale);
    WriteFiles(group, state.players[player].patchFile);
}

// FUNCTION: SWBF2 0x005e6690
bool ReadShellUpdate(NetPktGroup& group, ShellUpdate& update, ShellMove& move)
{
    bool changed = false;
    unsigned int index;
    group.ReadBits(index, 6);
    int host = index;
    unsigned int guestCount;
    group.ReadBits(guestCount, 3);
    if (!LocalLimits::IsValidGuestCount(guestCount))
    {
        group.SetBad(NET_REJECT_GUEST_COUNT);
        return false;
    }
    int guests[1];
    unsigned int i;
    for (i = 0; i < 1; ++i)
    {
        if (i < guestCount)
        {
            group.ReadBits(index, 6);
            guests[i] = index;
        }
        else
        {
            guests[i] = -1;
        }
    }

    if (group.IsBad())
    {
        return false;
    }

    bool isLocal = host == NetComm::GetLocalIndex(0);
    if (!isLocal)
    {
        NetCommP::GetLocalIndex(0) = host;
        UpdateCurMaxPlayers(1ULL << host, true);
        for (i = 0; i < guestCount; ++i)
        {
            NetCommP::GetLocalIndex(i + 1) = guests[i];
            NetGameP::SetGuestOf(guests[i], host);
            UpdateCurMaxPlayers(1ULL << guests[i], true);
        }
        changed = true;
    }

    NetAddr address;
    ReadAddr(group, address);
    NetMatch::ClientGetAddr(address);

    bool hasLimits;
    group.Read(hasLimits);
    if (hasLimits)
    {
        unsigned int limitA, limitB, limitC, limitD;
        group.ReadBits(limitA, 12);
        group.ReadBits(limitB, 12);
        group.ReadBits(limitC, 7);
        group.ReadBits(limitD, 7);
        if (!NetComm::IsHost() && !dragEnabled && !group.IsBad())
        {
            dragEnabled = hasLimits;
            dragLatency = limitA;
            dragBandwidth = limitB;
            dragLoss = limitC;
            dragJitter = limitD;
        }
    }
    bool flag;
    group.Read(flag);
    unsigned int value;
    group.ReadBits(value, 2);
    if (group.IsBad())
    {
        return false;
    }
    if (!isLocal)
    {
        netIsHostDedicated = flag;
    }
    if (!isLocal)
    {
        NetCommP::SetArch(64, (NetArch)value);
    }

    bool hasHostPlayer;
    group.Read(hasHostPlayer);
    if (hasHostPlayer)
    {
        group.ReadBits(value, 6);
        if (group.IsBad())
        {
            return false;
        }
        hostPlayer = value;
        if (!NetComm::IsConnected(hostPlayer))
        {
            unsigned __int64* address = NetCommP::GetAddr(64);
            NetCommP::Connect(hostPlayer, address);
        }
    }

    bool booted;
    group.Read(booted);
    bool quitKnown;
    group.Read(quitKnown);
    bool quit = false;
    if (quitKnown)
    {
        group.Read(quit);
    }
    if (group.IsBad())
    {
        return false;
    }
    update.bootMask = 0;
    if (booted)
    {
        update.bootMask = 1ULL << host;
    }
    update.quitMask = 0;
    if (quitKnown)
    {
        if (quit)
        {
            update.quitMask = 1ULL << host;
        }
        else
        {
            move.quitRequested = true;
        }
    }

    group.ReadBits(value, 10);
    int ping = value;
    group.ReadBits(value, 12);
    int up = value << 10;
    group.ReadBits(value, 12);
    int down = value << 10;
    if (group.IsBad())
    {
        return false;
    }
    if (!NetGame::IsHost())
    {
        int oldUp, oldDown;
        NetComm::GetBandwidth(64, oldUp, oldDown);
        if (up != oldUp || down != oldDown)
        {
            changed = true;
            NetComm::SetBandwidth(64, up, down);
        }
        NetComm::SetPing(64, ping);
    }

    unsigned __int64& connectMask = update.connectMask;
    connectMask = 0;
    for (int j = 0; j < 64; ++j)
    {
        bool present;
        group.Read(present);
        if (present)
        {
            connectMask |= 1ULL << j;
            if (!NetComm::IsConnected(j))
            {
                changed = true;
            }
            bool guest;
            group.Read(guest);
            if (guest)
            {
                unsigned int owner;
                group.ReadBits(owner, 6);
                NetGameP::SetGuestOf(j, owner);
            }
            else
            {
                group.ReadBits(value, 10);
                ping = value;
                group.ReadBits(value, 12);
                up = value << 10;
                group.ReadBits(value, 12);
                down = value << 10;
                if (!NetGame::IsHost())
                {
                    int curUp, curDown;
                    NetComm::GetBandwidth(j, curUp, curDown);
                    if (up != curUp || down != curDown)
                    {
                        changed = true;
                        NetComm::SetBandwidth(j, up, down);
                    }
                    NetComm::SetPing(j, ping);
                }
            }

            bool hasInfo;
            group.Read(hasInfo);
            if (hasInfo)
            {
                char name[32];
                ReadString(group, name, sizeof(name));
                bool flagA, flagB, teamOne;
                group.Read(flagA);
                group.Read(flagB);
                group.Read(teamOne);
                unsigned __int64 id;
                group.ReadBytes((char*)&id, sizeof(id));
                NetAddr peer;
                if (!guest)
                {
                    ReadAddr(group, peer);
                }
                if (!NetGame::IsHost())
                {
                    if (strcmp(NetGame::GetPlayerName(j), name))
                    {
                        changed = true;
                        NetGameP::SetPlayerName(j, name);
                    }
                    netAutoAssignTeams = flagA;
                    netAutoAssignTeamsOriginalSetting = flagB;
                    if ((NetGame::GetStartTeam(j) == 1) != teamOne)
                    {
                        changed = true;
                        NetGame::SetStartTeam(j, teamOne ? 1 : 2);
                    }
                    unsigned __int64 CurGSID = NetGame::GetPlayerGSID(j);
                    if (CurGSID != id)
                    {
                        changed = true;
                        NetGame::SetPlayerGSID(j, id);
                        NetGame::ClearPlayerCDKey(j);
                    }
                    if (!guest)
                    {
                        clientAddrs[j] = peer;
                    }
                }
            }
        }
        else if (!NetGame::IsHost() && NetComm::IsConnected(j))
        {
            changed = true;
            NetGameP::SetPlayerName(j, "");
            NetGame::SetPlayerGSID(j, 0);
            NetGame::ClearPlayerCDKey(j);
        }
    }
    NetGameP::SetPlaying(connectMask, 0);
    UpdateCurMaxPlayers(connectMask, true);

    bool hostLoadDone;
    group.Read(hostLoadDone);
    update.hostLoadDone = hostLoadDone;
    unsigned int turn;
    group.ReadBits(turn, 21);
    update.turnNumber = turn;
    group.ReadBits(intraSessionPlayerId[0], 32);
    for (i = 0; i < guestCount; ++i)
    {
        group.ReadBits(intraSessionPlayerId[i + 1], 32);
    }
    if (!isLocal)
    {
        NetGameP::SetKey(64, intraSessionPlayerId[0]);
    }

    unsigned __int64 playingMask;
    ReadDestMask(group, playingMask);
    char mission[64];
    ReadString(group, mission, sizeof(mission));
    unsigned int team1, team2;
    group.ReadBits(team1, 32);
    group.ReadBits(team2, 32);
    if (!isLocal)
    {
        MissionPlayList::SetCurrentTeamNames(team1, team2);
    }
    unsigned int missionHash;
    group.ReadBits(missionHash, 32);
    while (missionHash)
    {
        if (!DownloadableContent::IsMissionDownloaded(missionHash))
        {
            netJoinUpdatePending = true;
        }
        group.ReadBits(missionHash, 32);
    }
    unsigned int moveNum;
    group.ReadBits(moveNum, 32);
    unsigned int turnRate, turnScale;
    group.ReadBits(turnRate, 10);
    group.ReadBits(turnScale, 3);
    if (!isLocal)
    {
        NetGameP::SetTurnRate(turnRate, turnScale);
    }
    ReadFiles(group, update.patchFile);
    return changed;
}

// FUNCTION: SWBF2 0x005dfc50
void CNetJoinGame::Update()
{
    PblValues::Record(733, PVT_FULL);
    NetComm::AdvanceTime(true);
    float now = NetComm::GetTime();
    PblValues::Record(now, PVT_FULL);
    NetComm::Update();

    if (connecting)
    {
        if (connected)
        {
            connecting = false;
            hostAddrs[0] = hostAddr;
            PblValues::Record(736, PVT_FULL);
            NetSocketPeerRecord address;
            hostAddr.GetAddr(address, 0);
            NetCommP::Connect(64, &address);
            for (unsigned int i = 0; i < 2; ++i)
            {
                NetCommP::GetLocalIndex(i) = -1;
            }
            delete cFind;
            cFind = 0;
        }
        else
        {
            int reason;
            if (NetMatch::UpdateConnect(hostAddr, hostKey, 64, reason))
            {
                if (reason == 13)
                {
                    connected = true;
                }
                else if (NetComm::IsLAN())
                {
                    connecting = false;
                    joinFailed = true;
                    rejectReason = JRR_CONNECTION_LOST;
                    JRR_to_Errorbox((JoinRejectReason)rejectReason);
                }
            }
            else
            {
                connecting = false;
                joinFailed = true;
                rejectReason = reason;
                JRR_to_Errorbox((JoinRejectReason)rejectReason);
            }
        }
        NetComm::Update();
    }
    else if (!joinFailed)
    {
        PblValues::Record(NetComm::GetLocalIndex(0), PVT_FULL);
        PblValues::Record(timer, PVT_FULL);
        if (NetComm::GetLocalIndex(0) == -1)
        {
            if (now > timer && !shellMove.quitRequested)
            {
                ++tryCount;
                if (tryCount > 12)
                {
                    timer = now + 3.0f;
                }
                else if (tryCount > 6)
                {
                    timer = now + 1.0f;
                }
                else
                {
                    timer = now + 0.5f;
                }
                NetPktGroup group;
                group.BeginWrite();
                group.WriteBits(1, 12);
                unsigned int sequence = NetSetup::GetPassword();
                group.WriteBits(sequence, 32);
                unsigned int guestCount = 0;
                guestCount = GameLoop::GetNumCameras() - 1;
                WriteJoin(group, guestCount);
                group.EndWrite();
                NetComm::Send(64, PKT_JOIN, group.pktPtr);
                PblValues::Record(735, PVT_FULL);
            }
        }
        else if (now > timer)
        {
            bool sendLaunchAck = true;
            if (sendLaunchAck && (hostLaunched || (IsHostPlaying() && shellUpdate.patchFile.done)))
            {
                timer = now + 0.2f;
                NetPktGroup group;
                group.BeginWrite();
                group.WriteBits(0, 1);
                group.EndWrite();
                NetComm::Send(64, PKT_LAUNCH_ACK, group.pktPtr);
            }
            else
            {
                ++tryCount;
                if (shellMove.quitRequested)
                {
                    timer = now + 0.2f;
                }
                else if (!shellUpdate.patchFile.done)
                {
                    timer = now + 0.05f;
                }
                else if (tryCount > 12)
                {
                    timer = now + 3.0f;
                }
                else if (tryCount > 6)
                {
                    timer = now + 1.0f;
                }
                else
                {
                    timer = now + 0.2f;
                }
                NetPktGroup group;
                group.BeginWrite();
                shellMove.moveNum = moveNum;
                ++moveNum;
                shellMove.launchGuestHost = false;
                shellMove.isTeamOne = NetGame::GetStartTeam(NetComm::GetLocalIndex(0)) == 1;
                shellMove.hostRequest = hostRequest;
                WriteShellMove(group, shellUpdate, shellMove);
                group.EndWrite();
                NetComm::Send(64, PKT_SHELL_MOVE, group.pktPtr);
            }
        }

        while (true)
        {
            NetPacket* reject = NetComm::Recv(PKT_REJECT, PKT_NONE);
            if (!reject)
            {
                break;
            }
            NetPktGroup group;
            group.Set(reject);
            group.BeginRead();
            if (!joinFailed)
            {
                ReadJoinReject(group, *(JoinRejectReason*)&rejectReason);
                if (rejectReason == JRR_BUSY)
                {
                    rejectReason = 0;
                }
                else
                {
                    joinFailed = true;
                }
            }
            group.EndRead();
            group.Free();
        }

        netJoinUpdatePending = false;
        while (true)
        {
            NetPktGroup group;
            if (!NetComm::Recv(group, PKT_SHELL_UPDATE))
            {
                break;
            }
            int oldLocal = NetComm::GetLocalIndex(0);
            group.BeginRead();
            if (group.IsBad())
            {
                group.EndRead();
                group.Free();
                continue;
            }
            bool changed = ReadShellUpdate(group, shellUpdate, shellMove);
            if (changed)
            {
                ++changeCount;
            }
            bool bad = group.IsBad();
            if (!bad)
            {
                hostLaunched = shellUpdate.hostLaunched;
                if (shellUpdate.moveNum > ackedMoveNum)
                {
                    ackedMoveNum = shellUpdate.moveNum;
                }
            }
            group.EndRead();
            group.Free();
            if (!bad)
            {
                UpdateCurMaxPlayers(shellUpdate.connectMask, false);
            }

            const int newLocal = NetComm::GetLocalIndex(0);
            if (newLocal != oldLocal)
            {
                timer = 0.0f;
                tryCount = 0;
                int joystick = FLInputManager::Instance()->GetHWPort(0);
                GameVoiceChat::Node* node = joystick >= 0 ? GameVoiceChat::GetNode(joystick) : 0;
                if (node)
                {
                    const unsigned char channel = (unsigned char)newLocal;
                    node->SetID(channel);
                    GameVoiceChat::Transport* transport = GameVoiceChat::GetTransport();
                    transport->SetSendEnable(channel, channel, false, false);
                    transport->SetReceiveEnable(channel, channel, false, false);
                }
            }
            if (shellUpdate.bootMask)
            {
                joinFailed = true;
                rejectReason = JRR_BOOTED;
                JRR_to_Errorbox((JoinRejectReason)rejectReason);
            }
        }

        if (NetGame::GetLocalIndex(0) >= 0)
        {
            while (true)
            {
                NetPacket* launch = NetComm::Recv(PKT_LAUNCH, PKT_NONE);
                if (!launch)
                {
                    break;
                }
                if (!hostLaunched)
                {
                    NetPktGroup group;
                    group.Set(launch);
                    group.BeginRead();
                    bool acked;
                    ReadLaunch(group, acked);
                    group.EndRead();
                    hostLaunched = true;
                }
                if (launch)
                {
                    launch->Free();
                }
            }
        }

        while (true)
        {
            NetPacket* packet = NetComm::Recv(PKT_GAME_UPDATE, PKT_NONE);
            if (!packet)
            {
                packet = NetComm::Recv(PKT_LOAD, PKT_NONE);
            }
            if (!packet)
            {
                break;
            }
            packet->Free();
            NetComm::Disconnect(64);
        }

        float timeout = NetComm::GetLocalIndex(0) >= 0 ? 15.0f : 60.0f;
        if (NetComm::IsConnected(64) && now - NetComm::GetLastRecv(64) >= timeout)
        {
            NetComm::Disconnect(64);
            ++changeCount;
            shellUpdate.connectMask = 0;
            if (NetComm::GetLocalIndex(0) >= 0)
            {
                shellUpdate.connectMask |= 1ULL << NetComm::GetLocalIndex(0);
            }
            int* hostIndex = &NetGameP::GetHostIndex();
            if (*hostIndex >= 0)
            {
                *NetGameP::GetHostMode(*hostIndex) = 3;
                *hostIndex = -1;
            }
            joinFailed = true;
            rejectReason = JRR_CONNECTION_LOST;
            JRR_to_Errorbox((JoinRejectReason)rejectReason);
        }
        if (netJoinUpdatePending)
        {
            NetComm::Disconnect(64);
            ++changeCount;
            shellUpdate.connectMask = 0;
            if (NetComm::GetLocalIndex(0) >= 0)
            {
                shellUpdate.connectMask |= 1ULL << NetComm::GetLocalIndex(0);
            }
            int* hostIndex = &NetGameP::GetHostIndex();
            if (*hostIndex >= 0)
            {
                *NetGameP::GetHostMode(*hostIndex) = 3;
                *hostIndex = -1;
            }
            joinFailed = true;
            rejectReason = JRR_MISSING_DC;
            JRR_to_Errorbox((JoinRejectReason)rejectReason);
        }
        if (!HasEnteredLobby() && !NetComm::IsLAN())
        {
            joinFailed = true;
        }
    }

    if (NetQOS::Update(true))
    {
        ++changeCount;
    }
    NetMatch::UpdateLogin(-1);
    NetMatch::UpdatePresence(false);
    PblValues::Record(734, PVT_FULL);
}

// GLOBAL: SWBF2 0x007e8ee4
char* netGameVersion = "1.1";

galaxy::api::INetworking* GetGalaxyNetworkingInterface();


// GLOBAL: SWBF2 0x01faa724
int netHostUpdateCount;

// GLOBAL: SWBF2 0x01ead900
int netFindQueryCount;

// FUNCTION: SWBF2 0x005dcf90
void CNetHostGame::SendUpdates()
{
    for (int tries = 0; needSendMask && tries < 4; ++tries)
    {
        int start = netRandom.GetInt() & 0xff;
        for (int j = 0; j < 64; ++j)
        {
            int player = (j + start) % 64;
            if (needSendMask & (1ULL << player))
            {
                if (NetComm::IsConnected(player) && NetGame::GetGuestOf(player) < 0)
                {
                    ShellPlayer& shellPlayer = shellState.players[player];
                    NetPktGroup group;
                    group.BeginWrite();
                    ShellUpdate update;
                    update.postLoad = false;
                    update.moveNum = shellPlayer.moveNum;
                    update.bootMask = bootMask;
                    if (shellPlayer.quitRequested)
                    {
                        NetGameP::SetPlayerName(player, "");
                        update.quitMask = 1ULL << player;
                    }
                    WriteShellUpdate(group, player, update, shellState);
                    group.EndWrite();
                    NetComm::Send(player, PKT_SHELL_UPDATE, &group);
                }
                needSendMask &= ~(1ULL << player);
                break;
            }
        }
    }
}

// FUNCTION: SWBF2 0x005dc4e0
void CNetHostGame::Update()
{
    int i;
    ++netHostUpdateCount;
    PblValues::Record(netHostUpdateCount, PVT_FULL);
    NetComm::AdvanceTime(true);
    float now = NetComm::GetTime();
    NetComm::Update();
    int oldChangeCount = changeCount;

    while (true)
    {
        NetPacket* join = NetComm::Recv(PKT_JOIN, PKT_NONE);
        if (!join)
        {
            break;
        }
        int from = GetSrc(join);
        if (from < 64)
        {
            join->Free();
            join = 0;
            if (shellState.players[from].quitRequested)
            {
                DropPlayer(from);
                break;
            }
            needSendMask |= 1ULL << from;
        }
        else if (!BanIP(from, join))
        {
            int player = -1;
            for (i = 0; i < netCurMaxPlayers; ++i)
            {
                if (NetComm::IsConnected(i) && NetCommP::SameAddr(i, from))
                {
                    player = i;
                    break;
                }
            }
            if (player == -1)
            {
                unsigned int j;
                int guests[1];
                for (j = 0; j < 1; ++j)
                {
                    guests[j] = -1;
                }
                int slots = Min(netCurMaxPlayers + 2, netPlayerLimit);
                for (i = 0; i < slots; ++i)
                {
                    if (!NetComm::IsConnected(i))
                    {
                        if (player == -1)
                        {
                            player = i;
                        }
                        else
                        {
                            for (j = 0; j < 1; ++j)
                            {
                                if (guests[j] == -1)
                                {
                                    guests[j] = i;
                                    break;
                                }
                            }
                        }
                    }
                }

                NetPktGroup group;
                group.Set(join);
                group.BeginRead();
                unsigned int version;
                group.ReadBits(version, 12);
                unsigned int password;
                group.ReadBits(password, 32);
                unsigned int guestCount;
                group.ReadBits(guestCount, 3);
                bool hasRoom = player != -1;
                if (!LocalLimits::IsValidGuestCount(guestCount))
                {
                    NetRejectCount(NET_REJECT_GUEST_COUNT);
                    hasRoom = false;
                }
                for (j = 0; hasRoom && j < guestCount; ++j)
                {
                    if (guests[j] == -1)
                    {
                        hasRoom = false;
                    }
                }
                if (hasRoom)
                {
                    if (password != (unsigned int)NetSetup::GetPassword())
                    {
                        group.EndRead();
                        join->Free();
                        join = 0;
                        group.BeginWrite();
                        WriteJoinReject(group, JRR_PASSWORD);
                        group.EndWrite();
                        NetComm::Send(from, PKT_REJECT, &group);
                    }
                    else if (version == 1)
                    {
                        unsigned __int64* address = NetCommP::GetAddr(from);
                        NetCommP::Connect(player, address);
                        UpdateCurMaxPlayers(1ULL << player, true);
                        for (unsigned int k = 0; k < guestCount; ++k)
                        {
                            NetCommP::Connect(guests[k], address);
                            UpdateCurMaxPlayers(1ULL << guests[k], true);
                            NetGameP::SetGuestOf(guests[k], player);
                        }
                        ReadJoin(group, player, guestCount, shellState);
                        group.EndRead();
                        join->Free();
                        join = 0;
                        ++changeCount;
                        needSendMask |= 1ULL << from;
                        if (NetGame::IsDedicated())
                        {
                            launchTimer = NetComm::GetTime();
                        }
                    }
                    else
                    {
                        group.EndRead();
                        join->Free();
                        join = 0;
                        group.BeginWrite();
                        WriteJoinReject(group, JRR_VERSION);
                        group.EndWrite();
                        NetComm::Send(from, PKT_REJECT, &group);
                    }
                }
                else
                {
                    group.EndRead();
                    join->Free();
                    join = 0;
                    group.BeginWrite();
                    WriteJoinReject(group, JRR_FULL);
                    group.EndWrite();
                    NetComm::Send(from, PKT_REJECT, &group);
                }
            }
        }
        if (join)
        {
            join->Free();
        }
    }

    while (true)
    {
        NetPacket* packet = NetComm::Recv(PKT_SHELL_MOVE, PKT_NONE);
        if (!packet)
        {
            break;
        }
        int player = GetSrc(packet);
        if (player)
        {
            ++gDebugCount;
        }
        NetPktGroup group;
        group.Set(packet);
        group.BeginRead();
        ShellMove move;
        bool changed = ReadShellMove(group, player, move, shellState);
        if (changed)
        {
            ++changeCount;
        }
        group.EndRead();
        packet->Free();
        ShellPlayer& shellPlayer = shellState.players[player];
        if (move.hostRequest != shellPlayer.hostRequest)
        {
            shellPlayer.hostRequest = move.hostRequest;
            if (SelectNextHost(player))
            {
                ++changeCount;
            }
        }
        shellPlayer.moveNum = move.moveNum;
        needSendMask |= 1ULL << player;
    }

    if (!launchReady)
    {
        if (now - launchTimer > (netPrevSessionId.IsNULL() ? 0.1f : 5.0f))
        {
            launchReady = true;
        }
    }

    while (true)
    {
        NetPktGroup group;
        if (!NetComm::Recv(group, PKT_SHELL_UPDATE))
        {
            break;
        }
        group.Free();
    }

    if (now > timer)
    {
        timer = now + 0.2f;
        if (NetComm::GetLocalIndex(0) >= 0)
        {
            NetPktGroup group;
            group.BeginWrite();
            ShellMove move;
            move.moveNum = 0;
            move.hostRequest = 0;
            move.launchGuestHost = false;
            move.isTeamOne = NetGame::GetStartTeam(NetComm::GetLocalIndex(0)) == 1;
            move.quitRequested = false;
            WriteShellMove(group, shellUpdate, move);
            group.EndWrite();
            NetComm::Send(64, PKT_SHELL_MOVE, group.pktPtr);
        }
    }

    int oldPlayerCount = playerCount;
    playerCount = 0;
    unsigned __int64 joinedMask = 0;
    for (i = 0; i < netCurMaxPlayers; ++i)
    {
        if (NetComm::IsConnected(i) && shellState.players[i].joinAck)
        {
            ++playerCount;
            joinedMask |= 1ULL << i;
        }
    }
    if (playerCount != oldPlayerCount)
    {
        ++changeCount;
    }
    if (NetQOS::Update(true))
    {
        ++changeCount;
    }

    for (i = 0; i < netCurMaxPlayers; ++i)
    {
        if (!NetComm::IsConnected(i))
        {
            continue;
        }
        float elapsed = NetComm::GetTime() - NetComm::GetLastRecv(i);
        float timeout = 15.0f;
        if (shellState.players[i].quitRequested)
        {
            timeout = 5.0f;
        }
        if (elapsed > timeout)
        {
            DropPlayer(i);
            ++changeCount;
        }
        else if ((bootMask & (1ULL << i)) && now > bootTime[i])
        {
            DropPlayer(i);
            ++changeCount;
        }
    }
    if (changeCount != oldChangeCount)
    {
        needSendMask = joinedMask;
    }
    SendUpdates();
}

// FUNCTION: SWBF2 0x005ddc90
bool CNetFindGame::Update()
{
    PblValues::Record(533, PVT_FULL);
    SetupSleep();
    NetComm::AdvanceTime(true);
    NetComm::Update();
    NetMatch::UpdateFindGames();
    PblValues::Record(536, PVT_FULL);
    float now = NetComm::GetTime();
    bool updated = false;
    bool doQueries = true;

    if (doQueries && !joinGame)
    {
        PblValues::Record(gameList.GetCount(), PVT_FULL);
        int queryBudget = 4;
        PblList<NetGameDesc>::Itor itor = gameList.GetFirst();
        int maxConnecting = 64;
        int connectingCount = 0;
        while (itor)
        {
            NetGameDesc& game = *itor;
            ++itor;
            if (!game.mValid)
            {
                continue;
            }
            if (game.mFindData.mConnected || game.mFindData.mConnecting)
            {
                ++connectingCount;
            }
        }

        if (NetComm::IsLAN())
        {
            itor = gameList.Begin();
            while (itor && queryBudget > 0)
            {
                NetGameDesc& game = *itor;
                ++itor;
                if (!game.mValid)
                {
                    continue;
                }
                if (!game.mFindData.mConnected && !game.mFindData.mConnecting && gSelectionTimer + 1.0f > now)
                {
                    continue;
                }
                float connectTime = 5.0f;
                if (game.mFindData.mQuerySeqNo >= 8)
                {
                    if (game.mFindData.mConnected)
                    {
                        NetMatch::EndConnect(game.mAddr, game.mKey, 64);
                        game.mFindData.mConnected = false;
                        game.mFindData.mConnecting = false;
                    }
                    continue;
                }
                if (game.mFindData.mResponseRecvTime > 0.0f && game.mFindData.mQuerySeqNo >= 5)
                {
                    if (game.mFindData.mConnected)
                    {
                        NetMatch::EndConnect(game.mAddr, game.mKey, 64);
                        game.mFindData.mConnected = false;
                        game.mFindData.mConnecting = false;
                    }
                    game.mFindData.mQuerySeqNo = 8;
                    continue;
                }
                if (game.mFindData.mConnecting && !game.mFindData.mConnected && now > game.mFindData.mConnectTimeout)
                {
                    NetMatch::EndConnect(game.mAddr, game.mKey, 64);
                    game.mFindData.mConnected = false;
                    game.mFindData.mConnecting = false;
                    game.mFindData.mQuerySeqNo = 8;
                }
                PblValues::Record(game.mFindData.mQuerySentTime, PVT_FULL);
                int seqNo = game.mFindData.mQuerySeqNo;
                if (now > (float)(seqNo * seqNo) * 0.2f + game.mFindData.mQuerySentTime)
                {
                    --queryBudget;
                    PblValues::Record(game.mFindData.mConnected, PVT_FULL);
                    if (!game.mFindData.mConnected)
                    {
                        PblValues::Record(game.mFindData.mConnecting, PVT_FULL);
                    }
                    if (game.mFindData.mConnected)
                    {
                        NetSocketPeerRecord address;
                        game.mAddr.GetAddr(address, &game.mKey);
                        int dest = NetCommP::FindJoinSource((const int*)&address);
                        if (dest >= 0)
                        {
                            if (game.mFindData.mQuerySeqNo == 0)
                            {
                                game.mFindData.mQuerySentTime = NetComm::GetTime();
                            }
                            NetPktGroup group;
                            group.BeginWrite();
                            group.Write(false);
                            group.WriteBits(game.mFindData.mQueryId, 32);
                            group.WriteBits(game.mFindData.mQuerySeqNo, 4);
                            group.EndWrite();
                            NetComm::Send(dest, PKT_QUERY, group.pktPtr);
                            ++netFindQueryCount;
                            ++game.mFindData.mQuerySeqNo;
                        }
                    }
                    else if (game.mFindData.mConnecting)
                    {
                        int reason;
                        if (NetMatch::UpdateConnect(game.mAddr, game.mKey, 64, reason))
                        {
                            if (reason == 13)
                            {
                                game.mFindData.mConnected = true;
                                game.mFindData.mConnecting = false;
                            }
                            else
                            {
                                NetMatch::EndConnect(game.mAddr, game.mKey, 64);
                                game.mFindData.mConnecting = false;
                                game.mFindData.mQuerySeqNo = 8;
                            }
                        }
                    }
                    else if (connectingCount < maxConnecting)
                    {
                        game.mFindData.mConnecting = true;
                        game.mFindData.mConnectTimeout = now + connectTime;
                        if (NetMatch::StartConnect(game.mAddr, game.mKey, 64))
                        {
                            game.mFindData.mConnected = true;
                            game.mFindData.mConnecting = false;
                        }
                        ++connectingCount;
                    }
                }
            }
        }

        PblValues::Record(535, PVT_FULL);
        if (NetComm::IsLAN())
        {
            while (true)
            {
                NetPacket* response = NetComm::Recv(PKT_QUERY_RESPONSE, PKT_NONE);
                if (!response)
                {
                    break;
                }
                NetPktGroup group;
                group.Set(response);
                group.BeginRead();
                unsigned int queryId;
                group.ReadBits(queryId, 32);
                unsigned int seqNo;
                group.ReadBits(seqNo, 4);
                bool found = false;
                NetAddr address;
                FindGameData data;
                FindGameRules rules;
                if (findType == 2)
                {
                    queryDone = true;
                    NetMatch::SetAddr(address, queryAddr);
                    AddGame(address);
                }
                else
                {
                    itor = gameList.Begin();
                    for (itor = gameList.Begin(); itor; ++itor)
                    {
                        if (!itor->mValid)
                        {
                            continue;
                        }
                        if (itor->mFindData.mQueryId == queryId)
                        {
                            NetGameDesc& game = *itor;
                            address = game.mAddr;
                            data = game.mFindData;
                            found = true;
                            break;
                        }
                    }
                }

                unsigned int tps = 0;
                PblValues::Record(found, PVT_FULL);
                ReadString(group, data.mName, sizeof(data.mName));
                ReadString(group, data.mMapType, sizeof(data.mMapType));
                ReadString(group, data.mMapName, sizeof(data.mMapName));
                group.Read(rules.mAimAssist);
                group.Read(rules.mAutoTeams);
                group.Read(rules.mTeamDamage);
                group.Read(data.mLocked);
                unsigned int value;
                group.ReadBits(value, 8);
                rules.mNumAIUnits = value;
                group.ReadBits(value, 11);
                rules.mNumReinforcements1 = value;
                group.ReadBits(value, 11);
                rules.mNumReinforcements2 = value;
                group.ReadBits(value, 7);
                data.mNumPlayers = value;
                group.ReadBits(value, 7);
                data.mMaxPlayers = value;
                group.ReadBits(value, 3);
                data.mServerType = value;
                group.ReadBits(tps, 8);
                group.ReadBits(value, 7);
                rules.mMinPlayers = value;
                group.ReadBits(value, 2);
                rules.mAIDifficulty = value;
                group.Read(rules.mShowPlayerNames);
                group.ReadBits(value, 6);
                rules.mSpawnInvincibilityTime = value;
                group.Read(rules.mHeroes);
                if (rules.mHeroes)
                {
                    group.ReadBits(value, 4);
                    rules.mHeroUnlockRule = value;
                    group.ReadBits(value, 8);
                    rules.mHeroUnlockRuleVal = value;
                    group.ReadBits(value, 3);
                    rules.mHeroTeamRule = value;
                    group.ReadBits(value, 4);
                    rules.mHeroPlayerRule = value;
                    group.ReadBits(value, 2);
                    rules.mHeroRespawnRule = value;
                    group.ReadBits(value, 10);
                    rules.mHeroRespawnRuleVal = value;
                }
                group.EndRead();
                response->Free();

                if (findType == 2)
                {
                    UpdateGame(address, 0, 0, NetSetup::GetGameVersion(), data, &rules);
                }
                else if (found)
                {
                    NetGameDesc& game = *itor;
                    float ping = now - ((float)(int)(seqNo * seqNo) * 0.2f + game.mFindData.mQuerySentTime);
                    if (data.mPingTime > ping)
                    {
                        data.mPingTime = ping;
                    }
                    game.mStamp = NetComm::GetTime();
                    PlayerProfile::tProfile* profile = PlayerProfile::GetCurProfile();
                    unsigned char tpsLimit = profile ? profile->mOnlineOptions.mu8TurnsPerSecond : 30;
                    bool tpsOk = tps <= tpsLimit;
                    bool versionOk = true;
                    if (tpsOk && versionOk)
                    {
                        game.mHasSessionData = true;
                    }
                    else
                    {
                        game.mHasSessionData = false;
                    }
                    game.mRulesAreValid = true;
                    game.mFindRules = rules;
                    game.mFindData.mPingTime = data.mPingTime;
                    game.mFindData.mLocked = data.mLocked;
                    game.mFindData.mServerType = data.mServerType;
                    strlcpy(game.mFindData.mName, data.mName, sizeof(game.mFindData.mName));
                    strlcpy(game.mFindData.mMapType, data.mMapType, sizeof(game.mFindData.mMapType));
                    strlcpy(game.mFindData.mMapName, data.mMapName, sizeof(game.mFindData.mMapName));
                    game.mFindData.mNumPlayers = data.mNumPlayers;
                    game.mFindData.mMaxPlayers = data.mMaxPlayers;
                    game.mFindData.mbIsFavorite = false;
                    memset(game.mFindData.mGalaxyLobbyPasswordData, 0, sizeof(game.mFindData.mGalaxyLobbyPasswordData));
                    if (game.mHasSessionData && GetGameCount() == 1)
                    {
                        curGame = itor;
                        NetSocketPeerRecord endpoint;
                        curGame->mAddr.GetAddr(endpoint, &curGame->mKey);
                        int ip, port;
                        NetSocket::GetAddr(&endpoint, ip, port);
                    }
                }
                updated = true;
            }
        }
        else
        {
            itor = gameList.Begin();
            for (itor = gameList.Begin(); itor; ++itor)
            {
                if (!itor->mValid)
                {
                    continue;
                }
                void* networking = GetGalaxyNetworkingInterface();
                int ping = ((galaxy::api::INetworking*)networking)->GetPingWith(itor->mAddr.mGalaxyId);
                float pingTime = fmaxf(0.0f, (float)ping * 0.001f);
                itor->mFindData.mPingTime = pingTime;
            }
        }
        PblValues::Record(538, PVT_FULL);
    }

    RemoveOld(now - prevFindStartTime);
    NetMatch::UpdateLogin(-1);
    mPercentDone = NetMatch::GetPercentDone();
    PblValues::Record(535, PVT_FULL);
    NetMatch::UpdatePresence(false);
    PblValues::Record(534, PVT_FULL);
    if (netLoginError)
    {
        netInGame = false;
    }
    return updated;
}

// FUNCTION: SWBF2 0x005e1d50
void CNetLoadGame::Update()
{
    float now = NetComm::GetTime();
    int i;
    if (loadHost)
    {
        for (i = 0; i < 8; ++i)
        {
            int player = -1;
            float oldest = now;
            int j;
            for (j = 0; j < netCurMaxPlayers; ++j)
            {
                if (!NetGame::IsPlaying(j, false))
                {
                    continue;
                }
                if (NetGame::GetGuestOf(j) >= 0)
                {
                    continue;
                }
                if (sendTimes[j] > now - 0.2f)
                {
                    continue;
                }
                if (oldest > sendTimes[j])
                {
                    player = j;
                    oldest = sendTimes[j];
                }
            }
            if (player != -1)
            {
                NetPktGroup group;
                group.BeginWrite();
                unsigned __int64 playingMask = 0;
                for (j = 0; j < netCurMaxPlayers; ++j)
                {
                    if (NetGame::IsPlaying(j, false))
                    {
                        playingMask |= 1ULL << j;
                    }
                }
                WriteLoad(group, player, playingMask, ackedMask);
                group.EndWrite();
                NetComm::Send(player, PKT_LOAD, group.pktPtr);
                sendTimes[player] = now;
            }
        }
    }

    if (now > timer)
    {
        timer = now + 0.1f;
        if (NetComm::GetLocalIndex(0) >= 0)
        {
            NetPktGroup group;
            group.BeginWrite();
            group.WriteBits(0, 1);
            group.EndWrite();
            NetComm::Send(64, PKT_LOAD_ACK, group.pktPtr);
        }
    }

    if (loadHost)
    {
        while (true)
        {
            NetPacket* ack = NetComm::Recv(PKT_LOAD_ACK, PKT_NONE);
            if (!ack)
            {
                break;
            }
            int player = GetSrc(ack);
            if (!(ackedMask & (1ULL << player)))
            {
                ackedMask |= 1ULL << player;
                for (unsigned int slot = 0; slot < 1; ++slot)
                {
                    int guest = NetGame::GetMyGuest(player, slot);
                    if (guest >= 0)
                    {
                        ackedMask |= 1ULL << guest;
                    }
                }
            }
            ack->Free();
        }
        while (true)
        {
            NetPacket* load = NetComm::Recv(PKT_LOAD, PKT_NONE);
            if (!load)
            {
                break;
            }
            load->Free();
        }
        if (!loadAcked)
        {
            if (now > 60.0f)
            {
                for (i = 0; i < netCurMaxPlayers; ++i)
                {
                    if (!NetGame::IsPlaying(i, false))
                    {
                        continue;
                    }
                    if (ackedMask & (1ULL << i))
                    {
                        continue;
                    }
                    NetGameP::SetNotPlaying(i);
                    NetComm::Disconnect(i);
                    GameVoiceChat::GetTransport()->DisconnectNode((unsigned char)i);
                }
                loadDone = true;
            }
            unsigned __int64 playingMask = 0;
            for (i = 0; i < netCurMaxPlayers; ++i)
            {
                if (NetGame::IsPlaying(i, false))
                {
                    playingMask |= 1ULL << i;
                }
            }
            if (!(playingMask & ~ackedMask))
            {
                loadAcked = true;
                ackedTimer = now + 0.3f;
            }
        }
        else if (now > ackedTimer)
        {
            loadDone = true;
        }
    }
    else
    {
        while (true)
        {
            NetPacket* launch = NetComm::Recv(PKT_LAUNCH, PKT_NONE);
            if (!launch)
            {
                break;
            }
            launch->Free();
        }
        while (true)
        {
            NetPacket* load = NetComm::Recv(PKT_LOAD, PKT_NONE);
            if (!load)
            {
                break;
            }
            NetPktGroup group;
            group.Set(load);
            if (!loadDone)
            {
                hostKnowsILoaded = true;
                group.BeginRead();
                ReadLoad(group, loadDone);
                group.EndRead();
            }
            group.Free();
        }
        if (!loadDone && now > 60.0f)
        {
            loadDone = true;
            NetComm::Disconnect(64);
        }
    }
}

// FUNCTION: SWBF2 0x005e3290
void CNetPostLoadHostGame::SendUpdates()
{
    for (int tries = 0; needSendMask && tries < 4; ++tries)
    {
        int start = netRandom.GetInt() & 0xff;
        for (int j = 0; j < 64; ++j)
        {
            int player = (j + start) % 64;
            if (needSendMask & (1ULL << player))
            {
                if (NetComm::IsConnected(player) && NetGame::GetGuestOf(player) < 0)
                {
                    ShellPlayer& shellPlayer = shellState.players[player];
                    NetPktGroup group;
                    group.BeginWrite();
                    ShellUpdate update;
                    update.postLoad = true;
                    update.moveNum = shellPlayer.moveNum;
                    update.bootMask = 0;
                    if (shellPlayer.quitRequested)
                    {
                        NetGameP::SetPlayerName(player, "");
                        update.quitMask = 1ULL << player;
                    }
                    WriteShellUpdate(group, player, update, shellState);
                    group.EndWrite();
                    NetComm::Send(player, PKT_SHELL_UPDATE, &group);
                }
                needSendMask &= ~(1ULL << player);
                break;
            }
        }
    }
}

// FUNCTION: SWBF2 0x005e2640
void CNetPostLoadHostGame::Update()
{
    int i;
    NetComm::AdvanceTime(true);
    float now = NetComm::GetTime();
    if (timer > now)
    {
        return;
    }
    timer = now + 0.2f;

    while (true)
    {
        NetPacket* join = NetComm::Recv(PKT_JOIN, PKT_NONE);
        if (!join)
        {
            break;
        }
        int from = GetSrc(join);
        if (from < 64)
        {
            if (NetGame::IsPlaying(from, false))
            {
                join->Free();
                join = 0;
                NetGameP::SetNotPlaying(from);
                break;
            }
            join->Free();
            join = 0;
            needSendMask |= 1ULL << from;
        }
        else if (loadGame || GameLoop::IsGameOver())
        {
            NetPktGroup group;
            group.Set(join);
            group.BeginRead();
            unsigned int value;
            group.ReadBits(value, 12);
            if (value != 1)
            {
                group.EndRead();
                join->Free();
                join = 0;
                group.BeginWrite();
                WriteJoinReject(group, JRR_VERSION);
                group.EndWrite();
                NetComm::Send(from, PKT_REJECT, &group);
            }
            else
            {
                group.ReadBits(value, 32);
                if (value != (unsigned int)NetSetup::GetPassword())
                {
                    group.EndRead();
                    join->Free();
                    join = 0;
                    group.BeginWrite();
                    WriteJoinReject(group, JRR_PASSWORD);
                    group.EndWrite();
                    NetComm::Send(from, PKT_REJECT, &group);
                }
                else
                {
                    unsigned int guestCount;
                    group.ReadBits(guestCount, 3);
                    group.ReadBits(value, 2);
                    char name[32];
                    ReadString(group, name, sizeof(name));
                    group.EndRead();
                    join->Free();
                    join = 0;
                    group.BeginWrite();
                    WriteJoinReject(group, JRR_BUSY);
                    group.EndWrite();
                    NetComm::Send(from, PKT_REJECT, &group);
                }
            }
        }
        else if (!BanIP(from, join))
        {
            int player = -1;
            for (i = 0; i < netCurMaxPlayers; ++i)
            {
                if (NetComm::IsConnected(i) && NetCommP::SameAddr(i, from))
                {
                    player = i;
                    break;
                }
            }
            if (player == -1)
            {
                unsigned int j;
                int guests[1];
                for (j = 0; j < 1; ++j)
                {
                    guests[j] = -1;
                }
                int slots = Min(netCurMaxPlayers + 2, netPlayerLimit);
                for (i = 0; i < slots; ++i)
                {
                    if (!NetComm::IsConnected(i) && !NetGame::IsPlaying(i, false))
                    {
                        if (player == -1)
                        {
                            player = i;
                        }
                        else
                        {
                            for (j = 0; j < 1; ++j)
                            {
                                if (guests[j] == -1)
                                {
                                    guests[j] = i;
                                    break;
                                }
                            }
                        }
                    }
                }

                NetPktGroup group;
                group.Set(join);
                group.BeginRead();
                unsigned int version;
                group.ReadBits(version, 12);
                unsigned int password;
                group.ReadBits(password, 32);
                unsigned int guestCount;
                group.ReadBits(guestCount, 3);
                bool hasRoom = player != -1;
                if (!LocalLimits::IsValidGuestCount(guestCount))
                {
                    NetRejectCount(NET_REJECT_GUEST_COUNT);
                    hasRoom = false;
                }
                for (j = 0; hasRoom && j < guestCount; ++j)
                {
                    if (guests[j] == -1)
                    {
                        hasRoom = false;
                    }
                }
                if (hasRoom)
                {
                    if (password != (unsigned int)NetSetup::GetPassword())
                    {
                        group.EndRead();
                        join->Free();
                        join = 0;
                        group.BeginWrite();
                        WriteJoinReject(group, JRR_PASSWORD);
                        group.EndWrite();
                        NetComm::Send(from, PKT_REJECT, &group);
                    }
                    else if (version == 1)
                    {
                        unsigned __int64* address = NetCommP::GetAddr(GetSrc(join));
                        NetCommP::Connect(player, address);
                        UpdateCurMaxPlayers(1ULL << player, true);
                        for (unsigned int k = 0; k < guestCount; ++k)
                        {
                            NetCommP::Connect(guests[k], address);
                            UpdateCurMaxPlayers(1ULL << guests[k], true);
                            NetGameP::SetGuestOf(guests[k], player);
                        }
                        ReadJoin(group, player, guestCount, shellState);
                        group.EndRead();
                        join->Free();
                        join = 0;
                        needSendMask |= 1ULL << player;
                    }
                    else
                    {
                        group.EndRead();
                        join->Free();
                        join = 0;
                        group.BeginWrite();
                        WriteJoinReject(group, JRR_VERSION);
                        group.EndWrite();
                        NetComm::Send(from, PKT_REJECT, &group);
                    }
                }
                else
                {
                    group.EndRead();
                    join->Free();
                    join = 0;
                    group.BeginWrite();
                    WriteJoinReject(group, JRR_FULL);
                    group.EndWrite();
                    NetComm::Send(from, PKT_REJECT, &group);
                }
            }
        }
        if (join)
        {
            join->Free();
        }
    }

    while (true)
    {
        NetPacket* packet = NetComm::Recv(PKT_SHELL_MOVE, PKT_NONE);
        if (!packet)
        {
            break;
        }
        int player = GetSrc(packet);
        NetPktGroup group;
        group.Set(packet);
        group.BeginRead();
        ShellMove move;
        ReadShellMove(group, player, move, shellState);
        group.EndRead();
        packet->Free();
        ShellPlayer& shellPlayer = shellState.players[player];
        group.BeginWrite();
        ShellUpdate update;
        update.postLoad = true;
        update.moveNum = shellPlayer.moveNum;
        update.bootMask = 0;
        if (shellPlayer.quitRequested)
        {
            NetGameP::SetPlayerName(player, "");
            update.quitMask = 1ULL << player;
        }
        WriteShellUpdate(group, player, update, shellState);
        group.EndWrite();
        NetComm::Send(player, PKT_SHELL_UPDATE, &group);
    }

    while (true)
    {
        NetPacket* ack = NetComm::Recv(PKT_LAUNCH_ACK, PKT_NONE);
        if (!ack)
        {
            break;
        }
        int player = GetSrc(ack);
        ack->Free();
        NetPktGroup group;
        group.BeginWrite();
        WriteLaunch(group, true, player, true);
        group.EndWrite();
        NetComm::Send(player, PKT_LAUNCH, group.pktPtr);
        loadingMask |= 1ULL << player;
    }

    while (true)
    {
        NetPacket* ack = NetComm::Recv(PKT_LOAD_ACK, PKT_NONE);
        if (!ack)
        {
            break;
        }
        int player = GetSrc(ack);
        ack->Free();
        loadingMask &= ~(1ULL << player);
        unsigned __int64 mask = 0;
        for (i = 0; i < netCurMaxPlayers; ++i)
        {
            if (NetGame::IsPlayingNext(i))
            {
                mask |= 1ULL << i;
            }
        }
        mask |= 1ULL << player;
        for (unsigned int slot = 0; slot < 1; ++slot)
        {
            int guest = NetGame::GetMyGuest(player, slot);
            if (guest >= 0)
            {
                mask |= 1ULL << guest;
            }
        }
        NetGameP::SetPlaying(mask, true);
        NetPktGroup group;
        group.BeginWrite();
        unsigned __int64 ackedMask = mask;
        WriteLoad(group, player, mask, ackedMask);
        group.EndWrite();
        NetComm::Send(player, PKT_LOAD, group.pktPtr);
    }

    for (i = 0; i < netCurMaxPlayers; ++i)
    {
        if (!NetComm::IsConnected(i))
        {
            continue;
        }
        if (NetGame::IsPlaying(i, false))
        {
            continue;
        }
        float elapsed = NetComm::GetTime() - NetComm::GetLastRecv(i);
        float timeout = 15.0f;
        if (shellState.players[i].quitRequested)
        {
            timeout = 5.0f;
        }
        else if (loadingMask & (1ULL << i))
        {
            timeout = 60.0f;
        }
        if (elapsed > timeout)
        {
            NetGameP::SetPlayerName(i, "");
            NetGame::SetPlayerGSID(i, 0);
            NetGame::ClearPlayerCDKey(i);
            int hostIndex = *NetGameP::GetHostIndex(i);
            if (hostIndex >= 0)
            {
                *NetGameP::GetHostMode(hostIndex) = 3;
            }
            NetComm::Disconnect(i);
            shellState.players[i].Init();
        }
    }

    SendUpdates();
    NetMatch::UpdateLogin(-1);
    NetMatch::UpdateRegisterGame();
    PblValues::Record('c', PVT_FULL);
}

// FUNCTION: SWBF2 0x005db010
NetHostGame::~NetHostGame()
{
}

// FUNCTION: SWBF2 0x005e7170
char* NetSetup::GetGameVersion()
{
    return netGameVersion;
}

// FUNCTION: SWBF2 0x005e7390
bool NetSetup::GetSpecialJoinValid()
{
    return mSpecialJoinValid;
}

// GLOBAL: SWBF2 0x01ead929
bool NetSetup::mSpecialJoinValid;

// FUNCTION: SWBF2 0x005d6360
int GetLastSignInError()
{
    return netLoginFailureReason;
}

// GLOBAL: SWBF2 0x01ee7d50
unsigned __int64 netCreatedLobbyId;

// GLOBAL: SWBF2 0x01e65048
int netLanRegisterState;

// GLOBAL: SWBF2 0x01fa88d0
char const* netLoginErrorMessage;

// GLOBAL: SWBF2 0x01e64f04
int netLoginErrorLevel;

// GLOBAL: SWBF2 0x01e64f2c
int netNoCableCount;

// GLOBAL: SWBF2 0x01e65040
int netLobbyState;

// GLOBAL: SWBF2 0x01ee7d40
unsigned __int64 netEnteredLobbyId;

// GLOBAL: SWBF2 0x01e65034
int netConnectState;

// GLOBAL: SWBF2 0x01e65038
int netLoginFailureReason;

// GLOBAL: SWBF2 0x01e65031
bool netLobbyRequestPending;

// GLOBAL: SWBF2 0x01e65032
bool netLeavingLobby;


// FUNCTION: SWBF2 0x005e7350
const char* GetEncodedLobbyPassword()
{
    return (const char*)netEncodedPassword;
}

// LIBRARY: SWBF2 0x005d4700
// std::error_category::default_error_condition

// LIBRARY: SWBF2 0x005d4720
// std::error_category::equivalent

// LIBRARY: SWBF2 0x005d4760
// std::error_category::equivalent

// LIBRARY: SWBF2 0x005d4780
// std::_Generic_error_category::name

// LIBRARY: SWBF2 0x005d4790
// std::_Generic_error_category::message

// LIBRARY: SWBF2 0x005d4840
// std::_Iostream_error_category::name

// LIBRARY: SWBF2 0x005d4850
// std::_Iostream_error_category::message

// LIBRARY: SWBF2 0x005d48d0
// std::_System_error_category::name

// LIBRARY: SWBF2 0x005d48e0
// std::_System_error_category::message

// LIBRARY: SWBF2 0x005d4960
// std::_System_error_category::default_error_condition

// TEMPLATE: SWBF2 0x005d8d40
// utf8::internal::increase_safely<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >

// TEMPLATE: SWBF2 0x005d8bd0
// utf8::internal::get_sequence_1<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >

// TEMPLATE: SWBF2 0x005d8bf0
// utf8::internal::get_sequence_2<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >

// TEMPLATE: SWBF2 0x005d8c40
// utf8::internal::get_sequence_3<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >

// TEMPLATE: SWBF2 0x005d8cb0
// utf8::internal::get_sequence_4<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >

// TEMPLATE: SWBF2 0x005d8aa0
// ??$validate_next@V?$_String_iterator@V?$_String_val@U?$_Simple_types@D@std@@@std@@@std@@@internal@utf8@@YA?AW4utf_error@01@AAV?$_String_iterator@V?$_String_val@U?$_Simple_types@D@std@@@std@@@std@@V34@AAI@Z

// TEMPLATE: SWBF2 0x005d84e0
// utf8::append<std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >

// TEMPLATE: SWBF2 0x005d8430
// utf8::next<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >

// TEMPLATE: SWBF2 0x005d82e0
// utf8::replace_invalid<std::_String_iterator<std::_String_val<std::_Simple_types<char> > >,std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >

// FUNCTION: SWBF2 0x005d3460
void __fastcall CallEmptySessionElementHook(void*)
{
    sub_42E960();
}

// FUNCTION: SWBF2 0x005e7190
const char* GetNextMissionForLobby()
{
    return GameLoop::GetNextMission();
}

// LIBRARY: SWBF2 0x005d7d30
// std::basic_string<char,std::char_traits<char>,std::allocator<char> >::insert

// LIBRARY: SWBF2 0x005d8a40
// std::back_insert_iterator<std::string>::operator=(const char&)

// FUNCTION: SWBF2 0x005e73d0
void RemoveFoundGameByAddress(const NetAddr* address)
{
    ((CNetFindGame*)findGame)->RemoveGameByAddress(address);
}













// SYNTHETIC: SWBF2 0x004056c0
// PblMath::`dynamic initializer for 'fTwoPi''

// SYNTHETIC: SWBF2 0x004056e0
// PblMath::`dynamic initializer for 'fHalfPi''

// SYNTHETIC: SWBF2 0x00405700
// PblMath::`dynamic initializer for 'fOneOverPi''

// SYNTHETIC: SWBF2 0x00405720
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x00405740
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00405760
// `dynamic initializer for '_IdentityMatrix''

// SYNTHETIC: SWBF2 0x004059b0
// `dynamic initializer for 'autoNetTimeStamp''

// GLOBAL: SWBF2 0x01ef1f90
// autoNetTimeStamp
PblTimeBase::Stamp autoNetTimeStamp;

// SYNTHETIC: SWBF2 0x004059c0
// `dynamic initializer for 'NetSetup::mSpecialJoinAddress''

// GLOBAL: SWBF2 0x01ef254c
// NetSetup::mSpecialJoinAddress
NetSessionId NetSetup::mSpecialJoinAddress;

// SYNTHETIC: SWBF2 0x00405870
// `dynamic initializer for 'TRIGGER_TIMER_SECONDS_PER_NETTICK''

// SYNTHETIC: SWBF2 0x00405890
// `dynamic initializer for 'TRIGGER_TIMER_TICKS_PER_SECOND''

// SYNTHETIC: SWBF2 0x004058b0
// `dynamic initializer for 'TRIGGER_TIMER_SECONDS_PER_TICK''

// SYNTHETIC: SWBF2 0x004058d0
// `dynamic initializer for 'MAX_TAP_DURATION_TICKS''

// SYNTHETIC: SWBF2 0x00405900
// `dynamic initializer for 'MAX_DOUBLETAP_INTERVAL_TICKS''

// GLOBAL: SWBF2 0x01e6505b
// NetSetup::mPrivateGame
bool NetSetup::mPrivateGame = false;

// FUNCTION: SWBF2 0x005e7180
char* NetSetup::GetHostName()
{
    return netGameName;
}
