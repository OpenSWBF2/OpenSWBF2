#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"

namespace
{

const char* const kPerturbMissions[] = {"geo1c_con", "nab2g_c", "hot1g_con", "spa3c_ass",
                                        "dag1c_ctf", "kas2c_uber", "tan1g_con"};

struct Variant
{
    std::string sMode;
    int iSeed;
    std::string sExtra;
};

bool HasExactLine(const std::string& sLog, const std::string& sLine)
{
    const std::vector<std::string> kLines = MarkerLines(sLog, sLine.c_str());
    return std::find(kLines.begin(), kLines.end(), sLine) != kLines.end();
}

void RunPerturb(int iIndex)
{
    const std::string sMission = kPerturbMissions[iIndex];
    const int iSeed = 1 + iIndex % 2;
    const std::string sSeed = std::to_string(iSeed);
    std::vector<Variant> kVariants;
    kVariants.push_back(Variant{"base", 0, ""});
    const char* const apcModes[] = {"pad", "fill", "pool"};
    for (size_t i = 0; i < 3; ++i)
    {
        kVariants.push_back(Variant{apcModes[i], iSeed, " -perturbheap " + sSeed + ":" + apcModes[i]});
    }
    kVariants.push_back(Variant{"hog", 0, " -hogaddrspace"});
    kVariants.push_back(Variant{"repeat", 0, " -repeatmission"});
    const bool bSoak = sMission == "nab2g_c";
    const std::string sSoakArg = bSoak ? " -soakturns 100000" : " -soakturns " + std::to_string(kStandardTurnCap);
    std::vector<std::unique_ptr<GameRun> > kRuns;
    for (size_t i = 0; i < kVariants.size(); ++i)
    {
        const Variant& v = kVariants[i];
        const std::string sName = "perturb-" + sMission + "-" + v.sMode + (v.iSeed > 0 ? "-" + sSeed : "");
        kRuns.push_back(std::unique_ptr<GameRun>(new GameRun(sName.c_str())));
        GameRun& kRun = *kRuns.back();
        kRun.AddArg(sMission + " " + GameRunTail(false) + " -digesttrace 100000" + v.sExtra + sSoakArg);
        kRun.AddInGameTest("Game.Memory.MiOptions");
        if (v.sMode == "hog")
        {
            kRun.AddInGameTest("Boot.AddrSpace");
        }
    }
    for (size_t i = 0; i < kRuns.size(); ++i)
    {
        kRuns[i]->Start();
    }
    for (size_t i = 0; i < kRuns.size(); ++i)
    {
        kRuns[i]->Wait();
    }
    std::vector<std::string> kLogs;
    for (size_t i = 0; i < kRuns.size(); ++i)
    {
        const Variant& v = kVariants[i];
        kLogs.push_back(ReadRunLog(*kRuns[i]));
        const std::string& sLog = kLogs.back();
        const std::string sWho = sMission + " mode=" + v.sMode;
        if (v.iSeed > 0)
        {
            CHECK_MESSAGE(HasExactLine(sLog, "Perturb seed=" + sSeed + " modes=" + v.sMode),
                          sWho << ": run not proven, no PERTURB line for seed " << sSeed);
        }
        else
        {
            CHECK_MESSAGE(MarkerLines(sLog, "Perturb ").empty(), sWho << ": a PERTURB line in an unperturbed run");
        }
        if (v.sMode == "repeat")
        {
            CHECK_MESSAGE(HasExactLine(sLog, "Repeat to_shell mission=" + sMission), sWho << ": no REPEAT to_shell line");
            CHECK_MESSAGE(HasExactLine(sLog, "Repeat to_mission mission=" + sMission),
                          sWho << ": no REPEAT to_mission line");
        }
        if (bSoak)
        {
            CHECK_MESSAGE(HasLine(sLog, "Soak turns="), sWho << ": no SOAK line");
        }
    }
    std::string sWhy;
    REQUIRE_MESSAGE(CheckPerturbBase(kLogs[0], sWhy), "perturb base " << sMission << ": " << sWhy);
    for (size_t i = 1; i < kRuns.size(); ++i)
    {
        const Variant& v = kVariants[i];
        sWhy.clear();
        const bool bEqual = ComparePerturbRun(kLogs[0], kLogs[i], v.sMode == "repeat", sWhy);
        const std::string sResult = bEqual ? std::string("equal") : sWhy;
        MESSAGE("perturb:" << sMission << " mode=" << v.sMode << " seed=" << v.iSeed << " " << sResult);
        CHECK_MESSAGE(bEqual, "perturb:" << sMission << " mode=" << v.sMode << " seed=" << v.iSeed << " " << sWhy);
    }
}

}

TEST_CASE_NAMED(FPerturbTest1, "Game::Perturb::geo1c_con", "[ProductFilter][Perturb]")
{
    RunPerturb(0);
}

TEST_CASE_NAMED(FPerturbTest2, "Game::Perturb::nab2g_c", "[ProductFilter][Perturb]")
{
    RunPerturb(1);
}

TEST_CASE_NAMED(FPerturbTest3, "Game::Perturb::hot1g_con", "[ProductFilter][Perturb]")
{
    RunPerturb(2);
}

TEST_CASE_NAMED(FPerturbTest4, "Game::Perturb::spa3c_ass", "[ProductFilter][Perturb]")
{
    RunPerturb(3);
}

TEST_CASE_NAMED(FPerturbTest5, "Game::Perturb::dag1c_ctf", "[ProductFilter][Perturb]")
{
    RunPerturb(4);
}

TEST_CASE_NAMED(FPerturbTest6, "Game::Perturb::kas2c_uber", "[ProductFilter][Perturb]")
{
    RunPerturb(5);
}

TEST_CASE_NAMED(FPerturbTest7, "Game::Perturb::tan1g_con", "[ProductFilter][Perturb]")
{
    RunPerturb(6);
}
