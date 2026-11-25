#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stdio.h>

#include <string>
#include <vector>

#include "harness/compare.h"

namespace
{

const char* const kDigestA =
    "[2026.11.25-00.00.00:000][  1]LogSim: Inform: Sim digest turns=1500 time=50.000 rand=1/2 snd=49.923 state=0x65CD7716\r\n";
const char* const kDigestB =
    "[2026.11.25-00.00.00:000][  1]LogSim: Inform: Sim digest turns=1500 time=50.000 rand=1/2 snd=49.923 state=0x65CD7717\r\n";
const char* const kCloth = "LogMemory: Inform: Cloth free=12 enabled=1 low=4 high=8\r\n";
const char* const kClothOther = "LogMemory: Inform: Cloth free=11 enabled=1 low=4 high=8\r\n";

std::string Report(const char* pcSource, const char* pcRuntime)
{
    return std::string("LogMemory: Inform: Memory report source=") + pcSource + "\r\n" +
           "LogMemory: Inform: Memory report heap=Lua cap=1 peak=1 used=1 allocs=1 failed=0\r\n" +
           "LogMemory: Inform: Memory report heap=Runtime cap=9 " + pcRuntime + " allocs=3 failed=0\r\n";
}

std::string Commit(const char* pcState, long long llArena, long long llGameVm)
{
    return std::string("LogMemory: Inform: Commit state=") + pcState + " bytes=100\r\n" +
           "LogMemory: Inform: Commit parts state=" + pcState +
           " private=1 gamecrt=100 heaps=1 threads=1 threadcount=1 arena=" + std::to_string(llArena) +
           " crtbusy=1 crtcommitted=1 crtlarge=1 gamevm=" + std::to_string(llGameVm) + "\r\n";
}

std::string CommitLog(const long long* pllArena, int iCycles)
{
    std::string sLog;
    for (int i = 0; i < iCycles; ++i)
    {
        sLog += Commit("mission", pllArena[i], 900);
        sLog += Commit("shell", pllArena[i], 900);
    }
    return sLog;
}

const char* const kCommitTable = "# commit 79299a77\r\nsteam\thog\tstate\tarena\trest\r\nyes\t1\tmission\t4000\t1000\r\n"
                                 "yes\t1\tshell\t4000\t1000\r\n";

}

TEST_CASE_NAMED(FHarnessCompareTest1, "Unit::HarnessCompare::a table row is found once by name and section", "[SmokeFilter]")
{
    std::string sRow;
    CHECK(FindTableRow("# note\r\ngeo1c_con\tSim digest a\r\nnab2g_c\tb\r\n", 0, "geo1c_con", sRow));
    CHECK(sRow == "Sim digest a");
    CHECK(!FindTableRow("geo1c_con\ta\r\n", 0, "geo1c", sRow));
    CHECK(!FindTableRow("x\ta\r\nx\tb\r\n", 0, "x", sRow));
    CHECK(FindTableRow("## other\r\nx\ta\r\n## wanted\r\nx\tb\r\n", "wanted", "x", sRow));
    CHECK(sRow == "b");
    CHECK(!FindTableRow("## other\r\nx\ta\r\n", "wanted", "x", sRow));
}

TEST_CASE_NAMED(FHarnessCompareTest2, "Unit::HarnessCompare::the last digest line must equal the pinned row", "[SmokeFilter]")
{
    std::string sWhy;
    const std::string sRow = "Sim digest turns=1500 time=50.000 rand=1/2 snd=49.923 state=0x65CD7716";
    CHECK(CompareDigestRow(kDigestA, sRow, sWhy));
    CHECK(!CompareDigestRow(kDigestB, sRow, sWhy));
    CHECK(sWhy.find("state=0x65CD7717") != std::string::npos);
    CHECK(!CompareDigestRow("LogSim: Inform: nothing\r\n", sRow, sWhy));
    CHECK(!CompareDigestRow(kDigestA, "", sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTest3, "Unit::HarnessCompare::a digest trace line needs the turn and the hash shape", "[SmokeFilter]")
{
    const std::string sLog = "LogSim: Inform: Digest trace turn=100 state=0x0012ABCD rand=5/6\r\n";
    CHECK(HasDigestTrace(sLog, 100));
    CHECK(!HasDigestTrace(sLog, 1500));
    CHECK(!HasDigestTrace("LogSim: Inform: Digest trace turn=100 state=0x0012abcd rand=5/6\r\n", 100));
    CHECK(!HasDigestTrace("LogSim: Inform: Digest trace turn=1000 state=0x0012ABCD rand=5/6\r\n", 100));
}

TEST_CASE_NAMED(FHarnessCompareTest4, "Unit::HarnessCompare::the same run needs an equal digest and cloth line", "[SmokeFilter]")
{
    std::string sWhy;
    const std::string sRef = std::string(kDigestA) + kCloth;
    CHECK(CompareSameRun(sRef, sRef, sWhy));
    CHECK(!CompareSameRun(std::string(kDigestA) + kClothOther, sRef, sWhy));
    CHECK(sWhy.find("CLOTH differs") == 0);
    CHECK(!CompareSameRun(std::string(kDigestB) + kCloth, sRef, sWhy));
    CHECK(!CompareSameRun(kDigestA, kDigestA, sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTest5, "Unit::HarnessCompare::repeat cycles need the exact count of equal digests", "[SmokeFilter]")
{
    std::string sWhy;
    CHECK(CompareEqualDigests(std::string(kDigestA) + kDigestA, 2, sWhy));
    CHECK(!CompareEqualDigests(std::string(kDigestA) + kDigestB, 2, sWhy));
    CHECK(!CompareEqualDigests(kDigestA, 2, sWhy));
    CHECK(sWhy == "expected 2 equal SIM DIGEST lines, got 1");
}

TEST_CASE_NAMED(FHarnessCompareTest6, "Unit::HarnessCompare::the turn 1500 runtime heap must match the reference", "[SmokeFilter]")
{
    std::string sWhy;
    const std::string sRef = Report("turn1500", "peak=7 used=5") + Report("exit", "peak=8 used=0");
    CHECK(CompareTurn1500Runtime(Report("turn1500", "peak=1 used=1") + Report("turn1500", "peak=7 used=5"), sRef, sWhy));
    CHECK(!CompareTurn1500Runtime(Report("turn1500", "peak=7 used=6"), sRef, sWhy));
    CHECK(!CompareTurn1500Runtime(Report("exit", "peak=7 used=5"), sRef, sWhy));
    CHECK(!CompareTurn1500Runtime(Report("turn1500", "peak=7 used=5"), Report("exit", "peak=7 used=5"), sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTest7, "Unit::HarnessCompare::commit growth after the settled cycles stays within the bound", "[SmokeFilter]")
{
    std::string sWhy;
    const long long allFlat[] = {4000, 4000, 4000, 4000};
    const long long allGrow[] = {4000, 4000, 4000, 4001};
    const long long allDip[] = {4100, 4000, 4000, 4050};
    CHECK(CompareCommitFlat(CommitLog(allFlat, 4), 0, 4, 2, sWhy));
    CHECK(CompareCommitFlat(CommitLog(allDip, 4), 0, 4, 2, sWhy));
    CHECK(!CompareCommitFlat(CommitLog(allGrow, 4), 0, 4, 2, sWhy));
    CHECK(sWhy.find("mission 4 arena = 4001 is 1 above the peak 4000") == 0);
    CHECK(CompareCommitFlat(CommitLog(allGrow, 4), 1, 4, 2, sWhy));
    CHECK(!CompareCommitFlat(CommitLog(allFlat, 3), 0, 4, 2, sWhy));
    CHECK(!CompareCommitFlat(Commit("shell", 1, 1) + CommitLog(allFlat, 4), 0, 4, 2, sWhy));
    CHECK(!CompareCommitFlat(CommitLog(allFlat, 4), 0, 4, 4, sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTest8, "Unit::HarnessCompare::commit rows after the settled cycles equal the table", "[SmokeFilter]")
{
    std::string sWhy;
    const long long allFlat[] = {4000, 4000, 4000, 4000};
    const long long allOff[] = {4000, 4000, 4000, 4064};
    CHECK(CompareCommitRows(CommitLog(allFlat, 4), kCommitTable, true, true, 2, sWhy));
    CHECK(!CompareCommitRows(CommitLog(allOff, 4), kCommitTable, true, true, 2, sWhy));
    CHECK(!CompareCommitRows(CommitLog(allFlat, 4), kCommitTable, false, true, 2, sWhy));
    CHECK(sWhy.find("no commit row") == 0);
    CHECK(!CompareCommitRows(CommitLog(allFlat, 2), kCommitTable, true, true, 2, sWhy));
    CHECK(!CompareCommitRows(CommitLog(allFlat, 4), "steam\thog\tstate\tarena\trest\r\n", true, true, 2, sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTest9, "Unit::HarnessCompare::save files must be byte identical", "[SmokeFilter]")
{
    std::string sWhy;
    CHECK(CompareBytes(std::string("ab\0c", 4), std::string("ab\0c", 4), sWhy));
    CHECK(!CompareBytes(std::string("ab\0c", 4), std::string("ab\0d", 4), sWhy));
    CHECK(sWhy == "bytes differ at offset 3 (4 and 4 bytes)");
    CHECK(!CompareBytes("abc", "abcd", sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTestSmokeAll, "Unit::HarnessCompare::a smoke-all-digests row holds the last digest line and the final score after its first tab", "[SmokeFilter]")
{
    const std::string sScore = "geo1c_con\t 309s\tL=Rep\t28u@12P\r\nsecond line\r\n";
    std::string sWhy;
    const std::string sDigest = "Sim digest turns=1500 time=50.000 rand=1/2 snd=49.923 state=0x65CD7716";
    CHECK(CompareSmokeAllRow(kDigestA, sScore, sDigest + "\t 309s\tL=Rep\t28u@12P", sWhy));
    CHECK(!CompareSmokeAllRow(kDigestB, sScore, sDigest + "\t 309s\tL=Rep\t28u@12P", sWhy));
    CHECK(!CompareSmokeAllRow(kDigestA, "geo1c_con\t 310s\tL=Rep\t28u@12P\r\n", sDigest + "\t 309s\tL=Rep\t28u@12P", sWhy));
    CHECK(sWhy.find("smoke-all-digests row expected") == 0);
    CHECK(CompareSmokeAllRow("FATAL: exception\r\n", "", "none\tnone", sWhy));
    CHECK(!CompareSmokeAllRow(kDigestA, "", "none\tnone", sWhy));
    CHECK(!CompareSmokeAllRow("", "", "", sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTestOffThread, "Unit::HarnessCompare::an offthread row holds the last total before the soak line", "[SmokeFilter]")
{
    const std::string sZero = "Memory report offthread count=0 sources=0 dropped=0\r\n";
    const std::string sOne = "LogMemory: Inform: Memory report offthread count=1 sources=1 dropped=0\r\n";
    std::string sWhy;
    CHECK(CompareOffThreadRow(sZero, "0\t0\t0", sWhy));
    CHECK(!CompareOffThreadRow(sZero + sOne, "0\t0\t0", sWhy));
    CHECK(sWhy == "offthread row expected '0\t0\t0', actual '1\t1\t0'");
    CHECK(CompareOffThreadRow(sZero + "LogSim: Inform: Soak turns=300\r\n" + sOne, "0\t0\t0", sWhy));
    CHECK(CompareOffThreadRow("LogSim: Inform: nothing\r\n", "-\t-\t-", sWhy));
    CHECK(!CompareOffThreadRow("", "0\t0\t0", sWhy));
    CHECK(!CompareOffThreadRow(sZero, "", sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTestIni, "Unit::HarnessCompare::an ini line must equal a whole line and a key must start a word", "[SmokeFilter]")
{
    const std::string sIni = "[Display]\r\nVsync=2\r\nMyFps=1\r\n";
    CHECK(IniHasLine(sIni, "Vsync=2"));
    CHECK(!IniHasLine(sIni, "Vsync=20"));
    CHECK(!IniHasLine(sIni, "sync=2"));
    CHECK(IniHasKey(sIni, "Vsync"));
    CHECK(!IniHasKey(sIni, "Fps"));
    CHECK(!IniHasKey(sIni, "Refresh"));
    CHECK(IniHasKey("Fps=30", "Fps"));
    CHECK(!IniHasKey("", "Fps"));
}

namespace
{

std::string Trace(unsigned int uFrom, unsigned int uTo, unsigned int uSplit = 0, const char* pcPrefix = "")
{
    std::string sLog = pcPrefix;
    for (unsigned int t = uFrom; t <= uTo; ++t)
    {
        char ac[160];
        _snprintf(ac, sizeof(ac) - 1, "LogSim: Inform: Digest trace turn=%u state=0x%08X rand=%u/7\r\n", t,
                  t == uSplit ? 0xFFFFFFFFu : t, t);
        ac[sizeof(ac) - 1] = 0;
        sLog += ac;
    }
    return sLog;
}

const char* const kRepeat = "LogSim: Inform: Repeat to_mission mission=geo1c_con\r\n";

}

TEST_CASE_NAMED(FHarnessCompareTestMissions, "Unit::HarnessCompare::the missions file refuses blank and repeated lines and unsafe names", "[SmokeFilter]")
{
    std::vector<MissionLine> k;
    std::string sError;
    CHECK(ParseMissions("cor1c_c TimeOut\r\ncor1c_con Pass\r\ngeo1c_con\r\n", k, sError));
    REQUIRE(k.size() == 3);
    CHECK(k[0].sName == "cor1c_c");
    CHECK(k[0].sClass == "TimeOut");
    CHECK(k[2].sClass == "Pass");
    CHECK(!ParseMissions("cor1c_c TimeOut\r\n\r\ncor1c_con Pass\r\n", k, sError));
    CHECK(sError.find("blank") != std::string::npos);
    CHECK(!ParseMissions("cor1c_c TimeOut\r\n   \r\n", k, sError));
    CHECK(!ParseMissions("cor1c_c TimeOut\r\ncor1c_c Pass\r\n", k, sError));
    CHECK(sError.find("twice") != std::string::npos);
    CHECK(!ParseMissions("..\\evil Pass\r\n", k, sError));
    CHECK(!ParseMissions("geo1c_con\" -crashtest nullderef Pass\r\n", k, sError));
    CHECK(!ParseMissions("geo1c_con Fatal\r\n", k, sError));
    CHECK(!ParseMissions("geo1c_con Pass extra\r\n", k, sError));
    CHECK(!ParseMissions("", k, sError));
}

TEST_CASE_NAMED(FHarnessCompareTestClass, "Unit::HarnessCompare::a TimeOut mission must reach its soak turns and a Pass mission must end before the cap", "[SmokeFilter]")
{
    std::string sWhy;
    CHECK(CheckMissionClass("TimeOut", 9300, 9300, 617300, sWhy));
    CHECK(!CheckMissionClass("TimeOut", 4210, 9300, 617300, sWhy));
    CHECK(sWhy.find("4210") != std::string::npos);
    CHECK(CheckMissionClass("Pass", 4210, 9300, 617300, sWhy));
    CHECK(!CheckMissionClass("Pass", 617300, 9300, 617300, sWhy));
    CHECK(!CheckMissionClass("Fatal", 1, 9300, 617300, sWhy));
    CHECK(LastTurnsCovered("LogSim: Inform: Soak turns=1501\r\n", 1501, sWhy));
    CHECK(!LastTurnsCovered("LogSim: Inform: Soak turns=1501\r\n", 1500, sWhy));
    CHECK(!LastTurnsCovered(kDigestA, 1499, sWhy));
    CHECK(LastTurnsCovered("LogSim: Inform: nothing\r\n", 0, sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTestPair, "Unit::HarnessCompare::a pair needs equal final scores and equal last digests", "[SmokeFilter]")
{
    std::string sWhy;
    CHECK(FinalScoreKey("2026-11-25\tteam1 10 team2 3\r\n") == "team1 10 team2 3");
    CHECK(CompareFinalScores("a\tteam1 10\r\n", "b\tteam1 10\r\n", sWhy));
    CHECK(!CompareFinalScores("a\tteam1 10\r\n", "b\tteam1 11\r\n", sWhy));
    CHECK(!CompareFinalScores("", "", sWhy));
    CHECK(CompareLastDigests(kDigestA, kDigestA, sWhy));
    CHECK(!CompareLastDigests(kDigestA, kDigestB, sWhy));
    CHECK(sWhy.find("0x65CD7717") != std::string::npos);
    CHECK(!CompareLastDigests("", "", sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTestSplit, "Unit::HarnessCompare::two seeds split at the first turn whose state differs", "[SmokeFilter]")
{
    CHECK(TraceSplitTurn(Trace(1, 10), Trace(1, 10, 7)) == 7);
    CHECK(TraceSplitTurn(Trace(1, 10, 1), Trace(1, 10)) == 1);
    CHECK(TraceSplitTurn(Trace(1, 10), Trace(1, 10)) == 0);
    CHECK(TraceSplitTurn(Trace(1, 10), Trace(2, 10)) == -1);
    CHECK(TraceSplitTurn(Trace(1, 10), "") == -1);
    std::string sWhy;
    CHECK(TraceMatchesDigest(Trace(1, 20) + "LogSim: Inform: Sim digest turns=12 time=1 rand=1/2 snd=1 state=0x0000000C\r\n", sWhy));
    CHECK(!TraceMatchesDigest(Trace(1, 20) + "LogSim: Inform: Sim digest turns=12 time=1 rand=1/2 snd=1 state=0x0000000B\r\n", sWhy));
    CHECK(!TraceMatchesDigest(Trace(1, 20), sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTestPerturb, "Unit::HarnessCompare::a perturbed run must equal its base trace and digest", "[SmokeFilter]")
{
    std::string sWhy;
    const std::string sBase = Trace(1, 1600) + kDigestA;
    CHECK(CheckPerturbBase(sBase, sWhy));
    CHECK(!CheckPerturbBase(Trace(1, 1400) + kDigestA, sWhy));
    CHECK(!CheckPerturbBase(Trace(1, 1600) + kRepeat + Trace(1, 1600), sWhy));
    CHECK(ComparePerturbRun(sBase, sBase, false, sWhy));
    CHECK(!ComparePerturbRun(sBase, Trace(1, 1600, 900) + kDigestA, false, sWhy));
    CHECK(sWhy.find("split turn=900 ") != std::string::npos);
    CHECK(!ComparePerturbRun(sBase, Trace(1, 1601) + kDigestA, false, sWhy));
    CHECK(sWhy.find("split turn=1601 ") != std::string::npos);
    CHECK(!ComparePerturbRun(sBase, Trace(1, 1600) + kDigestB, false, sWhy));
    CHECK(!ComparePerturbRun(sBase, Trace(1, 1600), false, sWhy));
    CHECK(ComparePerturbRun(sBase, Trace(1, 1600) + kDigestA + kRepeat + Trace(1, 1600) + kDigestA, true, sWhy));
    CHECK(!ComparePerturbRun(sBase, Trace(1, 1600) + kDigestA, true, sWhy));
    CHECK(!ComparePerturbRun(sBase, Trace(1, 1600) + kDigestA + kRepeat + Trace(1, 1600, 3) + kDigestA, true, sWhy));
}

TEST_CASE_NAMED(FHarnessCompareTestGrown, "Unit::HarnessCompare::grown pools come from the pinned mem-peaks rows of one mission", "[SmokeFilter]")
{
    const char* const pcTable = "# commit 07eb13d5\r\n"
                                "mission\tsource\tkind\tname\tcap_or_count\tsize\tpeak\tused\tallocs_or_grown\tfailed\r\n"
                                "geo1c_con\texit\theap\tRuntime\t1\t-\t1\t1\t9\t0\r\n"
                                "geo1c_con\texit\tpool\tSoldierAnimation\t372\t12\t370\t-\t62\t-\r\n"
                                "geo1c_con\texit\tpool\tWeapon\t10\t12\t9\t-\t0\t-\r\n"
                                "nab2g_c\texit\tpool\tCloth\t10\t12\t9\t-\t4\t-\r\n";
    const std::vector<std::string> k = GrownPools(pcTable, "geo1c_con");
    REQUIRE(k.size() == 1);
    CHECK(k[0] == "SoldierAnimation");
    CHECK(GrownPools(pcTable, "geo1c").empty());
}

TEST_CASE_NAMED(FHarnessCompareTestFrames, "Unit::HarnessCompare::the frame count comes from one FRAMES line", "[SmokeFilter]")
{
    long long ll = 0;
    CHECK(FramesCount("[2026.11.25-00.00.00:000][ 12]LogCore: Inform: Frames count=4012\r\n", ll));
    CHECK(ll == 4012);
    CHECK(!FramesCount("LogCore: Inform: Frames count=4012\r\nLogCore: Inform: Frames count=4012\r\n", ll));
    CHECK(!FramesCount("LogCore: Inform: Frames count=x\r\n", ll));
    CHECK(!FramesCount("[2026.11.25-00.00.00:000][ 12]LogCore: Inform: **** TEST COMPLETE. EXIT CODE: 0 ****\r\n", ll));
}
