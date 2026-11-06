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
