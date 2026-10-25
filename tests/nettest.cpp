#include "third_party/doctest/doctest.h"

#include <string.h>

#include "nettest.h"
#include "Battlefront2/Source/pcNetSocket.h"

extern NetPktGroup inQueue;
extern NetPktGroup inPktGroups[64];
extern NetDest netDests[MAX_DESTS];

void NetTestReset()
{
    NetComm::Create(false);
    NetComm::SetConnectType(CT_LAN);
}

int NetCountPackets(int iHeld)
{
    int iCount = iHeld + freeBigPackets.pktCount + freeSmallPackets.pktCount + inQueue.pktCount;
    for (int i = 0; i < 64; ++i)
    {
        iCount += inPktGroups[i].pktCount;
    }
    for (int i = 0; i < MAX_DESTS; ++i)
    {
        iCount += netDests[i].outQueue.pktCount;
    }
    return iCount;
}

NetPacket* NetMakePacket(const unsigned char* pPayload, int iLen)
{
    NetPacket* pPacket = NetPacket::Alloc(iLen <= 100);
    REQUIRE(pPacket);
    pPacket->BeginWrite();
    if (iLen > 0)
    {
        pPacket->Write((const char*)pPayload, iLen);
    }
    pPacket->EndWrite();
    return pPacket;
}

void NetStampAndQueue(NetPacket* pPacket, unsigned short uGroup, unsigned char uType, unsigned char uSrc,
                      unsigned char uIdx, unsigned char uCount, unsigned char uCompressed)
{
    NetComm::PktHeader& header = NetComm::Header(pPacket);
    header.group = uGroup;
    header.type = uType;
    header.src = uSrc;
    header.idx = uIdx;
    header.count = uCount;
    header.compressed = uCompressed;
    inQueue.PushBack(pPacket);
}

void NetDeliverDatagram(const unsigned char* pData, unsigned int uiSize, unsigned int uiPrefix)
{
    if (uiSize > 1012 - uiPrefix)
    {
        return;
    }
    NetPacket* pPacket = NetPacket::Alloc(uiSize <= 116 - uiPrefix);
    pPacket->BeginWrite();
    memcpy(pPacket->mData + uiPrefix, pData, uiSize);
    pPacket->mCommIndex = (char)uiPrefix;
    pPacket->mMaxIndex = (short)(uiSize + uiPrefix);
    NetSocketPeerRecord peer;
    memset(&peer, 0, sizeof(peer));
    NetCommP::RecvPacket(pPacket, &peer);
}
