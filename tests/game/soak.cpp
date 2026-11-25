#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"

TEST_CASE_NAMED(FSoakTest1, "Game::Soak::asan-soak", "[ProductFilter][Soak]")
{
    GameRun kRun("asan-soak");
    kRun.AddArg(StandardArgs("asan-soak", "nab2g_c " + StandardTail(false) + " -soakturns 815800"));
    AddStandardTests(kRun);
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "asan-soak", 815800);
    const std::string sLog = ReadRunLog(kRun);
    CHECK_MESSAGE(HasLine(sLog, "Mission running"), "asan-soak: no MISSION RUNNING line");
    CHECK_MESSAGE(HasLine(sLog, "Sim digest turns="), "asan-soak: no SIM DIGEST line");
    CHECK_MESSAGE(HasLine(sLog, "Soak turns="), "asan-soak: no SOAK line");
}
