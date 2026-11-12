#pragma once

#include "Steam/galaxy_api.h"

struct NetSocketPeerRecord;
struct FindGamePlayer;
enum FindType;
class NetPacket;

struct NetKey
{
    unsigned int id;
    unsigned int key;
};

// SYNTHETIC: SWBF2 0x005d7620
// NetAddr::operator=
#pragma pack(push, 4)
struct NetAddr
{
    galaxy::api::GalaxyID mGalaxyId;
    bool hasPriv;
    int privAddr;
    int privPort;

    // FUNCTION: SWBF2 0x005d46a0
    NetAddr() : hasPriv(false), privAddr(0), privPort(0) {}

    NetAddr(int addr, int port, int overridePort);

    bool operator==(const NetAddr& other)
    {
        if (hasPriv)
        {
            if (privAddr != other.privAddr)
            {
                return false;
            }
            if (privPort != other.privPort)
            {
                return false;
            }
        }
        return other.mGalaxyId == mGalaxyId;
    }



    void GetAddr(NetSocketPeerRecord& out, NetKey* key) const;
};
#pragma pack(pop)

bool __fastcall IsGalaxyIdValid(const unsigned __int64* pThis);

struct NetSessionId
{

    // FUNCTION: SWBF2 0x00591430
    NetSessionId() : id(0) {}

    NetSessionId& operator=(const NetSessionId& other);

    unsigned __int64 id;

    // FUNCTION: SWBF2 0x00591440
    void SetNULL() { id = 0; }

    bool IsNULL() { return !IsGalaxyIdValid(&id); }
};

class NetMatch
{
public:

    static char* GetIPAddr();

    static void SetNoCableError();

    static void ClearGameError();

    static int GetPercentDone();

    static void __fastcall EndFindGames(bool bCancel);

    static void StartRegisterGame(bool bMatchService);

    static void StartLogin(int iViewport, bool bArg2, char const* pcArg3, char const* pcArg4,
                           char const* pcArg5, bool bArg6);

    static void StartJoinGame(NetAddr& address, NetKey& key);

    static void EndJoinGame();

    static bool StartConnect(NetAddr& address, NetKey& key, int destination);

    static bool UpdateConnect(NetAddr& address, NetKey& key, int destination, int& connected);

    static void EndConnect(NetAddr& address, NetKey& key, int destination);

    static bool IsDoneFindGames();

    static void UpdateFindGames();

    static bool PingServerByAddress(const NetAddr& address);

    static bool GetPlayerDataByAddress(const NetAddr& address, int player, FindGamePlayer& data);

    static void HostGetAddr(NetAddr& address, NetAddr& output, int destination);

    static void GetSessionId(NetSessionId& session);

    static void SetLocalGSID(unsigned __int64 id);

    static unsigned __int64 GetLocalGSID();

    static void __fastcall ReceiveLanRegisterPacket(NetPacket* pPacket, const void* pSource);

public:

    static void EndRegisterGame();
    static void SetGameError(const char* message, int severity);
    static void SetRegisterArchFilter();

    static char const* GetLoginOverrideName(int iViewport);
    static bool UpdateLogin(int iViewport);
    static void UpdateRegisterGame();
    static bool IsLoginSuccessful(int iViewport, bool bArg);
    static bool UpdatePresence(bool bArg);
    static void GetLoginError(char const*& pcError, int& iSeverity);
    static void CancelLogin(int iViewport, bool bArg);
    static void GetGameError(char const*& pcError, int& iSeverity);

    static void ChangeRegisterGame();
    static void __cdecl ReceiveRegisterPacket(const char* pData, int iLen, const void* pSource);
    static void StartFindGames(bool useMatch, FindType type, const char* address, NetSessionId* id);
    static void PauseFindGames();
    static void UpdateJoinGame();
    static void HandleLoginError(bool);
    static void StartFindAddress();
    static void EndFindAddress();
    static bool UpdateFindAddress();
    static void ClientGetAddr(NetAddr& address);
    static void SetAddr(NetAddr& address, const NetSocketPeerRecord& peer);
};

unsigned long GetPrivateIP();
void __fastcall HandleFindPacket(NetPacket* pPacket);
void __fastcall HandleFoundPacket(NetPacket* pPacket, const void* pSource);
void UpdateFindLanGames();

bool IsSameNet(NetAddr& left, NetAddr& right);

class NetPktGroup;
void ReadSessionId(NetPktGroup& group, NetSessionId& session);

void WriteAddr(NetPktGroup& group, NetAddr& address);

void ReadAddr(NetPktGroup& group, NetAddr& address);

void WriteSessionId(NetPktGroup& group, NetSessionId& session);
