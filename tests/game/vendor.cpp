#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "harness/gamerun.h"
#include "game/standard_case.h"
#include "testutil.h"

namespace
{

const char* const kStandard = "geo1c_con";

std::string Cap()
{
    char acCap[32];
    _snprintf(acCap, sizeof(acCap) - 1, " -soakturns %u", kStandardTurnCap);
    acCap[sizeof(acCap) - 1] = 0;
    return acCap;
}

std::string TurnOneArgs()
{
    return std::string(kStandard) + " " + GameRunTail(false) + " -soakturns 1";
}

std::string FastFailArgs()
{
    return std::string(kStandard) + " " + GameRunTail(false) + " -crashtest fastfail" + Cap();
}

void ExpectSummary(GameRun& kRun, const char* pcStates, const char* pcRuntimes)
{
    const std::string sRx = std::string("^Vendor ") + pcStates + R"( d3dx9=[a-z]:\\.*\\artifacts\\smoke-workers\\)" +
                            GameRun::Platform() + R"(\\[^\\]+\\[^\\]+\\w\d+\\d3dx9_43\.dll msvcr120=)" + pcRuntimes;
    kRun.AddExpectedMessage(sRx.c_str(), EAutomationExpectedMessageFlags::Contains, 1);
}

void ExpectOff(GameRun& kRun, const char* pcService, const char* pcDll, const char* pcWhy)
{
    const std::string sRx = std::string("^Vendor ") + pcService + " off: " + pcDll + " " + pcWhy;
    kRun.AddExpectedMessage(sRx.c_str(), RedLog::Warning, EAutomationExpectedMessageFlags::Contains, 1);
}

void ExpectSteamInterfaces(GameRun& kRun)
{
    static const char* const sapcInterfaces[] = {"SteamClient", "SteamUser", "SteamFriends", "SteamUtils"};
    for (size_t i = 0; i < sizeof(sapcInterfaces) / sizeof(sapcInterfaces[0]); ++i)
    {
        const std::string sRx = std::string("^Steam iface=") + sapcInterfaces[i] + " ok$";
        kRun.AddExpectedMessage(sRx.c_str(), EAutomationExpectedMessageFlags::Contains, 1);
    }
}

void NeedVs2013Runtime()
{
    char acSystem[MAX_PATH];
    const UINT uLen = GetSystemDirectoryA(acSystem, sizeof(acSystem));
    if (uLen == 0 || uLen >= sizeof(acSystem))
    {
        HarnessError("cannot read the system folder");
    }
    static const char* const sapcRuntimes[] = {"msvcr120.dll", "msvcp120.dll"};
    for (size_t i = 0; i < sizeof(sapcRuntimes) / sizeof(sapcRuntimes[0]); ++i)
    {
        if (GetFileAttributesA((std::string(acSystem) + "\\" + sapcRuntimes[i]).c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            SkipCase("VS2013 x86 runtime not installed");
        }
    }
}

void FinishTurnOne(GameRun& kRun)
{
    kRun.Start();
    kRun.Wait();
    CHECK_MESSAGE(kRun.Result().ProgressTurns >= 1, "progress turns " << kRun.Result().ProgressTurns << " below -soakturns 1");
}

void FinishFastFail(GameRun& kRun)
{
    kRun.ExpectExitCode((int)0xC0000409u);
    kRun.ExpectEndsEarly();
    kRun.CollectWerDump();
    kRun.ExpectDumps(GameRun::WerLocalDumps() ? 1 : 0);
    kRun.Start();
    kRun.Wait();
}

std::string GameDataDll(const char* pcName)
{
    const char* pcRoot = FindGameDataRoot();
    return std::string(pcRoot ? pcRoot : "GameData-not-found") + "\\" + pcName;
}

}

TEST_CASE_NAMED(FVendorTest1, "Game::Vendor::vendor-present", "[ProductFilter][Vendor]")
{
    NeedVs2013Runtime();
    GameRun kRun("vendor-present");
    kRun.AddArg(TurnOneArgs());
    ExpectSummary(kRun, "steam=on galaxy=on", "1 msvcp120=1$");
    ExpectSteamInterfaces(kRun);
    FinishTurnOne(kRun);
}

TEST_CASE_NAMED(FVendorTest2, "Game::Vendor::vendor-steam-missing", "[ProductFilter][Vendor]")
{
    GameRun kRun("vendor-steam-missing");
    kRun.AddArg(TurnOneArgs());
    kRun.OmitDll("steam_api.dll");
    ExpectOff(kRun, "steam", R"(steam_api\.dll)", "missing");
    ExpectSummary(kRun, "steam=off galaxy=on", "");
    FinishTurnOne(kRun);
}

TEST_CASE_NAMED(FVendorTest3, "Game::Vendor::vendor-galaxy-missing", "[ProductFilter][Vendor]")
{
    GameRun kRun("vendor-galaxy-missing");
    kRun.AddArg(TurnOneArgs());
    kRun.OmitDll("Galaxy.dll");
    ExpectOff(kRun, "galaxy", R"(Galaxy\.dll)", "missing");
    ExpectSummary(kRun, "steam=on galaxy=off", "0 msvcp120=0$");
    FinishTurnOne(kRun);
}

TEST_CASE_NAMED(FVendorTest4, "Game::Vendor::vendor-vcrt-off", "[ProductFilter][Vendor]")
{
    NeedVs2013Runtime();
    GameRun kRun("vendor-vcrt-off");
    kRun.AddArg(TurnOneArgs());
    kRun.OmitDll("Galaxy.dll");
    ExpectOff(kRun, "galaxy", R"(Galaxy\.dll)", "missing");
    ExpectSummary(kRun, "steam=on galaxy=off", "0 msvcp120=0$");
    FinishTurnOne(kRun);
}

TEST_CASE_NAMED(FVendorTest5, "Game::Vendor::vendor-galaxypeer-missing", "[ProductFilter][Vendor]")
{
    GameRun kRun("vendor-galaxypeer-missing");
    kRun.AddArg(TurnOneArgs());
    kRun.OmitDll("GalaxyPeer.dll");
    ExpectOff(kRun, "galaxy", R"(GalaxyPeer\.dll)", "missing");
    ExpectSummary(kRun, "steam=on galaxy=off", "");
    FinishTurnOne(kRun);
}

TEST_CASE_NAMED(FVendorTest6, "Game::Vendor::vendor-steam-wrongbits", "[ProductFilter][Vendor]")
{
    const char* pcVendor = getenv("SWBF2_VENDOR_X64");
    if (!pcVendor || !*pcVendor)
    {
        SkipCase("SWBF2_VENDOR_X64 not set");
    }
    const std::string sSteam64 = std::string(pcVendor) + "\\steam_api64.dll";
    if (GetFileAttributesA(sSteam64.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        SkipCase("steam_api64.dll not in SWBF2_VENDOR_X64");
    }
    GameRun kRun("vendor-steam-wrongbits");
    kRun.AddArg(TurnOneArgs());
    kRun.SwapDll("steam_api.dll", sSteam64);
    ExpectOff(kRun, "steam", R"(steam_api\.dll)", "wrong bitness");
    ExpectSummary(kRun, "steam=off galaxy=on", "");
    FinishTurnOne(kRun);
}

TEST_CASE_NAMED(FVendorTest7, "Game::Vendor::vendor-wan-nogalaxy", "[ProductFilter][Vendor]")
{
    GameRun kRun("vendor-wan-nogalaxy");
    kRun.AddArg(std::string("/autonet dedicated -nopregame -playercount 0 ") + kStandard + " 100 100 " + GameRunTail(false) +
                Cap());
    kRun.OmitDll("Galaxy.dll");
    kRun.ExpectFatal();
    kRun.ExpectExitCode(1);
    kRun.ExpectDumps(0);
    ExpectOff(kRun, "galaxy", R"(Galaxy\.dll)", "missing");
    ExpectSummary(kRun, R"(steam=\w+ galaxy=\w+)", "");
    kRun.AddExpectedMessage(R"(^Galaxy login skipped: Online lobbies are not available because Galaxy could not be loaded\. LAN and direct IP games work\.$)",
                            EAutomationExpectedMessageFlags::Contains, 1);
    kRun.AddExpectedMessage("^FATAL: online dedicated server needs Galaxy", EAutomationExpectedMessageFlags::Contains, 1);
    kRun.AddExpectedError("^online dedicated server needs Galaxy; use -lan for LAN and direct IP$",
                          EAutomationExpectedMessageFlags::Contains, 1);
    kRun.Start();
    kRun.Wait();
    CHECK(LineCount(kRun.Result(), "LogNet", RedLog::Fatal,
                    "^online dedicated server needs Galaxy; use -lan for LAN and direct IP$") == 1);
}

TEST_CASE_NAMED(FVendorTest8, "Game::Vendor::vendor-x64-none", "[ProductFilter][Vendor]")
{
    GameRun kRun("vendor-x64-none");
    kRun.AddArg(FastFailArgs());
    kRun.OmitDll("steam_api64.dll");
    kRun.OmitDll("Galaxy64.dll");
    kRun.OmitDll("GalaxyPeer64.dll");
    ExpectOff(kRun, "steam", R"(steam_api64\.dll)", "missing");
    ExpectOff(kRun, "galaxy", R"(Galaxy64\.dll)", "missing");
    ExpectSummary(kRun, "steam=off galaxy=off", "0 msvcp120=0$");
    FinishFastFail(kRun);
}

TEST_CASE_NAMED(FVendorTest9, "Game::Vendor::vendor-x64-steam-wrongbits", "[ProductFilter][Vendor]")
{
    GameRun kRun("vendor-x64-steam-wrongbits");
    kRun.AddArg(FastFailArgs());
    kRun.SwapDll("steam_api64.dll", GameDataDll("steam_api.dll"));
    ExpectOff(kRun, "steam", R"(steam_api64\.dll)", "wrong bitness");
    ExpectSummary(kRun, R"(steam=\w+ galaxy=off)", "0 msvcp120=0$");
    FinishFastFail(kRun);
}

TEST_CASE_NAMED(FVendorTest10, "Game::Vendor::vendor-x64-galaxy-wrongbits", "[ProductFilter][Vendor]")
{
    GameRun kRun("vendor-x64-galaxy-wrongbits");
    kRun.AddArg(FastFailArgs());
    kRun.SwapDll("Galaxy64.dll", GameDataDll("Galaxy.dll"));
    ExpectOff(kRun, "galaxy", R"(Galaxy64\.dll)", "wrong bitness");
    ExpectSummary(kRun, R"(steam=\w+ galaxy=off)", "0 msvcp120=0$");
    FinishFastFail(kRun);
}

TEST_CASE_NAMED(FVendorTest11, "Game::Vendor::vendor-x64-steam-on", "[ProductFilter][Vendor]")
{
    if (GetFileAttributesA((GameRun::GameDir() + "\\steam_api64.dll").c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        SkipCase("SWBF2_VENDOR_X64 not set");
    }
    GameRun kRun("vendor-x64-steam-on");
    kRun.AddArg(FastFailArgs());
    kRun.OmitDll("Galaxy64.dll");
    kRun.OmitDll("GalaxyPeer64.dll");
    ExpectOff(kRun, "galaxy", R"(Galaxy64\.dll)", "missing");
    ExpectSummary(kRun, "steam=on galaxy=off", "0 msvcp120=0$");
    ExpectSteamInterfaces(kRun);
    FinishFastFail(kRun);
}
