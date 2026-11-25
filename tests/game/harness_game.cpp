#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "harness/gamerun.h"

TEST_CASE_NAMED(FHarnessGameTest1, "Game::Harness::a passing in-game test exits 0", "[ProductFilter][Standard]")
{
    GameRun kRun("autotest-pass");
    kRun.AddArg(GameRunTail(false) + " -soakturns 1 -autotest Harness.AutoTest.Pass");
    kRun.AddExpectedMessagePlain("Test Completed. Result={Success} Name={Pass} Path={Harness.AutoTest.Pass}",
                                 RedLog::Inform, EAutomationExpectedMessageFlags::Exact, 1);
    kRun.Start();
    kRun.Wait();
    CHECK(kRun.Result().ExitCode == 0);
    CHECK(kRun.Result().HasEndMarker);
    CHECK(kRun.Result().MarkerExitCode == 0);
}

TEST_CASE_NAMED(FHarnessGameTest2, "Game::Harness::a failing in-game test exits -1", "[NegativeFilter][Standard]")
{
    GameRun kRun("autotest-fail");
    kRun.AddArg(GameRunTail(false) + " -soakturns 1 -autotest Harness.AutoTest.Fail");
    kRun.AddExpectedErrorPlain("Harness.AutoTest.Fail fails on purpose, parameters \"\"",
                               EAutomationExpectedMessageFlags::Exact, 1);
    kRun.AddExpectedMessagePlain("Test Completed. Result={Fail} Name={Fail} Path={Harness.AutoTest.Fail}",
                                 RedLog::Inform, EAutomationExpectedMessageFlags::Exact, 1);
    kRun.ExpectExitCode(-1);
    kRun.Start();
    kRun.Wait();
    CHECK(kRun.Result().ExitCode == 0xFFFFFFFFul);
    CHECK(kRun.Result().HasEndMarker);
    CHECK(kRun.Result().MarkerExitCode == -1);
}

TEST_CASE_NAMED(FHarnessGameTest3, "Game::Harness::a filter that selects no in-game test exits -1",
                "[ProductFilter][Standard]")
{
    GameRun kRun("autotest-none");
    kRun.AddArg(GameRunTail(false) + " -soakturns 1 -autotest Harness.AutoTest.NoSuchTest");
    kRun.AddExpectedErrorPlain("No automation tests matched 'Harness.AutoTest.NoSuchTest'",
                               EAutomationExpectedMessageFlags::Exact, 1);
    kRun.ExpectExitCode(-1);
    kRun.Start();
    kRun.Wait();
    CHECK(kRun.Result().ExitCode == 0xFFFFFFFFul);
    CHECK(kRun.Result().MarkerExitCode == -1);
}
