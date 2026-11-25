#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"

namespace
{

std::string SeedTail(int iSeed)
{
    std::string sTail = StandardTail(false);
    const std::string sFrom = "-seed 1 ";
    sTail.replace(sTail.find(sFrom), sFrom.size(), "-seed " + std::to_string(iSeed) + " ");
    return sTail;
}

void Prepare(GameRun& kRun, const char* pcName, const std::string& sArgs)
{
    kRun.AddArg(StandardArgs(pcName, sArgs));
    AddStandardTests(kRun);
}

void RunPair(GameRun& kA, GameRun& kB)
{
    kA.Start();
    kB.Start();
    kA.Wait();
    kB.Wait();
}

void CheckSoakLines(const GameRun& kRun, const char* pcName)
{
    const std::string sLog = ReadRunLog(kRun);
    CHECK_MESSAGE(HasLine(sLog, "Mission running"), pcName << ": no MISSION RUNNING line");
    CHECK_MESSAGE(HasLine(sLog, "Sim digest turns="), pcName << ": no SIM DIGEST line");
    CHECK_MESSAGE(HasLine(sLog, "Soak turns="), pcName << ": no SOAK line");
}

}

TEST_CASE_NAMED(FTwiceTest1, "Game::Twice::twice", "[ProductFilter][Twice]")
{
    const std::string sArgs = "geo1c_con " + StandardTail(false);
    GameRun kA("twice-1");
    GameRun kB("twice-2");
    Prepare(kA, "twice-1", sArgs);
    Prepare(kB, "twice-2", sArgs);
    RunPair(kA, kB);
    CheckStandardRun(kA, "twice-1", 0);
    CheckStandardRun(kB, "twice-2", 0);
    std::string sWhy;
    CHECK_MESSAGE(CompareFinalScores(ReadRunFile(kA, "finalscore.log"), ReadRunFile(kB, "finalscore.log"), sWhy),
                  "run-twice: " << sWhy);
}

TEST_CASE_NAMED(FTwiceTest2, "Game::Twice::twice-digest", "[ProductFilter][Twice]")
{
    const std::string sArgs = "nab2g_c " + StandardTail(false) + " -soakturns 1501";
    GameRun kA("twice-digest-1");
    GameRun kB("twice-digest-2");
    Prepare(kA, "twice-digest-1", sArgs);
    Prepare(kB, "twice-digest-2", sArgs);
    RunPair(kA, kB);
    CheckStandardRun(kA, "twice-digest-1", 1501);
    CheckStandardRun(kB, "twice-digest-2", 1501);
    CheckSoakLines(kA, "twice-digest-1");
    CheckSoakLines(kB, "twice-digest-2");
    std::string sWhy;
    CHECK_MESSAGE(CompareLastDigests(ReadRunLog(kA), ReadRunLog(kB), sWhy), "run-twice-digest: " << sWhy);
}

TEST_CASE_NAMED(FTwiceTest3, "Game::Twice::digest-hot1g_con", "[ProductFilter][Twice]")
{
    GameRun kRun("digest-hot1g_con");
    Prepare(kRun, "digest-hot1g_con", "hot1g_con " + StandardTail(false));
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "digest-hot1g_con", 0);
}

TEST_CASE_NAMED(FTwiceTest4, "Game::Twice::digest-spa3c_ass", "[ProductFilter][Twice]")
{
    GameRun kRun("digest-spa3c_ass");
    Prepare(kRun, "digest-spa3c_ass", "spa3c_ass " + StandardTail(false));
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "digest-spa3c_ass", 0);
}

TEST_CASE_NAMED(FTwiceTest5, "Game::Twice::trace-seed", "[ProductFilter][Twice]")
{
    GameRun kA("trace-seed-1");
    GameRun kB("trace-seed-2");
    Prepare(kA, "trace-seed-1", "hot1g_con " + SeedTail(1) + " -digesttrace 1600");
    Prepare(kB, "trace-seed-2", "hot1g_con " + SeedTail(2) + " -digesttrace 1600");
    RunPair(kA, kB);
    CheckStandardRun(kA, "trace-seed-1", 0);
    CheckStandardRun(kB, "trace-seed-2", 0);
    const std::string sLogA = ReadRunLog(kA);
    const std::string sLogB = ReadRunLog(kB);
    CHECK_MESSAGE(HasDigestTrace(sLogA, 1500), "trace-seed-1: no digest trace line for turn 1500");
    CHECK_MESSAGE(HasDigestTrace(sLogB, 1500), "trace-seed-2: no digest trace line for turn 1500");
    const int iSplit = TraceSplitTurn(sLogA, sLogB);
    CHECK_MESSAGE(iSplit != -1, "digest-trace: incomplete trace");
    CHECK_MESSAGE(iSplit != 0, "digest-trace: no split");
    MESSAGE("digest-trace split at turn " << iSplit);
    std::string sWhy;
    CHECK_MESSAGE(TraceMatchesDigest(sLogA, sWhy), "digest-trace: " << sWhy);
}
