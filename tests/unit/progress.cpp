#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <string.h>

#include "Battlefront2/Platform/Pc/pcProgress.h"
#include "testutil.h"

TEST_CASE_NAMED(FProgressTest1, "Unit::Progress::progress section counts turns and steps and reports a stall request", "[SmokeFilter]")
{
    char acName[64];
    pcProgress::FormatName(acName, sizeof(acName), 1234);
    CHECK(strcmp(acName, "Local\\OpenSWBF2Progress.1234") == 0);

    pcProgress::Open();
    pcProgress::FormatName(acName, sizeof(acName), GetCurrentProcessId());
    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, acName);
    REQUIRE(hMap != (HANDLE)0);
    volatile LONG* plView = (volatile LONG*)MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 4096);
    REQUIRE(plView != (volatile LONG*)0);

    const LONG lTurns = plView[0];
    const LONG lSteps = plView[1];
    CHECK(plView[2] == 0);
    CHECK_FALSE(pcProgress::StallRequested());

    pcProgress::Turn();
    pcProgress::Turn();
    pcProgress::Step();
    pcProgress::Step();
    pcProgress::Step();
    CHECK(plView[0] == lTurns + 2);
    CHECK(plView[1] == lSteps + 3);

    WatchdogHold(true);
    plView[2] = 1;
    const bool bRequested = pcProgress::StallRequested();
    plView[2] = 0;
    const bool bCleared = pcProgress::StallRequested();
    WatchdogHold(false);
    CHECK(bRequested);
    CHECK_FALSE(bCleared);

    pcProgress::Open();
    pcProgress::Turn();
    pcProgress::Step();
    CHECK(plView[0] == lTurns + 3);
    CHECK(plView[1] == lSteps + 4);

    UnmapViewOfFile((LPCVOID)plView);
    CloseHandle(hMap);
}
