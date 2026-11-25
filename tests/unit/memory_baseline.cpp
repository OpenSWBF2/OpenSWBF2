#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stdio.h>
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

TEST_CASE_NAMED(FMemoryBaselineTest1, "Unit::MemoryBaseline::baseline pool sizes parse and cover every mission", "[EngineFilter]")
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

TEST_CASE_NAMED(FMemoryBaselineTest2, "Unit::MemoryBaseline::baseline mem peaks parse and cover every mission", "[EngineFilter]")
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

TEST_CASE_NAMED(FMemoryBaselineTest3, "Unit::MemoryBaseline::baseline lua bins parse and cover every mission", "[EngineFilter]")
{
    CheckLuaBinsTable("tests\\baseline\\v143\\lua-bins.txt");
}

TEST_CASE_NAMED(FMemoryBaselineTest4, "Unit::MemoryBaseline::baseline cloth headroom parses and covers every mission", "[EngineFilter]")
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

TEST_CASE_NAMED(FMemoryBaselineTest5, "Unit::MemoryBaseline::baseline retail script values sit below the ceilings", "[EngineFilter]")
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

TEST_CASE_NAMED(FMemoryBaselineTest6, "Unit::MemoryBaseline::baseline label table follows the label abi", "[EngineFilter]")
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

struct CauseLine
{
    std::string sSubject;
    std::string sCause;
    std::string sText;
    size_t uBegin;
    size_t uEnd;
};

void ParseCauseLines(const std::string& sText, std::vector<CauseLine>& vOut)
{
    size_t uLine = 0;
    while (uLine < sText.size())
    {
        size_t uNext = sText.find('\n', uLine);
        if (uNext == std::string::npos)
        {
            uNext = sText.size();
        }
        const std::string sLine = sText.substr(uLine, uNext - uLine);
        if (sLine.compare(0, 3, "- `") == 0)
        {
            const size_t uSubjectEnd = sLine.find("` | ", 3);
            const size_t uCauseEnd = uSubjectEnd == std::string::npos ? std::string::npos : sLine.find(" | ", uSubjectEnd + 4);
            if (uCauseEnd != std::string::npos)
            {
                CauseLine kLine;
                kLine.sSubject = sLine.substr(3, uSubjectEnd - 3);
                kLine.sCause = sLine.substr(uSubjectEnd + 4, uCauseEnd - uSubjectEnd - 4);
                kLine.sText = sLine.substr(uCauseEnd + 3);
                kLine.uBegin = uLine;
                kLine.uEnd = uNext;
                vOut.push_back(kLine);
            }
        }
        uLine = uNext + 1;
    }
}

}

TEST_CASE_NAMED(FMemoryBaselineTest7, "Unit::MemoryBaseline::baseline causes add exactly one digest cause since the milestone base", "[EngineFilter]")
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
    std::vector<CauseLine> vLines;
    ParseCauseLines(sText, vLines);
    REQUIRE(!vLines.empty());

    const char* apcBase[] = { "C-OOB-SIDE", "C-SLOT-SOLDANIM", "C-SOLDIER-SLOPE", "C-ADDR-FLYER", "C-2DRADIUS",
        "C-PTR03-SENTINEL", "C-PIN-28", "C-D14-RECHECK", "C-SHADER-INIT", "C-WIRE-ORDER", "C-WALKER-GRAVITY",
        "C-PILOT-DELETE", "C-FLAG-PICKUP", "C-CRITSYS-SHIELD", "C-TAT3-STALE", "C-INPUT-ALIGN", "C-PBLLIST-RENDER",
        "C-DOT-ORDER-30", "C-FENV-TERRAIN", "C-SIM-REWRITE-30", "C-WIRE-ORDER-30" };
    const std::set<std::string> kBase(apcBase, apcBase + sizeof(apcBase) / sizeof(apcBase[0]));
    REQUIRE(kBase.size() == 21);

    std::set<std::string> kSeen;
    std::set<std::string> kCauses;
    std::string sSerial;
    for (size_t i = 0; i < vLines.size(); i++)
    {
        CHECK_MESSAGE(kSeen.insert(vLines[i].sSubject + " | " + vLines[i].sCause).second, vLines[i].sSubject);
        kCauses.insert(vLines[i].sCause);
        if (vLines[i].sCause == "C-LAYOUT-SERIAL")
        {
            sSerial += vLines[i].sText + "\n";
        }
    }
    for (std::set<std::string>::const_iterator it = kBase.begin(); it != kBase.end(); ++it)
    {
        CHECK_MESSAGE(kCauses.count(*it) == 1, *it);
    }
    REQUIRE(!sSerial.empty());

    std::vector<std::string> vRows;
    ReadBaselineLines("tests\\smoke\\digest-expected.txt", vRows);
    int iRows = 0;
    for (size_t i = 0; i < vRows.size(); i++)
    {
        std::vector<std::string> vRow;
        SplitOn(vRows[i], '\t', vRow);
        CHECK_MESSAGE(vRow.size() == 2, vRows[i]);
        if (vRow.size() != 2)
        {
            continue;
        }
        size_t uState = vRow[1].find("state=");
        CHECK_MESSAGE(uState != std::string::npos, vRows[i]);
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
        int iRowLines = 0;
        for (size_t j = 0; j < vLines.size(); j++)
        {
            if (vLines[j].sSubject == "digest-expected " + vRow[0])
            {
                iRowLines++;
                CHECK_MESSAGE(vLines[j].sCause == "C-LAYOUT-SERIAL", vRow[0]);
            }
        }
        CHECK_MESSAGE(iRowLines == 1, vRow[0]);
        CHECK_MESSAGE(sSerial.find("`" + vRow[0] + "`") != std::string::npos, vRow[0]);
        const std::string sWhere = vRow[0] + " " + sState;
        CHECK_MESSAGE(sSerial.find(sState) != std::string::npos, sWhere);
        for (size_t uAt = sText.find(sState); uAt != std::string::npos; uAt = sText.find(sState, uAt + 1))
        {
            bool bInSerial = false;
            for (size_t j = 0; j < vLines.size(); j++)
            {
                if (vLines[j].sCause == "C-LAYOUT-SERIAL" && uAt >= vLines[j].uBegin && uAt < vLines[j].uEnd)
                {
                    bInSerial = true;
                }
            }
            CHECK_MESSAGE(bInSerial, sState);
        }
    }
    CHECK(iRows > 0);

    for (size_t i = 0; i < vLines.size(); i++)
    {
        if (kBase.count(vLines[i].sCause) != 0 || vLines[i].sCause == "C-LAYOUT-SERIAL")
        {
            continue;
        }
        CHECK_MESSAGE(vLines[i].sSubject.compare(0, 16, "digest-expected ") != 0, vLines[i].sCause);
        CHECK_MESSAGE(vLines[i].sText.find("state=0x") == std::string::npos, vLines[i].sCause);
    }
}

TEST_CASE_NAMED(FMemoryBaselineTest8, "Unit::MemoryBaseline::baseline movie goldens hold every binkw32 frame of the three segments and pin the content frames", "[EngineFilter]")
{
    std::vector<std::string> vLines;
    ReadBaselineLines("tests\\baseline\\v143\\movie-goldens.txt", vLines);
    struct Segment
    {
        const char* pcMvs;
        const char* pcHash;
        unsigned int uFrames;
        const char* pcFrame30;
        const char* pcFrame300;
    };
    const Segment aSegments[] = { { "shell.mvs", "0xB4A21ED9", 1349, "578C6031CB845B6A", "4CAA07E9C37B43BD" },
        { "ingame.mvs", "0x2AB9757F", 1369, "5F6C7B0BF14F0103", "43BDE01AE68ADC34" },
        { "ingame.mvs", "0x0B78EBBC", 223, "8F9A77BA3D6D5E2A", "" } };
    const size_t uSegments = sizeof(aSegments) / sizeof(aSegments[0]);
    std::map<std::string, std::vector<std::string> > kFrames;
    std::map<std::string, std::string> kCheck;
    std::map<std::string, std::string> kColour;
    std::map<std::string, std::string> kDigest;
    std::string sSection;
    int iFacts = 0;
    for (size_t i = 0; i < vLines.size(); i++)
    {
        const std::string& sLine = vLines[i];
        if (sLine.compare(0, 3, "## ") == 0)
        {
            sSection = sLine.substr(3);
            continue;
        }
        if (sLine[0] == '#')
        {
            continue;
        }
        if (sSection == "colour")
        {
            const size_t uEq = sLine.find('=');
            REQUIRE_MESSAGE(uEq != std::string::npos, sLine);
            CHECK_MESSAGE(kColour.count(sLine.substr(0, uEq)) == 0, sLine);
            kColour[sLine.substr(0, uEq)] = sLine.substr(uEq + 1);
            continue;
        }
        std::vector<std::string> vRow;
        SplitOn(sLine, '\t', vRow);
        if (sSection == "binkw32 ingame digest")
        {
            REQUIRE_MESSAGE(vRow.size() == 2, sLine);
            CHECK_MESSAGE(kDigest.count(vRow[0]) == 0, sLine);
            kDigest[vRow[0]] = vRow[1];
            continue;
        }
        if (sSection == "binkw32 facts")
        {
            CHECK_MESSAGE(vRow.size() == 3, sLine);
            ++iFacts;
            continue;
        }
        REQUIRE_MESSAGE(vRow.size() == 4, sLine);
        CHECK_MESSAGE(vRow[3].size() == 16, sLine);
        CHECK_MESSAGE(vRow[3].find_first_not_of("0123456789ABCDEF") == std::string::npos, sLine);
        const std::string sKey = vRow[0] + " " + vRow[1];
        if (sSection == "binkw32 frames")
        {
            std::vector<std::string>& v = kFrames[sKey];
            CHECK_MESSAGE(atoi(vRow[2].c_str()) == (int)v.size() + 1, sLine);
            v.push_back(vRow[3]);
        }
        else
        {
            REQUIRE_MESSAGE(sSection == "binkw32 check set", sLine);
            kCheck[sKey + " " + vRow[2]] = vRow[3];
        }
    }
    CHECK(kFrames.size() == uSegments);
    CHECK(iFacts > 0);
    size_t uTotal = 0;
    for (size_t s = 0; s < uSegments; s++)
    {
        const Segment& rSeg = aSegments[s];
        const std::string sKey = std::string(rSeg.pcMvs) + " " + rSeg.pcHash;
        CAPTURE(sKey);
        const std::vector<std::string>& v = kFrames[sKey];
        REQUIRE(v.size() == rSeg.uFrames);
        uTotal += v.size();
        CHECK(v[29] == rSeg.pcFrame30);
        CHECK(kCheck[sKey + " 30"] == rSeg.pcFrame30);
        if (rSeg.pcFrame300[0])
        {
            CHECK(v[299] == rSeg.pcFrame300);
            CHECK(kCheck[sKey + " 300"] == rSeg.pcFrame300);
        }
        const unsigned int auCheck[] = { 1, 2, 3, 30, 300, rSeg.uFrames };
        for (size_t k = 0; k < sizeof(auCheck) / sizeof(auCheck[0]); k++)
        {
            if (auCheck[k] > rSeg.uFrames)
            {
                continue;
            }
            char acKey[16];
            sprintf(acKey, " %u", auCheck[k]);
            CHECK_MESSAGE(kCheck[sKey + acKey] == v[auCheck[k] - 1], acKey);
        }
    }
    CHECK(uTotal == 2941);
    CHECK(kCheck.size() == 17);
    CHECK(kColour.size() == 4);
    CHECK(kColour["model"] == "exact");
    CHECK(kColour["spec"].find("B=cv(5*u+y+24205) G=cv(y-(u+v)+30887) R=cv(2*v+y+25040)") != std::string::npos);
    CHECK(kColour["match"].find("2941/2941") == 0);
    CHECK(kDigest.size() == 1);
    CHECK(kDigest["ingame-movie-digest"].find("Sim digest turns=1500 time=") == 0);
    CHECK(kDigest["ingame-movie-digest"].find(" state=0x") != std::string::npos);
}

TEST_CASE_NAMED(FMemoryBaselineTest9, "Unit::MemoryBaseline::cause lines parse subject cause and text and skip malformed lines", "[EngineFilter]")
{
    const std::string sText =
        "# v143 causes\n"
        "- `digest-expected geo1c_con` | C-LAYOUT-SERIAL | `geo1c_con` moved.\n"
        "### C-OLD-HEADING\n"
        "- `no separator` C-X | text\n"
        "- plain bullet | C-X | text\n"
        "- `x64 fmod` | C-X64-FMOD\n"
        "  - `indented` | C-X | text\n"
        "- `movie gain` | C-MOVIE-HWGAIN | The movie gain is DirectSound volume.";
    std::vector<CauseLine> vLines;
    ParseCauseLines(sText, vLines);
    REQUIRE(vLines.size() == 2);
    CHECK(vLines[0].sSubject == "digest-expected geo1c_con");
    CHECK(vLines[0].sCause == "C-LAYOUT-SERIAL");
    CHECK(vLines[0].sText == "`geo1c_con` moved.");
    CHECK(sText.compare(vLines[0].uBegin, 3, "- `") == 0);
    CHECK(vLines[1].sSubject == "movie gain");
    CHECK(vLines[1].sCause == "C-MOVIE-HWGAIN");
    CHECK(vLines[1].uEnd == sText.size());
}
