#include "third_party/doctest/doctest.h"

#include "testutil.h"
#include "nettest.h"

extern NetPktGroup inQueue;
extern NetPktGroup inPktGroups[64];
extern int freeGroupCycle;

static NetPacket* MakeOne(unsigned int uTag)
{
    unsigned char acPayload[4];
    memcpy(acPayload, &uTag, 4);
    return NetMakePacket(acPayload, 4);
}

TEST_CASE("pool after create is 512" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    CHECK(freeBigPackets.pktCount == 256);
    CHECK(freeSmallPackets.pktCount == 256);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("group write read round trip" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    NetPktGroup g;
    g.BeginWrite();
    g.Write(0x12345678);
    g.Write(true);
    g.Write(1.5f);
    g.Write(-7);
    g.EndWrite();
    g.BeginRead();
    int iA = 0;
    bool bB = false;
    float fC = 0.0f;
    int iD = 0;
    g.Read(iA);
    g.Read(bB);
    g.Read(fC);
    g.Read(iD);
    CHECK(iA == 0x12345678);
    CHECK(bB);
    CHECK(fC == 1.5f);
    CHECK(iD == -7);
}

TEST_CASE("recv single packet" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    NetPktGroup g;
    g.BeginWrite();
    g.Write(0xCAFE);
    g.EndWrite();
    NetPacket* pPacket = g.GetFront();
    g.Reset();
    NetStampAndQueue(pPacket, 7, PKT_GAME_MOVE, 3, 0, 1, 0);
    NetPktGroup out;
    CHECK(NetComm::Recv(&out, PKT_GAME_MOVE));
    CHECK(out.pktCount == 1);
    out.BeginRead();
    int iValue = 0;
    out.Read(iValue);
    CHECK(iValue == 0xCAFE);
    CHECK(NetCountPackets(out.pktCount) == 512);
}

TEST_CASE("recv two fragments reordered" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    NetPacket* pSecond = MakeOne(0x22222222);
    NetPacket* pFirst = MakeOne(0x11111111);
    NetStampAndQueue(pSecond, 9, PKT_GAME_MOVE, 2, 1, 2, 0);
    NetStampAndQueue(pFirst, 9, PKT_GAME_MOVE, 2, 0, 2, 0);
    NetPktGroup out;
    CHECK(NetComm::Recv(&out, PKT_GAME_MOVE));
    CHECK(out.pktCount == 2);
    out.BeginRead();
    int iA = 0;
    int iB = 0;
    out.Read(iA);
    out.Read(iB);
    CHECK(iA == 0x11111111);
    CHECK(iB == 0x22222222);
    CHECK(NetCountPackets(out.pktCount) == 512);
}

TEST_CASE("alloc exhaustion 512" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    for (int i = 0; i < 512; ++i)
    {
        NetPacket::Alloc((i & 1) != 0);
    }
    CHECK(freeBigPackets.pktCount == 0);
    CHECK(freeSmallPackets.pktCount == 0);
}

TEST_CASE("alloc 513th is fatal and returns null" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    for (int i = 0; i < 512; ++i)
    {
        NetPacket::Alloc((i & 1) != 0);
    }
    CHECK(!NetPacket::Alloc(false));
    CHECK(gExitCalls == 1);
    CHECK(gLastExitCode == 3);
    CHECK(NetCountPackets(512) == 1024 - 512);
}

TEST_CASE("free group cycle never advances" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    inPktGroups[1].PushBack(MakeOne(1));
    inQueue.PushBack(MakeOne(2));
    NetCommP::FreePackets();
    inQueue.PushBack(MakeOne(3));
    NetCommP::FreePackets();
    CHECK(freeGroupCycle == 0);
    CHECK(inPktGroups[1].pktCount == 1);
}

TEST_CASE("duplicate fragment keeps accounting" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 2, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 0, 2, 0);
    NetPktGroup out;
    CHECK(!NetComm::Recv(&out, PKT_GAME_MOVE));
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("count zero is dropped" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 0, 0);
    NetPktGroup out;
    CHECK(!NetComm::Recv(&out, PKT_GAME_MOVE));
    CHECK(inPktGroups[0].pktCount == 0);
    CHECK(netRejectCounts[NET_REJECT_FRAG_COUNT_ZERO] == 1);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("negative readbytes keeps cursor and marks group bad" * doctest::test_suite("characterisation"))
{
    NetTestReset();
    NetPktGroup g;
    g.BeginWrite();
    g.Write(1);
    g.Write(2);
    g.EndWrite();
    g.BeginRead();
    int iValue = 0;
    g.Read(iValue);
    CHECK(g.pktPtr->mCurIndex == 18);
    char acBuf[8];
    g.ReadBytes(acBuf, -4);
    CHECK(g.pktPtr->mCurIndex == 18);
    CHECK(g.IsBad());
    g.Free();
}
