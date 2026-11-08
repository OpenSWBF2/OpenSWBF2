#include "third_party/doctest/doctest.h"

#include <stdlib.h>
#include <string.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "RedEngineFL/Memory/RedScriptLimits.h"
#include "testutil.h"

namespace
{

typedef std::vector<std::string> BaselineRow;

struct BaselineTable
{
    std::string sCommit;
    BaselineRow vHeader;
    std::vector<BaselineRow> vRows;
};

void SplitOn(const std::string& sText, char cSep, std::vector<std::string>& vOut)
{
    size_t uStart = 0;
    while (true)
    {
        size_t uEnd = sText.find(cSep, uStart);
        if (uEnd == std::string::npos)
        {
            vOut.push_back(sText.substr(uStart));
            return;
        }
        vOut.push_back(sText.substr(uStart, uEnd - uStart));
        uStart = uEnd + 1;
    }
}

void ReadBaselineLines(const char* pcRel, std::vector<std::string>& vOut)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRel, sText));
    std::vector<std::string> vRaw;
    SplitOn(sText, '\n', vRaw);
    for (size_t i = 0; i < vRaw.size(); i++)
    {
        std::string sLine = vRaw[i];
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        if (!sLine.empty())
        {
            vOut.push_back(sLine);
        }
    }
}

bool IsCommitLine(const std::string& sLine)
{
    const std::string sPrefix = "# commit ";
    if (sLine.size() != sPrefix.size() + 40 || sLine.compare(0, sPrefix.size(), sPrefix) != 0)
    {
        return false;
    }
    for (size_t i = sPrefix.size(); i < sLine.size(); i++)
    {
        char c = sLine[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
        {
            return false;
        }
    }
    return true;
}

void LoadBaselineTable(const char* pcRel, size_t uFields, BaselineTable& rTable)
{
    std::vector<std::string> vLines;
    ReadBaselineLines(pcRel, vLines);
    REQUIRE(vLines.size() >= 2);
    rTable.sCommit = vLines[0];
    SplitOn(vLines[1], '\t', rTable.vHeader);
    CHECK(rTable.vHeader.size() == uFields);
    for (size_t i = 2; i < vLines.size(); i++)
    {
        BaselineRow vRow;
        SplitOn(vLines[i], '\t', vRow);
        CHECK_MESSAGE(vRow.size() == uFields, vLines[i]);
        if (vRow.size() == uFields)
        {
            rTable.vRows.push_back(vRow);
        }
    }
}

std::set<std::string> AllMissions()
{
    std::vector<std::string> vLines;
    ReadBaselineLines("tests\\smoke\\missions-all.txt", vLines);
    std::set<std::string> kOut;
    for (size_t i = 0; i < vLines.size(); i++)
    {
        size_t uFirst = vLines[i].find_first_not_of(" \t");
        if (uFirst == std::string::npos)
        {
            continue;
        }
        size_t uEnd = vLines[i].find_first_of(" \t", uFirst);
        kOut.insert(vLines[i].substr(uFirst, uEnd == std::string::npos ? std::string::npos : uEnd - uFirst));
    }
    return kOut;
}

std::set<std::string> TableMissions(const BaselineTable& rTable)
{
    std::set<std::string> kOut;
    for (size_t i = 0; i < rTable.vRows.size(); i++)
    {
        kOut.insert(rTable.vRows[i][0]);
    }
    return kOut;
}

bool ParseCount(const std::string& sText, long long& rllOut)
{
    if (sText.empty() || sText.size() > 18)
    {
        return false;
    }
    size_t uStart = sText[0] == '-' ? 1 : 0;
    if (uStart == sText.size())
    {
        return false;
    }
    for (size_t i = uStart; i < sText.size(); i++)
    {
        if (sText[i] < '0' || sText[i] > '9')
        {
            return false;
        }
    }
    rllOut = _strtoi64(sText.c_str(), 0, 10);
    return true;
}

long long RequireCount(const std::string& sText)
{
    long long llValue = 0;
    CHECK_MESSAGE(ParseCount(sText, llValue), sText);
    return llValue;
}

}

TEST_CASE("baseline pool sizes parse and cover every mission" * doctest::test_suite("regression"))
{
    BaselineTable kTable;
    LoadBaselineTable("tests\\baseline\\v143\\pool-sizes.txt", 6, kTable);
    CHECK(IsCommitLine(kTable.sCommit));
    std::set<std::string> kMissions = AllMissions();
    CHECK(kMissions.size() == 109);
    CHECK(TableMissions(kTable) == kMissions);
    for (size_t i = 0; i < kTable.vRows.size(); i++)
    {
        const BaselineRow& r = kTable.vRows[i];
        if (r[1] == "-")
        {
            continue;
        }
        long long llValue = 0;
        CHECK_MESSAGE(ParseCount(r[3], llValue), r[3]);
        CHECK_MESSAGE(ParseCount(r[4], llValue), r[4]);
    }
}

TEST_CASE("baseline mem peaks parse and cover every mission" * doctest::test_suite("regression"))
{
    BaselineTable kTable;
    LoadBaselineTable("tests\\baseline\\v143\\mem-peaks.txt", 10, kTable);
    CHECK(IsCommitLine(kTable.sCommit));
    CHECK(TableMissions(kTable) == AllMissions());
    std::set<std::string> kWithBlock;
    std::set<std::string> kWithRuntime;
    for (size_t i = 0; i < kTable.vRows.size(); i++)
    {
        const BaselineRow& r = kTable.vRows[i];
        if (r[1] == "-")
        {
            continue;
        }
        kWithBlock.insert(r[0]);
        CHECK_MESSAGE((r[1] == "exit" || r[1] == "turn1500"), r[1]);
        CHECK_MESSAGE((r[2] == "heap" || r[2] == "pool"), r[2]);
        if (r[2] == "heap" && r[3] == "Runtime")
        {
            kWithRuntime.insert(r[0]);
        }
        for (size_t f = 4; f < r.size(); f++)
        {
            if (r[f] != "-")
            {
                long long llValue = 0;
                CHECK_MESSAGE((ParseCount(r[f], llValue) && llValue >= 0), r[f]);
            }
        }
    }
    CHECK(!kWithBlock.empty());
    CHECK(kWithRuntime == kWithBlock);
}

namespace
{

void CheckLuaBinsTable(const char* pcRel)
{
    BaselineTable kTable;
    LoadBaselineTable(pcRel, 8, kTable);
    CHECK(IsCommitLine(kTable.sCommit));
    CHECK(TableMissions(kTable) == AllMissions());
    std::set<std::string> kWithBlock;
    std::set<std::string> kWithBin;
    for (size_t i = 0; i < kTable.vRows.size(); i++)
    {
        const BaselineRow& r = kTable.vRows[i];
        if (r[1] == "-")
        {
            for (size_t f = 2; f < r.size(); f++)
            {
                CHECK_MESSAGE(r[f] == "-", r[f]);
            }
            continue;
        }
        kWithBlock.insert(r[0]);
        CHECK_MESSAGE((r[1] == "exit" || r[1] == "turn1500"), r[1]);
        if (r[2] == "-")
        {
            continue;
        }
        kWithBin.insert(r[0]);
        for (size_t f = 2; f < 7; f++)
        {
            long long llValue = 0;
            CHECK_MESSAGE((ParseCount(r[f], llValue) && llValue >= 0), r[f]);
        }
        if (r[7] != "-")
        {
            long long llValue = 0;
            CHECK_MESSAGE((ParseCount(r[7], llValue) && llValue >= 0), r[7]);
        }
    }
    CHECK(!kWithBlock.empty());
    CHECK(kWithBin == kWithBlock);
}

}

TEST_CASE("baseline lua bins parse and cover every mission" * doctest::test_suite("regression"))
{
    CheckLuaBinsTable("tests\\baseline\\v143\\lua-bins.txt");
}

TEST_CASE("baseline cloth headroom parses and covers every mission" * doctest::test_suite("regression"))
{
    BaselineTable kTable;
    LoadBaselineTable("tests\\baseline\\v143\\cloth-headroom.txt", 5, kTable);
    CHECK(IsCommitLine(kTable.sCommit));
    CHECK(TableMissions(kTable) == AllMissions());
    for (size_t i = 0; i < kTable.vRows.size(); i++)
    {
        const BaselineRow& r = kTable.vRows[i];
        CHECK_MESSAGE((r[2] == "0" || r[2] == "1" || r[2] == "-"), r[2]);
        if (r[2] == "1")
        {
            CHECK(RequireCount(r[3]) <= RequireCount(r[4]));
        }
    }
}

TEST_CASE("baseline retail script values sit below the ceilings" * doctest::test_suite("regression"))
{
    BaselineTable kPools;
    BaselineTable kPeaks;
    LoadBaselineTable("tests\\baseline\\v143\\pool-sizes.txt", 6, kPools);
    LoadBaselineTable("tests\\baseline\\v143\\mem-peaks.txt", 10, kPeaks);
    std::map<std::pair<std::string, std::string>, long long> kSize;
    std::map<std::pair<std::string, std::string>, long long> kCount;
    std::set<std::string> kReported;
    for (size_t i = 0; i < kPeaks.vRows.size(); i++)
    {
        const BaselineRow& r = kPeaks.vRows[i];
        if (r[2] != "pool")
        {
            continue;
        }
        kReported.insert(r[0]);
        long long llCount = RequireCount(r[4]);
        long long llSize = RequireCount(r[5]);
        kSize[std::make_pair(r[0], r[3])] = llSize;
        kCount[std::make_pair(r[0], r[3])] = llCount;
        const std::string sPool = r[0] + " " + r[3];
        CHECK_MESSAGE(llCount * llSize < (long long)RedScriptLimits::kPoolBytesMax, sPool);
    }
    const long long llPoolMax = RedScriptLimits::kPoolBytesMax;
    int iChecked = 0;
    for (size_t i = 0; i < kPools.vRows.size(); i++)
    {
        const BaselineRow& r = kPools.vRows[i];
        const std::string sWhere = r[0] + " " + r[1] + " " + r[2] + " " + r[3];
        if (r[1] == "-")
        {
            continue;
        }
        long long llRequested = RequireCount(r[3]);
        long long llApplied = RequireCount(r[4]);
        if (r[1] == "SetupTempHeap")
        {
            CHECK_MESSAGE(llRequested * 4 <= (long long)RedScriptLimits::kTempHeapMax, sWhere);
            CHECK_MESSAGE(llApplied == llRequested, sWhere);
            iChecked++;
        }
        else if (r[1] == "StealArtistHeap")
        {
            if (llRequested >= (long long)RedScriptLimits::kArtistStealMax)
            {
                CHECK_MESSAGE(llApplied == 0, sWhere);
            }
            else
            {
                CHECK_MESSAGE(llRequested >= (long long)RedScriptLimits::kArtistStealMin, sWhere);
                CHECK_MESSAGE(llApplied == llRequested, sWhere);
            }
            iChecked++;
        }
        else if (r[1] == "SetMemoryPoolSize" || r[1] == "AddWalkerType")
        {
            if (r[5] == "none")
            {
                CHECK_MESSAGE(llApplied == 0, sWhere);
                continue;
            }
            const std::pair<std::string, std::string> kKey = std::make_pair(r[0], r[5]);
            std::map<std::pair<std::string, std::string>, long long>::const_iterator itCount = kCount.find(kKey);
            if (r[1] == "SetMemoryPoolSize")
            {
                const long long llWrapped = llRequested < 0 ? llRequested + 0x100000000LL : llRequested;
                const bool bAsAsked = llApplied == llWrapped;
                const bool bExisting = itCount != kCount.end() &&
                                       (llApplied == itCount->second || (itCount->second == 0 && llApplied >= llWrapped));
                CHECK_MESSAGE((bAsAsked || bExisting), sWhere);
            }
            else
            {
                CHECK_MESSAGE(llApplied == llRequested, sWhere);
            }
            if (llRequested < 0)
            {
                iChecked++;
                continue;
            }
            if (kReported.count(r[0]) == 0)
            {
                continue;
            }
            std::map<std::pair<std::string, std::string>, long long>::const_iterator it = kSize.find(kKey);
            if (it == kSize.end())
            {
                CHECK_MESSAGE(llApplied == 0, sWhere);
                continue;
            }
            const long long llTotal = llApplied > llRequested ? llApplied : llRequested;
            CHECK_MESSAGE(llTotal * it->second * 4 <= llPoolMax, sWhere);
            iChecked++;
        }
        else
        {
            FAIL_CHECK(sWhere);
        }
    }
    CHECK(iChecked > 0);
}

TEST_CASE("baseline label table follows the label abi" * doctest::test_suite("regression"))
{
    BaselineTable kPools;
    LoadBaselineTable("tests\\baseline\\v143\\pool-sizes.txt", 6, kPools);
    int iChecked = 0;
    for (size_t i = 0; i < kPools.vRows.size(); i++)
    {
        const BaselineRow& r = kPools.vRows[i];
        if (r[1] != "SetMemoryPoolSize")
        {
            continue;
        }
        iChecked++;
        const std::string sWhere = r[0] + " " + r[2] + " " + r[5];
        CHECK_MESSAGE((r[5] == "none" || _strnicmp(r[5].c_str(), r[2].c_str(), 31) == 0), sWhere);
    }
    CHECK(iChecked > 0);
}

namespace
{

struct CauseSection
{
    std::string sName;
    size_t uBegin;
    size_t uEnd;
};

void ParseCauseSections(const std::string& sText, std::vector<CauseSection>& vOut)
{
    const size_t uNone = (size_t)-1;
    size_t uOpen = uNone;
    size_t uLine = 0;
    while (uLine <= sText.size())
    {
        size_t uNext = sText.find('\n', uLine);
        if (uNext == std::string::npos)
        {
            uNext = sText.size();
        }
        const bool bPart = sText.compare(uLine, 3, "## ") == 0;
        const bool bSection = sText.compare(uLine, 4, "### ") == 0;
        if (bPart || bSection || uNext == sText.size())
        {
            const size_t uClose = (bPart || bSection) ? uLine : sText.size();
            if (uOpen != uNone)
            {
                vOut[uOpen].uEnd = uClose;
                uOpen = uNone;
            }
            if (bSection && sText.compare(uLine, 6, "### C-") == 0)
            {
                CauseSection kCause;
                kCause.sName = sText.substr(uLine + 4, uNext - uLine - 4);
                while (!kCause.sName.empty() && kCause.sName[kCause.sName.size() - 1] == ' ')
                {
                    kCause.sName.erase(kCause.sName.size() - 1);
                }
                kCause.uBegin = uLine;
                kCause.uEnd = sText.size();
                uOpen = vOut.size();
                vOut.push_back(kCause);
            }
        }
        uLine = uNext + 1;
    }
}

}

TEST_CASE("baseline causes add exactly one digest cause since the milestone base" * doctest::test_suite("regression"))
{
    std::string sRaw;
    std::string sExpected;
    if (!ReadRepoText("tests\\baseline\\v143\\CAUSES.md", sRaw) || !ReadRepoText("tests\\smoke\\digest-expected.txt", sExpected))
    {
        SkipCase("baseline files not found");
    }
    std::string sText;
    for (size_t i = 0; i < sRaw.size(); i++)
    {
        if (sRaw[i] != '\r')
        {
            sText += sRaw[i];
        }
    }
    std::vector<CauseSection> vCauses;
    ParseCauseSections(sText, vCauses);

    const char* apcBase[] = { "C-OOB-SIDE", "C-SLOT-SOLDANIM", "C-SOLDIER-SLOPE", "C-ADDR-FLYER", "C-2DRADIUS",
        "C-PTR03-SENTINEL", "C-PIN-28", "C-D14-RECHECK", "C-SHADER-INIT", "C-WIRE-ORDER", "C-WALKER-GRAVITY",
        "C-PILOT-DELETE", "C-FLAG-PICKUP", "C-CRITSYS-SHIELD", "C-TAT3-STALE", "C-INPUT-ALIGN", "C-PBLLIST-RENDER",
        "C-DOT-ORDER-30", "C-FENV-TERRAIN", "C-SIM-REWRITE-30", "C-WIRE-ORDER-30" };
    const std::set<std::string> kBase(apcBase, apcBase + sizeof(apcBase) / sizeof(apcBase[0]));
    REQUIRE(kBase.size() == 21);

    std::set<std::string> kSeen;
    std::set<std::string> kAdded;
    const CauseSection* pSerial = 0;
    for (size_t i = 0; i < vCauses.size(); i++)
    {
        CHECK_MESSAGE(kSeen.insert(vCauses[i].sName).second, vCauses[i].sName);
        if (kBase.count(vCauses[i].sName) == 0)
        {
            kAdded.insert(vCauses[i].sName);
        }
        if (vCauses[i].sName == "C-LAYOUT-SERIAL")
        {
            pSerial = &vCauses[i];
        }
    }
    CHECK(kAdded.count("C-LAYOUT-SERIAL") == 1);
    REQUIRE(pSerial != (const CauseSection*)0);
    const std::string sSerial = sText.substr(pSerial->uBegin, pSerial->uEnd - pSerial->uBegin);

    std::vector<std::string> vLines;
    ReadBaselineLines("tests\\smoke\\digest-expected.txt", vLines);
    int iRows = 0;
    for (size_t i = 0; i < vLines.size(); i++)
    {
        std::vector<std::string> vRow;
        SplitOn(vLines[i], '\t', vRow);
        CHECK_MESSAGE(vRow.size() == 2, vLines[i]);
        if (vRow.size() != 2)
        {
            continue;
        }
        size_t uState = vRow[1].find("state=");
        CHECK_MESSAGE(uState != std::string::npos, vLines[i]);
        if (uState == std::string::npos)
        {
            continue;
        }
        uState += 6;
        size_t uStateEnd = vRow[1].find(' ', uState);
        const std::string sState =
            vRow[1].substr(uState, uStateEnd == std::string::npos ? std::string::npos : uStateEnd - uState);
        REQUIRE(!sState.empty());
        iRows++;
        CHECK_MESSAGE(sSerial.find("`" + vRow[0] + "`") != std::string::npos, vRow[0]);
        const std::string sWhere = vRow[0] + " " + sState;
        CHECK_MESSAGE(sSerial.find(sState) != std::string::npos, sWhere);
        for (size_t uAt = sText.find(sState); uAt != std::string::npos; uAt = sText.find(sState, uAt + 1))
        {
            CHECK_MESSAGE((uAt >= pSerial->uBegin && uAt < pSerial->uEnd), sState);
        }
    }
    CHECK(iRows > 0);

    for (size_t i = 0; i < vCauses.size(); i++)
    {
        if (kAdded.count(vCauses[i].sName) == 0 || &vCauses[i] == pSerial)
        {
            continue;
        }
        const std::string sCause = sText.substr(vCauses[i].uBegin, vCauses[i].uEnd - vCauses[i].uBegin);
        CHECK_MESSAGE(sCause.find("No digest row moved") != std::string::npos, vCauses[i].sName);
        CHECK_MESSAGE(sCause.find("state=0x") == std::string::npos, vCauses[i].sName);
    }
    CHECK(sSerial.find("No digest row moved") == std::string::npos);
}
