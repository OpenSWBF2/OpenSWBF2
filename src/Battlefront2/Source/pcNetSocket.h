#pragma once
#include <stddef.h>
#include <winsock.h>
#include "Steam/galaxy_api.h"

struct NetSocketPeerRecord
{

    galaxy::api::GalaxyID mGalaxyId;
    sockaddr_in mSocketAddress;
};
 
 

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

    static void SetAddr(NetSocketPeerRecord& peer, int ip, int port);
};
