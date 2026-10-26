#include "third_party/doctest/doctest.h"

#include <algorithm>
#include <vector>

#include "testutil.h"
#include "nettest.h"

extern NetPktGroup inPktGroups[64];

static NetPacket* MakeOne(unsigned int uTag)
{
    unsigned char acPayload[4];
    memcpy(acPayload, &uTag, 4);
    return NetMakePacket(acPayload, 4);
}

static void Drain(int iType)
{
    NetPktGroup out;
    out.Reset();
    while (NetComm::Recv(&out, iType))
    {
        out.Free();
        out.Reset();
    }
}

TEST_CASE("recvgroup rejects fragment idx ge count" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 5, 1, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 5, 1, 0);
    WatchdogArm("frag-idx-ge-count");
    Drain(PKT_GAME_MOVE);
    CHECK(netRejectCounts[NET_REJECT_FRAG_IDX] == 2);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("recvgroup rejects classic gap" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 2, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 2, 2, 0);
    WatchdogArm("frag-classic-gap");
    NetPktGroup out;
    CHECK(!NetComm::Recv(&out, PKT_GAME_MOVE));
    CHECK(netRejectCounts[NET_REJECT_FRAG_IDX] == 1);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("recvgroup rejects count zero" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 0, 0);
    WatchdogArm("count-zero");
    NetPktGroup out;
    CHECK(!NetComm::Recv(&out, PKT_GAME_MOVE));
    CHECK(inPktGroups[0].pktCount == 0);
    CHECK(netRejectCounts[NET_REJECT_FRAG_COUNT_ZERO] == 1);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("recvgroup rejects count over max" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 65, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 64, 255, 0);
    WatchdogArm("count-over-max");
    Drain(PKT_GAME_MOVE);
    CHECK(netRejectCounts[NET_REJECT_FRAG_TOO_MANY] == 2);
    CHECK(inPktGroups[0].pktCount == 0);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("recvgroup count mismatch frees partial and packet" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 3, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 1, 4, 0);
    WatchdogArm("count-mismatch");
    Drain(PKT_GAME_MOVE);
    CHECK(netRejectCounts[NET_REJECT_FRAG_COUNT_MISMATCH] == 1);
    CHECK(inPktGroups[0].pktCount == 0);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("recvgroup duplicate fragment keeps accounting" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 2, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 0, 2, 0);
    NetPktGroup out;
    CHECK(!NetComm::Recv(&out, PKT_GAME_MOVE));
    CHECK(inPktGroups[0].pktCount == 1);
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("recvgroup evicts oldest partial at per-source cap" * doctest::test_suite("regression"))
{
    NetTestReset();
    for (int i = 0; i < 9; ++i)
    {
        NetStampAndQueue(MakeOne(i), (unsigned short)(10 + i), PKT_GAME_MOVE, 1, 0, 2, 0);
    }
    NetStampAndQueue(MakeOne(100), 50, PKT_GAME_MOVE, 2, 0, 2, 0);
    WatchdogArm("partial-cap");
    NetPktGroup out;
    CHECK(!NetComm::Recv(&out, PKT_GAME_MOVE));
    int iFromOne = 0;
    int iFromTwo = 0;
    bool bOldestKept = false;
    bool bNewestKept = false;
    for (int i = 0; i < 64; ++i)
    {
        if (inPktGroups[i].pktCount)
        {
            const NetComm::PktHeader& header = NetComm::Header(NetPacket::Get(inPktGroups[i].firstPkt));
            iFromOne += header.src == 1;
            iFromTwo += header.src == 2;
            bOldestKept = bOldestKept || (header.src == 1 && header.group == 10);
            bNewestKept = bNewestKept || (header.src == 1 && header.group == 18);
        }
    }
    CHECK(iFromOne == 8);
    CHECK(!bOldestKept);
    CHECK(bNewestKept);
    CHECK(iFromTwo == 1);
    CHECK(netRejectCounts[NET_REJECT_PARTIAL_CAP] == 1);
    CHECK(NetCountPackets(0) == 512);
}

static void RoundTrip(int iFragments, bool bShuffle)
{
    NetTestReset();
    std::vector<int> order;
    for (int i = 0; i < iFragments; ++i)
    {
        order.push_back(i);
    }
    if (bShuffle)
    {
        for (size_t i = order.size(); i > 1; --i)
        {
            std::swap(order[i - 1], order[(i * 7 + 3) % i]);
        }
        std::reverse(order.begin(), order.end());
    }
    for (size_t i = 0; i < order.size(); ++i)
    {
        NetStampAndQueue(MakeOne(1000 + order[i]), 9, PKT_GAME_MOVE, 3, (unsigned char)order[i],
                         (unsigned char)iFragments, 0);
    }
    WatchdogArm("roundtrip");
    NetPktGroup out;
    out.Reset();
    CHECK(NetComm::Recv(&out, PKT_GAME_MOVE));
    CHECK(out.pktCount == iFragments);
    out.BeginRead();
    for (int i = 0; i < iFragments; ++i)
    {
        int iValue = 0;
        out.Read(iValue);
        CHECK(iValue == 1000 + i);
    }
    CHECK(!out.IsBad());
    out.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("recvgroup round trip in order" * doctest::test_suite("regression"))
{
    RoundTrip(2, false);
    RoundTrip(63, false);
    RoundTrip(64, false);
}

TEST_CASE("recvgroup round trip shuffled" * doctest::test_suite("regression"))
{
    RoundTrip(2, true);
    RoundTrip(63, true);
    RoundTrip(64, true);
}

TEST_CASE("recvgroup single fragment passes" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(77), 9, PKT_GAME_MOVE, 3, 0, 1, 0);
    NetPktGroup out;
    out.Reset();
    CHECK(NetComm::Recv(&out, PKT_GAME_MOVE));
    out.Free();
    CHECK(NetCountPackets(0) == 512);
}

TEST_CASE("recvgroup rejects 65 fragments" * doctest::test_suite("regression"))
{
    NetTestReset();
    for (int i = 0; i < 65; ++i)
    {
        NetStampAndQueue(MakeOne(i), 9, PKT_GAME_MOVE, 3, (unsigned char)i, 65, 0);
    }
    WatchdogArm("65-fragments");
    Drain(PKT_GAME_MOVE);
    CHECK(netRejectCounts[NET_REJECT_FRAG_TOO_MANY] == 65);
    CHECK(NetCountPackets(0) == 512);
}
