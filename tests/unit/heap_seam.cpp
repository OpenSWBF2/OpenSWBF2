#include "third_party/doctest/doctest.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"

namespace
{

alignas(16) char sacSeamArena[0x8000];

RedHeapStats SeamStats(int iHeap)
{
    RedHeapStats kStats;
    memset(&kStats, 0, sizeof(kStats));
    REQUIRE(RedGetHeapStats(iHeap, &kStats));
    return kStats;
}

class HeapScope
{
public:
    HeapScope() : miSaved(__RedCurrHeap), miCount(0)
    {
    }

    ~HeapScope()
    {
        RedSetCurrentHeap(miSaved);
        while (miCount > 0)
        {
            const Entry& rEntry = maEntry[--miCount];
            if (rEntry.pBlock)
            {
                RedFreeToHeap(rEntry.iHeap, rEntry.pBlock, 0);
            }
            else
            {
                RedDestroyHeap(rEntry.iHeap);
            }
        }
    }

    int Create(void* pStart, void* pEnd, unsigned int uiAlign, const char* pcName)
    {
        const int iHeap = RedCreateHeap(pStart, pEnd, uiAlign, pcName);
        if (iHeap >= 0)
        {
            Push(iHeap, 0);
        }
        return iHeap;
    }

    void* Alloc(int iHeap, size_t uSize, bool bTop)
    {
        void* pBlock = RedAllocFromHeap(iHeap, uSize, bTop);
        if (pBlock)
        {
            Push(iHeap, pBlock);
        }
        return pBlock;
    }

    void Destroy(int iHeap)
    {
        for (int i = miCount - 1; i >= 0; --i)
        {
            if (maEntry[i].iHeap == iHeap && !maEntry[i].pBlock)
            {
                RedDestroyHeap(iHeap);
                for (int j = i + 1; j < miCount; ++j)
                {
                    maEntry[j - 1] = maEntry[j];
                }
                --miCount;
                return;
            }
        }
    }

private:
    struct Entry
    {
        int iHeap;
        void* pBlock;
    };

    void Push(int iHeap, void* pBlock)
    {
        REQUIRE(miCount < kMaxEntries);
        maEntry[miCount].iHeap = iHeap;
        maEntry[miCount].pBlock = pBlock;
        ++miCount;
    }

    static const int kMaxEntries = 16;
    int miSaved;
    int miCount;
    Entry maEntry[kMaxEntries];
};

bool CrtBlockReleased(const void* p)
{
#if defined(__SANITIZE_ADDRESS__)
    return __asan_address_is_poisoned(p) != 0;
#else
    MEMORY_BASIC_INFORMATION kInfo;
    return VirtualQuery(p, &kInfo, sizeof(kInfo)) == sizeof(kInfo) && kInfo.State != MEM_COMMIT;
#endif
}

}

TEST_CASE("heap free is debited from the owning heap" * doctest::test_suite("regression"))
{
    HeapScope kScope;
    const int iA = kScope.Create(sacSeamArena, sacSeamArena + 0x1000, 0x10, "SeamA");
    const int iO = kScope.Create(sacSeamArena + 0x1000, sacSeamArena + 0x5000, 0x10, "SeamOuter");
    REQUIRE(iA >= 0);
    REQUIRE(iO > iA);
    char* pX = (char*)kScope.Alloc(iO, 0x2000, true);
    REQUIRE(pX);
    const int iI = kScope.Create(pX, pX + 0x1F00, 0x10, "SeamInner");
    REQUIRE(iI > iO);
    const unsigned int uABefore = SeamStats(iA).uUsed;
    const unsigned int uOBefore = SeamStats(iO).uUsed;
    void* p = RedAllocFromHeap(iI, 100, false);
    REQUIRE(p);
    RedSetCurrentHeap(iA);
    ::operator delete(p);
    RedSetCurrentHeap(0);
    CHECK(SeamStats(iI).uUsed == 0);
    CHECK(SeamStats(iO).uUsed == uOBefore);
    CHECK(SeamStats(iA).uUsed == uABefore);
}

TEST_CASE("heap free of a nested block goes to the inner heap" * doctest::test_suite("regression"))
{
    HeapScope kScope;
    const int iO = kScope.Create(sacSeamArena, sacSeamArena + 0x4000, 0x10, "SeamOuter");
    REQUIRE(iO >= 0);
    char* pX = (char*)kScope.Alloc(iO, 0x2000, true);
    REQUIRE(pX);
    const int iI = kScope.Create(pX, pX + 0x1F00, 0x10, "SeamInner");
    REQUIRE(iI >= 0);
    void* pY = RedAllocFromHeap(iI, 64, false);
    REQUIRE(pY);
    CHECK(RedHeapOwner(pY) == iI);
    CHECK(RedHeapOwner(pX) == iO);
    const unsigned int uOUsed = SeamStats(iO).uUsed;
    RedFreeToHeap(iO, pY, 0);
    CHECK(SeamStats(iI).uUsed == 0);
    CHECK(SeamStats(iO).uUsed == uOUsed);
}

TEST_CASE("heap free of an unowned block uses the crt" * doctest::test_suite("regression"))
{
#if defined(__SANITIZE_ADDRESS__)
    const size_t uBytes = 32;
#else
    const size_t uBytes = 0x400000;
#endif
    char* p = (char*)malloc(uBytes);
    REQUIRE(p);
    memset(p, 0x5A, uBytes);
    CHECK(RedHeapOwner(p) < 0);
    const RedHeapStats kBefore = SeamStats(0);
    RedFreeToHeap(0, p, 0);
    CHECK(CrtBlockReleased(p));
    const RedHeapStats kAfter = SeamStats(0);
    CHECK(kAfter.uUsed == kBefore.uUsed);
    CHECK(kAfter.uPeak == kBefore.uPeak);
    CHECK(kAfter.uAllocs == kBefore.uAllocs);
    CHECK(kAfter.uFailed == kBefore.uFailed);
}

TEST_CASE("heap slot ids are stable across destroy and re-create" * doctest::test_suite("regression"))
{
    char* pBase = sacSeamArena;
    {
        HeapScope kScope;
        const int iA = kScope.Create(pBase, pBase + 0x1000, 0x10, "SeamA");
        const int iB = kScope.Create(pBase + 0x1000, pBase + 0x2000, 0x10, "SeamB");
        const int iC = kScope.Create(pBase + 0x2000, pBase + 0x3000, 0x10, "SeamC");
        REQUIRE(iA >= 0);
        REQUIRE(iB >= 0);
        REQUIRE(iC >= 0);
        kScope.Destroy(iB);
        CHECK(kScope.Create(pBase + 0x1000, pBase + 0x2000, 0x10, "SeamB") == iB);
    }
    int aiFirst[4];
    int aiSecond[4];
    int* apIds[2] = {aiFirst, aiSecond};
    for (int iPass = 0; iPass < 2; ++iPass)
    {
        HeapScope kScope;
        int* piIds = apIds[iPass];
        piIds[0] = kScope.Create(pBase, pBase + 0x1000, 4, "SeamLua");
        piIds[1] = kScope.Create(pBase + 0x1000, pBase + 0x2000, 0x10, "SeamArtist");
        piIds[2] = kScope.Create(pBase + 0x2000, pBase + 0x8000, 0x10, "SeamRuntime");
        REQUIRE(piIds[0] >= 0);
        REQUIRE(piIds[1] >= 0);
        REQUIRE(piIds[2] >= 0);
        char* pTemp = (char*)RedAllocFromHeap(piIds[2], 0x2000, true);
        REQUIRE(pTemp);
        piIds[3] = kScope.Create(pTemp, pTemp + 0x1F00, 0x10, "SeamTemp");
        REQUIRE(piIds[3] >= 0);
        kScope.Destroy(piIds[3]);
        kScope.Destroy(piIds[2]);
        kScope.Destroy(piIds[0]);
        kScope.Destroy(piIds[1]);
    }
    for (int i = 0; i < 4; ++i)
    {
        CHECK(aiSecond[i] == aiFirst[i]);
    }
}

TEST_CASE("heap queries reject an id past maxheaps" * doctest::test_suite("regression"))
{
    const int iPast = maxheaps;
    REQUIRE(iPast >= 0);
    REQUIRE(iPast < kRedHeapSlots);
    const RedHeapSlot kSaved = gRedHeapSlots[iPast];
    gRedHeapSlots[iPast].name = "SeamPast";
    gRedHeapSlots[iPast].size = 0x1000;
    gRedHeapSlots[iPast].cap = 0x1000;
    gRedHeapSlots[iPast].defaultCap = 0x1000;
    gRedHeapSlots[iPast].used = 0;
    gRedHeapSlots[iPast].bLive = true;
    RedHeapStats kStats;
    memset(&kStats, 0, sizeof(kStats));
    const char* pcName = RedGetHeapName(iPast);
    const unsigned int uFree = RedGetHeapFree(iPast);
    const bool bStats = RedGetHeapStats(iPast, &kStats);
    gRedHeapSlots[iPast] = kSaved;
    CHECK(strcmp(pcName, "?") == 0);
    CHECK(uFree == 0);
    CHECK(!bStats);
    CHECK(strcmp(RedGetHeapName(-1), "?") == 0);
    CHECK(RedGetHeapFree(-1) == 0);
    CHECK(!RedGetHeapStats(-1, &kStats));
}

TEST_CASE("heap destroy ignores an id out of range" * doctest::test_suite("regression"))
{
    const int iPast = maxheaps;
    REQUIRE(iPast >= 0);
    REQUIRE(iPast < kRedHeapSlots);
    const RedHeapSlot kSaved = gRedHeapSlots[iPast];
    gRedHeapSlots[iPast].name = "SeamPast";
    gRedHeapSlots[iPast].size = 0x1000;
    gRedHeapSlots[iPast].cap = 0x1000;
    gRedHeapSlots[iPast].defaultCap = 0x1000;
    gRedHeapSlots[iPast].bLive = true;
    RedDestroyHeap(-1);
    RedDestroyHeap(iPast);
    const RedHeapSlot kAfter = gRedHeapSlots[iPast];
    gRedHeapSlots[iPast] = kSaved;
    CHECK(kAfter.bLive);
    CHECK(kAfter.size == 0x1000);
    CHECK(kAfter.cap == 0x1000);
    CHECK(kAfter.defaultCap == 0x1000);
}
