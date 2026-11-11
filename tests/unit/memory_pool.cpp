#include "third_party/doctest/doctest.h"

#include <algorithm>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

#include "testutil.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Memory/RedScriptLimits.h"
#include "RedEngineFL/Util/RedLog.h"
#include "Battlefront2/Source/GameMemory.h"
#include "Battlefront2/Source/LuaMemory.h"

TEST_CASE("memory pool tracks peak and grown chunks" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1peak", 16);
    kPool.Create(2, 16, 0);
    void* apBlock[3];
    for (int i = 0; i < 3; ++i)
    {
        apBlock[i] = kPool.Allocate(16);
        REQUIRE(apBlock[i]);
    }
    kPool.Free(apBlock[2]);
    kPool.Free(apBlock[1]);
    CHECK(kPool.GetPeak() == 3);
    CHECK(kPool.GetGrown() == 1);
    CHECK(kPool.mUsed == 1);
    kPool.Free(apBlock[0]);
    CHECK(kPool.mUsed == 0);
    CHECK(kPool.GetPeak() == 3);
}

TEST_CASE("memory pool set copies stats" * doctest::test_suite("regression"))
{
    MemoryPool kSource("t1setsrc", 16);
    kSource.Create(2, 16, 0);
    void* apBlock[3];
    for (int i = 0; i < 3; ++i)
    {
        apBlock[i] = kSource.Allocate(16);
        REQUIRE(apBlock[i]);
    }
    kSource.mFullReported = true;
    kSource.mFullReportedStrict = true;
    MemoryPool kAlias("t1setdst", 16);
    kAlias.Set(kSource);
    CHECK(kAlias.GetPeak() == 3);
    CHECK(kAlias.GetGrown() == 1);
    CHECK(kAlias.mFullReported);
    CHECK(kAlias.mFullReportedStrict);
    for (int i = 0; i < 3; ++i)
    {
        kSource.Free(apBlock[i]);
    }
}

TEST_CASE("memory pool stats survive reset and destroy for the exit report" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1destroy", 16);
    kPool.Create(2, 16, 0);
    void* apBlock[3];
    for (int i = 0; i < 3; ++i)
    {
        apBlock[i] = kPool.Allocate(16);
        REQUIRE(apBlock[i]);
    }
    kPool.mFullReported = true;
    for (int i = 0; i < 3; ++i)
    {
        kPool.Free(apBlock[i]);
    }
    kPool.Destroy();
    CHECK(kPool.GetPeak() == 3);
    CHECK(kPool.GetGrown() == 1);
    CHECK(kPool.mFullReported);
    kPool.Reset();
    CHECK(kPool.GetPeak() == 3);
    CHECK(kPool.GetGrown() == 1);
    CHECK(kPool.mFullReported);
    kPool.ClearStats();
    CHECK(kPool.GetPeak() == 0);
    CHECK(kPool.GetGrown() == 0);
    CHECK(!kPool.mFullReported);
}

namespace
{

void DestroyAllOf(MemoryPool& rPool)
{
    PblList<MemoryPool>* pList = GetMemoryPoolList();
    std::vector<MemoryPool*> vOthers;
    for (PblList<MemoryPool>::Itor itor(*pList); itor; ++itor)
    {
        if ((MemoryPool*)itor != &rPool)
        {
            vOthers.push_back(itor);
        }
    }
    for (size_t i = 0; i < vOthers.size(); ++i)
    {
        pList->Remove(&vOthers[i]->mNode);
    }
    REQUIRE(pList->GetCount() == 1);
    const bool bOldExpansion = MemoryPool::sErrorOnExpansion;
    const bool bOldPostLoad = MemoryPool::sErrorOnExpansionPostLoad;
    MemoryPool::DestroyAll();
    MemoryPool::sErrorOnExpansion = bOldExpansion;
    MemoryPool::sErrorOnExpansionPostLoad = bOldPostLoad;
    for (size_t i = 0; i < vOthers.size(); ++i)
    {
        pList->AddLast(&vOthers[i]->mNode, vOthers[i]);
    }
}

}

TEST_CASE("memory pool stats reset on destroy all" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1destroyall", 16);
    kPool.mPeak = 5;
    kPool.mGrown = 2;
    kPool.mFullReported = true;
    REQUIRE(!kPool.mPool);
    DestroyAllOf(kPool);
    CHECK(kPool.GetPeak() == 0);
    CHECK(kPool.GetGrown() == 0);
    CHECK(!kPool.mFullReported);
}

TEST_CASE("memory pool full line logs once on growth" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1full", 16);
    kPool.Create(2, 16, 0);
    LogCapture capture;
    void* apBlock[4];
    for (int i = 0; i < 4; ++i)
    {
        apBlock[i] = kPool.Allocate(16);
        REQUIRE(apBlock[i]);
    }
    CHECK(capture.Count("Memory pool \"t1full\" is full; raise count to at least 3") == 1);
    CHECK(capture.Count("t1full\" is full") == 1);
    for (int i = 0; i < 4; ++i)
    {
        kPool.Free(apBlock[i]);
    }
}

TEST_CASE("memory pool full line logs once when gated" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1gated", 16);
    kPool.Create(2, 16, 0);
    void* apBlock[2];
    {
        LogCapture capture;
        for (int i = 0; i < 2; ++i)
        {
            apBlock[i] = kPool.Allocate(16);
            REQUIRE(apBlock[i]);
        }
        CHECK(kPool.IsFull());
        CHECK(kPool.IsFull());
        CHECK(kPool.IsFull());
        CHECK(!kPool.IsSufficientSpace(1));
        CHECK(capture.Count("t1gated\" is full") == 1);
        CHECK(capture.Count("Memory pool \"t1gated\" is full; raise count to at least 3") == 1);
        for (int i = 0; i < 2; ++i)
        {
            kPool.Free(apBlock[i]);
        }
    }
    kPool.Destroy();
    kPool.ClearStats();
    kPool.Create(2, 16, 0);
    {
        LogCapture capture;
        for (int i = 0; i < 2; ++i)
        {
            apBlock[i] = kPool.Allocate(16);
            REQUIRE(apBlock[i]);
        }
        CHECK(kPool.IsFull());
        CHECK(capture.Count("t1gated\" is full") == 1);
        for (int i = 0; i < 2; ++i)
        {
            kPool.Free(apBlock[i]);
        }
    }
    kPool.Destroy();
}

TEST_CASE("memory pool full line is error when strict" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1strict", 16);
    kPool.Create(2, 16, 0);
    void* apBlock[3];
    {
        ExpectLog expect("Memory pool \"t1strict\" is full; raise count to at least 3");
        MemoryPool::sErrorOnExpansionPostLoad = true;
        for (int i = 0; i < 3; ++i)
        {
            apBlock[i] = kPool.Allocate(16);
            REQUIRE(apBlock[i]);
        }
        MemoryPool::sErrorOnExpansionPostLoad = false;
    }
    for (int i = 0; i < 3; ++i)
    {
        kPool.Free(apBlock[i]);
    }
}

TEST_CASE("memory pool full line escalates once when strict arms after a load report" *
          doctest::test_suite("regression"))
{
    MemoryPool kPool("t1escalate", 16);
    kPool.Create(2, 16, 0);
    void* apBlock[2];
    for (int i = 0; i < 2; ++i)
    {
        apBlock[i] = kPool.Allocate(16);
        REQUIRE(apBlock[i]);
    }
    {
        LogCapture capture;
        CHECK(kPool.IsFull());
        CHECK(capture.Count("t1escalate\" is full") == 1);
    }
    {
        LogCapture capture;
        ExpectLog expect("Memory pool \"t1escalate\" is full; raise count to at least 3");
        MemoryPool::sErrorOnExpansionPostLoad = true;
        CHECK(kPool.IsFull());
        CHECK(kPool.IsFull());
        CHECK(!kPool.IsSufficientSpace(1));
        MemoryPool::sErrorOnExpansionPostLoad = false;
        CHECK(capture.Count("t1escalate\" is full") == 1);
    }
    for (int i = 0; i < 2; ++i)
    {
        kPool.Free(apBlock[i]);
    }
    kPool.Destroy();
}

namespace
{

alignas(16) char sacHeapArena[0x2000];

RedHeapStats GetStats(int iHeap)
{
    RedHeapStats kStats;
    memset(&kStats, 0, sizeof(kStats));
    REQUIRE(RedGetHeapStats(iHeap, &kStats));
    return kStats;
}

void CreateAdjacentHeaps(int& iLow, int& iHigh)
{
    iLow = RedCreateHeap(sacHeapArena, sacHeapArena + 0x1000, 0x10, "T1Low");
    iHigh = RedCreateHeap(sacHeapArena + 0x1000, sacHeapArena + 0x2000, 0x10, "T1High");
    REQUIRE(iLow >= 0);
    REQUIRE(iHigh == iLow + 1);
}

}

TEST_CASE("heap stats count used peak allocs and failures" * doctest::test_suite("regression"))
{
    const int iHeap = RedCreateHeap(sacHeapArena, sacHeapArena + sizeof(sacHeapArena), 0x10, "T1Stats");
    REQUIRE(iHeap >= 0);
    RedHeapStats kStats = GetStats(iHeap);
    CHECK(kStats.uUsed == 0);
    CHECK(kStats.uAllocs == 0);
    CHECK(kStats.uFailed == 0);
    CHECK(kStats.uCap == RedGetHeapSize(iHeap));
    void* p1 = RedAllocFromHeap(iHeap, 100, false);
    void* p2 = RedAllocFromHeap(iHeap, 200, false);
    void* p3 = RedAllocFromHeap(iHeap, 300, true);
    REQUIRE(p1);
    REQUIRE(p2);
    REQUIRE(p3);
    const unsigned int uPeakUsed = GetStats(iHeap).uUsed;
    RedFreeToHeap(iHeap, p2, 0);
    kStats = GetStats(iHeap);
    CHECK(kStats.uPeak == uPeakUsed);
    CHECK(kStats.uPeak >= kStats.uUsed);
    CHECK(kStats.uAllocs == 3);
    CHECK(kStats.uFailed == 0);
    CHECK(!RedAllocFromHeap(iHeap, sizeof(sacHeapArena) * 2, false));
    kStats = GetStats(iHeap);
    CHECK(kStats.uFailed == 1);
    CHECK(kStats.uAllocs == 3);
    RedFreeToHeap(iHeap, p1, 0);
    RedFreeToHeap(iHeap, p3, 0);
    kStats = GetStats(iHeap);
    CHECK(kStats.uUsed == 0);
    RedDestroyHeap(iHeap);
    CHECK(!RedGetHeapStats(iHeap, &kStats));
    CHECK(!RedGetHeapStats(-1, &kStats));
    CHECK(!RedGetHeapStats(64, &kStats));
}

TEST_CASE("heap stats survive move heap end" * doctest::test_suite("regression"))
{
    int iLow = -1;
    int iHigh = -1;
    CreateAdjacentHeaps(iLow, iHigh);
    void* pLow = RedAllocFromHeap(iLow, 100, false);
    void* pHigh = RedAllocFromHeap(iHigh, 100, true);
    REQUIRE(pLow);
    REQUIRE(pHigh);
    const RedHeapStats kLowBefore = GetStats(iLow);
    const RedHeapStats kHighBefore = GetStats(iHigh);
    RedMoveHeapEnd(iLow, 64);
    CHECK(RedGetHeapSize(iLow) == 0x1040);
    CHECK(RedGetHeapSize(iHigh) == 0xFC0);
    RedMoveHeapEnd(iLow, -64);
    CHECK(RedGetHeapSize(iLow) == 0x1000);
    CHECK(RedGetHeapSize(iHigh) == 0x1000);
    const RedHeapStats kLowAfter = GetStats(iLow);
    const RedHeapStats kHighAfter = GetStats(iHigh);
    CHECK(kLowAfter.uUsed == kLowBefore.uUsed);
    CHECK(kHighAfter.uUsed == kHighBefore.uUsed);
    CHECK(kLowAfter.uAllocs == kLowBefore.uAllocs);
    CHECK(kHighAfter.uAllocs == kHighBefore.uAllocs);
    CHECK(kLowAfter.uPeak == kLowBefore.uPeak);
    CHECK(kHighAfter.uPeak == kHighBefore.uPeak);
    RedFreeToHeap(iLow, pLow, 0);
    RedFreeToHeap(iHigh, pHigh, 0);
    CHECK(GetStats(iLow).uUsed == 0);
    CHECK(GetStats(iHigh).uUsed == 0);
    RedDestroyHeap(iHigh);
    RedDestroyHeap(iLow);
}

TEST_CASE("move heap end refuses a failed allocation" * doctest::test_suite("regression"))
{
    int iLow = -1;
    int iHigh = -1;
    CreateAdjacentHeaps(iLow, iHigh);
    void* pFill = RedAllocFromHeap(iHigh, RedGetHeapFree(iHigh) - 0x10, false);
    REQUIRE(pFill);
    REQUIRE(RedGetHeapFree(iHigh) == 0);
    void* pFillLow = RedAllocFromHeap(iLow, RedGetHeapFree(iLow) - 0x10, false);
    REQUIRE(pFillLow);
    REQUIRE(RedGetHeapFree(iLow) == 0);
    const unsigned int uLowSize = RedGetHeapSize(iLow);
    const unsigned int uHighSize = RedGetHeapSize(iHigh);
    RedMoveHeapEnd(iLow, 0);
    RedMoveHeapEnd(iLow, 16);
    RedMoveHeapEnd(iLow, 64);
    CHECK(RedGetHeapSize(iLow) == uLowSize);
    RedMoveHeapEnd(iLow, -16);
    RedMoveHeapEnd(iLow, -64);
    CHECK(RedGetHeapSize(iLow) == uLowSize);
    CHECK(RedGetHeapSize(iHigh) == uHighSize);
    RedFreeToHeap(iLow, pFillLow, 0);
    RedFreeToHeap(iHigh, pFill, 0);
    RedDestroyHeap(iHigh);
    RedDestroyHeap(iLow);
}

TEST_CASE("move heap end moves only the aligned block it transfers" * doctest::test_suite("regression"))
{
    int iLow = -1;
    int iHigh = -1;
    CreateAdjacentHeaps(iLow, iHigh);
    RedMoveHeapEnd(iLow, 40);
    CHECK(RedGetHeapSize(iLow) == 0x1020);
    CHECK(RedGetHeapSize(iHigh) == 0xFE0);
    CHECK(RedGetHeapFree(iLow) == RedGetHeapSize(iLow));
    CHECK(RedGetHeapFree(iHigh) == RedGetHeapSize(iHigh));
    RedMoveHeapEnd(iLow, -40);
    CHECK(RedGetHeapSize(iLow) == 0x1000);
    CHECK(RedGetHeapSize(iHigh) == 0x1000);
    CHECK(RedGetHeapFree(iLow) == RedGetHeapSize(iLow));
    CHECK(RedGetHeapFree(iHigh) == RedGetHeapSize(iHigh));
    void* pLow = RedAllocFromHeap(iLow, RedGetHeapFree(iLow) - 0x10, false);
    REQUIRE(pLow);
    RedFreeToHeap(iLow, pLow, 0);
    CHECK(RedGetHeapFree(iLow) == RedGetHeapSize(iLow));
    RedDestroyHeap(iHigh);
    RedDestroyHeap(iLow);
}

TEST_CASE("memory pool create refuses overflowing count" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1ovf", 16);
    {
        LogCapture capture;
        kPool.Create(0x40000001, 16, 0);
        CHECK(!kPool.mPool);
        CHECK(!kPool.mFree);
        CHECK(kPool.mCount == 0);
        CHECK(capture.Count("Memory pool \"t1ovf\" refused count 1073741825 size 16") == 1);
    }
    CHECK(kPool.GetGrow() == 4);
    kPool.SetCount(0x40000001);
    {
        LogCapture capture;
        void* pFirst = kPool.Allocate(16);
        void* pSecond = kPool.Allocate(16);
        REQUIRE(pFirst);
        REQUIRE(pSecond);
        CHECK(kPool.mPool == pFirst);
        CHECK(kPool.mCount == kPool.GetGrow());
        CHECK(kPool.mUsed == 2);
        CHECK(!kPool.IsPresent((char*)kPool.mPool + kPool.GetGrow() * 16));
        CHECK(capture.Count("Memory pool \"t1ovf\" refused count 1073741825 size 16") == 1);
        CHECK(capture.Count("t1ovf\" refused growth") == 0);
        kPool.Free(pSecond);
        kPool.Free(pFirst);
    }
    kPool.Destroy();
}

TEST_CASE("memory pool create refuses count over ceiling" * doctest::test_suite("regression"))
{
    const unsigned int uOver = RedScriptLimits::kPoolBytesMax / 16 + 1;
    MemoryPool kPool("t1ceil", 16);
    {
        LogCapture capture;
        kPool.Create(uOver, 16, 0);
        CHECK(!kPool.mPool);
        CHECK(!kPool.mFree);
        CHECK(capture.Count("Memory pool \"t1ceil\" refused count 1048577 size 16") == 1);
    }
    {
        LogCapture capture;
        kPool.Create(RedScriptLimits::kPoolBytesMax / 16 / 1024, 16, 0);
        REQUIRE(kPool.mPool);
        CHECK(kPool.mCount == 1024);
        CHECK(capture.Count("t1ceil\" refused") == 0);
        void* pBlock = kPool.Allocate(16);
        CHECK(pBlock == kPool.mPool);
        kPool.Free(pBlock);
    }
    kPool.Destroy();
}

namespace
{

char sacTinyHeap[0x1000];
int siNoChunkLines;

void __cdecl CountNoChunk(int iSeverity, int iCategory, char const* pcLine)
{
    if (strstr(pcLine, "Memory pool \"t1nochunk\" chunk of 16000 bytes not allocated"))
    {
        ++siNoChunkLines;
    }
    TestLogObserver(iSeverity, iCategory, pcLine);
}

}

TEST_CASE("memory pool create survives failed heap block" * doctest::test_suite("regression"))
{
    const int iHeap = RedCreateHeap(sacTinyHeap, sacTinyHeap + sizeof(sacTinyHeap), 0x10, "T1Tiny");
    REQUIRE(iHeap >= 0);
    MemoryPool kPool("t1nochunk", 16);
    siNoChunkLines = 0;
    RedLog::Observer pOldObserver = RedLog::gObserver;
    RedLog::gObserver = CountNoChunk;
    const int iOldHeap = RedSetCurrentHeap(iHeap);
    kPool.Create(1000, 16, 0);
    RedSetCurrentHeap(iOldHeap);
    RedLog::gObserver = pOldObserver;
    CHECK(!kPool.mPool);
    CHECK(!kPool.mFree);
    CHECK(kPool.mCount == 0);
    CHECK(siNoChunkLines == 1);
    void* pBlock = kPool.Allocate(16);
    REQUIRE(pBlock);
    REQUIRE(kPool.mPool == pBlock);
    CHECK(siNoChunkLines == 1);
    CHECK(kPool.mCount == kPool.GetGrow());
    const char* pcChunk = (const char*)kPool.mPool;
    CHECK(kPool.IsPresent((void*)(pcChunk + (kPool.GetGrow() - 1) * 16)));
    CHECK(!kPool.IsPresent((void*)(pcChunk + kPool.GetGrow() * 16)));
    CHECK(!kPool.IsPresent((void*)(pcChunk + 999 * 16)));
    kPool.Free(pBlock);
    kPool.Destroy();
    RedDestroyHeap(iHeap);
}

TEST_CASE("memory pool growth refuses overflowing grow" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1grow", 16);
    kPool.Create(1, 16, 0x10000001);
    void* pBlock = kPool.Allocate(16);
    REQUIRE(pBlock);
    {
        LogCapture capture;
        CHECK(!kPool.Allocate(16));
        CHECK(capture.Count("Memory pool \"t1grow\" refused growth 268435457 size 16") == 1);
    }
    CHECK(kPool.mCount == 1);
    CHECK(kPool.mUsed == 1);
    kPool.Free(pBlock);
    kPool.Destroy();
}

namespace
{

alignas(16) char sacChunkArena[0x8000];
alignas(16) char sacChunkOther[0x2000];

class ChunkHeap
{
public:
    explicit ChunkHeap(const char* pcName)
    {
        miHeap = RedCreateHeap(sacChunkArena, sacChunkArena + sizeof(sacChunkArena), 0x10, pcName);
        REQUIRE(miHeap >= 0);
        miOldHeap = RedSetCurrentHeap(miHeap);
    }

    ~ChunkHeap()
    {
        RedSetCurrentHeap(miOldHeap);
        RedDestroyHeap(miHeap);
    }

    int miHeap;
    int miOldHeap;
};

unsigned int ChunkNeed(unsigned int uBytes)
{
    const unsigned int uHdr = (0x10 + kRedTagBytes - 1) & ~0xFu;
    return (uBytes + 0x10 - 1 + uHdr) & ~0xFu;
}

void GrowTwice(MemoryPool& rPool, void** apBlock)
{
    rPool.Create(2, 16, 2);
    for (int i = 0; i < 6; ++i)
    {
        apBlock[i] = rPool.Allocate(16);
        REQUIRE(apBlock[i]);
    }
    REQUIRE(rPool.GetGrown() == 2);
}

void CheckEveryChunkPresent(const MemoryPool& rPool, void* pOwnerPool, unsigned int uFirst)
{
    for (unsigned int i = 0; i < uFirst; ++i)
    {
        CHECK(rPool.IsPresent((char*)pOwnerPool + i * rPool.GetSize()));
    }
    CHECK(!rPool.IsPresent((char*)pOwnerPool + uFirst * rPool.GetSize()));
    for (const MemoryPool::GrowthChunk* pNode = rPool.mChunks; pNode != 0; pNode = pNode->pNext)
    {
        for (unsigned int i = 0; i < pNode->uCount; ++i)
        {
            CHECK(rPool.IsPresent((char*)pNode->pChunk + i * rPool.GetSize()));
        }
        CHECK(!rPool.IsPresent((char*)pNode->pChunk + pNode->uCount * rPool.GetSize()));
    }
}

}

TEST_CASE("memory pool destroy frees every growth chunk" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkDestroy");
    const unsigned int uBefore = GetStats(kHeap.miHeap).uUsed;
    MemoryPool kPool("t1chunkdestroy", 16);
    void* apBlock[6];
    GrowTwice(kPool, apBlock);
    CHECK(GetStats(kHeap.miHeap).uUsed > uBefore);
    kPool.Destroy();
    CHECK(GetStats(kHeap.miHeap).uUsed == uBefore);
    CHECK(!kPool.mPool);
    CHECK(!kPool.mChunks);
}

TEST_CASE("memory pool destructor frees every growth chunk" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkDtor");
    const unsigned int uBefore = GetStats(kHeap.miHeap).uUsed;
    {
        MemoryPool kPool("t1chunkdtor", 16);
        void* apBlock[6];
        GrowTwice(kPool, apBlock);
        CHECK(GetStats(kHeap.miHeap).uUsed > uBefore);
    }
    CHECK(GetStats(kHeap.miHeap).uUsed == uBefore);
}

TEST_CASE("memory pool destroy all frees every growth chunk" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkAll");
    const unsigned int uBefore = GetStats(kHeap.miHeap).uUsed;
    MemoryPool kPool("t1chunkall", 16);
    void* apBlock[6];
    GrowTwice(kPool, apBlock);
    DestroyAllOf(kPool);
    CHECK(GetStats(kHeap.miHeap).uUsed == uBefore);
}

TEST_CASE("memory pool is present covers every chunk and nothing else" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkPresent");
    void* pOther = RedAllocFromHeap(kHeap.miHeap, 16, 0);
    REQUIRE(pOther);
    MemoryPool kPool("t1chunkpresent", 16);
    void* apBlock[6];
    GrowTwice(kPool, apBlock);
    for (int j = 0; j < 6; ++j)
    {
        CHECK(kPool.IsPresent(apBlock[j]));
    }
    REQUIRE(kPool.mChunks);
    REQUIRE(kPool.mChunks->pNext);
    CHECK(!kPool.mChunks->pNext->pNext);
    CheckEveryChunkPresent(kPool, kPool.mPool, 2);
    CHECK(!kPool.IsPresent(pOther));
    kPool.Destroy();
    RedFreeToHeap(kHeap.miHeap, pOther, 0);
}

TEST_CASE("memory pool first chunk keeps its own extent" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1chunkfirst", 16);
    kPool.Create(2, 16, 8);
    void* apBlock[3];
    for (int i = 0; i < 3; ++i)
    {
        apBlock[i] = kPool.Allocate(16);
        REQUIRE(apBlock[i]);
    }
    CHECK(kPool.mCount == 10);
    CHECK(kPool.mFirstCount == 2);
    CHECK(!kPool.IsPresent((char*)kPool.mPool + 2 * 16));
    CHECK(kPool.IsPresent((char*)kPool.mPool + 16));
    kPool.Destroy();
}

TEST_CASE("memory pool view sees its chunks and frees nothing" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkView");
    const unsigned int uBefore = GetStats(kHeap.miHeap).uUsed;
    MemoryPool kOwner("t1chunkowner", 16);
    void* apBlock[6];
    GrowTwice(kOwner, apBlock);
    const unsigned int uGrown = GetStats(kHeap.miHeap).uUsed;
    MemoryPool::GrowthChunk* pOwnerHead = kOwner.mChunks;
    {
        MemoryPool kView("t1chunkview", 16);
        kView.Set(kOwner);
        for (int j = 0; j < 6; ++j)
        {
            CHECK(kView.IsPresent(apBlock[j]));
        }
        CheckEveryChunkPresent(kView, kOwner.mPool, 2);
        kView.Destroy();
        CHECK(GetStats(kHeap.miHeap).uUsed == uGrown);
    }
    CHECK(GetStats(kHeap.miHeap).uUsed == uGrown);
    CHECK(kOwner.mChunks == pOwnerHead);
    {
        MemoryPool kGrowing("t1chunkgrowing", 16);
        kGrowing.Set(kOwner);
        REQUIRE(!kGrowing.mFree);
        void* pViewBlock = kGrowing.Allocate(16);
        REQUIRE(pViewBlock);
        MemoryPool::GrowthChunk* pViewNode = kGrowing.mChunks;
        REQUIRE(pViewNode);
        CHECK(pViewNode != pOwnerHead);
        CHECK(pViewNode->pNext == pOwnerHead);
        CHECK(kOwner.mChunks == pOwnerHead);
        CHECK(kGrowing.IsPresent(pViewBlock));
        CHECK(!kOwner.IsPresent(pViewBlock));
        RedFreeToHeap(kHeap.miHeap, pViewNode->pChunk, 0);
        pViewNode->pNext = 0;
        kGrowing.mPoolOwner = true;
        kGrowing.Reset();
    }
    kOwner.Destroy();
    CHECK(GetStats(kHeap.miHeap).uUsed == uBefore);
}

TEST_CASE("memory pool chunk charges reconcile with the heap" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkCharge");
    const unsigned int uBefore = GetStats(kHeap.miHeap).uUsed;
    MemoryPool kPool("t1chunkcharge", 16);
    void* apBlock[6];
    GrowTwice(kPool, apBlock);
    CHECK(GetStats(kHeap.miHeap).uUsed - uBefore == ChunkNeed(2 * 16) + ChunkNeed(2 * 16) + ChunkNeed(2 * 16));
    CHECK(kPool.GetCount() * kPool.GetSize() == 6 * 16);
    kPool.Destroy();
}

TEST_CASE("memory pool growth charges the pool's heap and not the current heap" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkOwnHeap");
    const int iOther = RedCreateHeap(sacChunkOther, sacChunkOther + sizeof(sacChunkOther), 0x10, "T1ChunkOther");
    REQUIRE(iOther >= 0);
    MemoryPool kPool("t1chunkownheap", 16);
    kPool.Create(2, 16, 2);
    REQUIRE(kPool.Allocate(16));
    REQUIRE(kPool.Allocate(16));
    const unsigned int uOwnBefore = GetStats(kHeap.miHeap).uUsed;
    const unsigned int uOtherBefore = GetStats(iOther).uUsed;
    RedSetCurrentHeap(iOther);
    void* pGrown = kPool.Allocate(16);
    RedSetCurrentHeap(kHeap.miHeap);
    CHECK(pGrown);
    CHECK(GetStats(kHeap.miHeap).uUsed - uOwnBefore == ChunkNeed(2 * 16));
    CHECK(GetStats(iOther).uUsed == uOtherBefore);
    kPool.Destroy();
    RedDestroyHeap(iOther);
}

TEST_CASE("lua free routes a growth chunk block to its pool" * doctest::test_suite("regression"))
{
    const unsigned int uiSize = 13;
    LuaMemory::CreatePools(true);
    MemoryPool* pPool = LuaMemory::LocatePool(uiSize, 0);
    REQUIRE(pPool);
    std::vector<void*> vBlocks;
    while (pPool->mUsed < pPool->mCount)
    {
        void* pBlock = pPool->Allocate(uiSize);
        REQUIRE(pBlock);
        vBlocks.push_back(pBlock);
    }
    void* pGrown = pPool->Allocate(uiSize);
    REQUIRE(pGrown);
    REQUIRE(pPool->GetGrown() == 1);
    REQUIRE(pPool->mChunks);
    CHECK(LuaMemory::LocatePool(uiSize, pGrown) == pPool);
    const unsigned int uUsed = pPool->mUsed;
    Lua_Red_Free(pGrown, uiSize);
    CHECK(pPool->mUsed == uUsed - 1);
    void* pHeapBlock = RedAllocFromHeap(GameMemory::LuaHeap, uiSize, 0);
    REQUIRE(pHeapBlock);
    const unsigned int uHeapUsed = GetStats(GameMemory::LuaHeap).uUsed;
    CHECK(!LuaMemory::LocatePool(uiSize, pHeapBlock));
    Lua_Red_Free(pHeapBlock, uiSize);
    CHECK(pPool->mUsed == uUsed - 1);
    CHECK(GetStats(GameMemory::LuaHeap).uUsed < uHeapUsed);
    for (size_t i = 0; i < vBlocks.size(); ++i)
    {
        Lua_Red_Free(vBlocks[i], uiSize);
    }
    LuaMemory::Reset();
}

TEST_CASE("memory pool destroy then re-create starts with no old chunks" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkAgain");
    const unsigned int uBefore = GetStats(kHeap.miHeap).uUsed;
    MemoryPool kPool("t1chunkagain", 16);
    kPool.Create(2, 16, 4);
    void* apBlock[3];
    for (int j = 0; j < 3; ++j)
    {
        apBlock[j] = kPool.Allocate(16);
        REQUIRE(apBlock[j]);
    }
    REQUIRE(kPool.mChunks);
    kPool.Destroy();
    kPool.Create(2, 16, 4);
    CHECK(!kPool.mChunks);
    CHECK(!kPool.IsPresent(apBlock[2]));
    kPool.Destroy();
    CHECK(GetStats(kHeap.miHeap).uUsed == uBefore);
}

TEST_CASE("memory pool first growth of an empty pool is its first chunk" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1chunkempty", 16);
    kPool.Create(0, 16, 4);
    REQUIRE(!kPool.mPool);
    void* pBlock = kPool.Allocate(16);
    REQUIRE(pBlock);
    CHECK(kPool.mPool == pBlock);
    CHECK(!kPool.mChunks);
    for (unsigned int i = 0; i < 4; ++i)
    {
        CHECK(kPool.IsPresent((char*)kPool.mPool + i * 16));
    }
    CHECK(!kPool.IsPresent((char*)kPool.mPool + 4 * 16));
    kPool.Free(pBlock);
    kPool.Destroy();
}

TEST_CASE("memory pool reset of an owner clears its records and keeps the chunks charged" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkReset");
    MemoryPool kPool("t1chunkreset", 16);
    void* apBlock[6];
    GrowTwice(kPool, apBlock);
    const unsigned int uUsed = GetStats(kHeap.miHeap).uUsed;
    kPool.Reset();
    CHECK(GetStats(kHeap.miHeap).uUsed == uUsed);
    CHECK(!kPool.mChunks);
    CHECK(!kPool.mFirstCount);
    for (int j = 0; j < 6; ++j)
    {
        CHECK(!kPool.IsPresent(apBlock[j]));
    }
}

TEST_CASE("memory pool reset of a view leaves the owner's records" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkResetV");
    const unsigned int uBefore = GetStats(kHeap.miHeap).uUsed;
    MemoryPool kOwner("t1chunkresetown", 16);
    void* apBlock[6];
    GrowTwice(kOwner, apBlock);
    MemoryPool kView("t1chunkresetview", 16);
    kView.Set(kOwner);
    kView.Reset();
    CHECK(!kView.mChunks);
    for (int j = 0; j < 6; ++j)
    {
        CHECK(kOwner.IsPresent(apBlock[j]));
    }
    CheckEveryChunkPresent(kOwner, kOwner.mPool, 2);
    kOwner.Destroy();
    CHECK(GetStats(kHeap.miHeap).uUsed == uBefore);
}

TEST_CASE("memory pool destroy all frees the chunk records of swapped views" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkSwap");
    MemoryPool kOwner("t1chunkswapown", 16);
    void* apBlock[6];
    GrowTwice(kOwner, apBlock);
    REQUIRE(MemoryPool::sChunkRecords);
    MemoryPool kHost("t1chunkswaphost", 16);
    kHost.Set(kOwner);
    kOwner.Set(kHost);
    REQUIRE(!kOwner.mPoolOwner);
    MemoryPool::DestroyAll();
    CHECK(!MemoryPool::sChunkRecords);
    CHECK(!kOwner.mChunks);
    CHECK(!kHost.mChunks);
}

TEST_CASE("memory pool chunk record list drops an owner's records" * doctest::test_suite("regression"))
{
    ChunkHeap kHeap("T1ChunkForget");
    MemoryPool kKeep("t1chunkkeep", 16);
    MemoryPool kDrop("t1chunkdrop", 16);
    void* apBlock[6];
    GrowTwice(kKeep, apBlock);
    GrowTwice(kDrop, apBlock);
    REQUIRE(MemoryPool::sChunkRecords == kDrop.mChunks);
    kDrop.Destroy();
    CHECK(MemoryPool::sChunkRecords == kKeep.mChunks);
    kKeep.Destroy();
    CHECK(!MemoryPool::sChunkRecords);
}

TEST_CASE("memory pool reset frees chunk records only for an owner (source pin)" * doctest::test_suite("regression"))
{
    std::string sSource;
    REQUIRE(ReadRepoText("src\\RedEngineFL\\Memory\\RedMemoryPool.cpp", sSource));
    const size_t uStart = sSource.find("void MemoryPool::Reset()");
    REQUIRE(uStart != std::string::npos);
    const size_t uEnd = sSource.find("void MemoryPool::ClearStats()", uStart);
    REQUIRE(uEnd != std::string::npos);
    const std::string sBody = sSource.substr(uStart, uEnd - uStart);
    const size_t uOwner = sBody.find("if (mPoolOwner)");
    const size_t uFree = sBody.find("PblCrtFree(pNode);");
    REQUIRE(uOwner != std::string::npos);
    REQUIRE(uFree != std::string::npos);
    CHECK(uOwner < uFree);
}

namespace
{

int RunIndexAfterFrees(const int* aiFree, int iFreeCount, unsigned int uRun)
{
    MemoryPool kPool("t1runset", 16);
    kPool.Create(8, 16, 0);
    for (int i = 0; i < 8; ++i)
    {
        REQUIRE(kPool.Allocate(16) == (char*)kPool.mPool + i * 16);
    }
    for (int i = 0; i < iFreeCount; ++i)
    {
        kPool.Free((char*)kPool.mPool + aiFree[i] * 16);
    }
    char* pcRun = (char*)kPool.AllocateRun(uRun, 16);
    const int iIndex = pcRun ? (int)(pcRun - (char*)kPool.mPool) / 16 : -1;
    kPool.Destroy();
    return iIndex;
}

void CheckRunResult(const int* aiFree, int iFreeCount, unsigned int uRun, int iExpected)
{
    std::vector<int> vOrder(aiFree, aiFree + iFreeCount);
    CHECK(RunIndexAfterFrees(&vOrder[0], iFreeCount, uRun) == iExpected);
    std::reverse(vOrder.begin(), vOrder.end());
    CHECK(RunIndexAfterFrees(&vOrder[0], iFreeCount, uRun) == iExpected);
    for (int i = 0; i < iFreeCount; ++i)
    {
        std::swap(vOrder[i], vOrder[(i * 5 + 3) % iFreeCount]);
    }
    CHECK(RunIndexAfterFrees(&vOrder[0], iFreeCount, uRun) == iExpected);
}

}

TEST_CASE("memory pool free pushes front and allocate pops the head" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1pushfront", 16);
    kPool.Create(8, 16, 0);
    void* apBlock[8];
    for (int i = 0; i < 8; ++i)
    {
        apBlock[i] = kPool.Allocate(16);
        REQUIRE(apBlock[i]);
    }
    kPool.Free(apBlock[2]);
    kPool.Free(apBlock[5]);
    kPool.Free(apBlock[0]);
    CHECK(kPool.mUsed == 5);
    CHECK(kPool.Allocate(16) == apBlock[0]);
    CHECK(kPool.Allocate(16) == apBlock[5]);
    CHECK(kPool.Allocate(16) == apBlock[2]);
    kPool.Destroy();
}

TEST_CASE("memory pool run search depends only on the free set" * doctest::test_suite("regression"))
{
    const int aiMixed[] = { 0, 1, 3, 4, 5, 7 };
    CheckRunResult(aiMixed, 6, 1, 0);
    CheckRunResult(aiMixed, 6, 2, 0);
    CheckRunResult(aiMixed, 6, 3, 3);
    CheckRunResult(aiMixed, 6, 4, -1);
    const int aiGaps[] = { 0, 2, 4, 6 };
    CheckRunResult(aiGaps, 4, 1, 0);
    CheckRunResult(aiGaps, 4, 2, -1);
    CheckRunResult(aiGaps, 4, 3, -1);
}

TEST_CASE("memory pool run search sees an inversion after a satisfying run" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1runinvert", 16);
    kPool.Create(11, 16, 0);
    for (int i = 0; i < 11; ++i)
    {
        REQUIRE(kPool.Allocate(16));
    }
    char* pcPool = (char*)kPool.mPool;
    for (int i = 3; i >= 0; --i)
    {
        kPool.Free(pcPool + i * 16);
    }
    kPool.Free(pcPool + 10 * 16);
    CHECK(kPool.AllocateRun(1, 16) == pcPool);
    CHECK(kPool.mUsed == 7);
    kPool.Destroy();
}

TEST_CASE("memory pool run search leaves the free set unchanged apart from the run" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1runfreeset", 16);
    kPool.Create(8, 16, 0);
    for (int i = 0; i < 8; ++i)
    {
        REQUIRE(kPool.Allocate(16));
    }
    char* pcPool = (char*)kPool.mPool;
    const int aiFree[] = { 6, 1, 4, 0, 7, 3 };
    for (int i = 0; i < 6; ++i)
    {
        kPool.Free(pcPool + aiFree[i] * 16);
    }
    CHECK(kPool.AllocateRun(2, 16) == pcPool);
    CHECK(kPool.mUsed == 4);
    unsigned int uDrained = 0;
    for (int i = 0; i < 4; ++i)
    {
        char* pcBlock = (char*)kPool.Allocate(16);
        REQUIRE(pcBlock);
        uDrained |= 1u << ((pcBlock - pcPool) / 16);
    }
    CHECK(uDrained == ((1u << 3) | (1u << 4) | (1u << 6) | (1u << 7)));
    CHECK(kPool.mUsed == 8);
    CHECK(!kPool.mFree);
    kPool.Destroy();
}

TEST_CASE("memory pool fullness and growth rule are count based" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1gates", 16);
    kPool.Create(3, 16, 0);
    CHECK(kPool.GetGrow() == 4);
    void* apBlock[4];
    for (int i = 0; i < 2; ++i)
    {
        apBlock[i] = kPool.Allocate(16);
        REQUIRE(apBlock[i]);
        CHECK(!kPool.IsFull());
    }
    CHECK(kPool.IsSufficientSpace(1));
    CHECK(!kPool.IsSufficientSpace(2));
    apBlock[2] = kPool.Allocate(16);
    REQUIRE(apBlock[2]);
    CHECK(kPool.IsFull());
    CHECK(kPool.IsSufficientSpace(0));
    apBlock[3] = kPool.Allocate(16);
    REQUIRE(apBlock[3]);
    CHECK(kPool.mCount == 7);
    CHECK(kPool.GetGrown() == 1);
    CHECK(!kPool.IsFull());
    kPool.Destroy();
    kPool.Create(1, 100, 0);
    CHECK(kPool.GetGrow() == 1);
    kPool.Destroy();
}
