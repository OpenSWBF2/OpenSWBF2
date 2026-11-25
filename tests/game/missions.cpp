#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"
#include "testutil.h"

namespace
{

const unsigned int kSoakShortTurns = 9300;
const unsigned int kSoakLongTurns = 617300;

bool ReadMissions(std::vector<MissionLine>& kMissions, std::string& sError)
{
    std::string sText;
    if (!ReadRepoText("tests\\smoke\\missions-all.txt", sText))
    {
        sError = "cannot read tests\\smoke\\missions-all.txt";
        return false;
    }
    return ParseMissions(sText, kMissions, sError);
}

bool AllMissions(std::vector<std::string>& kParams, std::string& sError)
{
    std::vector<MissionLine> kMissions;
    if (!ReadMissions(kMissions, sError))
    {
        return false;
    }
    for (size_t i = 0; i < kMissions.size(); ++i)
    {
        kParams.push_back(kMissions[i].sName);
    }
    return true;
}

bool IsObjectiveMode(const std::string& sMission)
{
    const char* const apcModes[] = {"_1flag", "_ctf", "_ass"};
    for (size_t i = 0; i < sizeof(apcModes) / sizeof(apcModes[0]); ++i)
    {
        const size_t uLen = strlen(apcModes[i]);
        if (sMission.size() > uLen && sMission.compare(sMission.size() - uLen, uLen, apcModes[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

std::string SoakArg(unsigned int uTurns)
{
    char ac[32];
    _snprintf(ac, sizeof(ac) - 1, " -soakturns %u", uTurns);
    ac[sizeof(ac) - 1] = 0;
    return ac;
}

}

COMPLEX_TEST_CASE_NAMED(FAllMissionsTest, "Game::All", "[ProductFilter][All]", AllMissions)
{
    std::vector<MissionLine> kMissions;
    std::string sError;
    if (!ReadMissions(kMissions, sError))
    {
        HarnessError(sError.c_str());
    }
    const std::string sMission = GameRun::Param();
    const MissionLine* pLine = 0;
    for (size_t i = 0; i < kMissions.size(); ++i)
    {
        if (kMissions[i].sName == sMission)
        {
            pLine = &kMissions[i];
        }
    }
    if (!pLine)
    {
        HarnessError(("Game::All has no missions line for '" + sMission + "'").c_str());
    }
    const bool bTimeOut = pLine->sClass == "TimeOut";
    std::string sRow;
    const bool bRow = DigestRow(sMission.c_str(), sRow);
    const bool bNoVictory = IsObjectiveMode(sMission) && !bRow;
    if (bNoVictory && !bTimeOut)
    {
        HarnessError((sMission + " has no digest row and runs with -novictory, so it cannot Pass").c_str());
    }
    const unsigned int uSoak = GameRun::LongSoak() ? kSoakLongTurns : kSoakShortTurns;
    std::string sArgs = sMission + " " + StandardTail(false) + " -logcmds \"LogMemory Verbose\" -digesttrace 1500";
    if (bNoVictory)
    {
        sArgs += " -novictory";
    }
    sArgs += SoakArg(bTimeOut ? uSoak : kStandardTurnCap);
    GameRun kRun(sMission.c_str());
    kRun.AddArg(sArgs);
    AddStandardTests(kRun);
    kRun.Start();
    kRun.Wait();
    const std::string sLog = ReadRunLog(kRun);
    std::string sWhy;
    CHECK_MESSAGE(CheckMissionClass(pLine->sClass, kRun.Result().ProgressTurns, uSoak, kStandardTurnCap, sWhy),
                  sMission << ": " << sWhy);
    CHECK_MESSAGE(LastTurnsCovered(sLog, kRun.Result().ProgressTurns, sWhy), sMission << ": " << sWhy);
    CHECK_MESSAGE(HasDigestTrace(sLog, 100), sMission << ": no digest trace line for turn 100");
    if (bTimeOut || bRow)
    {
        CHECK_MESSAGE(HasDigestTrace(sLog, 1500), sMission << ": no digest trace line for turn 1500");
    }
    if (bTimeOut)
    {
        CHECK_MESSAGE(HasLine(sLog, "Mission running"), sMission << ": no MISSION RUNNING line");
        CHECK_MESSAGE(HasLine(sLog, "Sim digest turns=1500 "), sMission << ": no SIM DIGEST line at turn 1500");
        CHECK_MESSAGE(HasLine(sLog, "Soak turns="), sMission << ": no SOAK line");
    }
    if (bNoVictory)
    {
        CHECK_MESSAGE(HasLine(sLog, "No victory"), sMission << ": no NOVICTORY line");
    }
    if (bRow)
    {
        CheckDigestRow(sLog, sRow, sMission);
    }
}
