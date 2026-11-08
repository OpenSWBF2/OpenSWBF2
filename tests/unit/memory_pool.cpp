#include "third_party/doctest/doctest.h"

#include <string.h>
#include <vector>

#include "testutil.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryBackend.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Memory/RedScriptLimits.h"
#include "RedEngineFL/Util/RedLog.h"

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

TEST_CASE("memory pool stats reset on destroy all" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1destroyall", 16);
    kPool.mPeak = 5;
    kPool.mGrown = 2;
    kPool.mFullReported = true;
    REQUIRE(!kPool.mPool);
    PblList<MemoryPool>* pList = GetMemoryPoolList();
    std::vector<MemoryPool*> vOthers;
    for (PblList<MemoryPool>::Itor itor(*pList); itor; ++itor)
    {
        if ((MemoryPool*)itor != &kPool)
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

unsigned int HeapUsedFromFree(int iHeap)
{
    return RedGetHeapSize(iHeap) - RedArenaFreeTagSum(iHeap);
}

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
    const unsigned int uPeakUsed = HeapUsedFromFree(iHeap);
    RedFreeToHeap(iHeap, p2, 0);
    kStats = GetStats(iHeap);
    CHECK(kStats.uUsed == HeapUsedFromFree(iHeap));
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
    CHECK(kStats.uUsed == HeapUsedFromFree(iHeap));
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
    CHECK(GetStats(iLow).uUsed == HeapUsedFromFree(iLow));
    CHECK(GetStats(iHigh).uUsed == HeapUsedFromFree(iHigh));
    RedMoveHeapEnd(iLow, -64);
    CHECK(RedGetHeapSize(iLow) == 0x1000);
    CHECK(RedGetHeapSize(iHigh) == 0x1000);
    const RedHeapStats kLowAfter = GetStats(iLow);
    const RedHeapStats kHighAfter = GetStats(iHigh);
    CHECK(kLowAfter.uUsed == HeapUsedFromFree(iLow));
    CHECK(kHighAfter.uUsed == HeapUsedFromFree(iHigh));
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
    RedMoveHeapEnd(iLow, -16);
    RedMoveHeapEnd(iLow, -64);
    CHECK(RedGetHeapSize(iLow) == uLowSize);
    CHECK(RedGetHeapSize(iHigh) == uHighSize);
    CHECK(GetStats(iLow).uUsed == HeapUsedFromFree(iLow));
    CHECK(GetStats(iHigh).uUsed == HeapUsedFromFree(iHigh));
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

TEST_CASE("move heap end refuses a block away from the boundary" * doctest::test_suite("regression"))
{
    int iLow = -1;
    int iHigh = -1;
    CreateAdjacentHeaps(iLow, iHigh);
    void* pBottom = RedAllocFromHeap(iHigh, 100, false);
    void* pTop = RedAllocFromHeap(iLow, 100, true);
    REQUIRE(pBottom);
    REQUIRE(pTop);
    const RedHeapStats kLowBefore = GetStats(iLow);
    const RedHeapStats kHighBefore = GetStats(iHigh);
    RedMoveHeapEnd(iLow, 64);
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
    CHECK(kLowAfter.uUsed == HeapUsedFromFree(iLow));
    CHECK(kHighAfter.uUsed == HeapUsedFromFree(iHigh));
    RedFreeToHeap(iHigh, pBottom, 0);
    RedFreeToHeap(iLow, pTop, 0);
    CHECK(RedGetHeapFree(iLow) == RedGetHeapSize(iLow));
    CHECK(RedGetHeapFree(iHigh) == RedGetHeapSize(iHigh));
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
