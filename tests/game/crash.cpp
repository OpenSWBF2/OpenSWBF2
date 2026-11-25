#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stdio.h>
#include <string>
#include <vector>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"
#include "testutil.h"

namespace
{

const char* const kStandard = "geo1c_con";

bool IsX64()
{
    return GameRun::Platform() == "x64";
}

std::string Cap()
{
    char acCap[32];
    _snprintf(acCap, sizeof(acCap) - 1, " -soakturns %u", kStandardTurnCap);
    acCap[sizeof(acCap) - 1] = 0;
    return acCap;
}

std::string CrashArgs(const char* pcKind, const char* pcMission = kStandard, const std::string& sBefore = std::string())
{
    return std::string(pcMission) + " " + GameRunTail(false) + (sBefore.empty() ? "" : " " + sBefore) + " -crashtest " +
           pcKind;
}

void ExpectCrash(GameRun& kRun, int iCode, int iDumps)
{
    kRun.ExpectFatal();
    kRun.ExpectExitCode(iCode);
    kRun.ExpectDumps(iDumps);
    if (iDumps > 0)
    {
        kRun.ExpectDumpMatchesFatal();
    }
}

void ExpectFatalLine(GameRun& kRun, const char* pcRegex)
{
    kRun.AddExpectedMessage(pcRegex, EAutomationExpectedMessageFlags::Contains, 1);
}

void Finish(GameRun& kRun, const char* pcRow)
{
    kRun.Start();
    kRun.Wait();
    CheckGroupRows(kRun, pcRow);
}

void FinishPass(GameRun& kRun, const char* pcRow)
{
    Finish(kRun, pcRow);
    CHECK_MESSAGE(kRun.Result().ProgressTurns < kStandardTurnCap,
                  pcRow << ": the run reached the turn cap " << kStandardTurnCap);
}

void RunFault(const char* pcName, const char* pcKind)
{
    GameRun kRun(pcName);
    kRun.AddArg(CrashArgs(pcKind) + Cap());
    ExpectCrash(kRun, 3, 1);
    ExpectFatalLine(kRun, "^FATAL: .*\\.dmp");
    Finish(kRun, pcName);
}

void RunFatalAtStart(const char* pcName, const char* pcKind)
{
    GameRun kRun(pcName);
    kRun.AddArg(CrashArgs(pcKind) + Cap());
    ExpectCrash(kRun, 3, 1);
    ExpectFatalLine(kRun, "^FATAL: exception 0xC0000005 at 0x[0-9A-F]+ \\(BattlefrontII\\.exe\\+0x[0-9A-F]+\\) build [0-9A-F]+ dump .*\\.dmp");
    Finish(kRun, pcName);
    const GameRunResult& r = kRun.Result();
    const int iFatal = LineIndex(r, "FATAL:");
    REQUIRE(iFatal >= 0);
    const int iOpened = LineIndex(r, "Opened logfile");
    const int iStart = LineIndex(r, "log start");
    CHECK_MESSAGE((iOpened < 0 || iOpened > iFatal), pcName << ": the log was opened before the fault");
    CHECK_MESSAGE((iStart < 0 || iStart > iFatal), pcName << ": the log started before the fault");
}

void RunOutOfMemory(const char* pcName, const char* pcKind, const char* pcFatal)
{
    GameRun kRun(pcName);
    kRun.AddArg(CrashArgs(pcKind) + Cap());
    ExpectCrash(kRun, 1, 0);
    ExpectFatalLine(kRun, pcFatal);
    Finish(kRun, pcName);
}

void RunScenario(const char* pcName, const char* pcKind, const char* pcMission = kStandard)
{
    GameRun kRun(pcName);
    kRun.AddArg(CrashArgs(pcKind, pcMission) + Cap());
    kRun.AddInGameTest("Game.Memory.MiOptions");
    kRun.AddInGameTest("Game.Crash.Scenario");
    FinishPass(kRun, pcName);
}

void ScenarioIni(GameRun& kRun, const char* pcKind)
{
    kRun.AddArg(CrashArgs(pcKind));
    kRun.SetDisplayIni("{CASEDIR}\\display.ini");
    kRun.AddArg(Cap());
    kRun.AddInGameTest("Game.Memory.MiOptions");
    kRun.AddInGameTest("Game.Crash.Scenario");
}

void CheckNoKeys(const std::string& sIni, const char* const* ppcKeys, size_t uKeys)
{
    for (size_t i = 0; i < uKeys; ++i)
    {
        CHECK_MESSAGE(!IniHasKey(sIni, ppcKeys[i]), "display.ini holds " << ppcKeys[i] << "=");
    }
}

}

TEST_CASE_NAMED(FCrashTest1, "Game::Crash::crash-nullderef", "[ProductFilter][Crash]")
{
    if (!IsX64())
    {
        RunFault("crash-nullderef", "nullderef");
        return;
    }
    GameRun kRun("crash-nullderef");
    kRun.AddArg(CrashArgs("nullderef") + " -hogaddrspace" + Cap());
    ExpectCrash(kRun, 3, 1);
    ExpectFatalLine(kRun, "^FATAL: exception 0xC0000005 at 0x[0-9A-F]{9,16} \\(BattlefrontII\\.exe\\+0x[0-9A-F]+\\) build [0-9A-F]+ dump .*\\.dmp");
    Finish(kRun, "crash-nullderef");
}

TEST_CASE_NAMED(FCrashTest2, "Game::Crash::crash-divzero", "[ProductFilter][Crash]")
{
    RunFault("crash-divzero", "divzero");
}

TEST_CASE_NAMED(FCrashTest3, "Game::Crash::crash-stackoverflow", "[ProductFilter][Crash]")
{
    RunFault("crash-stackoverflow", "stackoverflow");
}

TEST_CASE_NAMED(FCrashTest4, "Game::Crash::crash-debugbreak", "[ProductFilter][Crash]")
{
    RunFault("crash-debugbreak", "debugbreak");
}

TEST_CASE_NAMED(FCrashTest5, "Game::Crash::crash-abort", "[ProductFilter][Crash]")
{
    RunFault("crash-abort", "abort");
}

TEST_CASE_NAMED(FCrashTest6, "Game::Crash::crash-invalidparam", "[ProductFilter][Crash]")
{
    RunFault("crash-invalidparam", "invalidparam");
}

TEST_CASE_NAMED(FCrashTest7, "Game::Crash::crash-fastfail", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-fastfail");
    kRun.AddArg(CrashArgs("fastfail") + Cap());
    kRun.ExpectExitCode((int)0xC0000409u);
    kRun.ExpectEndsEarly();
    kRun.CollectWerDump();
    kRun.ExpectDumps(GameRun::WerLocalDumps() ? 1 : 0);
    Finish(kRun, "crash-fastfail");
}

TEST_CASE_NAMED(FCrashTest8, "Game::Crash::crash-delayload", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-delayload");
    kRun.AddArg(CrashArgs("delayload") + Cap());
    kRun.OmitDll("steam_api.dll");
    ExpectCrash(kRun, 3, 1);
    ExpectFatalLine(kRun, "^FATAL: exception 0xC06D007E at 0x[0-9A-F]+ \\([^)]+\\) dll=steam_api\\.dll build .*\\.dmp");
    kRun.AddExpectedMessage("^Vendor steam off: steam_api\\.dll missing", RedLog::Warning,
                            EAutomationExpectedMessageFlags::Contains, 1);
    Finish(kRun, "crash-delayload");
}

TEST_CASE_NAMED(FCrashTest9, "Game::Crash::crash-delayload-x64", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-delayload-x64");
    kRun.AddArg(CrashArgs("delayload") + Cap());
    kRun.OmitDll("steam_api64.dll");
    ExpectCrash(kRun, 3, 1);
    ExpectFatalLine(kRun, "^FATAL: exception 0xC06D007E at 0x[0-9A-F]+ \\([^)]+\\) dll=steam_api64\\.dll build .*\\.dmp");
    kRun.AddExpectedMessage("^Vendor steam off: steam_api64\\.dll missing", RedLog::Warning,
                            EAutomationExpectedMessageFlags::Contains, 1);
    Finish(kRun, "crash-delayload");
}

TEST_CASE_NAMED(FCrashTest10, "Game::Crash::crash-early", "[ProductFilter][Crash]")
{
    RunFatalAtStart("crash-early", "early");
}

TEST_CASE_NAMED(FCrashTest11, "Game::Crash::crash-heapinit", "[ProductFilter][Crash]")
{
    RunFatalAtStart("crash-heapinit", "heapinit");
}

TEST_CASE_NAMED(FCrashTest12, "Game::Crash::fatal-init", "[ProductFilter][Crash]")
{
    GameRun kRun("fatal-init");
    kRun.AddArg(std::string("no_such_mission ") + GameRunTail(false) + Cap());
    ExpectCrash(kRun, 1, 0);
    ExpectFatalLine(kRun, "^FATAL: initialization failed");
    kRun.AddExpectedError("^Could not open MISSION\\\\no_such_mission\\.lvl$", EAutomationExpectedMessageFlags::Contains, 1);
    Finish(kRun, "fatal-init");
    CHECK(LineCount(kRun.Result(), "LogLoad", RedLog::Fatal, "^Could not open MISSION\\\\no_such_mission\\.lvl$") == 1);
}

TEST_CASE_NAMED(FCrashTest13, "Game::Crash::crash-arenaoom", "[ProductFilter][Crash]")
{
    RunOutOfMemory("crash-arenaoom", "arenaoom",
                   "^FATAL: out of memory allocating application heap \\(\\d+ bytes\\) load \\d+% phys \\d+ MB virt \\d+ MB");
}

TEST_CASE_NAMED(FCrashTest14, "Game::Crash::crash-debugheapoom", "[ProductFilter][Crash]")
{
    RunOutOfMemory("crash-debugheapoom", "debugheapoom",
                   "^FATAL: out of memory allocating debug heap \\(\\d+ bytes\\) load \\d+% phys \\d+ MB virt \\d+ MB");
}

TEST_CASE_NAMED(FCrashTest15, "Game::Crash::crash-buildheapfail", "[ProductFilter][Crash]")
{
    RunOutOfMemory("crash-buildheapfail", "buildheapfail",
                   IsX64() ? "^FATAL: out of memory allocating Lua heap \\(22478848 bytes\\) load \\d+% phys \\d+ MB virt \\d+ MB"
                           : "^FATAL: out of memory allocating Lua heap \\(16777216 bytes\\) load \\d+% phys \\d+ MB virt \\d+ MB");
}

TEST_CASE_NAMED(FCrashTest16, "Game::Crash::crash-heapfail", "[ProductFilter][Crash]")
{
    const char* const pcLine = "^alloc failure heap Runtime \\(\\d+\\) size 268435456 free \\d+ load \\d+% phys \\d+ MB virt "
                               "\\d+ MB used \\d+ cap \\d+ peak \\d+ largestfree \\d+$";
    GameRun kRun("crash-heapfail");
    kRun.AddArg(CrashArgs("heapfail") + Cap());
    kRun.AddInGameTest("Game.Memory.MiOptions");
    kRun.AddExpectedError(pcLine, EAutomationExpectedMessageFlags::Contains, 1);
    FinishPass(kRun, "crash-heapfail");
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Error, pcLine) == 1);
}

TEST_CASE_NAMED(FCrashTest17, "Game::Crash::crash-capexhaust", "[ProductFilter][Crash]")
{
    const char* const pcLine = "^alloc failure heap Runtime \\(\\d+\\) size 1048576 .* cap 536870912 ";
    GameRun kRun("crash-capexhaust");
    kRun.AddArg(CrashArgs("capexhaust", kStandard, "-heapcap Runtime=512M") + Cap());
    ExpectCrash(kRun, 3, 1);
    ExpectFatalLine(kRun, "^FATAL: ");
    kRun.AddExpectedError(pcLine, EAutomationExpectedMessageFlags::Contains, 1);
    Finish(kRun, "crash-capexhaust");
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Error, pcLine) == 1);
}

TEST_CASE_NAMED(FCrashTest18, "Game::Crash::crash-midoublefree", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-midoublefree");
    kRun.AddArg(CrashArgs("midoublefree") + Cap());
    ExpectCrash(kRun, 3, 1);
    ExpectFatalLine(kRun, "^FATAL: ");
    kRun.AddExpectedError("^Mimalloc error err=11$", EAutomationExpectedMessageFlags::Contains, 1);
    Finish(kRun, "crash-midoublefree");
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Error, "^Mimalloc error err=11$") == 1);
}

TEST_CASE_NAMED(FCrashTest19, "Game::Crash::crash-poolcount", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-poolcount");
    kRun.AddArg(CrashArgs("poolcount") + Cap());
    kRun.AddInGameTest("Game.Memory.MiOptions");
    FinishPass(kRun, "crash-poolcount");
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Warning,
                    "^SetMemoryPoolSize\\(crashtestpool\\): script value 2147483647 clamped to \\d+") == 1);
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Warning,
                    "^Memory pool \"crashtesthuge\" refused count 1073741825 size 16") == 1);
}

TEST_CASE_NAMED(FCrashTest20, "Game::Crash::pool-full", "[ProductFilter][Crash]")
{
    GameRun kRun("pool-full");
    kRun.AddArg(CrashArgs("poolfull") + Cap());
    kRun.AddInGameTest("Game.Memory.MiOptions");
    FinishPass(kRun, "pool-full");
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Warning,
                    "^Memory pool \"crashtestpool\" is full; raise count to at least 3$") == 1);
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Error, "crashtestpool") == 0);
}

TEST_CASE_NAMED(FCrashTest21, "Game::Crash::pool-full-strict", "[ProductFilter][Crash]")
{
    const char* const pcLine = "^Memory pool \"crashtestpool\" is full; raise count to at least 3$";
    std::string sPeaks;
    REQUIRE(ReadRepoText(IsX64() ? "tests\\baseline\\v143-x64\\mem-peaks.txt" : "tests\\baseline\\v143\\mem-peaks.txt", sPeaks));
    const std::vector<std::string> kGrown = GrownPools(sPeaks, kStandard);
    REQUIRE_MESSAGE(!kGrown.empty(), "the pinned mem-peaks rows name no grown pool for " << kStandard);
    GameRun kRun("pool-full-strict");
    kRun.AddArg(CrashArgs("poolfull") + " -poolerrors" + Cap());
    kRun.AddInGameTest("Game.Memory.MiOptions");
    kRun.AddExpectedError(pcLine, EAutomationExpectedMessageFlags::Contains, 1);
    std::vector<std::string> kFull;
    for (size_t i = 0; i < kGrown.size(); ++i)
    {
        kFull.push_back("Memory pool \"" + kGrown[i] + "\" is full; raise count to at least ");
        kRun.AddExpectedErrorPlain(kFull.back().c_str(), EAutomationExpectedMessageFlags::Contains, -1);
    }
    FinishPass(kRun, "pool-full-strict");
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Error, pcLine) == 1);
    for (size_t i = 0; i < kFull.size(); ++i)
    {
        int iCount = 0;
        for (size_t j = 0; j < kRun.Result().Lines.size(); ++j)
        {
            const LogLine& kLine = kRun.Result().Lines[j];
            iCount += kLine.bParsed && kLine.iSeverity == RedLog::Error && kLine.sMessage.compare(0, kFull[i].size(), kFull[i]) == 0;
        }
        CHECK_MESSAGE(iCount <= 1, kGrown[i] << " logged its full line at Error " << iCount << " times");
    }
}

TEST_CASE_NAMED(FCrashTest22, "Game::Crash::crash-stickdelete", "[ProductFilter][Crash]")
{
    RunScenario("crash-stickdelete", "stickdelete", "spa3c_ass");
}

TEST_CASE_NAMED(FCrashTest23, "Game::Crash::crash-particlesound", "[ProductFilter][Crash]")
{
    RunScenario("crash-particlesound", "particlesound");
}

TEST_CASE_NAMED(FCrashTest24, "Game::Crash::crash-particleburst", "[ProductFilter][Crash]")
{
    RunScenario("crash-particleburst", "particleburst");
}

TEST_CASE_NAMED(FCrashTest25, "Game::Crash::crash-luaoom", "[ProductFilter][Crash]")
{
    const std::string sSize = IsX64() ? "67108896" : "67108880";
    const std::string sHeap = "^alloc failure heap Lua \\(\\d+\\) size " + sSize + " ";
    const std::string sLua = "^Lua out of memory \\(\\d+ used in \\d+ blocks, trying to alloc " + sSize + "\\)$";
    GameRun kRun("crash-luaoom");
    kRun.AddArg(CrashArgs("luaoom") + Cap());
    kRun.AddInGameTest("Game.Memory.MiOptions");
    kRun.AddExpectedError(sHeap.c_str(), EAutomationExpectedMessageFlags::Contains, 1);
    kRun.AddExpectedError(sLua.c_str(), EAutomationExpectedMessageFlags::Contains, 1);
    FinishPass(kRun, "crash-luaoom");
    CHECK(LineCount(kRun.Result(), "LogMemory", RedLog::Error, sHeap.c_str()) == 1);
    CHECK(LineCount(kRun.Result(), "LogLua", RedLog::Error, sLua.c_str()) == 1);
}

TEST_CASE_NAMED(FCrashTest26, "Game::Crash::crash-luapanic", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-luapanic");
    kRun.AddArg(CrashArgs("luapanic") + Cap());
    ExpectCrash(kRun, 4, 0);
    ExpectFatalLine(kRun, "^FATAL: lua panic");
    kRun.AddExpectedError("^Lua panic: crashtest$", EAutomationExpectedMessageFlags::Contains, 1);
    Finish(kRun, "crash-luapanic");
    CHECK(LineCount(kRun.Result(), "LogLua", RedLog::Fatal, "^Lua panic: crashtest$") == 1);
}

TEST_CASE_NAMED(FCrashTest27, "Game::Crash::crash-hang", "[ProductFilter][Crash]")
{
    std::string sTail = GameRunTail(false);
    sTail.erase(sTail.find(" -hangtimeout 0"), 15);
    GameRun kRun("crash-hang");
    kRun.AddArg(std::string(kStandard) + " " + sTail + " -crashtest hang -hangtimeout 5" + Cap());
    ExpectCrash(kRun, 5, 1);
    ExpectFatalLine(kRun, "^FATAL: hang");
    Finish(kRun, "crash-hang");
}

TEST_CASE_NAMED(FCrashTest28, "Game::Crash::crash-stall", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-stall");
    kRun.AddArg(CrashArgs("hang") + Cap());
    ExpectCrash(kRun, 5, 1);
    ExpectFatalLine(kRun, "^FATAL: hang \\(supervisor saw no progress");
    Finish(kRun, "crash-stall");
    CHECK_MESSAGE(kRun.Result().Stalled, "the runner did not flag the stall");
}

TEST_CASE_NAMED(FCrashTest29, "Game::Crash::crash-check", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-check");
    kRun.AddArg(CrashArgs("check") + Cap());
    ExpectCrash(kRun, 3, 1);
    ExpectFatalLine(kRun, "^FATAL: check failed");
    kRun.AddExpectedMessage("^ensure summary: 1 failures at 1 sites$", EAutomationExpectedMessageFlags::Contains, 1);
    kRun.AddExpectedError("^ensure failed: false$", EAutomationExpectedMessageFlags::Contains, 1);
    Finish(kRun, "crash-check");
    CHECK(LineCount(kRun.Result(), "LogCore", RedLog::Error, "^ensure failed: false$") == 1);
}

TEST_CASE_NAMED(FCrashTest30, "Game::Crash::crash-resetinitlost", "[ProductFilter][Crash]")
{
    RunScenario("crash-resetinitlost", "resetinitlost");
}

TEST_CASE_NAMED(FCrashTest31, "Game::Crash::crash-resetrecover", "[ProductFilter][Crash]")
{
    RunScenario("crash-resetrecover", "resetrecover");
}

TEST_CASE_NAMED(FCrashTest32, "Game::Crash::crash-resetflap", "[ProductFilter][Crash]")
{
    RunScenario("crash-resetflap", "resetflap");
}

TEST_CASE_NAMED(FCrashTest33, "Game::Crash::crash-resetlaterinit", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-resetlaterinit");
    kRun.AddArg(CrashArgs("resetlaterinit") + Cap());
    kRun.ExpectEndsEarly();
    Finish(kRun, "crash-resetlaterinit");
    const GameRunResult& r = kRun.Result();
    const int iRunning = LineIndex(r, "Mission running");
    const int iClose = iRunning < 0 ? -1 : LineIndex(r, "Close deferred until loading finishes", iRunning + 1);
    const int iReset = iClose < 0 ? -1 : LineIndex(r, "Reset recovered after 2 attempts", iClose + 1);
    CHECK_MESSAGE(iReset >= 0, "the mission ran, the close waited for loading and the reset recovered after 2 attempts, in order");
}

TEST_CASE_NAMED(FCrashTest34, "Game::Crash::crash-resetfatal", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-resetfatal");
    kRun.AddArg(CrashArgs("resetfatal") + Cap());
    ExpectCrash(kRun, 1, 1);
    ExpectFatalLine(kRun, "^FATAL: device reset failed hr=0x8876086C");
    kRun.AddExpectedMessage("^Reset reverting asked=\\d+x\\d+ windowed=\\d used=\\d+x\\d+ windowed=\\d", RedLog::Warning,
                            EAutomationExpectedMessageFlags::Contains, 1);
    kRun.AddExpectedError("^device reset failed$", EAutomationExpectedMessageFlags::Contains, 1);
    Finish(kRun, "crash-resetfatal");
    const GameRunResult& r = kRun.Result();
    CHECK(LineCount(r, "LogRender", RedLog::Fatal, "^device reset failed$") == 1);
    const int iRevert = LineIndex(r, "Reset reverting asked=");
    const int iFatal = LineIndex(r, "FATAL: device reset failed");
    CHECK_MESSAGE((iRevert >= 0 && iRevert < iFatal), "the reset reverted before the fatal reset failure");
    int iFatalLines = 0;
    for (size_t i = 0; i < r.Lines.size(); ++i)
    {
        iFatalLines += r.Lines[i].sText.find("FATAL:") != std::string::npos ? 1 : 0;
    }
    CHECK(iFatalLines == 1);
}

TEST_CASE_NAMED(FCrashTest35, "Game::Crash::crash-applymode", "[ProductFilter][Crash]")
{
    RunScenario("crash-applymode", "applymode");
}

TEST_CASE_NAMED(FCrashTest36, "Game::Crash::crash-applymodesmall", "[ProductFilter][Crash]")
{
    RunScenario("crash-applymodesmall", "applymodesmall");
}

TEST_CASE_NAMED(FCrashTest37, "Game::Crash::crash-applyfail", "[ProductFilter][Crash]")
{
    RunScenario("crash-applyfail", "applyfail");
}

TEST_CASE_NAMED(FCrashTest38, "Game::Crash::crash-applypresentfail", "[ProductFilter][Crash]")
{
    RunScenario("crash-applypresentfail", "applypresentfail");
}

TEST_CASE_NAMED(FCrashTest39, "Game::Crash::crash-applytargetfail", "[ProductFilter][Crash]")
{
    RunScenario("crash-applytargetfail", "applytargetfail");
}

TEST_CASE_NAMED(FCrashTest40, "Game::Crash::crash-gammapass", "[ProductFilter][Crash]")
{
    RunScenario("crash-gammapass", "gammapass");
}

TEST_CASE_NAMED(FCrashTest41, "Game::Crash::crash-gammafallback", "[ProductFilter][Crash]")
{
    RunScenario("crash-gammafallback", "gammafallback");
}

TEST_CASE_NAMED(FCrashTest42, "Game::Crash::crash-gammalost", "[ProductFilter][Crash]")
{
    RunScenario("crash-gammalost", "gammalost");
}

TEST_CASE_NAMED(FCrashTest43, "Game::Crash::crash-shellstretch", "[ProductFilter][Crash]")
{
    RunScenario("crash-shellstretch", "shellstretch");
}

TEST_CASE_NAMED(FCrashTest44, "Game::Crash::crash-gammadrawfail", "[ProductFilter][Crash]")
{
    RunScenario("crash-gammadrawfail", "gammadrawfail");
}

TEST_CASE_NAMED(FCrashTest45, "Game::Crash::crash-applyadapter", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-applyadapter");
    ScenarioIni(kRun, "applyadapter");
    FinishPass(kRun, "crash-applyadapter");
    CHECK_MESSAGE(kRun.Result().DisplayIni.empty(), "display.ini was written");
}

TEST_CASE_NAMED(FCrashTest46, "Game::Crash::crash-adaptercancel", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-adaptercancel");
    ScenarioIni(kRun, "adaptercancel");
    FinishPass(kRun, "crash-adaptercancel");
    CHECK_MESSAGE(kRun.Result().DisplayIni.empty(), "display.ini was written");
}

TEST_CASE_NAMED(FCrashTest47, "Game::Crash::crash-adapteraccept", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-adapteraccept");
    ScenarioIni(kRun, "adapteraccept");
    FinishPass(kRun, "crash-adapteraccept");
    const std::string& sIni = kRun.Result().DisplayIni;
    CHECK(IniHasLine(sIni, "Adapter=0"));
    CHECK(sIni.find("Adapter=1") == std::string::npos);
    static const char* const sapcKeys[] = {"Vsync"};
    CheckNoKeys(sIni, sapcKeys, 1);
}

TEST_CASE_NAMED(FCrashTest48, "Game::Crash::crash-menuconfirm", "[ProductFilter][Crash]")
{
    RunScenario("crash-menuconfirm", "menuconfirm");
}

TEST_CASE_NAMED(FCrashTest49, "Game::Crash::crash-menukeep", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-menukeep");
    ScenarioIni(kRun, "menukeep");
    FinishPass(kRun, "crash-menukeep");
    const std::string& sIni = kRun.Result().DisplayIni;
    CHECK(IniHasLine(sIni, "Vsync=2"));
    static const char* const sapcKeys[] = {"Fps", "Refresh", "NoWaitQuery", "Borderless"};
    CheckNoKeys(sIni, sapcKeys, 4);
}

TEST_CASE_NAMED(FCrashTest50, "Game::Crash::crash-menusavefail", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-menusavefail");
    ScenarioIni(kRun, "menusavefail");
    FinishPass(kRun, "crash-menusavefail");
    static const char* const sapcKeys[] = {"Vsync"};
    CheckNoKeys(kRun.Result().DisplayIni, sapcKeys, 1);
}

TEST_CASE_NAMED(FCrashTest51, "Game::Crash::crash-menuorder", "[ProductFilter][Crash]")
{
    RunScenario("crash-menuorder", "menuorder");
}

TEST_CASE_NAMED(FCrashTest52, "Game::Crash::crash-menuorderrev", "[ProductFilter][Crash]")
{
    RunScenario("crash-menuorderrev", "menuorderrev");
}

TEST_CASE_NAMED(FCrashTest53, "Game::Crash::crash-menurevertsave", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-menurevertsave");
    ScenarioIni(kRun, "menurevertsave");
    FinishPass(kRun, "crash-menurevertsave");
    const std::string& sIni = kRun.Result().DisplayIni;
    CHECK((IniHasLine(sIni, "Fps=30") || IniHasLine(sIni, "Fps=60")));
    static const char* const sapcKeys[] = {"Vsync", "Borderless", "Refresh"};
    CheckNoKeys(sIni, sapcKeys, 3);
}

TEST_CASE_NAMED(FCrashTest54, "Game::Crash::crash-menuapplyfail", "[ProductFilter][Crash]")
{
    GameRun kRun("crash-menuapplyfail");
    ScenarioIni(kRun, "menuapplyfail");
    FinishPass(kRun, "crash-menuapplyfail");
    const std::string& sIni = kRun.Result().DisplayIni;
    CHECK((IniHasLine(sIni, "Fps=30") || IniHasLine(sIni, "Fps=60")));
    static const char* const sapcKeys[] = {"Vsync", "Borderless", "Refresh", "NoWaitQuery"};
    CheckNoKeys(sIni, sapcKeys, 4);
}
