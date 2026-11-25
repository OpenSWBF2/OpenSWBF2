#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"
#include "testutil.h"

namespace
{

void RunAll(std::vector<std::unique_ptr<GameRun> >& kRuns)
{
    for (size_t i = 0; i < kRuns.size(); ++i)
    {
        kRuns[i]->Start();
    }
    for (size_t i = 0; i < kRuns.size(); ++i)
    {
        kRuns[i]->Wait();
    }
}

GameRun& AddRun(std::vector<std::unique_ptr<GameRun> >& kRuns, const std::string& sName, const std::string& sArgs)
{
    kRuns.push_back(std::unique_ptr<GameRun>(new GameRun(sName.c_str())));
    kRuns.back()->AddArg(sArgs);
    return *kRuns.back();
}

bool ReadBytes(const std::string& sPath, std::string& sBytes)
{
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return false;
    }
    char acBuf[65536];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sBytes.append(acBuf, uRead);
    }
    fclose(pFile);
    return true;
}

size_t LaaFlagOffset(const std::string& sPe)
{
    if (sPe.size() < 0x40 || sPe[0] != 'M' || sPe[1] != 'Z')
    {
        return 0;
    }
    const unsigned int uPe = (unsigned char)sPe[0x3C] | ((unsigned char)sPe[0x3D] << 8) | ((unsigned char)sPe[0x3E] << 16) |
                             ((unsigned int)(unsigned char)sPe[0x3F] << 24);
    if (uPe < 0x40 || (size_t)uPe + 24 > sPe.size() || sPe.compare(uPe, 4, std::string("PE\0\0", 4)) != 0)
    {
        return 0;
    }
    return (size_t)uPe + 4 + 18;
}

}

TEST_CASE_NAMED(FCaptureTest1, "Game::Capture::collect", "[ProductFilter][Capture][OnRequest]")
{
    const std::string sTail = GameRunTail(false);
    std::vector<std::unique_ptr<GameRun> > kRuns;
    const char* const apcFactory[] = {"geo1c_con", "nab2g_c", "hot1g_con", "spa3c_ass"};
    for (size_t i = 0; i < 4; ++i)
    {
        const std::string sMission = apcFactory[i];
        AddRun(kRuns, sMission, sMission + " " + sTail + " -soakturns 1600 -factorylog -digesttrace 1600");
        AddRun(kRuns, "hog-" + sMission, sMission + " " + sTail + " -hogaddrspace -soakturns 1600 -factorylog -digesttrace 1600");
    }
    const char* const apcTrace[] = {"dag1c_ctf", "kas2c_uber"};
    for (size_t i = 0; i < 2; ++i)
    {
        AddRun(kRuns, apcTrace[i], std::string(apcTrace[i]) + " " + sTail + " -soakturns 1600 -digesttrace 1600");
    }
    RunAll(kRuns);
}

TEST_CASE_NAMED(FLaaTest1, "Game::Laa::compare", "[ProductFilter][Laa][OnRequest]")
{
    if (GameRun::GameDir().empty())
    {
        SkipCase("game runs start from the runner with --game-dir");
    }
    const int iParam = GameRun::Param().empty() ? 5 : atoi(GameRun::Param().c_str());
    const int iRuns = iParam < 5 ? 5 : iParam;
    std::string sExe;
    REQUIRE_MESSAGE(ReadBytes(GameRun::GameDir() + "\\BattlefrontII.exe", sExe), "cannot read the game exe");
    const size_t uFlag = LaaFlagOffset(sExe);
    REQUIRE_MESSAGE(uFlag != 0, "the game exe has no PE header");
    REQUIRE_MESSAGE((sExe[uFlag] & 0x20) != 0, "the built exe lacks the large address aware flag");
    const std::string sTail = GameRunTail(false);
    std::vector<std::unique_ptr<GameRun> > kRuns;
    for (int i = 1; i <= iRuns; ++i)
    {
        const std::string sIndex = std::to_string(i);
        AddRun(kRuns, "laa-base-" + sIndex, "geo1c_con " + sTail + " -digesttrace 1500 -soakturns " + std::to_string(kStandardTurnCap));
        AddRun(kRuns, "laa-laa-" + sIndex,
               "geo1c_con " + sTail + " -hogaddrspace -digesttrace 1500 -soakturns " + std::to_string(kStandardTurnCap));
    }
    const std::string sNoLaa = kRuns[0]->RunDir() + "-nolaa.exe";
    CreateDirectoryA(sNoLaa.substr(0, sNoLaa.find_last_of('\\')).c_str(), 0);
    sExe[uFlag] = (char)(sExe[uFlag] & ~0x20);
    FILE* pFile = fopen(sNoLaa.c_str(), "wb");
    REQUIRE_MESSAGE((pFile && fwrite(sExe.data(), 1, sExe.size(), pFile) == sExe.size()), "cannot write " << sNoLaa);
    fclose(pFile);
    for (size_t i = 0; i < kRuns.size(); i += 2)
    {
        kRuns[i]->UseExe(sNoLaa);
    }
    RunAll(kRuns);
    std::set<std::string> kKeys;
    const std::regex kArena("Address space arena=0x([0-9A-F]+)");
    for (size_t i = 0; i < kRuns.size(); ++i)
    {
        const bool bLaa = i % 2 == 1;
        const std::string sLog = ReadRunLog(*kRuns[i]);
        const std::string sKey = FinalScoreKey(ReadRunFile(*kRuns[i], "finalscore.log"));
        long long llFrames = 0;
        std::smatch kMatch;
        const bool bArena = std::regex_search(sLog, kMatch, kArena);
        const std::string sArena = bArena ? kMatch[1].str() : std::string("none");
        const unsigned long long ullArena = bArena ? _strtoui64(sArena.c_str(), 0, 16) : 0;
        const std::string sName = "laa run " + std::to_string(i + 1);
        CHECK_MESSAGE(!sKey.empty(), sName << ": no finalscore.log");
        CHECK_MESSAGE(FramesCount(sLog, llFrames), sName << ": no single FRAMES line");
        CHECK_MESSAGE(bArena, sName << ": no ADDRSPACE line");
        const char* const pcSide = bLaa ? " below 2 GB" : " at or above 2 GB";
        CHECK_MESSAGE((!bArena || (bLaa ? ullArena >= 0x80000000ull : ullArena < 0x80000000ull)),
                      sName << ": arena 0x" << sArena << pcSide);
        kKeys.insert(sKey + " [f=" + std::to_string(llFrames) + "]");
    }
    std::string sAll;
    for (std::set<std::string>::const_iterator it = kKeys.begin(); it != kKeys.end(); ++it)
    {
        sAll += (sAll.empty() ? "" : " | ") + *it;
    }
    CHECK_MESSAGE(kKeys.size() == 1, "runs differ: " << sAll);
    const size_t uRuns = kRuns.size();
    MESSAGE("all " << uRuns << " runs: " << sAll);
}
