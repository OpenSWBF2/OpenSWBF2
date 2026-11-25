#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <map>
#include <set>
#include <stdlib.h>

#include "harness/gamerun.h"
#include "testutil.h"

namespace
{

const char* const kStandardMission = "geo1c_con";

bool IsX64()
{
    return GameRun::Platform() == "x64";
}

struct HeapPeaks
{
    std::map<std::string, unsigned long long> kPeak;
    std::map<std::string, std::set<std::string> > kCaps;
};

std::vector<std::string> SplitText(const std::string& sText, char cSep)
{
    std::vector<std::string> kParts;
    size_t uStart = 0;
    for (;;)
    {
        const size_t uAt = sText.find(cSep, uStart);
        kParts.push_back(sText.substr(uStart, uAt == std::string::npos ? std::string::npos : uAt - uStart));
        if (uAt == std::string::npos)
        {
            return kParts;
        }
        uStart = uAt + 1;
    }
}

bool IsNumber(const std::string& s)
{
    if (s.empty())
    {
        return false;
    }
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] < '0' || s[i] > '9')
        {
            return false;
        }
    }
    return true;
}

bool ReadHeapPeaks(const char* pcPath, HeapPeaks& kOut, std::string& sError)
{
    std::string sText;
    if (!ReadRepoText(pcPath, sText))
    {
        sError = std::string(pcPath) + " not found";
        return false;
    }
    const std::vector<std::string> kLines = SplitText(sText, '\n');
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::string sLine = kLines[i];
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        if (sLine.empty() || sLine[0] == '#' || sLine.compare(0, 8, "mission\t") == 0)
        {
            continue;
        }
        const std::vector<std::string> kCells = SplitText(sLine, '\t');
        if (kCells.size() != 10 || kCells[2] != "heap")
        {
            continue;
        }
        if (!IsNumber(kCells[4]) || !IsNumber(kCells[6]))
        {
            sError = std::string(pcPath) + " heap row without a cap and peak: " + sLine;
            return false;
        }
        const unsigned long long ullPeak = _strtoui64(kCells[6].c_str(), 0, 10);
        if (!kOut.kPeak.count(kCells[3]) || ullPeak > kOut.kPeak[kCells[3]])
        {
            kOut.kPeak[kCells[3]] = ullPeak;
        }
        kOut.kCaps[kCells[3]].insert(kCells[4]);
    }
    return true;
}

bool DerivedHeapCapsX64(std::string& sParameters, std::string& sError)
{
    const char* pcX64 = "tests\\baseline\\v143-x64\\mem-peaks.txt";
    const char* pcX86 = "tests\\baseline\\v143\\mem-peaks.txt";
    HeapPeaks k64;
    HeapPeaks k86;
    if (!ReadHeapPeaks(pcX64, k64, sError) || !ReadHeapPeaks(pcX86, k86, sError))
    {
        return false;
    }
    sParameters.clear();
    for (std::map<std::string, std::set<std::string> >::const_iterator it = k86.kCaps.begin(); it != k86.kCaps.end();
         ++it)
    {
        if (it->second.size() != 1)
        {
            sError = it->first + " has several caps in " + pcX86;
            return false;
        }
        if (!k64.kPeak.count(it->first))
        {
            sError = it->first + " has no heap rows in " + pcX64;
            return false;
        }
        const unsigned long long ullCap86 = _strtoui64(it->second.begin()->c_str(), 0, 10);
        const unsigned long long ullPeak64 = k64.kPeak[it->first];
        const unsigned long long ullPeak86 = k86.kPeak[it->first];
        unsigned long long ullWant = ullCap86;
        if (ullPeak64 > 0 && ullPeak86 > 0)
        {
            const unsigned long long ullScaled = (ullPeak64 * ullCap86 + ullPeak86 - 1) / ullPeak86;
            const unsigned long long ullAligned = (ullScaled + 65535) / 65536 * 65536;
            if (ullAligned > ullCap86)
            {
                ullWant = ullAligned;
            }
        }
        char acCap[32];
        _snprintf(acCap, sizeof(acCap), "%llu", ullWant);
        acCap[sizeof(acCap) - 1] = 0;
        sParameters += (sParameters.empty() ? "" : ",") + it->first + ":" + acCap;
    }
    for (std::map<std::string, unsigned long long>::const_iterator it = k64.kPeak.begin(); it != k64.kPeak.end(); ++it)
    {
        if (!k86.kCaps.count(it->first))
        {
            sError = it->first + " has no heap rows in " + pcX86;
            return false;
        }
    }
    return true;
}

void AddBootTests(GameRun& kRun)
{
    kRun.AddInGameTest("Game.Memory.MiOptions");
    if (IsX64())
    {
        kRun.AddInGameTest("Boot.AddrSpace");
    }
}

void RunBootMission(const char* pcMission)
{
    GameRun kRun((std::string("boot-") + pcMission).c_str());
    kRun.AddArg(std::string(pcMission) + " " + GameRunTail(false) + " -hogaddrspace -soakturns 1501");
    AddBootTests(kRun);
    kRun.AddInGameTest("Game.Memory.PoolSizes");
    if (GameRun::HasInGameTests())
    {
        std::string sCaps;
        std::string sError;
        if (!DerivedHeapCapsX64(sCaps, sError))
        {
            FAIL("x64 heap caps: " << sError);
        }
        kRun.AddInGameTest("Boot.HeapCaps", sCaps);
    }
    kRun.Start();
    kRun.Wait();
    CHECK(kRun.Result().ProgressTurns >= 1501u);
}

}

TEST_CASE_NAMED(FBootTest1, "Game::Boot::boot-shell", "[ProductFilter][Boot]")
{
    GameRun kRun("boot-shell");
    kRun.AddArg(GameRunTail(false) + (IsX64() ? " -hogaddrspace" : "") + " -soakturns 300");
    AddBootTests(kRun);
    kRun.Start();
    kRun.Wait();
    CHECK(kRun.Result().ProgressSteps >= 300u);
    CHECK(kRun.Result().ProgressTurns == 0u);
}

TEST_CASE_NAMED(FBootTest2, "Game::Boot::boot-sound-on", "[ProductFilter][Boot]")
{
    GameRun kRun("boot-sound-on");
    kRun.AddArg(std::string(kStandardMission) + " " + GameRunTail(true) + (IsX64() ? " -hogaddrspace" : "") +
                " -soakturns 300");
    AddBootTests(kRun);
    kRun.Start();
    kRun.Wait();
    CHECK(kRun.Result().ProgressTurns >= 300u);
}

TEST_CASE_NAMED(FBootTest3, "Game::Boot::boot-hot1g_con", "[ProductFilter][Boot]")
{
    RunBootMission("hot1g_con");
}

TEST_CASE_NAMED(FBootTest4, "Game::Boot::boot-tat2g_eli", "[ProductFilter][Boot]")
{
    RunBootMission("tat2g_eli");
}

TEST_CASE_NAMED(FBootTest5, "Game::Boot::boot-nab2g_c", "[ProductFilter][Boot]")
{
    RunBootMission("nab2g_c");
}
