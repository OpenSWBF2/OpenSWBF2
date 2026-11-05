#include "third_party/doctest/doctest.h"

#include <string.h>

#include "testutil.h"
#include "nettest.h"

void __fastcall HandleCommPacketIn(NetPacket* pPacket);

namespace
{

__declspec(noinline) void PoisonStack()
{
    volatile unsigned char acFill[4096];
    for (unsigned int i = 0; i < sizeof(acFill); ++i)
    {
        acFill[i] = 0xA5;
    }
}

}

TEST_CASE("net header read of a short packet leaves zeroed fields" * doctest::test_suite("regression"))
{
    const unsigned char acData[3] = {PKT_GAME_MOVE, 0x34, 0x12};
    const unsigned int auLength[3] = {0, 1, 3};
    for (int t = 0; t < 3; ++t)
    {
        const unsigned int uLength = auLength[t];
        CAPTURE(uLength);
        NetTestReset();
        NetPacket* pPacket = NetPacket::Alloc(true);
        REQUIRE(pPacket);
        pPacket->BeginWrite();
        memset(pPacket->mData, 0xA5, 3);
        memcpy(pPacket->mData + 3, acData, uLength);
        pPacket->mCommIndex = 3;
        pPacket->mMaxIndex = (short)(3 + uLength);
        PoisonStack();
        HandleCommPacketIn(pPacket);
        const NetComm::PktHeader& header = NetComm::Header(pPacket);
        CHECK(header.type == (uLength >= 1 ? PKT_GAME_MOVE : 0));
        CHECK(header.group == (uLength >= 3 ? 0x1234 : 0));
        CHECK(header.src == 0);
        CHECK(header.idx == 0);
        CHECK(header.count == 0);
        CHECK(header.compressed == 0);
        NetPktGroup complete;
        complete.Reset();
        CHECK_FALSE(RecvGroup(complete, pPacket));
        CHECK(netRejectCounts[NET_REJECT_FRAG_COUNT_ZERO] == 1);
        CHECK(NetCountPackets(0) == 512);
    }
}

TEST_CASE("empty datagram is dropped" * doctest::test_suite("regression"))
{
    NetTestReset();
    unsigned char acByte = 0;
    NetDeliverDatagram(&acByte, 0, 3);
    CHECK(netRejectCounts[NET_REJECT_SHORT_DATAGRAM] == 1);
    CHECK(NetCountPackets(0) == 512);
    CHECK(freeBigPackets.pktCount == 256);
    CHECK(freeSmallPackets.pktCount == 256);
}

TEST_CASE("short datagram with a low type is dropped" * doctest::test_suite("regression"))
{
    NetTestReset();
    const unsigned char acData[3] = {0x0b, 0x07, 0x00};
    NetDeliverDatagram(acData, 3, 3);
    CHECK(netRejectCounts[NET_REJECT_SHORT_DATAGRAM] == 1);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("short datagram with a pre-auth type is accepted by framing" * doctest::test_suite("regression"))
{
    NetTestReset();
    const unsigned char acData[3] = {0xfe, 0x01, 0x02};
    NetDeliverDatagram(acData, 3, 3);
    CHECK(netRejectCounts[NET_REJECT_SHORT_DATAGRAM] == 0);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("oversize datagram is dropped and counted" * doctest::test_suite("regression"))
{
    NetTestReset();
    unsigned char acData[1100];
    memset(acData, 0, sizeof(acData));
    acData[0] = 0x0b;
    NetDeliverDatagram(acData, sizeof(acData), 3);
    CHECK(netRejectCounts[NET_REJECT_OVERSIZE_DATAGRAM] == 1);
    CHECK(NetCountPackets(0) == 512);
}

static void Flood(unsigned int uSize, int iReserve, const NetPktGroup& pool)
{
    unsigned char acData[200];
    memset(acData, 0, sizeof(acData));
    acData[0] = 0x0b;
    acData[1] = 0x07;
    acData[4] = 1;
    for (int i = 0; i < 400; ++i)
    {
        NetDeliverDatagram(acData, uSize, 3);
    }
    CHECK(pool.pktCount == iReserve);
    CHECK(netRejectCounts[NET_REJECT_POOL_EXHAUSTED] > 0);
}

TEST_CASE("big datagram flood leaves the send path alive" * doctest::test_suite("regression"))
{
    NetTestReset();
    Flood(200, kRecvReserveBig, freeBigPackets);
    NetPacket* pSend = NetPacket::Alloc(false);
    CHECK(pSend);
    CHECK(gExitCalls == 0);
    CHECK(NetCountPackets(1) == 512);
}

TEST_CASE("small datagram flood leaves the send path alive" * doctest::test_suite("regression"))
{
    NetTestReset();
    Flood(50, kRecvReserveSmall, freeSmallPackets);
    NetPacket* pSend = NetPacket::Alloc(true);
    CHECK(pSend);
    CHECK(gExitCalls == 0);
    CHECK(NetCountPackets(1) == 512);
}
