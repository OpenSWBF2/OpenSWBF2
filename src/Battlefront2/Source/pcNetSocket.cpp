#include <winsock2.h>

#include "pcNetSocket.h"
#include <string.h>
#include "NetPacket.h"
#include "NetComm.h"
#include "PebbleFL/Common/PblJournal.h"
#include "Steam/galaxy_api.h"

extern SOCKET netDirectSocket;
extern SOCKET netBroadcastSendSocket;
extern SOCKET netBroadcastRecvSocket;
extern int hostshort;

extern int netSocketErrorCount;

// FUNCTION: SWBF2 0x00618540
void NetSocket::GetAddr(const void* pPeer, int& iIP, int& iPort)
{
    const NetSocketPeerRecord* pRecord = (const NetSocketPeerRecord*)pPeer;
    const sockaddr_in& addr = pRecord->mSocketAddress;
    iIP = (int)addr.sin_addr.s_addr;
    iPort = ntohs(addr.sin_port);
}

// FUNCTION: SWBF2 0x00618560
void NetSocket::SetAddr(NetSocketPeerRecord& peer, int ip, int port)
{
    peer.mSocketAddress.sin_family = AF_INET;
    peer.mSocketAddress.sin_addr.s_addr = ip;
    peer.mSocketAddress.sin_port = htons(port);
}

// FUNCTION: SWBF2 0x00618590
void DecodeError(int iError)
{
    switch (iError)
    {
    case 10022: case 10035: case 10038: case 10048:
    case 10057: case 10065: case 10093:
        ++netSocketErrorCount;
        break;
    }
}

// FUNCTION: SWBF2 0x00617bf0
void NetSocket::StartSockets()
{

    WSADATA wsaData;
    WSAStartup(0x0202, &wsaData);
}

// FUNCTION: SWBF2 0x00617c10
void NetSocket::OpenBroadcastSockets()
{
    sockaddr_in addr;
    netBroadcastSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    BOOL bBroadcast = TRUE;
    setsockopt(netBroadcastSendSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&bBroadcast,
               sizeof(bBroadcast));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port = htons(3656);
    if (connect(netBroadcastSendSocket, (const sockaddr*)&addr, sizeof(addr)) < 0)
    {
        DecodeError(WSAGetLastError());
    }

    netBroadcastRecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    BOOL bReuse = TRUE;
    setsockopt(netBroadcastRecvSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuse,
               sizeof(bReuse));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(3656);
    bind(netBroadcastRecvSocket, (const sockaddr*)&addr, sizeof(addr));
    u_long ulNonBlocking = 1;
    ioctlsocket(netBroadcastRecvSocket, FIONBIO, &ulNonBlocking);
}

// FUNCTION: SWBF2 0x00617d10
void NetSocket::CloseBroadcastSockets()
{
    closesocket((SOCKET)netBroadcastSendSocket);
    netBroadcastSendSocket = INVALID_SOCKET;
    closesocket((SOCKET)netBroadcastRecvSocket);
    netBroadcastRecvSocket = INVALID_SOCKET;
}

// FUNCTION: SWBF2 0x00617d40
int NetSocket::OpenDirectSocket(bool bHost)
{
    netDirectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (netDirectSocket == INVALID_SOCKET)
    {
        DecodeError(WSAGetLastError());
        return 0;
    }
    int iPort = hostshort;
    if (!bHost)
    {
        ++iPort;
    }
    int iResult;
    do
    {
        sockaddr_in addr;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_family = AF_INET;
        addr.sin_port = htons((u_short)iPort);
        iResult = bind(netDirectSocket, (const sockaddr*)&addr, sizeof(addr));
        if (iResult == SOCKET_ERROR)
        {
            DecodeError(WSAGetLastError());
            ++iPort;
        }
    } while (iResult == SOCKET_ERROR);
    int iSize = 0x8000;
    setsockopt(netDirectSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&iSize, sizeof(iSize));
    iSize = 0x10000;
    setsockopt(netDirectSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&iSize, sizeof(iSize));
    u_long ulNonBlocking = 1;
    ioctlsocket(netDirectSocket, FIONBIO, &ulNonBlocking);
    return iPort;
}

// FUNCTION: SWBF2 0x00617e30
void NetSocket::CloseDirectSocket()
{
    closesocket(netDirectSocket);
    netDirectSocket = INVALID_SOCKET;
}

extern unsigned __int64* __fastcall CopyEnteredLobbyId(unsigned __int64* pID);
namespace gf
{
bool isInitialized();
}
extern galaxy::api::INetworking* GetGalaxyNetworking();

// FUNCTION: SWBF2 0x00617e50
int NetSocket::SendSocket(int iDest, char* pData, int iLen)
{
    if (!NetComm::IsLAN())
    {
        const NetSocketPeerRecord* pPeer = (const NetSocketPeerRecord*)NetCommP::GetAddr(iDest);
        if (gf::isInitialized())
        {
            unsigned __int64 lobbyId;
            if (iDest <= 64 && *CopyEnteredLobbyId(&lobbyId))
            {
                galaxy::api::INetworking* pNetworking = GetGalaxyNetworking();
                return pNetworking->SendP2PPacket(
                           pPeer->mGalaxyId, pData, iLen, 2, 0)
                     ? iLen : -1;
            }
            return -1;
        }
        return 0;
    }
    else
    {
        const NetSocketPeerRecord* pPeer = (const NetSocketPeerRecord*)NetCommP::GetAddr(iDest);
        int n = iDest > 64 && iDest == 68
              ? send((SOCKET)netBroadcastSendSocket, pData, iLen, 0)
              : sendto((SOCKET)netDirectSocket, pData, iLen, 0,
                       (const sockaddr*)&pPeer->mSocketAddress, sizeof(sockaddr_in));
        if (n == SOCKET_ERROR)
        {
            DecodeError(WSAGetLastError());
        }
        return n;
    }
}

// FUNCTION: SWBF2 0x00617f40
int NetSocket::SendVoiceSocket(int iDest, char* pData, int iLen, int iChannel)
{
    return SendSocket(iDest, pData, iLen);
}

extern bool netIsHost;
extern int netHostIndex;

// GLOBAL: SWBF2 0x01fab0d0
unsigned char netGalaxyReceiveBuffer[2048];

// GLOBAL: SWBF2 0x01fab8e0
unsigned char netLanReceiveBuffer[2048];

static void ProcessReceivedPacket(const unsigned char* pData, unsigned int size,
                                   unsigned int prefix, bool classifyByType,
                                   const NetSocketPeerRecord& peer)
{
    NetPacket* pPacket = NetCommP::MakeReceivedPacket(pData, size, prefix, classifyByType);
    if (pPacket)
    {
        NetCommP::RecvPacket(pPacket, &peer);
    }
}

struct ReceivePacketPrefix
{
    int value;
    ReceivePacketPrefix() : value(3) {}
};

// FUNCTION: SWBF2 0x00617f50
void NetSocket::RecvSockets()
{

    // GLOBAL: SWBF2 0x01fab8dc
    static ReceivePacketPrefix packetPrefix;

    sockaddr_in from;
    int fromLength;
    int remaining = 80;
    int received = SOCKET_ERROR;
    do
    {
        fromLength = sizeof(from);
        --remaining;
        received = recvfrom(netDirectSocket, (char*)netLanReceiveBuffer,
                            sizeof(netLanReceiveBuffer), 0, (sockaddr*)&from, &fromLength);
        if (received == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
            {
                DecodeError(error);
            }
            break;
        }
        JournalBytes(reinterpret_cast<char*>(netLanReceiveBuffer), received);
        JournalBytes(reinterpret_cast<char*>(&from), sizeof(from));
        NetSocketPeerRecord peer;
        peer.mSocketAddress = from;
        ProcessReceivedPacket(netLanReceiveBuffer, received, packetPrefix.value, true, peer);
    }
    while (remaining);
    if (!remaining)
    {
        do
        {
            fromLength = sizeof(from);
            received = recvfrom(netDirectSocket, (char*)netLanReceiveBuffer,
                                sizeof(netLanReceiveBuffer), 0, (sockaddr*)&from, &fromLength);
            if (received != SOCKET_ERROR)
            {
                JournalBytes(reinterpret_cast<char*>(netLanReceiveBuffer), 1);
                JournalBytes(reinterpret_cast<char*>(&from), sizeof(from));
            }
        }
        while (received != SOCKET_ERROR);
    }

    if (netBroadcastRecvSocket != INVALID_SOCKET)
    {
        remaining = 64;
        do
        {
            fromLength = sizeof(from);
            --remaining;
            received = recvfrom(netBroadcastRecvSocket, (char*)netLanReceiveBuffer,
                                1012 - packetPrefix.value, 0, (sockaddr*)&from, &fromLength);
            if (received == SOCKET_ERROR)
            {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK)
                {
                    DecodeError(error);
                }
                break;
            }
            JournalBytes(reinterpret_cast<char*>(netLanReceiveBuffer), received);
            JournalBytes(reinterpret_cast<char*>(&from), sizeof(from));
            NetSocketPeerRecord peer;
            peer.mSocketAddress = from;
            ProcessReceivedPacket(netLanReceiveBuffer, received, packetPrefix.value, false, peer);
        }
        while (remaining);
        if (!remaining)
        {
            do
            {
                fromLength = sizeof(from);
                received = recvfrom(netBroadcastRecvSocket, (char*)netLanReceiveBuffer,
                                    sizeof(netLanReceiveBuffer), 0, (sockaddr*)&from, &fromLength);
            }
            while (received != SOCKET_ERROR);
        }
    }
}

extern galaxy::api::INetworking* GetGalaxyNetworking();
extern galaxy::api::INetworking* GetGalaxyHostNetworking();

static bool PollGalaxy(galaxy::api::INetworking* pNetworking, unsigned int& size)
{
    return pNetworking->IsP2PPacketAvailable(&size, 0);
}

static bool ReadGalaxy(galaxy::api::INetworking* pNetworking, unsigned int capacity,
                       unsigned int& size, NetSocketPeerRecord& peer)
{
    return pNetworking->ReadP2PPacket(netGalaxyReceiveBuffer, capacity, &size, peer.mGalaxyId, 0);
}

static void DiscardGalaxy(galaxy::api::INetworking* pNetworking)
{
    pNetworking->PopP2PPacket(0);
}

// FUNCTION: SWBF2 0x00618250
void NetSocket::RecvGalaxySockets()
{

    // GLOBAL: SWBF2 0x01fab8d0
    static ReceivePacketPrefix packetPrefix;

    unsigned int size = 0;
    for (int count = 0; ; )
    {
        galaxy::api::INetworking* pNetworking = GetGalaxyNetworking();
        if (!PollGalaxy(pNetworking, size))
        {
            break;
        }
        if (++count >= 80 || size > 2048)
        {
            DiscardGalaxy(GetGalaxyNetworking());
            continue;
        }
        NetSocketPeerRecord peer = { galaxy::api::GalaxyID(0) };
        if (ReadGalaxy(GetGalaxyNetworking(), 2048, size, peer))
        {
            JournalBytes(reinterpret_cast<char*>(netGalaxyReceiveBuffer), size);
            JournalBytes(reinterpret_cast<char*>(&peer), sizeof(peer));
            ProcessReceivedPacket(netGalaxyReceiveBuffer, size, packetPrefix.value, true, peer);
        }
    }

    if (netIsHost || netHostIndex >= 0)
    {
        for (int count = 0; ; )
        {
            galaxy::api::INetworking* pNetworking = GetGalaxyHostNetworking();
            if (!PollGalaxy(pNetworking, size))
            {
                break;
            }
            if (++count >= 64 || size > 2048)
            {
                DiscardGalaxy(GetGalaxyHostNetworking());
                continue;
            }
            NetSocketPeerRecord peer = { galaxy::api::GalaxyID(0) };
            if (ReadGalaxy(GetGalaxyHostNetworking(), 1012 - packetPrefix.value, size, peer))
            {
                JournalBytes(reinterpret_cast<char*>(netGalaxyReceiveBuffer), size);
                JournalBytes(reinterpret_cast<char*>(&peer), sizeof(peer));
                ProcessReceivedPacket(netGalaxyReceiveBuffer, size, packetPrefix.value, false, peer);
            }
        }
    }
}
