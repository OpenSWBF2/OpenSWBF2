#pragma once
#include <stddef.h>

struct NetSocketPeerRecord
{

    unsigned __int64 mGalaxyId;
    unsigned char mSocketAddress[16];
};

#pragma pack(push, 4)
struct NetJoinSlot
{
    float mLastTime;
    NetSocketPeerRecord mPeer;
};
#pragma pack(pop)
 
 

class NetSocket
{
public:

    static void StartSockets();

    static void OpenBroadcastSockets();
    static int OpenDirectSocket(bool bHost);

    static void CloseDirectSocket();

    static void GetAddr(const void* pPeer, int& iIP, int& iPort);

    static int SendSocket(int iDest, char* pData, int iLen);
    static int SendVoiceSocket(int iDest, char* pData, int iLen, int iChannel);

    static void RecvSockets();

    static void RecvGalaxySockets();

    static unsigned short __fastcall SetAddr(void* pDest, unsigned int uIP, unsigned short uPort);
};
