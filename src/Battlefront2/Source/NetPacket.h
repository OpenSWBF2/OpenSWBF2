#pragma once
#include <stddef.h>

#include "PebbleFL/Common/PblAsan.h"

class NetPacket
{
public:
    static unsigned int sNextSeqNo;

    static NetPacket* Get(int iNum);

    int GetNum() const;

    // FUNCTION: SWBF2 0x005b2340
    NetPacket* GetNext() { return Get(mNextPkt); }

    static void SetHiRes(bool bHiRes);

    static void Reset();
    static void Create();

    void Free();

    NetPacket* Encrypt(int iDest);

    NetPacket* Decrypt(unsigned int key);

    bool Read(unsigned short& value);

    static NetPacket* Alloc(bool bSmall);
    static NetPacket* AllocRecv(bool bSmall);

    void BeginWrite();

    void EndWrite();

    void BeginRead();

    bool Write(unsigned int const& uValue);
    bool Read(unsigned int& uValue);

    bool Read(unsigned char& uValue);

    bool Write(unsigned char uValue);
    bool Write(unsigned short uValue);

    int Write(char const* pData, int iLen);
    int Read(char* pData, int iLen);

    short mNextPkt;
    short mPrevPkt;
    short mCurIndex;
    short mMaxIndex;
    char mCommIndex;
    char mDataIndex;
    unsigned short mSeqNo;
    char mData[4];
};
 
 

class NetBigPacket : public NetPacket
{
public:
    char mData2[1008];
};
 

class NetSmallPacket : public NetPacket
{
public:
    char mData2[112];
};
 

const int MAX_BIG_PACKETS = 0x100;
const int MAX_SMALL_PACKETS = 0x100;

class NetPktGroupOffset
{
public:
    int pktNum;
    int byteNum;
    int bitNum;
};

enum NetRejectReason
{
    NET_REJECT_FRAG_COUNT_ZERO,
    NET_REJECT_FRAG_IDX,
    NET_REJECT_FRAG_COUNT_MISMATCH,
    NET_REJECT_FRAG_TOO_MANY,
    NET_REJECT_POOL_EXHAUSTED,
    NET_REJECT_SHORT_DATAGRAM,
    NET_REJECT_NEGATIVE_LENGTH,
    NET_REJECT_GROUP_BAD,
    NET_REJECT_PARTIAL_CAP,
    NET_REJECT_OVERSIZE_DATAGRAM,
    NET_REJECT_CHAT_OVERSIZE,
    NET_REJECT_CHAT_FIELDS,
    NET_REJECT_UPLOAD_LENGTH,
    NET_REJECT_UPLOAD_NAME,
    NET_REJECT_UPLOAD_UNDERFLOW,
    NET_REJECT_UPLOAD_DISABLED,
    NET_REJECT_RCON_LINE,
    NET_REJECT_RCON_AUTH_SHORT,
    NET_REJECT_RCON_LOGIN_RATE,
    NET_REJECT_PLAYER_COUNT,
    NET_REJECT_TURN_RATE,
    NET_REJECT_BAD_ADDR,
    NET_REJECT_SPAWN_SLOT,
    NET_REJECT_OBJECT_REF,
    NET_REJECT_GUEST_COUNT,
    NET_REJECT_COUNT
};

class NetPktGroup
{
public:

    // FUNCTION: SWBF2 0x005aeca0
    NetPktGroup() : isSmall(false), bad(false) {}
    NetPktGroup(bool bSmall) : isSmall(bSmall), bad(false) {}

    void Reset();

    int GetBytes();

    PBL_ASAN_NO_SANITIZE void PushBack(NetPacket* pPkt);
    void InsertBefore(NetPacket* pBefore, NetPacket* pNew);

    PBL_ASAN_NO_SANITIZE NetPacket* PopFront();

    void Free();

    void Remove(NetPacket* pPkt);

    bool IsEnd();
    void BeginWrite();
    void EndWrite();
    PBL_ASAN_NO_SANITIZE void PushFront(NetPacket* pPkt);
    NetPacket* PopBack();

    void WritePacked();
    void ReadPacked();
    void WriteBits(unsigned int uValue, int iBits);
    void ReadBits(unsigned int& uValue, int iBits);
    // FUNCTION: SWBF2 0x0053e0a0
    void Write(bool bValue)
    {
        WriteBits(bValue != 0, 1);
    }
    void Write(int iValue);
    void Write(const float& fValue);
    // FUNCTION: SWBF2 0x0053e0c0
    void Read(bool& bValue)
    {
        unsigned int uValue;
        ReadBits(uValue, 1);
        bValue = uValue != 0;
    }
    void Read(float& fValue);
    void Read(int& iValue);
    void WriteFixed(float f, float fMin, float fMax, int iBits);
    void ReadFixed(float& f, float fMin, float fMax, int iBits);
    void WriteBytes(char const* pData, int iLen);
    void ReadBytes(char* pData, int iLen);

    NetPacket* GetFront();

    bool IsBad() const { return bad; }
    void SetBad(int iReason = NET_REJECT_GROUP_BAD);

    void BeginRead();
    void EndRead();
    void Seek(const NetPktGroupOffset& offset);

    void Set(NetPacket* pPkt);
    void Tell(NetPktGroupOffset& offset);

    static unsigned int sDebugBitCounter;

    bool isSmall;
    bool bad;
    int firstPkt;
    int lastPkt;
    int pktCount;
    NetPacket* pktPtr;
    int curBit;
    unsigned int pktBits;
};

static_assert(sizeof(NetPktGroup) == 28, "NetPktGroup layout");

extern int netRejectCounts[NET_REJECT_COUNT];
void NetRejectCount(int iReason);
void NetRejectSetClock(float (*pfnClock)());
void NetRejectReset();

static const int kRecvReserveBig = 128;
static const int kRecvReserveSmall = 32;

template <class CONVERT>
void WriteFixed(CONVERT& convert, NetPktGroup& group, float fValue)
{
    convert.TrackValue(fValue);
    group.WriteFixed(fValue, convert.GetMin(), convert.GetMax(), convert.GetBits());
}

template <class CONVERT>
void ReadFixed(CONVERT& convert, NetPktGroup& group, float& fValue)
{
    group.ReadFixed(fValue, convert.GetMin(), convert.GetMax(), convert.GetBits());
}

class ConvertValues
{
public:

    // FUNCTION: SWBF2 0x005b4000
    void TrackValue(float fValue) {}
};
 

void NetPktGroupClampFixed(float& f, float fMin, float fMax, int iBits);

extern bool doHiRes;

extern NetBigPacket netBigPackets[MAX_BIG_PACKETS];
extern NetSmallPacket netSmallPackets[MAX_SMALL_PACKETS];
extern NetPktGroup freeBigPackets;
extern NetPktGroup freeSmallPackets;
