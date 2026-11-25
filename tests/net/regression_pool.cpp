#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "testutil.h"
#include "nettest.h"

static NetPacket* MakeOne(unsigned int uTag)
{
    unsigned char acPayload[4];
    memcpy(acPayload, &uTag, 4);
    return NetMakePacket(acPayload, 4);
}

static void BuildInts(NetPktGroup& g, int iCount)
{
    g.BeginWrite();
    for (int i = 0; i < iCount; ++i)
    {
        g.Write(i + 1);
    }
    g.EndWrite();
}

TEST_CASE_NAMED(FRegressionPoolTest1, "Net::RegressionPool::alloc exhaustion is fatal and does not recycle", "[EngineFilter]")
{
    AddExpectedError("net packet pool exhausted");
    NetTestReset();
    LogCapture capture;
    for (int i = 0; i < 256; ++i)
    {
        CHECK(NetPacket::Alloc(false));
    }
    CHECK(!NetPacket::Alloc(false));
    CHECK(gExitCalls == 1);
    CHECK(gLastExitCode == 3);
    CHECK(capture.Contains("net packet pool exhausted"));
    CHECK(netRejectCounts[NET_REJECT_POOL_EXHAUSTED] == 1);
    CHECK(freeBigPackets.pktCount == 0);
    CHECK(freeSmallPackets.pktCount == 256);
    CHECK(NetCountPackets(256) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest2, "Net::RegressionPool::alloc recv keeps the big reserve", "[EngineFilter]")
{
    NetTestReset();
    int iHeld = 0;
    while (NetPacket::AllocRecv(false))
    {
        ++iHeld;
    }
    CHECK(iHeld == 256 - kRecvReserveBig);
    CHECK(freeBigPackets.pktCount == kRecvReserveBig);
    CHECK(netRejectCounts[NET_REJECT_POOL_EXHAUSTED] == 1);
    CHECK(NetPacket::Alloc(false));
    ++iHeld;
    CHECK(gExitCalls == 0);
    CHECK(NetCountPackets(iHeld) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest3, "Net::RegressionPool::alloc recv keeps the small reserve", "[EngineFilter]")
{
    NetTestReset();
    int iHeld = 0;
    while (NetPacket::AllocRecv(true))
    {
        ++iHeld;
    }
    CHECK(iHeld == 256 - kRecvReserveSmall);
    CHECK(freeSmallPackets.pktCount == kRecvReserveSmall);
    CHECK(netRejectCounts[NET_REJECT_POOL_EXHAUSTED] == 1);
    CHECK(NetPacket::Alloc(true));
    ++iHeld;
    CHECK(gExitCalls == 0);
    CHECK(NetCountPackets(iHeld) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest4, "Net::RegressionPool::packet read with non positive length copies nothing", "[EngineFilter]")
{
    NetTestReset();
    NetPacket* pPacket = MakeOne(0x01020304);
    pPacket->BeginRead();
    const short sBefore = pPacket->mCurIndex;
    char acBuf[16];
    CHECK(pPacket->Read(acBuf, -1) == 0);
    CHECK(pPacket->mCurIndex == sBefore);
    CHECK(pPacket->Read(acBuf, 0) == 0);
    CHECK(pPacket->mCurIndex == sBefore);
    CHECK(netRejectCounts[NET_REJECT_NEGATIVE_LENGTH] == 1);
    CHECK(pPacket->Read(acBuf, 0x7fffffff) == 4);
    CHECK(pPacket->Read(acBuf, 4) == 0);
    pPacket->Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest5, "Net::RegressionPool::group bad discards later readers", "[EngineFilter]")
{
    NetTestReset();
    NetPktGroup g;
    BuildInts(g, 3);
    g.BeginRead();
    int iFirst = 0;
    g.Read(iFirst);
    CHECK(iFirst == 1);
    g.SetBad();
    NetPacket* pBefore = g.pktPtr;
    const short sBefore = g.pktPtr->mCurIndex;
    const int iBitBefore = g.curBit;
    int iThird = 99;
    bool bThird = true;
    float fThird = 5.0f;
    g.Read(iThird);
    g.Read(bThird);
    g.Read(fThird);
    unsigned int uBits = 77;
    g.ReadBits(uBits, 5);
    float fFixed = 3.0f;
    g.ReadFixed(fFixed, 0.0f, 1.0f, 8);
    char acBuf[4] = { 1, 1, 1, 1 };
    g.ReadBytes(acBuf, 4);
    CHECK(iThird == 0);
    CHECK(!bThird);
    CHECK(fThird == 0.0f);
    CHECK(uBits == 0);
    CHECK(fFixed == 0.0f);
    CHECK(acBuf[0] == 0);
    CHECK(g.pktPtr == pBefore);
    CHECK(g.pktPtr->mCurIndex == sBefore);
    CHECK(g.curBit == iBitBefore);
    CHECK(netRejectCounts[NET_REJECT_GROUP_BAD] == 1);
    g.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest6, "Net::RegressionPool::begin read clears a stale bad flag", "[EngineFilter]")
{
    NetTestReset();
    NetPktGroup g;
    BuildInts(g, 1);
    g.SetBad();
    CHECK(g.IsBad());
    g.BeginRead();
    CHECK(!g.IsBad());
    g.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest7, "Net::RegressionPool::begin read on an empty first packet is bad", "[EngineFilter]")
{
    NetTestReset();
    NetPktGroup g;
    g.Reset();
    g.PushBack(NetMakePacket(0, 0));
    g.BeginRead();
    CHECK(g.IsBad());
    CHECK(netRejectCounts[NET_REJECT_GROUP_BAD] == 1);
    g.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest8, "Net::RegressionPool::read past the end marks bad but a zero bit read does not", "[EngineFilter]")
{
    NetTestReset();
    NetPktGroup g;
    BuildInts(g, 1);
    g.BeginRead();
    int iValue = 0;
    g.Read(iValue);
    g.Read(iValue);
    CHECK(!g.IsBad());
    unsigned int uBits = 5;
    g.ReadBits(uBits, 0);
    CHECK(!g.IsBad());
    g.BeginRead();
    g.Read(iValue);
    g.Read(iValue);
    g.ReadBits(uBits, 1);
    CHECK(g.IsBad());
    CHECK(uBits == 0);
    CHECK(netRejectCounts[NET_REJECT_GROUP_BAD] == 1);
    g.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest9, "Net::RegressionPool::group read bytes negative length keeps cursor and marks bad", "[EngineFilter]")
{
    NetTestReset();
    NetPktGroup g;
    BuildInts(g, 2);
    g.BeginRead();
    int iValue = 0;
    g.Read(iValue);
    const short sBefore = g.pktPtr->mCurIndex;
    char acBuf[8];
    g.ReadBytes(acBuf, -1);
    CHECK(g.pktPtr->mCurIndex == sBefore);
    CHECK(g.IsBad());
    CHECK(netRejectCounts[NET_REJECT_NEGATIVE_LENGTH] == 1);
    g.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE_NAMED(FRegressionPoolTest10, "Net::RegressionPool::group read bytes past the end marks bad", "[EngineFilter]")
{
    NetTestReset();
    NetPktGroup g;
    BuildInts(g, 1);
    g.BeginRead();
    char acBuf[64];
    memset(acBuf, 1, sizeof(acBuf));
    g.ReadBytes(acBuf, 64);
    CHECK(g.IsBad());
    CHECK(acBuf[63] == 0);
    g.Free();
    CHECK(NetCountPackets(0) == 512);
}
