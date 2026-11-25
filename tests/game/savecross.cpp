#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"
#include "testutil.h"

namespace
{

const char* const kPlatforms[] = {"Win32", "x64"};

std::string GameDir(const char* pcPlatform)
{
    return RepoRoot() + "\\bin\\" + pcPlatform + "\\Test";
}

std::string Tail(const char* pcPlatform, const std::string& sSaveTest)
{
    char acCap[32];
    _snprintf(acCap, sizeof(acCap) - 1, " -soakturns %u", kStandardTurnCap);
    acCap[sizeof(acCap) - 1] = 0;
    return GameRunTail(false) + (strcmp(pcPlatform, "x64") == 0 ? " -hogaddrspace" : "") + " " + sSaveTest + acCap;
}

void AddPlatformTests(GameRun& kRun, const char* pcPlatform)
{
    kRun.AddInGameTest("Game.Memory.MiOptions");
    if (strcmp(pcPlatform, "x64") == 0)
    {
        kRun.AddInGameTest("Boot.AddrSpace");
    }
}

bool ReadAll(const std::string& sPath, std::string& sText)
{
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return false;
    }
    char acBuf[4096];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    return true;
}

bool SidecarName(const std::string& sSidecar, int iType, std::string& sName)
{
    std::string sText;
    if (!ReadAll(sSidecar, sText))
    {
        return false;
    }
    char acPrefix[8];
    _snprintf(acPrefix, sizeof(acPrefix) - 1, "%d ", iType);
    acPrefix[sizeof(acPrefix) - 1] = 0;
    const size_t uAt = sText.compare(0, 2, acPrefix) == 0 ? 0 : sText.find(std::string("\n") + acPrefix);
    if (uAt == std::string::npos)
    {
        return false;
    }
    const size_t uStart = uAt + (uAt == 0 ? 2 : 3);
    const size_t uEnd = sText.find(' ', uStart);
    if (uEnd == std::string::npos)
    {
        return false;
    }
    sName = sText.substr(uStart, uEnd - uStart);
    return !sName.empty();
}

}

TEST_CASE_NAMED(FSaveCrossTest1, "Game::SaveCross::save-cross-load", "[ProductFilter][SaveCross]")
{
    for (int i = 0; i < 2; ++i)
    {
        if (GetFileAttributesA((GameDir(kPlatforms[i]) + "\\BattlefrontII.exe").c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            SkipCase((std::string("the ") + kPlatforms[i] + " Test exe is not built").c_str());
        }
    }
    std::string asSidecar[2];
    std::string asSaves[2];
    bool bWritten = true;
    for (int i = 0; i < 2; ++i)
    {
        GameRun kWrite((std::string("savecross-write-") + kPlatforms[i]).c_str(), kPlatforms[i], GameDir(kPlatforms[i]));
        asSidecar[i] = kWrite.RunDir() + "\\save-fields.txt";
        asSaves[i] = kWrite.RunDir() + "\\SaveGames";
        kWrite.AddArg(Tail(kPlatforms[i], "-savetest write"));
        AddPlatformTests(kWrite, kPlatforms[i]);
        kWrite.AddInGameTest("Game.Save.Write", asSidecar[i]);
        kWrite.KeepWorkerFolder("SaveGames");
        kWrite.Start();
        bWritten = kWrite.Wait() && bWritten;
    }
    REQUIRE(bWritten);
    static const char* const sapcExt[] = {".profile", ".rote"};
    static const int saiType[] = {1, 3};
    for (int t = 0; t < 2; ++t)
    {
        std::string asName[2];
        std::string asBytes[2];
        for (int i = 0; i < 2; ++i)
        {
            REQUIRE_MESSAGE(SidecarName(asSidecar[i], saiType[t], asName[i]),
                            kPlatforms[i] << " wrote no type " << saiType[t] << " save");
            REQUIRE_MESSAGE(ReadAll(asSaves[i] + "\\" + asName[i] + sapcExt[t], asBytes[i]),
                            kPlatforms[i] << " has no SaveGames\\" << asName[i] << sapcExt[t]);
        }
        std::string sWhy;
        CHECK_MESSAGE(CompareBytes(asBytes[0], asBytes[1], sWhy),
                      "Win32 " << asName[0] << sapcExt[t] << " and x64 " << asName[1] << sapcExt[t] << ": " << sWhy);
    }
    for (int iFrom = 0; iFrom < 2; ++iFrom)
    {
        const int iTo = 1 - iFrom;
        for (int t = 0; t < 2; ++t)
        {
            char acType[4];
            _snprintf(acType, sizeof(acType) - 1, "%d", saiType[t]);
            acType[sizeof(acType) - 1] = 0;
            GameRun kLoad((std::string("savecross-") + kPlatforms[iFrom] + "-" + kPlatforms[iTo] + "-" + acType).c_str(),
                          kPlatforms[iTo], GameDir(kPlatforms[iTo]));
            kLoad.AddArg(Tail(kPlatforms[iTo], std::string("-savetest load ") + acType + " SaveTest"));
            AddPlatformTests(kLoad, kPlatforms[iTo]);
            kLoad.AddInGameTest("Game.Save.Load", std::string(acType) + "|" + asSidecar[iFrom]);
            kLoad.SeedWorkerFolder("SaveGames", asSaves[iFrom]);
            kLoad.Start();
            kLoad.Wait();
        }
    }
}
