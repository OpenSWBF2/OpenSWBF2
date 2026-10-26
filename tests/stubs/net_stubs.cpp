#include "Battlefront2/Source/NetComm.h"
#include "Battlefront2/Source/NetGame.h"
#include "Battlefront2/Source/NetSetup.h"
#include "Battlefront2/Source/GameLoop.h"
#include "Battlefront2/Source/pcNetSocket.h"
#include "Steam/galaxy_api.h"
#include "Steam/gfGalaxy.h"
#include "Battlefront2/Source/pcCrash.h"
#include <string.h>
#include "nettest.h"

class NetDrag
{
public:
    static void Send(NetPacket* pPacket, int iDest);
    static void Update();
    static NetPacket* ReceiveLocal();
};

bool netEnabled;
bool netEnabledNext;
bool netInShell;
int netCurMaxPlayers;
int netCurMaxHosts;
int netDirectPort;
int gDebugCount;
int TEMP_bandwith;
unsigned __int64 netEnteredLobbyId;
unsigned int GameLoop::mNumCameras;
NetPktGroup bandwidthQueue[3];
NetPktGroup lossQueue;
NetPktGroup latencyQueue;
NetPktGroup localQueue;

void NetDrag::Send(NetPacket*, int) {}
void NetDrag::Update() {}
NetPacket* NetDrag::ReceiveLocal() { return 0; }
void NetGameP::FreePackets() {}
int NetGame::GetMyGuest(int, int) { return -1; }
unsigned int NetGame::GetKey(int) { return 0; }
void NetSetupP::Recv(NetPacket*, const void*) {}
void NetSetupP::RecvLan(NetPacket* pPacket, const void*) { pPacket->Free(); }
void NetSocket::StartSockets() {}
void NetSocket::RecvSockets() {}
void NetSocket::RecvGalaxySockets() {}
void NetSocket::GetAddr(const void*, int& iIP, int& iPort) { iIP = 0; iPort = 0; }
int __fastcall NetCommP::FindJoinSource(const int*) { return -1; }
bool __fastcall IsAdminPacketAuthorized(const unsigned char*) { return false; }
namespace gf { bool checkError() { return false; } }
namespace galaxy { namespace api { IGalaxy* GalaxyFactory::GetInstance() { return 0; } } }
int gExitCalls;
int gLastExitCode;
namespace pcCrash { void __cdecl Exit(int iCode) { ++gExitCalls; gLastExitCode = iCode; } }
int gCheckCalls;
char gLastCheckExpr[256];
namespace pcCrash { void __cdecl Check(char const* pcExpr, char const*, int) { ++gCheckCalls; strncpy(gLastCheckExpr, pcExpr, sizeof(gLastCheckExpr) - 1); gLastCheckExpr[sizeof(gLastCheckExpr) - 1] = 0; } }
