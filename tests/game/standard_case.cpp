#include "game/standard_case.h"

#include "third_party/doctest/doctest.h"

#include <regex>
#include <stdio.h>

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "testutil.h"

static bool IsX64()
{
    return GameRun::Platform() == "x64";
}

std::string StandardTail(bool bSound)
{
    return GameRunTail(bSound) + (IsX64() ? " -hogaddrspace" : "");
}

std::string RealTimeTail()
{
    std::string sTail = StandardTail(false);
    const std::string sFixed = " -fixedrate 30";
    sTail.erase(sTail.find(sFixed), sFixed.size());
    return sTail;
}

std::string RenderTail()
{
    const std::string sTail = StandardTail(true);
    const std::string sNoRender = "-norender ";
    return sTail.substr(sTail.find(sNoRender) + sNoRender.size());
}

std::string SmokeFile(const char* pcName)
{
    return RepoRoot() + "\\tests\\smoke\\" + pcName;
}

bool DigestRow(const char* pcName, std::string& sRow)
{
    std::string sTable;
    return ReadRepoText("tests\\smoke\\digest-expected.txt", sTable) && FindTableRow(sTable, 0, pcName, sRow);
}

std::string StandardArgs(const char* pcName, const std::string& sArgs)
{
    std::string sOut = sArgs;
    std::string sRow;
    if (DigestRow(pcName, sRow) && !HasArgWord(sOut, "-digesttrace"))
    {
        sOut += " -digesttrace 1500";
    }
    if (!HasArgWord(sOut, "-soakturns"))
    {
        char acCap[32];
        _snprintf(acCap, sizeof(acCap) - 1, " -soakturns %u", kStandardTurnCap);
        acCap[sizeof(acCap) - 1] = 0;
        sOut += acCap;
    }
    return sOut;
}

void AddStandardTests(GameRun& kRun)
{
    kRun.AddInGameTest("Game.Memory.MiOptions");
    if (IsX64())
    {
        kRun.AddInGameTest("Boot.AddrSpace");
    }
}

std::string ReadRunLog(const GameRun& kRun)
{
    std::string sText;
    FILE* pFile = fopen((kRun.RunDir() + "\\run.log").c_str(), "rb");
    if (pFile)
    {
        char acBuf[4096];
        size_t uRead;
        while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
        {
            sText.append(acBuf, uRead);
        }
        fclose(pFile);
    }
    return sText;
}

void CheckStandardRun(const GameRun& kRun, const char* pcName, unsigned int uSoak)
{
    const unsigned int uTurns = kRun.Result().ProgressTurns;
    if (uSoak > 0)
    {
        CHECK_MESSAGE(uTurns >= uSoak, pcName << ": progress turns " << uTurns << " below -soakturns " << uSoak);
    }
    else
    {
        CHECK_MESSAGE(uTurns < kStandardTurnCap, pcName << ": the run reached the turn cap " << kStandardTurnCap);
    }
    const std::string sLog = ReadRunLog(kRun);
    std::string sWhy;
    CHECK_MESSAGE(LastTurnsCovered(sLog, uTurns, sWhy), pcName << ": " << sWhy);
    std::string sRow;
    if (!DigestRow(pcName, sRow))
    {
        return;
    }
    CHECK_MESSAGE(HasDigestTrace(sLog, 100), pcName << ": no digest trace line for turn 100");
    CHECK_MESSAGE(HasDigestTrace(sLog, 1500), pcName << ": no digest trace line for turn 1500");
    CheckDigestRow(sLog, sRow, pcName);
}

void CheckDigestRow(const std::string& sLog, const std::string& sRow, const std::string& sName)
{
    std::string sWhy;
    if (GameRun::Config() == "Asan")
    {
        if (!CompareDigestRow(sLog, sRow, sWhy))
        {
            MESSAGE(sName << ": digest finding (Asan, not gated): " << sWhy);
        }
    }
    else if (GameRun::Config() == "Release" || GameRun::Config() == "Test")
    {
        CHECK_MESSAGE(CompareDigestRow(sLog, sRow, sWhy), sName << ": " << sWhy);
    }
}

bool HasLine(const std::string& sLog, const char* pcText)
{
    return !LastMarkerLine(sLog, pcText).empty();
}

std::string ReadRunFile(const GameRun& kRun, const char* pcLeaf)
{
    std::string sText;
    FILE* pFile = fopen((kRun.RunDir() + "\\" + pcLeaf).c_str(), "rb");
    if (pFile)
    {
        char acBuf[4096];
        size_t uRead;
        while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
        {
            sText.append(acBuf, uRead);
        }
        fclose(pFile);
    }
    return sText;
}

void CheckGroupRows(const GameRun& kRun, const char* pcRow)
{
    std::string sTable;
    std::string sRow;
    std::string sWhy;
    if (GameRun::Config() == "Release")
    {
        REQUIRE(ReadRepoText("tests\\baseline\\v143\\smoke-all-digests.txt", sTable));
        const std::string sDigestRow = DigestRowName(pcRow);
        REQUIRE_MESSAGE(FindTableRow(sTable, 0, sDigestRow, sRow), "no single smoke-all-digests row for " << sDigestRow);
        CHECK_MESSAGE(CompareSmokeAllRow(ReadRunLog(kRun), ReadRunFile(kRun, "finalscore.log"), sRow, sWhy),
                      pcRow << ": " << sWhy);
    }
    else if (GameRun::Config() == "Test")
    {
        REQUIRE(ReadRepoText(IsX64() ? "tests\\baseline\\v143-x64\\offthread.txt" : "tests\\baseline\\v143\\offthread.txt",
                             sTable));
        REQUIRE_MESSAGE(FindTableRow(sTable, "", pcRow, sRow), "no single offthread row for " << pcRow);
        CHECK_MESSAGE(CompareOffThreadRow(ReadRunLog(kRun), sRow, sWhy), pcRow << ": " << sWhy);
    }
}

int LineCount(const GameRunResult& kResult, const char* pcCategory, int iSeverity, const char* pcRegex)
{
    const std::regex kRx(pcRegex);
    int iCount = 0;
    for (size_t i = 0; i < kResult.Lines.size(); ++i)
    {
        const LogLine& kLine = kResult.Lines[i];
        if (kLine.bParsed && kLine.sCategory == pcCategory && kLine.iSeverity == iSeverity &&
            std::regex_search(kLine.sMessage, kRx))
        {
            ++iCount;
        }
    }
    return iCount;
}

int LineIndex(const GameRunResult& kResult, const char* pcText, int iFrom)
{
    for (size_t i = iFrom < 0 ? 0 : (size_t)iFrom; i < kResult.Lines.size(); ++i)
    {
        if (kResult.Lines[i].sText.find(pcText) != std::string::npos)
        {
            return (int)i;
        }
    }
    return -1;
}
