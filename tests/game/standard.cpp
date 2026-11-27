#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <tlhelp32.h>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"
#include "testutil.h"

namespace
{

const char* const kStandard = "geo1c_con";

std::string Mission(const char* pcMission, const std::string& sExtra = std::string())
{
    return std::string(pcMission) + " " + StandardTail(false) + (sExtra.empty() ? "" : " " + sExtra);
}

void PacingTest(GameRun& kRun, const char* pcParameters)
{
    kRun.AddInGameTest("Game.Render.Pacing", pcParameters);
}

void RunStandardMission(const char* pcName, const std::string& sArgs, unsigned int uSoak, const char* pcPacing = 0,
                        const char* pcDpi = 0)
{
    GameRun kRun(pcName);
    kRun.AddArg(StandardArgs(pcName, sArgs));
    AddStandardTests(kRun);
    if (pcPacing)
    {
        PacingTest(kRun, pcPacing);
    }
    if (pcDpi)
    {
        kRun.AddInGameTest("Game.Window.Dpi", pcDpi);
    }
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, pcName, uSoak);
}

bool SteamRunning()
{
    bool bProcess = false;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 kEntry;
        kEntry.dwSize = sizeof(kEntry);
        for (BOOL bMore = Process32First(hSnap, &kEntry); bMore && !bProcess; bMore = Process32Next(hSnap, &kEntry))
        {
            bProcess = _stricmp(kEntry.szExeFile, "steam.exe") == 0;
        }
        CloseHandle(hSnap);
    }
    if (!bProcess)
    {
        return false;
    }
    DWORD uUser = 0;
    DWORD uSize = sizeof(uUser);
    return RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", "ActiveUser", RRF_RT_REG_DWORD, 0,
                        &uUser, &uSize) == ERROR_SUCCESS &&
           uUser != 0;
}

}

TEST_CASE_NAMED(FStandardTest1, "Game::Standard::geo1c_con", "[ProductFilter][Standard]")
{
    GameRun kRun(kStandard);
    kRun.AddArg(StandardArgs(kStandard, Mission(kStandard)));
    AddStandardTests(kRun);
    PacingTest(kRun, "fps:80,vsync:off,interval:0x80000000,waitquery:0,timer:none");
    kRun.AddInGameTest("Game.Window.Dpi", "awareness:pmv2|pm|system|none");
    kRun.AddInGameTest("Game.Render.Gamma");
    kRun.AddInGameTest("Game.Input.Cursor", "mode:hw,image:skipped");
    kRun.AddInGameTest("Game.Memory.Report", "heaps:Runtime|Lua|Application|Artist|Gamespy|Debug,pools:Lua*|Weapon");
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, kStandard, 0);
}

TEST_CASE_NAMED(FStandardTest2, "Game::Standard::heapcap", "[ProductFilter][Standard]")
{
    GameRun kRef("heapcap-ref-geo1c_con");
    kRef.AddArg(StandardArgs(kStandard, Mission(kStandard)));
    AddStandardTests(kRef);
    GameRun kRun("heapcap");
    kRun.AddArg(StandardArgs(
        "heapcap", Mission(kStandard, "-heapcap Runtime=1M -heapcap Runtime=96M -heapcap Bogus=1M -heapcap Runtime=12x")));
    AddStandardTests(kRun);
    kRun.AddInGameTest("Game.Memory.Report", "cap:Runtime/100663296");
    kRun.AddExpectedMessagePlain("-heapcap needs <Name>=<bytes>[K|M], ignored: Bogus=1M", RedLog::Warning,
                                 EAutomationExpectedMessageFlags::Exact, 1);
    kRun.AddExpectedMessagePlain("-heapcap needs <Name>=<bytes>[K|M], ignored: Runtime=12x", RedLog::Warning,
                                 EAutomationExpectedMessageFlags::Exact, 1);
    kRef.Start();
    kRun.Start();
    kRef.Wait();
    kRun.Wait();
    CheckStandardRun(kRef, kStandard, 0);
    CheckStandardRun(kRun, "heapcap", 0);
    std::string sWhy;
    CHECK_MESSAGE(CompareSameRun(ReadRunLog(kRun), ReadRunLog(kRef), sWhy), "heapcap against geo1c_con: " << sWhy);
}

TEST_CASE_NAMED(FStandardTest3, "Game::Standard::sound-on", "[ProductFilter][Standard]")
{
    GameRun kRun("sound-on");
    kRun.AddArg(StandardArgs("sound-on", std::string(kStandard) + " " + StandardTail(true) + " -soakturns 300"));
    AddStandardTests(kRun);
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "sound-on", 300);
}

TEST_CASE_NAMED(FStandardTest4, "Game::Standard::display-switches", "[ProductFilter][Standard]")
{
    GameRun kRun("display-switches");
    kRun.AddArg(StandardArgs("display-switches", Mission(kStandard, "-borderless -adapter 0 -refresh 60 -vsync half -fps 120 "
                                                                    "-nowaitquery -nodpi -soakturns 1")));
    AddStandardTests(kRun);
    PacingTest(kRun, "fps:120,vsync:on,interval:0x00000000,waitquery:0,timer:none");
    kRun.AddInGameTest("Game.Window.Dpi", "awareness:off");
    kRun.AddExpectedMessagePlain("pacing: fps cap 120 is above 80, frame-count effects are not fixed yet", RedLog::Warning,
                                 EAutomationExpectedMessageFlags::Exact, 1);
    kRun.AddExpectedMessagePlain("vsync: requested interval not supported here, using on", RedLog::Warning,
                                 EAutomationExpectedMessageFlags::Exact, 1);
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "display-switches", 1);
}

TEST_CASE_NAMED(FStandardTest5, "Game::Standard::cursor-sw", "[ProductFilter][Standard]")
{
    GameRun kRun("cursor-sw");
    kRun.AddArg(StandardArgs("cursor-sw", Mission(kStandard, "-swcursor -soakturns 1")));
    AddStandardTests(kRun);
    kRun.AddInGameTest("Game.Input.Cursor", "mode:sw,image:skipped");
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "cursor-sw", 1);
}

TEST_CASE_NAMED(FStandardTest6, "Game::Standard::cursor-sw-ini", "[ProductFilter][Standard]")
{
    GameRun kRun("cursor-sw-ini");
    kRun.AddArg(StandardArgs("cursor-sw-ini", Mission(kStandard, "-displayini " + QuoteArg(SmokeFile("display-swcursor.ini")) +
                                                                     " -soakturns 1")));
    AddStandardTests(kRun);
    kRun.AddInGameTest("Game.Input.Cursor", "mode:sw,image:skipped");
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "cursor-sw-ini", 1);
}

TEST_CASE_NAMED(FStandardTest7, "Game::Standard::window-borderless-headless", "[ProductFilter][Standard]")
{
    RunStandardMission("window-borderless-headless", Mission(kStandard, "-borderless -adapter 5 -soakturns 1"), 1, 0,
                       "awareness:pmv2|pm|system|none");
}

TEST_CASE_NAMED(FStandardTest8, "Game::Standard::fps-0", "[ProductFilter][Standard]")
{
    GameRun kRun("fps-0");
    kRun.AddArg(StandardArgs("fps-0", Mission(kStandard, "-fps 0 -soakturns 1")));
    AddStandardTests(kRun);
    PacingTest(kRun, "fps:0");
    kRun.AddExpectedMessagePlain("pacing: fps cap is uncapped, frame-count effects are not fixed yet", RedLog::Warning,
                                 EAutomationExpectedMessageFlags::Exact, 1);
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "fps-0", 1);
}

TEST_CASE_NAMED(FStandardTest9, "Game::Standard::fps-30", "[ProductFilter][Standard]")
{
    RunStandardMission("fps-30", Mission(kStandard, "-fps 30 -soakturns 1"), 1, "fps:30");
}

TEST_CASE_NAMED(FStandardTest10, "Game::Standard::pacing-realtime", "[ProductFilter][Standard]")
{
    RunStandardMission("pacing-realtime", "nab2g_c " + RealTimeTail() + " -fps 30 -soakturns 300", 300, "fps:30");
}

TEST_CASE_NAMED(FStandardTest11, "Game::Standard::pacing-realtime-uncapped", "[ProductFilter][Standard]")
{
    RunStandardMission("pacing-realtime-uncapped", "nab2g_c " + RealTimeTail() + " -fps 0 -soakturns 300", 300, "fps:0");
}

TEST_CASE_NAMED(FStandardTest12, "Game::Standard::display-badini", "[ProductFilter][Standard]")
{
    RunStandardMission("display-badini",
                       Mission(kStandard, "-adapter abc -refresh 99999 -displayini " + QuoteArg(SmokeFile("display-bad.ini")) +
                                              " -soakturns 1"),
                       1);
}

TEST_CASE_NAMED(FStandardTest13, "Game::Standard::pacing-waitquery-ini", "[ProductFilter][Standard]")
{
    RunStandardMission("pacing-waitquery-ini",
                       Mission(kStandard, "-displayini " + QuoteArg(SmokeFile("display-waitquery.ini")) + " -soakturns 1"),
                       1, "fps:80,vsync:off,interval:0x80000000,waitquery:1,timer:none");
}

TEST_CASE_NAMED(FStandardTest14, "Game::Standard::digest-dag1c_ctf", "[ProductFilter][Standard]")
{
    RunStandardMission("digest-dag1c_ctf", Mission("dag1c_ctf"), 0);
}

TEST_CASE_NAMED(FStandardTest15, "Game::Standard::novictory-con", "[ProductFilter][Standard]")
{
    GameRun kRun("novictory-con");
    kRun.AddArg(StandardArgs("novictory-con", Mission(kStandard, "-novictory -digesttrace 9300 -soakturns 18500")));
    AddStandardTests(kRun);
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "novictory-con", 18500);
    CHECK_MESSAGE(HasDigestTrace(ReadRunLog(kRun), 9300), "novictory-con has no digest trace line for turn 9300");
}

TEST_CASE_NAMED(FStandardTest16, "Game::Standard::digest-kas2c_uber", "[ProductFilter][Standard]")
{
    RunStandardMission("digest-kas2c_uber", Mission("kas2c_uber"), 0);
}

TEST_CASE_NAMED(FStandardTest17, "Game::Standard::asan-hot1g_con", "[ProductFilter][Standard]")
{
    RunStandardMission("asan-hot1g_con", Mission("hot1g_con"), 0);
}

TEST_CASE_NAMED(FStandardTest18, "Game::Standard::asan-spa3c_ass", "[ProductFilter][Standard]")
{
    RunStandardMission("asan-spa3c_ass", Mission("spa3c_ass"), 0);
}

TEST_CASE_NAMED(FStandardTest19, "Game::Standard::shell-first", "[ProductFilter][Standard]")
{
    GameRun kRef("shell-first-ref-geo1c_con");
    kRef.AddArg(StandardArgs(kStandard, Mission(kStandard)));
    AddStandardTests(kRef);
    GameRun kRun("shell-first");
    kRun.AddArg(StandardArgs("shell-first", Mission(kStandard, "-shellfirst")));
    AddStandardTests(kRun);
    kRun.AddInGameTest("Game.State.ShellFirst");
    kRef.Start();
    kRun.Start();
    kRef.Wait();
    kRun.Wait();
    CheckStandardRun(kRef, kStandard, 0);
    CheckStandardRun(kRun, "shell-first", 0);
    const std::string sLog = ReadRunLog(kRun);
    std::string sWhy;
    CHECK_MESSAGE(CompareEqualDigests(sLog, 1, sWhy), "shell-first: " << sWhy);
    CHECK_MESSAGE(CompareTurn1500Runtime(sLog, ReadRunLog(kRef), sWhy), "shell-first against geo1c_con: " << sWhy);
}

TEST_CASE_NAMED(FStandardTest20, "Game::Standard::commit-flat", "[ProductFilter][Standard]")
{
    GameRun kRun("commit-flat");
    const std::string sArgs = StandardArgs("commit-flat", Mission(kStandard, "-repeatmission 16"));
    kRun.AddArg(sArgs);
    AddStandardTests(kRun);
    const bool bSteam = SteamRunning();
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "commit-flat", 0);
    const std::string sLog = ReadRunLog(kRun);
    std::string sWhy;
    CHECK_MESSAGE(CompareEqualDigests(sLog, 17, sWhy), "commit-flat: " << sWhy);
    CHECK_MESSAGE(CompareCommitFlat(sLog, 0, 16, 8, sWhy), "commit-flat: " << sWhy);
    std::string sTable;
    if (GameRun::Platform() == "x64")
    {
        bool bHog = false;
        REQUIRE_MESSAGE(CommitHogMode(GameRun::Platform(), sArgs, bHog, sWhy), "commit-flat: " << sWhy);
        REQUIRE(ReadRepoText("tests\\baseline\\v143-x64\\commit.txt", sTable));
        CHECK_MESSAGE(CompareCommitRows(sLog, sTable, bSteam, bHog, 8, sWhy), "commit-flat against v143-x64\\commit.txt: " << sWhy);
    }
}

TEST_CASE_NAMED(FStandardTest21, "Game::Standard::repeat-water", "[ProductFilter][Standard]")
{
    GameRun kRun("repeat-water");
    kRun.AddArg(StandardArgs("repeat-water", Mission("nab2g_c", "-repeatmission -soakturns 1501")));
    AddStandardTests(kRun);
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "repeat-water", 1501);
    std::string sWhy;
    CHECK_MESSAGE(CompareEqualDigests(ReadRunLog(kRun), 2, sWhy), "repeat-water: " << sWhy);
}

TEST_CASE_NAMED(FStandardTest22, "Game::Standard::repeat-digest", "[ProductFilter][Standard]")
{
    GameRun kRef("repeat-digest-ref-geo1c_con");
    kRef.AddArg(StandardArgs(kStandard, Mission(kStandard)));
    AddStandardTests(kRef);
    GameRun kRun("repeat-digest");
    kRun.AddArg(StandardArgs("repeat-digest", Mission(kStandard, "-repeatmission")));
    AddStandardTests(kRun);
    kRef.Start();
    kRun.Start();
    kRef.Wait();
    kRun.Wait();
    CheckStandardRun(kRef, kStandard, 0);
    CheckStandardRun(kRun, "repeat-digest", 0);
    const std::string sLog = ReadRunLog(kRun);
    std::string sWhy;
    CHECK_MESSAGE(CompareEqualDigests(sLog, 2, sWhy), "repeat-digest: " << sWhy);
    CHECK_MESSAGE(CompareTurn1500Runtime(sLog, ReadRunLog(kRef), sWhy), "repeat-digest against geo1c_con: " << sWhy);
}

TEST_CASE_NAMED(FStandardTest23, "Game::Standard::digest-detail", "[ProductFilter][Standard]")
{
    GameRun kRun("digest-detail");
    kRun.AddArg(StandardArgs("digest-detail", Mission(kStandard, "-digestdetail 4 -soakturns 1501")));
    AddStandardTests(kRun);
    kRun.AddInGameTest("Game.Sim.DigestDetail", "4");
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "digest-detail", 1501);
    std::string sRow;
    std::string sWhy;
    REQUIRE(DigestRow(kStandard, sRow));
    CHECK_MESSAGE(CompareDigestRow(ReadRunLog(kRun), sRow, sWhy), "digest-detail against the geo1c_con row: " << sWhy);
}

TEST_CASE_NAMED(FStandardTest24, "Game::Standard::save-roundtrip", "[ProductFilter][Standard]")
{
    GameRun kWrite("save-roundtrip");
    const std::string sSidecar = kWrite.RunDir() + "\\save-fields.txt";
    kWrite.AddArg(StandardArgs("save-roundtrip", StandardTail(false) + " -savetest write"));
    AddStandardTests(kWrite);
    kWrite.AddInGameTest("Game.Save.Write", sSidecar);
    kWrite.KeepWorkerFolder("SaveGames");
    kWrite.Start();
    REQUIRE(kWrite.Wait());
    const char* const apcLoads[] = {"1", "3"};
    for (int i = 0; i < 2; ++i)
    {
        GameRun kLoad((std::string("save-roundtrip-load-") + apcLoads[i]).c_str());
        kLoad.AddArg(StandardArgs("save-roundtrip", StandardTail(false) + " -savetest load " + apcLoads[i] + " SaveTest"));
        AddStandardTests(kLoad);
        kLoad.AddInGameTest("Game.Save.Load", std::string(apcLoads[i]) + "|" + sSidecar);
        kLoad.SeedWorkerFolder("SaveGames", kWrite.RunDir() + "\\SaveGames");
        kLoad.Start();
        kLoad.Wait();
    }
}

TEST_CASE_NAMED(FStandardTest25, "Game::Standard::render-sound-on", "[ProductFilter][Standard]")
{
    GameRun kRun("render-sound-on");
    kRun.AddArg(StandardArgs("render-sound-on", std::string(kStandard) + " " + RenderTail() + " -win -handsoff -soakturns 300"));
    AddStandardTests(kRun);
    PacingTest(kRun, "fps:80,vsync:off,interval:0x80000000,waitquery:0,timer:hires|period");
    kRun.AddInGameTest("Game.Render.Gamma");
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "render-sound-on", 300);
}
