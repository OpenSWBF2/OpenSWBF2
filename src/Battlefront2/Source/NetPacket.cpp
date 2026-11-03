#include <string.h>

#include "NetPacket.h"
#include "NetComm.h"
#include "NetGame.h"
#include "pcCrash.h"
#include "RedEngineFL/Util/RedWarning.h"
#include "RedEngineFL/Util/RedLog.h"

class NetDrag
{
public:
    static void Send(NetPacket* pPacket, int iDest);
    static void FreePackets();
};

extern NetPktGroup bandwidthQueue[3];
extern NetPktGroup lossQueue;
extern NetPktGroup latencyQueue;
extern NetPktGroup localQueue;

// FUNCTION: SWBF2 0x0053e0f0
void NetPktGroup::Write(const float& fValue)
{
    WriteBits(*(const unsigned int*)&fValue, 32);
}

// FUNCTION: SWBF2 0x0053e110
void NetPktGroup::Read(float& fValue)
{
    ReadBits(*(unsigned int*)&fValue, 32);
}

// FUNCTION: SWBF2 0x0053e130
void NetPktGroup::Write(int iValue)
{
    WriteBits((unsigned int)iValue, 32);
}

// FUNCTION: SWBF2 0x0053e150
void NetPktGroup::Read(int& iValue)
{
    ReadBits(*(unsigned int*)&iValue, 32);
}

// GLOBAL: SWBF2 0x01e65160
NetBigPacket netBigPackets[MAX_BIG_PACKETS];
// GLOBAL: SWBF2 0x01ea5160
NetSmallPacket netSmallPackets[MAX_SMALL_PACKETS];

// GLOBAL: SWBF2 0x01ee7d58
NetPktGroup freeBigPackets;
// GLOBAL: SWBF2 0x01ee7d74
NetPktGroup freeSmallPackets;

// GLOBAL: SWBF2 0x01e65054
unsigned int NetPacket::sNextSeqNo;

inline void SetCompressed(NetPacket* packet, bool compressed)
{
    NetComm::Header(packet).compressed = compressed;
}

inline int GetNumSmall(const NetPacket* packet)
{
    int index = (int)(static_cast<const NetSmallPacket*>(packet) - netSmallPackets);
    if (index >= 0 && index < MAX_SMALL_PACKETS)
    {
        return index;
    }
    return -1;
}

// FUNCTION: SWBF2 0x005d8df0
NetPacket* NetPacket::Alloc(bool bSmall)
{
    NetPktGroup& freePackets = bSmall ? freeSmallPackets : freeBigPackets;
    NetPacket* pPacket = freePackets.PopFront();
    if (!pPacket)
    {
        NetDrag::FreePackets();
        NetGameP::FreePackets();
        NetCommP::FreePackets();
        pPacket = freePackets.PopFront();
        if (!pPacket)
        {
            for (int i = 0; i < _countof(netBigPackets); ++i)
            {
                freeBigPackets.PushBack(&netBigPackets[i]);
            }
            for (int i = 0; i < _countof(netSmallPackets); ++i)
            {
                freeSmallPackets.PushBack(&netSmallPackets[i]);
            }
            pPacket = freePackets.PopFront();
        }
    }
    PblAsanUnpoison(pPacket, bSmall ? sizeof(NetSmallPacket) : sizeof(NetBigPacket));
    pPacket->mSeqNo = (unsigned short)sNextSeqNo++;
    pPacket->mCurIndex = 2;
    return pPacket;
}

NetPacket* NetPacket::AllocRecv(bool bSmall)
{
    NetPktGroup& freePackets = bSmall ? freeSmallPackets : freeBigPackets;
    if (freePackets.pktCount <= (bSmall ? kRecvReserveSmall : kRecvReserveBig))
    {
        NetRejectCount(NET_REJECT_POOL_EXHAUSTED);
        return 0;
    }
    NetPacket* pPacket = freePackets.PopFront();
    PblAsanUnpoison(pPacket, bSmall ? sizeof(NetSmallPacket) : sizeof(NetBigPacket));
    pPacket->mSeqNo = (unsigned short)sNextSeqNo++;
    pPacket->mCurIndex = 2;
    return pPacket;
}

// FUNCTION: SWBF2 0x005d8f60
void NetPacket::BeginWrite()
{
    const int iSmall = GetNumSmall(this);
    unsigned int* headerWords = (unsigned int*)mData;
    headerWords[0] = 0;
    headerWords[1] = 0;
    SetCompressed(this, false);
    NetComm::Header(this).group = 0xffff;
    mDataIndex = 14;
    mMaxIndex = mDataIndex + (iSmall >= 0 ? 102 : 998);
    mCurIndex = mDataIndex;
}

// FUNCTION: SWBF2 0x005d8d80
void NetPacket::Create()
{
    freeBigPackets.Reset();
    freeSmallPackets.Reset();

    for (unsigned int i = 0; i < MAX_BIG_PACKETS; i++)
    {
        freeBigPackets.PushBack(NetPacket::Get(i));
    }
    for (unsigned int i = MAX_BIG_PACKETS; i < MAX_BIG_PACKETS + MAX_SMALL_PACKETS; i++)
    {
        freeSmallPackets.PushBack(NetPacket::Get(i));
    }
    PblAsanPoison(netBigPackets, sizeof(netBigPackets));
    PblAsanPoison(netSmallPackets, sizeof(netSmallPackets));
}

// FUNCTION: SWBF2 0x005d8ed0
int NetPacket::GetNum() const
{
    const int iSmall = (int)((NetSmallPacket const*)this - netSmallPackets);
    if ((unsigned int)iSmall <= (unsigned int)(MAX_SMALL_PACKETS - 1))
    {
        return iSmall + MAX_BIG_PACKETS;
    }
    return (int)((NetBigPacket const*)this - netBigPackets);
}

// FUNCTION: SWBF2 0x005d8f00
NetPacket* NetPacket::Get(int iNum)
{
    if (iNum >= 0)
    {
        if (iNum < MAX_BIG_PACKETS)
        {
            return &netBigPackets[iNum];
        }
        if (iNum < MAX_BIG_PACKETS + MAX_SMALL_PACKETS)
        {
            return &netSmallPackets[iNum - MAX_BIG_PACKETS];
        }
    }
    return 0;
}

// FUNCTION: SWBF2 0x005d91e0
void NetPktGroup::Reset()
{
    bad = false;
    pktCount = 0;
    lastPkt = -1;
    firstPkt = -1;
}

// FUNCTION: SWBF2 0x005d93c0
PBL_ASAN_NO_SANITIZE void NetPktGroup::PushBack(NetPacket* pPkt)
{
    const int iNum = pPkt->GetNum();
    pPkt->mPrevPkt = (short)lastPkt;
    if (pktCount == 0)
    {
        firstPkt = iNum;
    }
    else
    {
        NetPacket::Get(lastPkt)->mNextPkt = (short)iNum;
    }
    lastPkt = iNum;
    pPkt->mNextPkt = -1;
    pktCount++;
}

// GLOBAL: SWBF2 0x01e65058
bool doHiRes;

// GLOBAL: SWBF2 0x01e6505c
unsigned int NetPktGroup::sDebugBitCounter;

// FUNCTION: SWBF2 0x005d8d70
void NetPacket::SetHiRes(bool bHiRes)
{
    doHiRes = bHiRes;
}

// FUNCTION: SWBF2 0x005d8fc0
void NetPacket::EndWrite()
{
    mMaxIndex = mCurIndex;
}

// FUNCTION: SWBF2 0x005d8fd0
void NetPacket::BeginRead()
{
    mCurIndex = mDataIndex;
}

// FUNCTION: SWBF2 0x005d90c0
bool NetPacket::Write(unsigned int const& uValue)
{
    if (mCurIndex + 4 > mMaxIndex)
    {
        return false;
    }
    char* pDst = &mData[mCurIndex];
    const char* pSrc = (const char*)&uValue;
    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
    pDst[3] = pSrc[3];
    mCurIndex += 4;
    return true;
}

// FUNCTION: SWBF2 0x005d9110
bool NetPacket::Read(unsigned int& uValue)
{
    if (mCurIndex + 4 > mMaxIndex)
    {
        return false;
    }
    const char* pSrc = &mData[mCurIndex];
    char* pDst = (char*)&uValue;
    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
    pDst[3] = pSrc[3];
    mCurIndex += 4;
    return true;
}

// FUNCTION: SWBF2 0x005d9160
int NetPacket::Write(char const* pData, int iLen)
{
    int iEnd = mCurIndex + iLen;
    if (iEnd > mMaxIndex)
    {
        iEnd = mMaxIndex;
    }
    const int iCount = iEnd - mCurIndex;
    if (iCount > 0)
    {
        memcpy(&mData[mCurIndex], pData, iCount);
    }
    mCurIndex += (short)iCount;
    return iCount;
}

// FUNCTION: SWBF2 0x005d91a0
int NetPacket::Read(char* pData, int iLen)
{
    if (iLen <= 0)
    {
        if (iLen < 0)
        {
            NetRejectCount(NET_REJECT_NEGATIVE_LENGTH);
        }
        return 0;
    }
    const int iAvail = mMaxIndex - mCurIndex;
    if (iAvail <= 0)
    {
        return 0;
    }
    const int iCount = iLen < iAvail ? iLen : iAvail;
    memcpy(pData, &mData[mCurIndex], iCount);
    mCurIndex += (short)iCount;
    return iCount;
}

// FUNCTION: SWBF2 0x005d95d0
void NetPktGroup::WritePacked()
{
    if (!pktPtr)
    {
        pktPtr = NetPacket::Alloc(isSmall);
        PushBack(pktPtr);
        pktPtr->BeginWrite();
    }
    if (!pktPtr->Write(pktBits))
    {
        pktPtr->EndWrite();
        pktPtr = NetPacket::Alloc(isSmall);
        PushBack(pktPtr);
        pktPtr->BeginWrite();
        pktPtr->Write(pktBits);
    }
}

// FUNCTION: SWBF2 0x005d9640
void NetPktGroup::ReadPacked()
{
    if (bad)
    {
        pktBits = 0;
        return;
    }
    if (pktPtr && pktPtr->Read(pktBits))
    {
        return;
    }
    pktPtr = pktPtr ? NetPacket::Get(pktPtr->mNextPkt) : 0;
    if (pktPtr)
    {
        pktPtr->BeginRead();
        pktPtr->Read(pktBits);
    }
    else
    {
        pktBits = 0;
    }
}

// FUNCTION: SWBF2 0x005d9690
void NetPktGroup::WriteBits(unsigned int uValue, int iBits)
{
    sDebugBitCounter += iBits;
    for (;;)
    {
        int iCount = 32 - curBit;
        if (iCount > iBits)
        {
            iCount = iBits;
        }
        const unsigned int uPart = (iCount == 32) ? uValue : uValue & ((1 << iCount) - 1);
        pktBits |= uPart << curBit;
        curBit += iCount;
        if (curBit == 32)
        {
            WritePacked();
            curBit = 0;
            pktBits = 0;
        }
        iBits -= iCount;
        if (iBits == 0)
        {
            break;
        }
        uValue >>= iCount;
    }
}

// FUNCTION: SWBF2 0x005d9710
void NetPktGroup::ReadBits(unsigned int& uValue, int iBits)
{
    uValue = 0;
    if (bad)
    {
        return;
    }
    int iShift = 0;
    for (;;)
    {
        if (curBit == 32)
        {
            ReadPacked();
            curBit = 0;
            if (!pktPtr && iBits > 0)
            {
                SetBad(NET_REJECT_GROUP_BAD);
                uValue = 0;
                return;
            }
        }
        int iCount = 32 - curBit;
        if (iCount > iBits)
        {
            iCount = iBits;
        }
        const unsigned int uMask = (iCount == 32) ? 0xffffffff : (1 << iCount) - 1;
        const unsigned int uPart = pktBits >> curBit;
        curBit += iCount;
        uValue |= (uPart & uMask) << iShift;
        iBits -= iCount;
        if (iBits == 0)
        {
            break;
        }
        iShift += iCount;
    }
}

// FUNCTION: SWBF2 0x005d9790
void NetPktGroup::WriteFixed(float f, float fMin, float fMax, int iBits)
{
    if (doHiRes)
    {
        WriteBits(*(const unsigned int*)&f, 32);
        return;
    }
    if (iBits >= 32)
    {
        WriteBits(*(const unsigned int*)&f, 32);
        return;
    }
    unsigned int uValue;
    if (f <= fMin)
    {
        uValue = 0;
    }
    else if (f >= fMax)
    {
        uValue = 0xffffffff;
    }
    else
    {
        uValue = (unsigned int)((float)(unsigned int)((1 << iBits) - 2) * ((f - fMin) / (fMax - fMin)) + 0.5f);
    }
    WriteBits(uValue, iBits);
}

// FUNCTION: SWBF2 0x005d9860
void NetPktGroup::ReadFixed(float& f, float fMin, float fMax, int iBits)
{
    if (doHiRes)
    {
        ReadBits(*(unsigned int*)&f, 32);
        return;
    }
    if (iBits >= 32)
    {
        ReadBits(*(unsigned int*)&f, 32);
        return;
    }
    unsigned int uValue;
    ReadBits(uValue, iBits);
    if (uValue == 0)
    {
        f = fMin;
        return;
    }
    if (uValue == (unsigned int)((1 << iBits) - 1))
    {
        f = fMax;
        return;
    }
    f = (float)uValue / (float)(unsigned int)((1 << iBits) - 2) *
        (fMax - fMin) + fMin;
}

// FUNCTION: SWBF2 0x005d9920
void NetPktGroupClampFixed(float& f, float fMin, float fMax, int iBits)
{
    if (iBits >= 32)
    {
        return;
    }
    unsigned int uValue;
    if (f <= fMin)
    {
        uValue = 0;
    }
    else if (f >= fMax)
    {
        uValue = 0xffffffff;
    }
    else
    {
        uValue = (unsigned int)((f - fMin) / (fMax - fMin) * (float)(unsigned int)((1 << iBits) - 2) + 0.5f);
    }
    const unsigned int uMax = (1 << iBits) - 1;
    uValue &= uMax;
    if (uValue == 0)
    {
        f = fMin;
    }
    else if (uValue == uMax)
    {
        f = fMax;
    }
    else
    {
        f = (float)uValue / (float)(unsigned int)((1 << iBits) - 2) * (fMax - fMin) + fMin;
    }
}

// FUNCTION: SWBF2 0x005d9a30
void NetPktGroup::WriteBytes(char const* pData, int iLen)
{
    if (!pktPtr)
    {
        pktPtr = NetPacket::Alloc(isSmall);
        PushBack(pktPtr);
        pktPtr->BeginWrite();
    }
    if (curBit > 0)
    {
        WriteBits(0, 32 - curBit);
    }
    int iCount = pktPtr->Write(pData, iLen);
    sDebugBitCounter += iCount * 8;
    while (iCount != iLen)
    {
        pData += iCount;
        iLen -= iCount;
        pktPtr->EndWrite();
        pktPtr = NetPacket::Alloc(isSmall);
        PushBack(pktPtr);
        pktPtr->BeginWrite();
        iCount = pktPtr->Write(pData, iLen);
        sDebugBitCounter += iCount * 8;
    }
}

// FUNCTION: SWBF2 0x005d9ae0
void NetPktGroup::ReadBytes(char* pData, int iLen)
{
    if (iLen < 0)
    {
        SetBad(NET_REJECT_NEGATIVE_LENGTH);
        return;
    }
    if (bad)
    {
        if (iLen > 0)
        {
            memset(pData, 0, iLen);
        }
        return;
    }
    if (curBit > 0)
    {
        curBit = 32;
        pktBits = 0;
    }
    while (pktPtr)
    {
        const int iCount = pktPtr->Read(pData, iLen);
        if (iCount == iLen)
        {
            return;
        }
        pData += iCount;
        iLen -= iCount;
        pktPtr = NetPacket::Get(pktPtr->mNextPkt);
        if (!pktPtr)
        {
            break;
        }
        pktPtr->BeginRead();
    }
    if (iLen > 0)
    {
        memset(pData, 0, iLen);
        SetBad(NET_REJECT_GROUP_BAD);
    }
}

// FUNCTION: SWBF2 0x005d8e90
void NetPacket::Free()
{
    mCurIndex = 0;
    NetPktGroup& freePackets = GetNumSmall(this) >= 0 ? freeSmallPackets : freeBigPackets;
    freePackets.PushFront(this);
}

// FUNCTION: SWBF2 0x005d8f40
NetPacket* NetPacket::Encrypt(int) { return this; }

// FUNCTION: SWBF2 0x005d8fe0
bool NetPacket::Write(unsigned char value)
{
    if (mCurIndex + 1 > mMaxIndex)
    {
        return false;
    }
    mData[mCurIndex++] = value;
    return true;
}

// FUNCTION: SWBF2 0x005d9010
bool NetPacket::Read(unsigned char& value)
{
    int iByte = mCurIndex;
    if (iByte + 1 > mMaxIndex)
    {
        return false;
    }
    value = mData[iByte];
    mCurIndex += 1;
    return true;
}

// FUNCTION: SWBF2 0x005d9040
bool NetPacket::Write(unsigned short value)
{
    const int iByte = mCurIndex;
    if (iByte + 2 > mMaxIndex)
    {
        return false;
    }
    const char* pSrc = (const char*)&value;
    mData[iByte] = pSrc[0];
    mData[iByte + 1] = pSrc[1];
    mCurIndex += 2;
    return true;
}

// FUNCTION: SWBF2 0x005d9200
void NetPktGroup::Set(NetPacket* pPkt)
{
    const int iFirst = pPkt->GetNum();
    pktCount = 1;
    firstPkt = iFirst;
    int last = iFirst;
    while (pPkt->mNextPkt != -1)
    {
        ++pktCount;
        last = pPkt->mNextPkt;
        pPkt = NetPacket::Get(last);
    }
    lastPkt = last;
}

// FUNCTION: SWBF2 0x005d92a0
bool NetPktGroup::IsEnd() const
{
    if (!pktPtr)
    {
        return true;
    }
    if (pktPtr->GetNum() != lastPkt)
    {
        return false;
    }
    return pktPtr->mCurIndex >= pktPtr->mMaxIndex;
}

// FUNCTION: SWBF2 0x005d92d0
void NetPktGroup::BeginWrite()
{
    bad = false;
    pktCount = 0;
    lastPkt = -1;
    firstPkt = -1;
    pktPtr = 0;
    curBit = 0;
    pktBits = 0;
}

// FUNCTION: SWBF2 0x005d9300
void NetPktGroup::EndWrite()
{
    WritePacked();
    if (pktPtr)
    {
        pktPtr->EndWrite();
    }
}

// FUNCTION: SWBF2 0x005d9320
void NetPktGroup::BeginRead()
{
    bad = false;
    pktBits = 0;
    curBit = 32;
    pktPtr = NetPacket::Get(firstPkt);
    if (!pktPtr)
    {
        SetBad(NET_REJECT_GROUP_BAD);
        return;
    }
    pktPtr->BeginRead();
    if (pktPtr->mDataIndex + 4 > pktPtr->mMaxIndex)
    {
        SetBad(NET_REJECT_GROUP_BAD);
    }
}

// FUNCTION: SWBF2 0x005d9350
void NetPktGroup::EndRead() {}

// FUNCTION: SWBF2 0x005d9360
PBL_ASAN_NO_SANITIZE void NetPktGroup::PushFront(NetPacket* pPkt)
{
    int num = pPkt->GetNum();
    pPkt->mNextPkt = (short)firstPkt;
    if (pktCount == 0)
    {
        lastPkt = num;
    }
    else
    {
        NetPacket::Get(firstPkt)->mPrevPkt = (short)num;
    }
    firstPkt = num;
    pPkt->mPrevPkt = -1;
    ++pktCount;
}

// FUNCTION: SWBF2 0x005d9480
NetPacket* NetPktGroup::PopBack()
{
    if (!pktCount)
    {
        return 0;
    }
    NetPacket* pPkt = NetPacket::Get(lastPkt);
    lastPkt = pPkt->mPrevPkt;
    --pktCount;
    if (!pktCount)
    {
        firstPkt = -1;
    }
    else
    {
        NetPacket::Get(lastPkt)->mNextPkt = -1;
    }
    pPkt->mPrevPkt = -1;
    pPkt->mNextPkt = -1;
    return pPkt;
}

// FUNCTION: SWBF2 0x005d9500
void NetPktGroup::Remove(NetPacket* pPkt)
{
    if (pPkt->mPrevPkt == -1)
    {
        PopFront();
        return;
    }
    if (pPkt->mNextPkt == -1)
    {
        PopBack();
        return;
    }
    NetPacket* next = NetPacket::Get(pPkt->mNextPkt);
    NetPacket* prev = NetPacket::Get(pPkt->mPrevPkt);
    prev->mNextPkt = pPkt->mNextPkt;
    next->mPrevPkt = pPkt->mPrevPkt;
    --pktCount;
    pPkt->mNextPkt = pPkt->mPrevPkt = -1;
}

// FUNCTION: SWBF2 0x005d9b40
void NetPktGroup::Tell(NetPktGroupOffset& offset)
{
    offset.pktNum = 0;
    offset.bitNum = curBit;
    if (!pktPtr)
    {
        offset.byteNum = 0;
        return;
    }
    for (NetPacket* pPkt = NetPacket::Get(pktPtr->mPrevPkt); pPkt;
         pPkt = NetPacket::Get(pPkt->mPrevPkt))
    {
        ++offset.pktNum;
    }
    offset.byteNum = pktPtr->mCurIndex - pktPtr->mDataIndex;
}

// FUNCTION: SWBF2 0x005d9280
int NetPktGroup::GetBytes()
{
    if (!pktPtr)
    {
        return 0;
    }
    return 998 * pktCount - 1012 + pktPtr->mCurIndex;
}

// FUNCTION: SWBF2 0x005d8de0
void NetPacket::Reset()
{
    doHiRes = false;
}

// FUNCTION: SWBF2 0x005d9250
void NetPktGroup::Free()
{
    NetPacket* pPkt;
    while ((pPkt = PopFront()) != 0)
    {
        pPkt->Free();
    }
}

// FUNCTION: SWBF2 0x005d9420
PBL_ASAN_NO_SANITIZE NetPacket* NetPktGroup::PopFront()
{
    if (pktCount == 0)
    {
        return 0;
    }

    NetPacket* pPkt = NetPacket::Get(firstPkt);
    firstPkt = pPkt->mNextPkt;
    if (--pktCount == 0)
    {
        lastPkt = -1;
    }
    else
    {
        NetPacket::Get(firstPkt)->mPrevPkt = -1;
    }
    pPkt->mPrevPkt = -1;
    pPkt->mNextPkt = -1;
    return pPkt;
}

// FUNCTION: SWBF2 0x005d94e0
NetPacket* NetPktGroup::GetFront()
{
    if (pktCount == 0)
    {
        return 0;
    }
    return NetPacket::Get(firstPkt);
}

// FUNCTION: SWBF2 0x005d9ba0
void NetPktGroup::Seek(const NetPktGroupOffset& offset)
{
    bad = false;
    pktPtr = NetPacket::Get(firstPkt);
    for (int n = offset.pktNum; n; --n)
    {
        pktPtr = NetPacket::Get(pktPtr->mNextPkt);
    }
    pktPtr->BeginRead();
    pktPtr->mCurIndex += (short)offset.byteNum;
    ReadPacked();
    curBit = offset.bitNum;
}

// FUNCTION: SWBF2 0x005d8f50
NetPacket* NetPacket::Decrypt(unsigned int)
{
    return this;
}

// FUNCTION: SWBF2 0x005d9080
bool NetPacket::Read(unsigned short& value)
{
    int iByte = mCurIndex;
    if (iByte + 2 > mMaxIndex)
    {
        return false;
    }
    unsigned char* pDest = (unsigned char*)&value;
    pDest[0] = (unsigned char)mData[iByte];
    pDest[1] = (unsigned char)mData[iByte + 1];
    mCurIndex += 2;
    return true;
}

// FUNCTION: SWBF2 0x005d9570
void NetPktGroup::InsertBefore(NetPacket* pBefore, NetPacket* pNew)
{
    pNew->mPrevPkt = pBefore->mPrevPkt;
    int iBefore = pBefore->GetNum();
    pNew->mNextPkt = (short)iBefore;
    int iNew = pNew->GetNum();
    pBefore->mPrevPkt = (short)iNew;
    ++pktCount;
    if (firstPkt == iBefore)
    {
        firstPkt = iNew;
    }
    else
    {
        NetPacket::Get(pNew->mPrevPkt)->mNextPkt = (short)iNew;
    }
}
