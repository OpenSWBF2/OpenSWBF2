#include "third_party/doctest/doctest.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryBackend.h"

namespace
{

alignas(16) char sacCapsArena[0x10000];
alignas(16) char sacCapsSecond[0x1000];
__declspec(align(64)) char sacCapsExact[0x2000];
alignas(16) char sacCapsAgn16[0x10000];
alignas(16) char sacCapsAgn4[0x10000];

long slFakeAllocs[kRedHeapSlots];

void* FakeInit(void* start, void* end, int iMax)
{
    return kRedArenaBackend.init(start, end, iMax);
}

size_t FakeCreate(int heap, void* start, void* end, unsigned int align, const char* name)
{
    return kRedArenaBackend.create(heap, start, end, align, name);
}

void FakeDestroy(int heap)
{
    kRedArenaBackend.destroy(heap);
}

void* FakeAlloc(int heap, size_t size, bool bTop, size_t* pReserved)
{
    if (heap >= 0 && heap < kRedHeapSlots)
    {
        ++slFakeAllocs[heap];
    }
    return kRedArenaBackend.alloc(heap, size, bTop, pReserved);
}

size_t FakeFree(int heap, void* block)
{
    return kRedArenaBackend.free(heap, block);
}

int FakeOwner(const void* block)
{
    return kRedArenaBackend.owner(block);
}

const RedHeapBackend kCountingBackend = {
    FakeInit, FakeCreate, FakeDestroy, FakeAlloc, FakeFree, FakeOwner,
};

static const RedHeapBackend* const kHeapCapsBackends[] = { &kRedArenaBackend };

class BackendSwap
{
public:
    explicit BackendSwap(const RedHeapBackend* pBackend) : mpSaved(gRedHeapBackend)
    {
        gRedHeapBackend = pBackend;
    }

    ~BackendSwap()
    {
        gRedHeapBackend = mpSaved;
    }

private:
    const RedHeapBackend* mpSaved;
};

int siFailHeap;
unsigned int suFailSize;
int siFailCalls;

void CaptureFailure(int heap, unsigned int size)
{
    siFailHeap = heap;
    suFailSize = size;
    ++siFailCalls;
}

class FailureCapture
{
public:
    FailureCapture() : mpSaved(OnFailure)
    {
        siFailHeap = -1;
        suFailSize = 0;
        siFailCalls = 0;
        OnFailure = CaptureFailure;
    }

    ~FailureCapture()
    {
        OnFailure = mpSaved;
    }

private:
    void (*mpSaved)(int heap, unsigned int size);
};

class ScratchHeap
{
public:
    ScratchHeap(char* pStart, char* pEnd, unsigned int uAlign, const char* pcName)
        : miHeap(RedCreateHeap(pStart, pEnd, uAlign, pcName))
    {
    }

    ~ScratchHeap()
    {
        if (miHeap >= 0)
        {
            RedDestroyHeap(miHeap);
        }
    }

    int miHeap;
};

RedHeapStats CapsStats(int iHeap)
{
    RedHeapStats kStats;
    memset(&kStats, 0, sizeof(kStats));
    REQUIRE(RedGetHeapStats(iHeap, &kStats));
    return kStats;
}

unsigned int Need(unsigned int uSize, unsigned int uAlign)
{
    const unsigned int uHdr = (uAlign + 12 - 1) & ~(uAlign - 1);
    return ((uSize ? uSize : 1) + uAlign - 1 + uHdr) & ~(uAlign - 1);
}

}

TEST_CASE("heap default caps equal the created sizes" * doctest::test_suite("regression"))
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsScratch");
    ScratchHeap kOdd(sacCapsSecond + 1, sacCapsSecond + sizeof(sacCapsSecond) - 1, 0x10, "CapsOdd");
    REQUIRE(kScratch.miHeap >= 0);
    REQUIRE(kOdd.miHeap >= 0);
    REQUIRE(strcmp(RedGetHeapName(1), "Lua") == 0);
    const int aiHeaps[3] = { 1, kScratch.miHeap, kOdd.miHeap };
    const unsigned int auCap[3] = { 0x1000000, 0x10000, 0xFE0 };
    for (int i = 0; i < 3; ++i)
    {
        const int iHeap = aiHeaps[i];
        CAPTURE(i);
        CHECK(CapsStats(iHeap).uCap == auCap[i]);
        CHECK(gRedHeapSlots[iHeap].defaultCap == auCap[i]);
        CHECK(gRedHeapSlots[iHeap].cap == auCap[i]);
        CHECK(RedGetHeapSize(iHeap) == auCap[i]);
    }
    CHECK(RedGetHeapFree(kScratch.miHeap) == 0x10000);
    CHECK(RedGetHeapFree(kOdd.miHeap) == 0xFE0);
}

TEST_CASE("heap over-cap allocation fails through OnFailure without a backend call" *
          doctest::test_suite("regression"))
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsOver");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    BackendSwap kSwap(&kCountingBackend);
    FailureCapture kCapture;
    const unsigned int uCap = gRedHeapSlots[iH].cap;
    gRedHeapSlots[iH].cap = 256;
    const unsigned int uFailedBefore = CapsStats(iH).uFailed;
    const long lCalls = slFakeAllocs[iH];
    void* pOver = RedAllocFromHeap(iH, 512, false);
    CHECK(!pOver);
    CHECK(slFakeAllocs[iH] == lCalls);
    CHECK(siFailCalls == 1);
    CHECK(siFailHeap == iH);
    CHECK(suFailSize == 512);
    CHECK(CapsStats(iH).uFailed == uFailedBefore + 1);
    void* pFit = RedAllocFromHeap(iH, 64, false);
    CHECK(pFit);
    CHECK(slFakeAllocs[iH] == lCalls + 1);
    if (pFit)
    {
        RedFreeToHeap(iH, pFit, 0);
    }
    if (pOver)
    {
        RedFreeToHeap(iH, pOver, 0);
    }
    gRedHeapSlots[iH].cap = uCap;
}

TEST_CASE("heap over-cap failure records the failing heap and need" * doctest::test_suite("regression"))
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsFailInfo");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    FailureCapture kCapture;
    const int iSavedHeap = RedMemory::failHeap;
    const unsigned int uSavedSize = RedMemory::failSize;
    const unsigned int uCap = gRedHeapSlots[iH].cap;
    gRedHeapSlots[iH].cap = 256;
    RedMemory::failHeap = -1;
    RedMemory::failSize = 0;
    void* pOver = RedAllocFromHeap(iH, 500, false);
    const int iFailHeap = RedMemory::failHeap;
    const unsigned int uFailSize = RedMemory::failSize;
    gRedHeapSlots[iH].cap = uCap;
    RedMemory::failHeap = -1;
    RedMemory::failSize = 0;
    void* pWrap = RedAllocFromHeap(iH, 0xFFFFFFF0u, false);
    const int iWrapHeap = RedMemory::failHeap;
    const unsigned int uWrapSize = RedMemory::failSize;
    RedMemory::failHeap = iSavedHeap;
    RedMemory::failSize = uSavedSize;
    CHECK(!pOver);
    CHECK(!pWrap);
    CHECK(siFailCalls == 2);
    CHECK(iFailHeap == iH);
    CHECK(uFailSize == Need(500, 0x10));
    CHECK(iWrapHeap == iH);
    CHECK(uWrapSize == 0xFFFFFFFFu);
    if (pOver)
    {
        RedFreeToHeap(iH, pOver, 0);
    }
    if (pWrap)
    {
        RedFreeToHeap(iH, pWrap, 0);
    }
}

TEST_CASE("wrapping heap request fails" * doctest::test_suite("regression"))
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsWrap");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    BackendSwap kSwap(&kCountingBackend);
    FailureCapture kCapture;
    const long lCalls = slFakeAllocs[iH];
    int iExpectFails = 3;
    if (sizeof(size_t) > sizeof(unsigned int))
    {
        const size_t uPastUint = (size_t)UINT_MAX + 1;
        CHECK(!RedAllocFromHeap(iH, uPastUint, false));
        CHECK(!RedAllocFromHeap(iH, uPastUint + 0x10, false));
        iExpectFails += 2;
    }
    CHECK(!RedAllocFromHeap(iH, (size_t)~0u - (0xF + 0x10) + 1, false));
    CHECK(!RedAllocFromHeap(iH, 0xFFFFFFF0u, false));
    CHECK(!RedAllocFromHeap(iH, SIZE_MAX, true));
    CHECK(!RedAllocFromHeap(iH, 0xFFFFFFFFu, true));
    CHECK(slFakeAllocs[iH] == lCalls);
    CHECK(siFailCalls == iExpectFails + 1);
    CHECK(siFailHeap == iH);
    CHECK(suFailSize == 0xFFFFFFFFu);
}

TEST_CASE("heap cap check fires only where the arena would fail" * doctest::test_suite("regression"))
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsLcg");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    REQUIRE(gRedHeapSlots[iH].cap == RedGetHeapSize(iH));
    BackendSwap kSwap(&kCountingBackend);
    FailureCapture kCapture;
    void* apLive[64];
    int iLive = 0;
    int iCapNulls = 0;
    int iArenaNulls = 0;
    int iBadCapNulls = 0;
    unsigned int uLcg = 0x1234567u;
    for (int iStep = 0; iStep < 4000; ++iStep)
    {
        uLcg = uLcg * 1664525u + 1013904223u;
        if (iLive > 0 && (iLive == 64 || ((uLcg >> 16) % 3) == 0))
        {
            const int iPick = (int)((uLcg >> 8) % (unsigned int)iLive);
            RedFreeToHeap(iH, apLive[iPick], 0);
            apLive[iPick] = apLive[--iLive];
            continue;
        }
        const unsigned int uSize = 1 + (uLcg >> 4) % 0x3000;
        const unsigned int uNeed = Need(uSize, 0x10);
        const unsigned int uFreeTags = RedArenaFreeTagSum(iH);
        const long lCalls = slFakeAllocs[iH];
        void* p = RedAllocFromHeap(iH, uSize, (iStep & 1) != 0);
        if (p)
        {
            apLive[iLive++] = p;
        }
        else if (slFakeAllocs[iH] == lCalls)
        {
            ++iCapNulls;
            if (!(uFreeTags < uNeed))
            {
                ++iBadCapNulls;
            }
        }
        else
        {
            ++iArenaNulls;
        }
    }
    while (iLive > 0)
    {
        RedFreeToHeap(iH, apLive[--iLive], 0);
    }
    CHECK(iBadCapNulls == 0);
    CHECK(iCapNulls > 0);
    CHECK(iArenaNulls > 0);
    CHECK(CapsStats(iH).uUsed == 0);
}

TEST_CASE("heap free is cap minus used on a fixed script" * doctest::test_suite("regression"))
{
    ScratchHeap kLow(sacCapsExact, sacCapsExact + 0x1000, 0x10, "CapsExactLow");
    ScratchHeap kHigh(sacCapsExact + 0x1000, sacCapsExact + 0x2000, 0x10, "CapsExactHigh");
    const int iL = kLow.miHeap;
    const int iH = kHigh.miHeap;
    REQUIRE(iL >= 0);
    REQUIRE(iH == iL + 1);
    static const unsigned int kauExpect[15][2] = {
        { 4096, 4096 }, { 3968, 4096 }, { 3968, 3568 }, { 3744, 3568 }, { 3616, 3568 },
        { 3840, 3568 }, { 3616, 3568 }, { 3536, 3568 }, { 3664, 3568 }, { 3888, 3568 },
        { 4016, 3568 }, { 4272, 3312 }, { 4016, 3568 }, { 4096, 3568 }, { 4096, 4096 },
    };
    void* pA = 0;
    void* pB = 0;
    void* pC = 0;
    void* pD = 0;
    void* pE = 0;
    void* pH1 = 0;
    for (int iStep = 0; iStep < 15; ++iStep)
    {
        switch (iStep)
        {
        case 1: pA = RedAllocFromHeap(iL, 100, false); REQUIRE(pA); break;
        case 2: pH1 = RedAllocFromHeap(iH, 512, true); REQUIRE(pH1); break;
        case 3: pB = RedAllocFromHeap(iL, 200, false); REQUIRE(pB); break;
        case 4: pC = RedAllocFromHeap(iL, 100, false); REQUIRE(pC); break;
        case 5: RedFreeToHeap(iL, pB, 0); break;
        case 6: pD = RedAllocFromHeap(iL, 180, false); REQUIRE(pD); CHECK(pD == pB); break;
        case 7: pE = RedAllocFromHeap(iL, 50, true); REQUIRE(pE); break;
        case 8: RedFreeToHeap(iL, pA, 0); break;
        case 9: RedFreeToHeap(iL, pD, 0); break;
        case 10: RedFreeToHeap(iL, pC, 0); break;
        case 11: RedMoveHeapEnd(iL, 0x100); break;
        case 12: RedMoveHeapEnd(iL, -0x100); break;
        case 13: RedFreeToHeap(iL, pE, 0); break;
        case 14: RedFreeToHeap(iH, pH1, 0); break;
        default: break;
        }
        CAPTURE(iStep);
        CHECK(RedGetHeapFree(iL) == RedArenaFreeTagSum(iL));
        CHECK(RedGetHeapFree(iH) == RedArenaFreeTagSum(iH));
        CHECK(RedGetHeapFree(iL) == kauExpect[iStep][0]);
        CHECK(RedGetHeapFree(iH) == kauExpect[iStep][1]);
    }
    CHECK(RedGetHeapSize(iL) == 0x1000);
    CHECK(RedGetHeapSize(iH) == 0x1000);
}

TEST_CASE("heap charge follows the need formula on every backend" * doctest::test_suite("regression"))
{
    static const unsigned int kauSizes[24] = {
        0, 1, 3, 4, 12, 15, 16, 17, 100, 0x3FF, 0, 1,
        3, 4, 12, 15, 16, 17, 100, 0x3FF, 17, 100, 0, 0x3FF,
    };
    static const int kaiFreeOrder[24] = {
        5, 17, 0, 23, 11, 8, 2, 20, 14, 1, 22, 9,
        6, 19, 3, 12, 21, 7, 16, 4, 13, 10, 18, 15,
    };
    for (size_t b = 0; b < sizeof(kHeapCapsBackends) / sizeof(kHeapCapsBackends[0]); ++b)
    {
        BackendSwap kSwap(kHeapCapsBackends[b]);
        ScratchHeap kH16(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsAgn16");
        ScratchHeap kH4(sacCapsAgn4, sacCapsAgn4 + sizeof(sacCapsAgn4), 4, "CapsAgn4");
        const int aiHeaps[2] = { kH16.miHeap, kH4.miHeap };
        const unsigned int auAlign[2] = { 0x10, 4 };
        for (int h = 0; h < 2; ++h)
        {
            const int iH = aiHeaps[h];
            const unsigned int uAlign = auAlign[h];
            CAPTURE(b);
            CAPTURE(uAlign);
            REQUIRE(iH >= 0);
            const unsigned int uSize = RedGetHeapSize(iH);
            REQUIRE(uSize >= 0x10000);
            void* apBlock[24];
            unsigned int uLive = 0;
            for (int i = 0; i < 24; ++i)
            {
                apBlock[i] = RedAllocFromHeap(iH, kauSizes[i], (i & 1) != 0);
                REQUIRE(apBlock[i]);
                uLive += Need(kauSizes[i], uAlign);
                CAPTURE(i);
                CHECK(CapsStats(iH).uUsed == uLive);
                CHECK(RedGetHeapFree(iH) == uSize - uLive);
            }
            const unsigned int uHdr = (uAlign + 12 - 1) & ~(uAlign - 1);
            const unsigned int uTail = uSize - uLive;
            REQUIRE(uTail > uAlign + uHdr);
            const unsigned int uSkip = uTail - uAlign - uHdr;
            REQUIRE(Need(uSkip, uAlign) == uTail - uAlign);
            void* pSkip = RedAllocFromHeap(iH, uSkip, false);
            REQUIRE(pSkip);
            CHECK(CapsStats(iH).uUsed == uSize);
            CHECK(RedGetHeapFree(iH) == 0);
            RedFreeToHeap(iH, pSkip, 0);
            CHECK(CapsStats(iH).uUsed == uLive);
            CHECK(RedGetHeapFree(iH) == uSize - uLive);
            for (int i = 0; i < 24; ++i)
            {
                const int iPick = kaiFreeOrder[i];
                RedFreeToHeap(iH, apBlock[iPick], 0);
                uLive -= Need(kauSizes[iPick], uAlign);
                CAPTURE(i);
                CHECK(CapsStats(iH).uUsed == uLive);
                CHECK(RedGetHeapFree(iH) == uSize - uLive);
            }
            CHECK(RedGetHeapFree(iH) == uSize);
        }
    }
}

TEST_CASE("raised heap cap does not change the free value" * doctest::test_suite("regression"))
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsRaised");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    void* p = RedAllocFromHeap(iH, 1000, false);
    REQUIRE(p);
    const unsigned int uFree = RedGetHeapFree(iH);
    const unsigned int uCap = gRedHeapSlots[iH].cap;
    gRedHeapSlots[iH].cap = uCap * 2;
    CHECK(RedGetHeapFree(iH) == uFree);
    gRedHeapSlots[iH].cap = uCap;
    RedFreeToHeap(iH, p, 0);
}

TEST_CASE("heap free never wraps and is zero for a destroyed heap" * doctest::test_suite("regression"))
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsWrapFree");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    const long lUsed = gRedHeapSlots[iH].used;
    gRedHeapSlots[iH].used = (long)(gRedHeapSlots[iH].defaultCap + 16);
    CHECK(RedGetHeapFree(iH) == 0);
    gRedHeapSlots[iH].used = lUsed;
    const int iD = RedCreateHeap(sacCapsSecond, sacCapsSecond + sizeof(sacCapsSecond), 0x10, "CapsDead");
    REQUIRE(iD >= 0);
    REQUIRE(RedAllocFromHeap(iD, 64, false));
    RedDestroyHeap(iD);
    CHECK(RedGetHeapFree(iD) == 0);
    CHECK(RedGetHeapFree(-1) == 0);
    CHECK(RedGetHeapFree(maxheaps) == 0);
}
