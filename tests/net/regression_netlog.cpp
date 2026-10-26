#include "third_party/doctest/doctest.h"

#include "testutil.h"
#include "nettest.h"

static float sTestClock = 0.0f;

static float TestClock()
{
    return sTestClock;
}

TEST_CASE("reject flood logs once per second with a suppressed count" * doctest::test_suite("regression"))
{
    NetTestReset();
    sTestClock = 10.0f;
    NetRejectSetClock(TestClock);
    {
        LogCapture capture;
        NetRejectCount(NET_REJECT_BAD_ADDR);
        CHECK(capture.Contains("net reject bad_addr (0 suppressed)"));
    }
    CHECK(netRejectCounts[NET_REJECT_BAD_ADDR] == 1);
    {
        LogCapture quiet;
        for (int i = 0; i < 1000; ++i)
        {
            NetRejectCount(NET_REJECT_BAD_ADDR);
        }
        CHECK(!quiet.Contains("net reject"));
    }
    CHECK(netRejectCounts[NET_REJECT_BAD_ADDR] == 1001);
    sTestClock = 11.0f;
    LogCapture later;
    NetRejectCount(NET_REJECT_BAD_ADDR);
    CHECK(later.Contains("(1000 suppressed)"));
    NetRejectReset();
}

TEST_CASE("reject logger logs again after the clock goes backwards" * doctest::test_suite("regression"))
{
    NetTestReset();
    sTestClock = 3600.0f;
    NetRejectSetClock(TestClock);
    {
        LogCapture capture;
        NetRejectCount(NET_REJECT_BAD_ADDR);
        CHECK(capture.Contains("net reject bad_addr (0 suppressed)"));
    }
    sTestClock = 0.5f;
    {
        LogCapture capture;
        NetRejectCount(NET_REJECT_BAD_ADDR);
        CHECK(capture.Contains("net reject bad_addr (0 suppressed)"));
    }
    {
        LogCapture quiet;
        NetRejectCount(NET_REJECT_BAD_ADDR);
        CHECK(!quiet.Contains("net reject"));
    }
    sTestClock = 1.5f;
    LogCapture later;
    NetRejectCount(NET_REJECT_BAD_ADDR);
    CHECK(later.Contains("(1 suppressed)"));
    CHECK(netRejectCounts[NET_REJECT_BAD_ADDR] == 4);
    NetRejectReset();
}

TEST_CASE("reject reasons rate limit independently" * doctest::test_suite("regression"))
{
    NetTestReset();
    sTestClock = 5.0f;
    NetRejectSetClock(TestClock);
    LogCapture capture;
    NetRejectCount(NET_REJECT_TURN_RATE);
    NetRejectCount(NET_REJECT_PLAYER_COUNT);
    CHECK(capture.Contains("net reject turn_rate"));
    CHECK(capture.Contains("net reject player_count"));
    NetRejectCount(-1);
    NetRejectCount(NET_REJECT_COUNT);
    CHECK(netRejectCounts[NET_REJECT_TURN_RATE] == 1);
    NetRejectReset();
}
