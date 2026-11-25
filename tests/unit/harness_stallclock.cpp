#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "harness/stallclock.h"

namespace
{

const double kWindow = 3000.0;
const unsigned int kUserRequest = 6;
const unsigned int kPageIn = 2;
const unsigned int kFreePage = 1;

StallState Seen(unsigned int uSteps, double fCpu, double fSilence, double fMax, bool bStalled)
{
    StallState s = NewStallState();
    s.bHasProgress = true;
    s.uTurns = 1;
    s.uSteps = uSteps;
    s.bSeen = true;
    s.bHasCpu = true;
    s.fCpuMs = fCpu;
    s.fSilenceMs = fSilence;
    s.fSilentMaxMs = fMax;
    s.bStalled = bStalled;
    return s;
}

StallSample Sample(int iSteps, bool bWaiting, unsigned int uReason, double fCpu, double fInterval)
{
    StallSample k = NewStallSample();
    if (iSteps > 0)
    {
        k.bHasProgress = true;
        k.uTurns = 1;
        k.uSteps = (unsigned int)iSteps;
    }
    k.bWaiting = bWaiting;
    k.uWaitReason = uReason;
    k.bHasCpu = true;
    k.fCpuMs = fCpu;
    k.fIntervalMs = fInterval;
    return k;
}

void Expect(const StallState& s, double fSilence, double fMax, bool bStalled, bool bKill)
{
    CHECK(s.fSilenceMs == fSilence);
    CHECK(s.fSilentMaxMs == fMax);
    CHECK(s.bStalled == bStalled);
    CHECK(s.bKill == bKill);
}

}

TEST_CASE_NAMED(FHarnessStallclockTest1, "Unit::HarnessStallclock::changed progress resets the silence", "[SmokeFilter]")
{
    StallState s = Seen(1, 100.0, 2500.0, 2500.0, false);
    s = StepStallClock(s, Sample(2, true, kUserRequest, 150.0, 1000.0), kWindow);
    Expect(s, 0.0, 2500.0, false, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest2, "Unit::HarnessStallclock::a waiting main thread that is not starved adds the interval",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 100.0, 0.0, 0.0, false);
    s = StepStallClock(s, Sample(1, true, kUserRequest, 150.0, 1000.0), kWindow);
    Expect(s, 1000.0, 1000.0, false, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest3, "Unit::HarnessStallclock::a running main thread adds its cpu time not the interval",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 100.0, 0.0, 0.0, false);
    s = StepStallClock(s, Sample(1, false, 0, 400.0, 1000.0), kWindow);
    Expect(s, 300.0, 300.0, false, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest4, "Unit::HarnessStallclock::a ready main thread adds only its cpu time", "[SmokeFilter]")
{
    StallState s = Seen(1, 100.0, 500.0, 500.0, false);
    s = StepStallClock(s, Sample(1, false, 0, 100.0, 1000.0), kWindow);
    s = StepStallClock(s, Sample(1, false, 0, 300.0, 1000.0), kWindow);
    Expect(s, 700.0, 700.0, false, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest5, "Unit::HarnessStallclock::a standby or transition main thread adds only its cpu time",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 100.0, 0.0, 0.0, false);
    Expect(StepStallClock(s, Sample(1, false, 0, 100.0, 1000.0), kWindow), 0.0, 0.0, false, false);
    Expect(StepStallClock(s, Sample(1, false, 0, 150.0, 1000.0), kWindow), 50.0, 50.0, false, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest6,
                "Unit::HarnessStallclock::a waiting main thread while another thread is ready adds only the process cpu time",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 100.0, 0.0, 0.0, false);
    s.bHasProcCpu = true;
    s.fProcCpuMs = 100.0;
    StallSample k = Sample(1, true, kUserRequest, 100.0, 1000.0);
    k.bHasProcCpu = true;
    k.fProcCpuMs = 250.0;
    k.bOtherReady = true;
    s = StepStallClock(s, k, kWindow);
    k.fProcCpuMs = 260.0;
    k.bOtherReady = false;
    s = StepStallClock(s, k, kWindow);
    Expect(s, 1150.0, 1150.0, false, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest7, "Unit::HarnessStallclock::a starved thread waiting for a page adds only its cpu time",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 100.0, 0.0, 0.0, false);
    s = StepStallClock(s, Sample(1, true, kPageIn, 120.0, 1000.0), kWindow);
    s = StepStallClock(s, Sample(1, true, kFreePage, 120.0, 1000.0), kWindow);
    Expect(s, 20.0, 20.0, false, false);
    for (unsigned int u = 0; u < 40; ++u)
    {
        CAPTURE(u);
        const bool bStarved = u == 1 || u == 2 || u == 3 || u == 5 || u == 8 || u == 9 || u == 10 || u == 12 || u == 18 ||
                              u == 19;
        CHECK(IsStarvedWaitReason(u) == bStarved);
    }
}

TEST_CASE_NAMED(FHarnessStallclockTest8, "Unit::HarnessStallclock::silence in cpu time flags a stall", "[SmokeFilter]")
{
    StallState s = Seen(1, 0.0, 0.0, 0.0, false);
    s = StepStallClock(s, Sample(1, false, 0, 2000.0, 2000.0), kWindow);
    s = StepStallClock(s, Sample(1, false, 0, 3000.0, 1000.0), kWindow);
    Expect(s, 0.0, 3000.0, true, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest9, "Unit::HarnessStallclock::without a section the first stall kills", "[SmokeFilter]")
{
    StallState s = NewStallState();
    for (int i = 0; i < 2; ++i)
    {
        s = StepStallClock(s, Sample(0, true, kUserRequest, 0.0, 1000.0), kWindow);
    }
    Expect(s, 2000.0, 2000.0, false, false);
    s = StepStallClock(s, Sample(0, true, kUserRequest, 0.0, 1000.0), kWindow);
    Expect(s, 0.0, 3000.0, true, true);
}

TEST_CASE_NAMED(FHarnessStallclockTest10, "Unit::HarnessStallclock::with a section the first stall is reported and does not kill",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 0.0, 0.0, 0.0, false);
    s = StepStallClock(s, Sample(1, true, kUserRequest, 0.0, 2000.0), kWindow);
    s = StepStallClock(s, Sample(1, true, kUserRequest, 0.0, 1500.0), kWindow);
    s = StepStallClock(s, Sample(1, true, kUserRequest, 0.0, 1000.0), kWindow);
    Expect(s, 1000.0, 3500.0, true, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest11, "Unit::HarnessStallclock::a second window of silence after the stall kills",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 0.0, 0.0, 3000.0, true);
    for (int i = 0; i < 3; ++i)
    {
        s = StepStallClock(s, Sample(1, true, kUserRequest, 0.0, 1000.0), kWindow);
    }
    Expect(s, 0.0, 3000.0, true, true);
}

TEST_CASE_NAMED(FHarnessStallclockTest12, "Unit::HarnessStallclock::a starved thread never stalls however long it waits",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 0.0, 0.0, 0.0, false);
    for (int i = 0; i < 10; ++i)
    {
        s = StepStallClock(s, Sample(1, true, kPageIn, 0.0, 1000.0), kWindow);
    }
    Expect(s, 0.0, 0.0, false, false);
}

TEST_CASE_NAMED(FHarnessStallclockTest13, "Unit::HarnessStallclock::silent max keeps the largest silence across a reset",
                "[SmokeFilter]")
{
    StallState s = Seen(1, 0.0, 0.0, 0.0, false);
    s = StepStallClock(s, Sample(1, true, kUserRequest, 0.0, 2000.0), kWindow);
    s = StepStallClock(s, Sample(2, true, kUserRequest, 0.0, 1000.0), kWindow);
    s = StepStallClock(s, Sample(2, false, 0, 500.0, 1000.0), kWindow);
    Expect(s, 500.0, 2000.0, false, false);
}
