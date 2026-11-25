#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "harness/judge.h"

namespace
{

const char* const kMarker0 = "[2026.11.25-00.00.00:000][  0]LogCore: Inform: **** TEST COMPLETE. EXIT CODE: 0 ****\r\n";
const char* const kMarkerFail = "[2026.11.25-00.00.00:000][  0]LogCore: Inform: **** TEST COMPLETE. EXIT CODE: -1 ****\r\n";
const char* const kHeapError =
    "[2026.11.25-00.00.00:000][ 12]LogMemory: Error: alloc failure heap Runtime (3) size 268435456\r\n";

RunFacts Facts(const std::string& sLog, unsigned long uCode = 0)
{
    RunFacts k;
    ParseLogText(sLog, k.kLog);
    k.uExitCode = uCode;
    return k;
}

RunVerdict Judge(const RunFacts& kFacts, const RunExpect& kExpect = RunExpect())
{
    ExpectedMessages kMessages;
    return JudgeRun(kFacts, kExpect, kMessages);
}

}

TEST_CASE_NAMED(FHarnessJudgeTest1, "Unit::HarnessJudge::a clean run with the marker passes", "[SmokeFilter]")
{
    const RunVerdict k = Judge(Facts(std::string("LogSim: Inform: Soak turns=300\r\n") + kMarker0));
    CHECK(k.bPass);
    CHECK(k.sReason.empty());
}

TEST_CASE_NAMED(FHarnessJudgeTest2, "Unit::HarnessJudge::an uncovered error fails and a covered one passes", "[SmokeFilter]")
{
    const RunFacts kFacts = Facts(std::string(kHeapError) + kMarker0);
    const RunVerdict kBad = Judge(kFacts);
    CHECK(!kBad.bPass);
    CHECK(kBad.sReason.find("no expectation covers") != std::string::npos);
    REQUIRE(kBad.kErrors.size() == 1);
    ExpectedMessages kMessages;
    kMessages.Add("alloc failure heap Runtime", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, 1, true);
    CHECK(JudgeRun(kFacts, RunExpect(), kMessages).bPass);
    CHECK(!Judge(Facts(std::string("Message Severity: 4\r\n") + kMarker0)).bPass);
    CHECK(!Judge(Facts(std::string("LogNet: Fatal: online dedicated server needs Galaxy\r\n") + kMarker0)).bPass);
    CHECK(Judge(Facts(std::string("LogSim: Warning: pool full\r\n") + kMarker0)).bPass);
}

TEST_CASE_NAMED(FHarnessJudgeTest3, "Unit::HarnessJudge::an expectation count one off fails", "[SmokeFilter]")
{
    const RunFacts kFacts = Facts(std::string(kHeapError) + kHeapError + kMarker0);
    for (int iCount = 1; iCount <= 3; ++iCount)
    {
        ExpectedMessages kMessages;
        kMessages.Add("alloc failure heap Runtime", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, iCount, true);
        const RunVerdict k = JudgeRun(kFacts, RunExpect(), kMessages);
        CHECK(k.bPass == (iCount == 2));
        if (iCount != 2)
        {
            CHECK(k.sReason.find("but it was found 2 time(s)") != std::string::npos);
        }
    }
}

TEST_CASE_NAMED(FHarnessJudgeTest4, "Unit::HarnessJudge::a missing marker fails a normal run and not a fatal one",
                "[SmokeFilter]")
{
    const RunVerdict kNormal = Judge(Facts("LogSim: Inform: Mission running geo1c_con\r\n"));
    CHECK(!kNormal.bPass);
    CHECK(kNormal.sReason.find("no end marker") != std::string::npos);
    RunExpect kFatal;
    kFatal.bFatal = true;
    kFatal.iExitCode = 1;
    CHECK(Judge(Facts("FATAL: initialization failed, exiting with code 1\r\n", 1), kFatal).bPass);
    CHECK(!Judge(Facts("LogSim: Inform: nothing fatal\r\n", 1), kFatal).bPass);
    const RunVerdict kUnplanned = Judge(Facts(std::string("FATAL: lua panic, exiting with code 4\r\n") + kMarker0));
    CHECK(!kUnplanned.bPass);
    CHECK(kUnplanned.sReason.find("fatal: FATAL: lua panic") == 0);
}

TEST_CASE_NAMED(FHarnessJudgeTest5, "Unit::HarnessJudge::the exit code must match the expected code and the marker",
                "[SmokeFilter]")
{
    CHECK(!Judge(Facts(kMarker0, 1)).bPass);
    CHECK(!Judge(Facts(kMarkerFail, 0)).bPass);
    CHECK(!Judge(Facts(kMarker0, 0xFFFFFFFFul)).bPass);
    RunExpect kFail;
    kFail.iExitCode = -1;
    CHECK(Judge(Facts(kMarkerFail, 0xFFFFFFFFul), kFail).bPass);
    const RunVerdict kMismatch = Judge(Facts(kMarker0, 0xFFFFFFFFul), kFail);
    CHECK(!kMismatch.bPass);
    CHECK(kMismatch.sReason.find("the end marker says 0") != std::string::npos);
}

TEST_CASE_NAMED(FHarnessJudgeTest6, "Unit::HarnessJudge::an ensure fails unless an expectation covers it", "[SmokeFilter]")
{
    const RunFacts kFacts =
        Facts(std::string("[2026.11.25-00.00.00:000][  1]LogCore: Error: ensure failed: false\r\n") + kMarker0);
    const RunVerdict kBad = Judge(kFacts);
    CHECK(!kBad.bPass);
    CHECK(kBad.sReason.find("ensure: ") == 0);
    ExpectedMessages kMessages;
    kMessages.Add("ensure failed: false", RedLog::Warning, EAutomationExpectedMessageFlags::Exact, 1, true);
    CHECK(JudgeRun(kFacts, RunExpect(), kMessages).bPass);
}

TEST_CASE_NAMED(FHarnessJudgeTest7, "Unit::HarnessJudge::a run killed by the stall rule fails", "[SmokeFilter]")
{
    RunFacts kFacts = Facts(kMarker0);
    kFacts.bKilled = true;
    const RunVerdict k = Judge(kFacts);
    CHECK(!k.bPass);
    CHECK(k.sReason == "killed by the stall rule");
}

TEST_CASE_NAMED(FHarnessJudgeTest8, "Unit::HarnessJudge::a dump count one off fails", "[SmokeFilter]")
{
    RunFacts kFacts = Facts("FATAL: exception 0xC0000005 at 0x00401000 (BattlefrontII.exe+0x1000) build 0 dump x\r\n", 3);
    RunExpect kExpect;
    kExpect.bFatal = true;
    kExpect.iExitCode = 3;
    kExpect.iDumps = 1;
    for (int iDumps = 0; iDumps <= 2; ++iDumps)
    {
        kFacts.iDumps = iDumps;
        const RunVerdict k = Judge(kFacts, kExpect);
        CHECK(k.bPass == (iDumps == 1));
    }
    RunFacts kClean = Facts(kMarker0);
    kClean.iDumps = 1;
    CHECK(!Judge(kClean).bPass);
}

TEST_CASE_NAMED(FHarnessJudgeTest10, "Unit::HarnessJudge::a run expected to end early needs its exit code and no end marker", "[SmokeFilter]")
{
    RunExpect kExpect;
    kExpect.bEndsEarly = true;
    CHECK(Judge(Facts("LogCore: Inform: Close deferred until loading finishes\r\n"), kExpect).bPass);
    const RunVerdict kMarker = Judge(Facts(kMarker0), kExpect);
    CHECK(!kMarker.bPass);
    CHECK(kMarker.sReason == "an end marker on a run expected to end before its planned end");
    kExpect.iExitCode = (int)0xC0000409u;
    CHECK(Judge(Facts("", 0xC0000409ul), kExpect).bPass);
    CHECK(!Judge(Facts("", 3), kExpect).bPass);
    CHECK(!Judge(Facts("FATAL: hang\r\n", 0xC0000409ul), kExpect).bPass);
}

TEST_CASE_NAMED(FHarnessJudgeTest9, "Unit::HarnessJudge::an asan report fails a passing run", "[SmokeFilter]")
{
    RunFacts kFacts = Facts(kMarker0);
    CHECK(Judge(kFacts).bPass);
    kFacts.bAsanReport = true;
    const RunVerdict k = Judge(kFacts);
    CHECK(!k.bPass);
    CHECK(k.sReason == "Asan report");
}
