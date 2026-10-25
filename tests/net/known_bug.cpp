#include "third_party/doctest/doctest.h"

#include "testutil.h"
#include "nettest.h"

static NetPacket* MakeOne(unsigned int uTag)
{
    unsigned char acPayload[4];
    memcpy(acPayload, &uTag, 4);
    return NetMakePacket(acPayload, 4);
}

TEST_CASE("known bug fragment idx ge count" * doctest::test_suite("known_bug"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 5, 1, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 5, 1, 0);
    WatchdogArm("frag-idx-ge-count");
    NetPktGroup out;
    NetComm::Recv(&out, PKT_GAME_MOVE);
    CHECK(true);
}

TEST_CASE("known bug fragment classic gap" * doctest::test_suite("known_bug"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 2, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 2, 2, 0);
    WatchdogArm("frag-classic-gap");
    NetPktGroup out;
    NetComm::Recv(&out, PKT_GAME_MOVE);
    CHECK(true);
}

TEST_CASE("known bug duplicate fragment keeps accounting" * doctest::test_suite("known_bug"))
{
    NetTestReset();
    NetStampAndQueue(MakeOne(1), 5, PKT_GAME_MOVE, 1, 0, 2, 0);
    NetStampAndQueue(MakeOne(2), 5, PKT_GAME_MOVE, 1, 0, 2, 0);
    NetPktGroup out;
    NetComm::Recv(&out, PKT_GAME_MOVE);
    CHECK(NetCountPackets(0) == 512);
}
