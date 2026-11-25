#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string>

#include "harness/gamerun.h"
#include "game/standard_case.h"

TEST_CASE_NAMED(FDedicatedTest1, "Game::Dedicated::dedicated", "[ProductFilter][Dedicated]")
{
    GameRun kRun("dedicated");
    kRun.AddArg(StandardArgs("dedicated", "/autonet dedicated -lan -nopregame -playercount 0 geo1c_con 100 100 " +
                                              StandardTail(false)));
    AddStandardTests(kRun);
    kRun.Start();
    kRun.Wait();
    CHECK_MESSAGE(kRun.Result().ProgressTurns < kStandardTurnCap,
                  "dedicated: the run reached the turn cap " << kStandardTurnCap);
    if (GameRun::Config() == "Test")
    {
        CheckGroupRows(kRun, "dedicated");
    }
}
