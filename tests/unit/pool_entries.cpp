#include "third_party/doctest/doctest.h"

#include <stdio.h>
#include <string.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "testutil.h"
#include "Battlefront2/Source/EntityMine.h"
#include "Battlefront2/Source/StickInfo.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Memory/RedScriptLimits.h"
#include "RedEngineFL/Util/RedLog.h"

namespace
{

const unsigned int kX86PoolBytes = 0x1000000u;
const unsigned int kX86StickListCapacity = 349525u;
const unsigned int kX86MineListCapacity = 24966u;

struct EntryReason
{
    const char* pcLabel;
    const char* pcReason;
};

const EntryReason kGrowthOnDemand[] = {
    { "AmmoCounter", "growth on demand" },
    { "ConnectivityGraphFollower", "growth on demand" },
    { "EnergyBar", "growth on demand" },
    { "ParticleTransformer::SizeTransf", "growth on demand" },
    { "SoldierAnimation", "growth on demand" },
};

const EntryReason kPackedPeaks[] = {
    { "Combo::DamageSample", "C-COMBO-BLOCKS" },
};

const EntryReason kSoundFLOwned[] = {
    { "Snd::EngineBase::smVoiceStorage", "Phase 50 AV-01" },
    { "Snd::EngineBase::smStreamStorage", "Phase 50 AV-01" },
};

struct RuntimeAllocSource
{
    const char* pcMission;
    const char* pcSource;
    const char* pcCallsite;
    unsigned int uX86;
    unsigned int uX64;
};

const RuntimeAllocSource kRuntimeAllocSources[] = {
    { "geo1c_xl", "exit", "pvs::Sector::RegisterDynamic SubAllocator page", 1, 2 },
    { "hot1g_xl", "exit", "pvs::Sector::RegisterDynamic SubAllocator page", 1, 3 },
    { "kas2c_uber", "exit", "pvs::Sector::RegisterDynamic SubAllocator page", 3, 7 },
    { "kas2c_uber", "exit", "pvs::Sector::UnregisterDynamic SubAllocator free list", 1, 2 },
    { "kas2c_xl", "exit", "pvs::Sector::RegisterDynamic SubAllocator page", 2, 4 },
    { "kas2g_xl", "exit", "pvs::Sector::RegisterDynamic SubAllocator page", 2, 4 },
};

struct RuntimeBlock
{
    bool bHeap;
    unsigned int uAllocs;
    std::map<std::string, unsigned int> kGrown;
};

typedef std::map<std::pair<std::string, std::string>, RuntimeBlock> RuntimeTable;

struct PoolRow
{
    unsigned int uCount;
    unsigned int uSize;
    unsigned int uPeak;
    unsigned int uGrown;
};

typedef std::map<std::pair<std::string, std::string>, PoolRow> PoolTable;

void SplitTabs(const std::string& sLine, std::vector<std::string>& vOut)
{
    size_t uStart = 0;
    while (true)
    {
        const size_t uEnd = sLine.find('\t', uStart);
        if (uEnd == std::string::npos)
        {
            vOut.push_back(sLine.substr(uStart));
            return;
        }
        vOut.push_back(sLine.substr(uStart, uEnd - uStart));
        uStart = uEnd + 1;
    }
}

unsigned int ParseCount(const std::string& sText)
{
    REQUIRE(!sText.empty());
    unsigned long long ullValue = 0;
    for (size_t i = 0; i < sText.size(); ++i)
    {
        REQUIRE((sText[i] >= '0' && sText[i] <= '9'));
        ullValue = ullValue * 10 + (unsigned int)(sText[i] - '0');
        REQUIRE(ullValue <= 0xFFFFFFFFull);
    }
    return (unsigned int)ullValue;
}

bool IsLuaBin(const std::string& sLabel)
{
    if (sLabel.size() <= 3 || sLabel.compare(0, 3, "Lua") != 0)
    {
        return false;
    }
    for (size_t i = 3; i < sLabel.size(); ++i)
    {
        if (sLabel[i] < '0' || sLabel[i] > '9')
        {
            return false;
        }
    }
    return true;
}

void LoadPoolRows(const char* pcRel, PoolTable& rTable)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRel, sText));
    size_t uStart = 0;
    int iLine = 0;
    while (uStart < sText.size())
    {
        size_t uEnd = sText.find('\n', uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uStart, uEnd - uStart);
        uStart = uEnd + 1;
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        ++iLine;
        if (iLine == 1)
        {
            REQUIRE(sLine.compare(0, 9, "# commit ") == 0);
            continue;
        }
        if (iLine == 2)
        {
            REQUIRE(sLine == "mission\tsource\tkind\tname\tcap_or_count\tsize\tpeak\tused\tallocs_or_grown\tfailed");
            continue;
        }
        if (sLine.empty())
        {
            continue;
        }
        std::vector<std::string> vField;
        SplitTabs(sLine, vField);
        REQUIRE_MESSAGE(vField.size() == 10, sLine);
        if (vField[2] != "pool" || IsLuaBin(vField[3]))
        {
            continue;
        }
        PoolRow kRow;
        kRow.uCount = ParseCount(vField[4]);
        kRow.uSize = ParseCount(vField[5]);
        kRow.uPeak = ParseCount(vField[6]);
        kRow.uGrown = ParseCount(vField[8]);
        REQUIRE_MESSAGE(kRow.uSize != 0, sLine);
        CHECK_MESSAGE(rTable.insert(std::make_pair(std::make_pair(vField[0], vField[3]), kRow)).second, sLine);
    }
}

void LoadRuntimeRows(const char* pcRel, RuntimeTable& rTable)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRel, sText));
    size_t uStart = 0;
    int iLine = 0;
    while (uStart < sText.size())
    {
        size_t uEnd = sText.find('\n', uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uStart, uEnd - uStart);
        uStart = uEnd + 1;
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        ++iLine;
        if (iLine <= 2 || sLine.empty())
        {
            continue;
        }
        std::vector<std::string> vField;
        SplitTabs(sLine, vField);
        REQUIRE_MESSAGE(vField.size() == 10, sLine);
        const bool bRuntime = vField[2] == "heap" && vField[3] == "Runtime";
        const bool bPool = vField[2] == "pool" && !IsLuaBin(vField[3]);
        if (!bRuntime && !bPool)
        {
            continue;
        }
        std::map<std::pair<std::string, std::string>, RuntimeBlock>::iterator it =
            rTable.find(std::make_pair(vField[0], vField[1]));
        if (it == rTable.end())
        {
            RuntimeBlock kNew;
            kNew.bHeap = false;
            kNew.uAllocs = 0;
            it = rTable.insert(std::make_pair(std::make_pair(vField[0], vField[1]), kNew)).first;
        }
        if (bRuntime)
        {
            CHECK_MESSAGE(!it->second.bHeap, sLine);
            it->second.bHeap = true;
            it->second.uAllocs = ParseCount(vField[8]);
        }
        else
        {
            CHECK_MESSAGE(it->second.kGrown.insert(std::make_pair(vField[3], ParseCount(vField[8]))).second, sLine);
        }
    }
}

unsigned int ByteStep(unsigned int uSize)
{
    return (uSize + 63) / uSize;
}

unsigned int PoolStep(unsigned int uSize)
{
    MemoryPool kPool("t1entrystep", 4);
    kPool.Create(0, uSize, 0);
    CHECK(kPool.GetSize() == uSize);
    return kPool.GetGrow();
}

unsigned int ReplayCapacity(unsigned int uConfigured, unsigned int uStep, unsigned int uPeak, unsigned int& ruGrown)
{
    unsigned int uCapacity = uConfigured;
    ruGrown = 0;
    while (uCapacity < uPeak)
    {
        uCapacity += uStep;
        ++ruGrown;
    }
    return uCapacity;
}

bool Listed(const EntryReason* pList, size_t uCount, const std::string& sLabel)
{
    for (size_t i = 0; i < uCount; ++i)
    {
        if (sLabel == pList[i].pcLabel)
        {
            return true;
        }
    }
    return false;
}

std::string Where(const std::string& sMission, const std::string& sLabel)
{
    return sMission + " " + sLabel;
}

alignas(16) char sacEntryHeap[0x10000];
const char* spcEntryNeedle;
int siEntryLines;

void __cdecl CountEntryLines(int iSeverity, int iCategory, char const* pcLine)
{
    if (spcEntryNeedle && strstr(pcLine, spcEntryNeedle))
    {
        ++siEntryLines;
    }
    TestLogObserver(iSeverity, iCategory, pcLine);
}

class EntryProbe
{
public:
    explicit EntryProbe(const char* pcNeedle)
    {
        miHeap = RedCreateHeap(sacEntryHeap, sacEntryHeap + sizeof(sacEntryHeap), 0x10, "T1Entries");
        REQUIRE(miHeap >= 0);
        spcEntryNeedle = pcNeedle;
        siEntryLines = 0;
        mpOldObserver = RedLog::gObserver;
        RedLog::gObserver = CountEntryLines;
        miOldHeap = RedSetCurrentHeap(miHeap);
    }

    ~EntryProbe()
    {
        RedSetCurrentHeap(miOldHeap);
        RedLog::gObserver = mpOldObserver;
        spcEntryNeedle = 0;
        RedDestroyHeap(miHeap);
    }

    int miHeap;
    int miOldHeap;
    RedLog::Observer mpOldObserver;
};

bool CreateRefuses(unsigned int uCount, unsigned int uSize)
{
    MemoryPool kPool("t1entrycreate", 4);
    int iLines = 0;
    {
        EntryProbe kProbe("Memory pool \"t1entrycreate\" refused count");
        kPool.Create(uCount, uSize, 0);
        kPool.Destroy();
        iLines = siEntryLines;
    }
    return iLines == 1;
}

bool GrowthRefuses(unsigned int uGrow, unsigned int uSize)
{
    const bool bOldExpansion = MemoryPool::sErrorOnExpansion;
    MemoryPool kPool("t1entrygrow", 4);
    int iLines = 0;
    void* pFirst = 0;
    void* pSecond = 0;
    {
        EntryProbe kProbe("Memory pool \"t1entrygrow\" refused growth");
        kPool.Create(1, uSize, uGrow);
        pFirst = kPool.Allocate(uSize);
        pSecond = kPool.Allocate(uSize);
        kPool.Free(pFirst);
        kPool.Destroy();
        iLines = siEntryLines;
    }
    MemoryPool::sErrorOnExpansion = bOldExpansion;
    REQUIRE(pFirst);
    CHECK(!pSecond);
    return iLines == 1;
}

unsigned int ScriptCeiling(const RedScriptLimits::ScriptLimit& rLimit, unsigned int uSize)
{
    LogCapture capture;
    return RedScriptLimits::ClampScriptSize(rLimit, "t1entries", 0x7FFFFFFF, (uSize + 3) & ~3u);
}

}

TEST_CASE("pool capacities in entries" * doctest::test_suite("regression"))
{
    PoolTable kX86;
    PoolTable kX64;
    LoadPoolRows("tests\\baseline\\v143\\mem-peaks.txt", kX86);
    LoadPoolRows("tests\\baseline\\v143-x64\\mem-peaks.txt", kX64);
    REQUIRE(!kX86.empty());
    REQUIRE(kX86.size() == kX64.size());

    const bool bX64 = sizeof(void*) == 8;
    const unsigned int uPlatformBytes = kX86PoolBytes / 4u * (unsigned int)sizeof(void*);
    std::map<std::string, std::pair<unsigned int, unsigned int> > kSizes;
    std::set<std::string> kGrowthSeen;
    std::set<std::string> kPackedSeen;

    for (PoolTable::const_iterator it = kX86.begin(); it != kX86.end(); ++it)
    {
        const std::string& sMission = it->first.first;
        const std::string& sLabel = it->first.second;
        const std::string sWhere = Where(sMission, sLabel);
        PoolTable::const_iterator itX64 = kX64.find(it->first);
        REQUIRE_MESSAGE(itX64 != kX64.end(), sWhere);
        const PoolRow& r86 = it->second;
        const PoolRow& r64 = itX64->second;

        std::map<std::string, std::pair<unsigned int, unsigned int> >::iterator itSize = kSizes.find(sLabel);
        if (itSize == kSizes.end())
        {
            kSizes[sLabel] = std::make_pair(r86.uSize, r64.uSize);
        }
        else
        {
            CHECK_MESSAGE(itSize->second.first == r86.uSize, sWhere);
            CHECK_MESSAGE(itSize->second.second == r64.uSize, sWhere);
        }
        CHECK_MESSAGE(r86.uSize % 4 == 0, sWhere);
        CHECK_MESSAGE(r64.uSize % 8 == 0, sWhere);
        CHECK_MESSAGE(r64.uSize <= 2 * r86.uSize, sWhere);

        if (Listed(kPackedPeaks, sizeof(kPackedPeaks) / sizeof(kPackedPeaks[0]), sLabel))
        {
            CHECK_MESSAGE(r64.uPeak <= r86.uPeak, sWhere);
            if (r64.uPeak != r86.uPeak)
            {
                kPackedSeen.insert(sLabel);
            }
        }
        else
        {
            CHECK_MESSAGE(r64.uPeak == r86.uPeak, sWhere);
        }

        if (r86.uCount != r64.uCount)
        {
            kGrowthSeen.insert(sLabel);
            CHECK_MESSAGE(Listed(kGrowthOnDemand, sizeof(kGrowthOnDemand) / sizeof(kGrowthOnDemand[0]), sLabel),
                          sWhere);
        }

        if (r86.uCount == 0 || r64.uCount == 0)
        {
            CHECK_MESSAGE(r86.uCount == r64.uCount, sWhere);
            CHECK_MESSAGE(r86.uGrown == r64.uGrown, sWhere);
            continue;
        }

        const unsigned int uStep86 = bX64 ? ByteStep(r86.uSize) : PoolStep(r86.uSize);
        const unsigned int uStep64 = bX64 ? PoolStep(r64.uSize) : ByteStep(r64.uSize);
        REQUIRE_MESSAGE(uStep86 * r86.uGrown <= r86.uCount, sWhere);
        REQUIRE_MESSAGE(uStep64 * r64.uGrown <= r64.uCount, sWhere);
        const unsigned int uConfigured86 = r86.uCount - uStep86 * r86.uGrown;
        const unsigned int uConfigured64 = r64.uCount - uStep64 * r64.uGrown;
        CHECK_MESSAGE(uConfigured86 == uConfigured64, sWhere);

        unsigned int uGrown86 = 0;
        unsigned int uGrown64 = 0;
        CHECK_MESSAGE(ReplayCapacity(uConfigured86, uStep86, r86.uPeak, uGrown86) == r86.uCount, sWhere);
        CHECK_MESSAGE(uGrown86 == r86.uGrown, sWhere);
        CHECK_MESSAGE(ReplayCapacity(uConfigured64, uStep64, r64.uPeak, uGrown64) == r64.uCount, sWhere);
        CHECK_MESSAGE(uGrown64 == r64.uGrown, sWhere);
    }

    for (size_t i = 0; i < sizeof(kGrowthOnDemand) / sizeof(kGrowthOnDemand[0]); ++i)
    {
        CHECK_MESSAGE(strcmp(kGrowthOnDemand[i].pcReason, "growth on demand") == 0, kGrowthOnDemand[i].pcLabel);
        CHECK_MESSAGE(kGrowthSeen.count(kGrowthOnDemand[i].pcLabel) == 1, kGrowthOnDemand[i].pcLabel);
    }
    for (size_t i = 0; i < sizeof(kPackedPeaks) / sizeof(kPackedPeaks[0]); ++i)
    {
        CHECK_MESSAGE(kPackedSeen.count(kPackedPeaks[i].pcLabel) == 1, kPackedPeaks[i].pcLabel);
    }
    for (size_t i = 0; i < sizeof(kSoundFLOwned) / sizeof(kSoundFLOwned[0]); ++i)
    {
        CHECK_MESSAGE(strcmp(kSoundFLOwned[i].pcReason, "Phase 50 AV-01") == 0, kSoundFLOwned[i].pcLabel);
        CHECK_MESSAGE(kSizes.count(kSoundFLOwned[i].pcLabel) == 0, kSoundFLOwned[i].pcLabel);
    }

    for (std::map<std::string, std::pair<unsigned int, unsigned int> >::const_iterator it = kSizes.begin();
         it != kSizes.end(); ++it)
    {
        const std::string& sLabel = it->first;
        const unsigned int uSize86 = it->second.first;
        const unsigned int uSize = bX64 ? it->second.second : uSize86;
        const unsigned int uX86Ceiling = kX86PoolBytes / uSize86;
        const unsigned int uCeiling = uPlatformBytes / uSize;
        char acWhere[192];
        snprintf(acWhere, sizeof(acWhere), "%s size %u x86 size %u ceiling %u x86 ceiling %u", sLabel.c_str(), uSize, uSize86, uCeiling,
                uX86Ceiling);
        CHECK_MESSAGE(uCeiling >= uX86Ceiling, acWhere);
        CHECK_MESSAGE(!CreateRefuses(uX86Ceiling, uSize), acWhere);
        CHECK_MESSAGE(!CreateRefuses(uCeiling, uSize), acWhere);
        CHECK_MESSAGE(CreateRefuses(uCeiling + 1, uSize), acWhere);
        CHECK_MESSAGE(!GrowthRefuses(uX86Ceiling, uSize), acWhere);
        CHECK_MESSAGE(!GrowthRefuses(uCeiling, uSize), acWhere);
        CHECK_MESSAGE(GrowthRefuses(uCeiling + 1, uSize), acWhere);
        CHECK_MESSAGE(ScriptCeiling(RedScriptLimits::kSetMemoryPoolSize, uSize) == uCeiling, acWhere);
        CHECK_MESSAGE(ScriptCeiling(RedScriptLimits::kAddWalkerType, uSize) == uCeiling, acWhere);
    }
}

TEST_CASE("pool capacities in entries for the stick and mine lists" * doctest::test_suite("regression"))
{
    const unsigned int uPlatformBytes = kX86PoolBytes / 4u * (unsigned int)sizeof(void*);
    CHECK(kStickListCapacity >= kX86StickListCapacity);
    CHECK(kMineListCapacity >= kX86MineListCapacity);
    CHECK(kStickListCapacity == uPlatformBytes / (unsigned int)sizeof(StickInfo));
    CHECK(kMineListCapacity == uPlatformBytes / (unsigned int)sizeof(EntityMine));
    CHECK(kStickListCapacity == (sizeof(void*) == 8 ? 524288u : kX86StickListCapacity));
    CHECK(kMineListCapacity == (sizeof(void*) == 8 ? 33288u : kX86MineListCapacity));
}

TEST_CASE("x64 runtime allocs equal the extra pool growths plus their named sources" * doctest::test_suite("regression"))
{
    RuntimeTable kX86;
    RuntimeTable kX64;
    LoadRuntimeRows("tests\\baseline\\v143\\mem-peaks.txt", kX86);
    LoadRuntimeRows("tests\\baseline\\v143-x64\\mem-peaks.txt", kX64);
    REQUIRE(!kX86.empty());

    const size_t uSources = sizeof(kRuntimeAllocSources) / sizeof(kRuntimeAllocSources[0]);
    std::vector<bool> vSeen(uSources, false);
    std::set<std::pair<std::string, std::string> > kKeys;
    for (RuntimeTable::const_iterator it = kX86.begin(); it != kX86.end(); ++it)
    {
        kKeys.insert(it->first);
    }
    for (RuntimeTable::const_iterator it = kX64.begin(); it != kX64.end(); ++it)
    {
        kKeys.insert(it->first);
    }

    unsigned int uRows = 0;
    for (std::set<std::pair<std::string, std::string> >::const_iterator it = kKeys.begin(); it != kKeys.end(); ++it)
    {
        const std::string sWhere = it->first + " " + it->second;
        RuntimeTable::const_iterator it86 = kX86.find(*it);
        RuntimeTable::const_iterator it64 = kX64.find(*it);
        const bool bHeap86 = it86 != kX86.end() && it86->second.bHeap;
        const bool bHeap64 = it64 != kX64.end() && it64->second.bHeap;
        CHECK_MESSAGE(bHeap86 == bHeap64, sWhere);
        if (!bHeap86 || !bHeap64)
        {
            continue;
        }
        ++uRows;

        long long llGrown = 0;
        std::set<std::string> kLabels;
        for (std::map<std::string, unsigned int>::const_iterator itG = it86->second.kGrown.begin();
             itG != it86->second.kGrown.end(); ++itG)
        {
            kLabels.insert(itG->first);
        }
        for (std::map<std::string, unsigned int>::const_iterator itG = it64->second.kGrown.begin();
             itG != it64->second.kGrown.end(); ++itG)
        {
            kLabels.insert(itG->first);
        }
        for (std::set<std::string>::const_iterator itL = kLabels.begin(); itL != kLabels.end(); ++itL)
        {
            std::map<std::string, unsigned int>::const_iterator itG86 = it86->second.kGrown.find(*itL);
            std::map<std::string, unsigned int>::const_iterator itG64 = it64->second.kGrown.find(*itL);
            llGrown += (long long)(itG64 != it64->second.kGrown.end() ? itG64->second : 0);
            llGrown -= (long long)(itG86 != it86->second.kGrown.end() ? itG86->second : 0);
        }

        long long llNamed = 0;
        for (size_t i = 0; i < uSources; ++i)
        {
            if (it->first == kRuntimeAllocSources[i].pcMission && it->second == kRuntimeAllocSources[i].pcSource)
            {
                vSeen[i] = true;
                llNamed += (long long)kRuntimeAllocSources[i].uX64 - (long long)kRuntimeAllocSources[i].uX86;
            }
        }

        const long long llDelta = (long long)it64->second.uAllocs - (long long)it86->second.uAllocs;
        char acWhere[256];
        snprintf(acWhere, sizeof(acWhere), "%s runtime allocs x64 %u x86 %u delta %lld extra pool growths %lld named %lld",
                 sWhere.c_str(), it64->second.uAllocs, it86->second.uAllocs, llDelta, llGrown, llNamed);
        CHECK_MESSAGE(llDelta == llGrown + llNamed, acWhere);
    }
    CHECK(uRows == 109u);

    for (size_t i = 0; i < uSources; ++i)
    {
        const std::string sEntry = std::string(kRuntimeAllocSources[i].pcMission) + " " +
                                   kRuntimeAllocSources[i].pcSource + " " + kRuntimeAllocSources[i].pcCallsite;
        CHECK_MESSAGE(vSeen[i], sEntry);
        CHECK_MESSAGE(kRuntimeAllocSources[i].uX64 > kRuntimeAllocSources[i].uX86, sEntry);
    }
}
