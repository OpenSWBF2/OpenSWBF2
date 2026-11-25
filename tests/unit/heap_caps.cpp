#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"
#include "RedEngineFL/Memory/RedScriptLimits.h"
#include "mimalloc/include/mimalloc.h"
#include "testutil.h"

namespace
{

alignas(16) char sacCapsArena[0x10000];
alignas(16) char sacCapsSecond[0x1000];
__declspec(align(64)) char sacCapsExact[0x2000];
alignas(16) char sacCapsAgn16[0x10000];
alignas(16) char sacCapsAgn4[0x10000];

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

int Knob(const char* pcSpec)
{
    int iKnob = -1;
    unsigned int uBytes = 0;
    bool bClamped = false;
    REQUIRE(RedParseHeapCapArg(pcSpec, (int)strlen(pcSpec), &iKnob, &uBytes, &bClamped));
    return iKnob;
}

class HeapCapScope
{
public:
    ~HeapCapScope()
    {
        for (int k = 0; k < 8; ++k)
        {
            RedNoteHeapCap(k, 0);
        }
        RedHeapCapCloseWindow();
    }
};

unsigned int OtherCaps(int iA, int iB)
{
    unsigned int uSum = 0;
    for (int i = 0; i < maxheaps; ++i)
    {
        if (i != iA && i != iB && gRedHeapSlots[i].bLive)
        {
            uSum += gRedHeapSlots[i].cap;
        }
    }
    return uSum;
}

}

TEST_CASE_NAMED(FHeapCapsTest1, "Unit::HeapCaps::heap default caps equal the created sizes", "[EngineFilter]")
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

TEST_CASE_NAMED(FHeapCapsTest2, "Unit::HeapCaps::heap over-cap allocation fails through OnFailure", "[EngineFilter]")
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsOver");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    FailureCapture kCapture;
    const unsigned int uCap = gRedHeapSlots[iH].cap;
    gRedHeapSlots[iH].cap = 256;
    const unsigned int uFailedBefore = CapsStats(iH).uFailed;
    const unsigned int uAllocs = CapsStats(iH).uAllocs;
    void* pOver = RedAllocFromHeap(iH, 512, false);
    CHECK(!pOver);
    CHECK(CapsStats(iH).uAllocs == uAllocs);
    CHECK(siFailCalls == 1);
    CHECK(siFailHeap == iH);
    CHECK(suFailSize == 512);
    CHECK(CapsStats(iH).uFailed == uFailedBefore + 1);
    void* pFit = RedAllocFromHeap(iH, 64, false);
    CHECK(pFit);
    CHECK(CapsStats(iH).uAllocs == uAllocs + 1);
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

TEST_CASE_NAMED(FHeapCapsTest3, "Unit::HeapCaps::heap over-cap failure records the failing heap and need", "[EngineFilter]")
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

TEST_CASE_NAMED(FHeapCapsTest4, "Unit::HeapCaps::wrapping heap request fails", "[EngineFilter]")
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsWrap");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    FailureCapture kCapture;
    const unsigned int uAllocs = CapsStats(iH).uAllocs;
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
    CHECK(CapsStats(iH).uAllocs == uAllocs);
    CHECK(siFailCalls == iExpectFails + 1);
    CHECK(siFailHeap == iH);
    CHECK(suFailSize == 0xFFFFFFFFu);
}

TEST_CASE_NAMED(FHeapCapsTest5, "Unit::HeapCaps::heap charge rolls back when mimalloc refuses the block", "[EngineFilter]")
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsRollback");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    FailureCapture kCapture;
    const unsigned int uCap = gRedHeapSlots[iH].cap;
    const RedHeapStats kBefore = CapsStats(iH);
    gRedHeapSlots[iH].cap = 0x7FF10000u;
    const long lDisallowOs = mi_option_get(mi_option_disallow_os_alloc);
    mi_option_set(mi_option_disallow_os_alloc, 1);
    void* p = RedAllocFromHeap(iH, 0x7FF00000u, false);
    mi_option_set(mi_option_disallow_os_alloc, lDisallowOs);
    gRedHeapSlots[iH].cap = uCap;
    const RedHeapStats kAfter = CapsStats(iH);
    CHECK(!p);
    CHECK(kAfter.uUsed == kBefore.uUsed);
    CHECK(kAfter.uAllocs == kBefore.uAllocs);
    CHECK(kAfter.uFailed == kBefore.uFailed + 1);
    CHECK(siFailCalls == 1);
    CHECK(siFailHeap == iH);
    if (p)
    {
        RedFreeToHeap(iH, p, 0);
    }
}

TEST_CASE_NAMED(FHeapCapsTest6, "Unit::HeapCaps::heap free is cap minus used on a fixed script", "[EngineFilter]")
{
    ScratchHeap kLow(sacCapsExact, sacCapsExact + 0x1000, 0x10, "CapsExactLow");
    ScratchHeap kHigh(sacCapsExact + 0x1000, sacCapsExact + 0x2000, 0x10, "CapsExactHigh");
    const int iL = kLow.miHeap;
    const int iH = kHigh.miHeap;
    REQUIRE(iL >= 0);
    REQUIRE(iH == iL + 1);
    static const unsigned int kauExpect[15][2] = {
        { 4096, 4096 }, { 3968, 4096 }, { 3968, 3568 }, { 3744, 3568 }, { 3616, 3568 },
        { 3840, 3568 }, { 3632, 3568 }, { 3552, 3568 }, { 3680, 3568 }, { 3888, 3568 },
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
        case 6: pD = RedAllocFromHeap(iL, 180, false); REQUIRE(pD); break;
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
        CHECK(RedGetHeapFree(iL) == kauExpect[iStep][0]);
        CHECK(RedGetHeapFree(iH) == kauExpect[iStep][1]);
    }
    CHECK(RedGetHeapSize(iL) == 0x1000);
    CHECK(RedGetHeapSize(iH) == 0x1000);
}

TEST_CASE_NAMED(FHeapCapsTest7, "Unit::HeapCaps::heap charge follows the need formula", "[EngineFilter]")
{
    static const unsigned int kauSizes[24] = {
        0, 1, 3, 4, 12, 15, 16, 17, 100, 0x3FF, 0, 1,
        3, 4, 12, 15, 16, 17, 100, 0x3FF, 17, 100, 0, 0x3FF,
    };
    static const int kaiFreeOrder[24] = {
        5, 17, 0, 23, 11, 8, 2, 20, 14, 1, 22, 9,
        6, 19, 3, 12, 21, 7, 16, 4, 13, 10, 18, 15,
    };
    ScratchHeap kH16(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsAgn16");
    ScratchHeap kH4(sacCapsAgn4, sacCapsAgn4 + sizeof(sacCapsAgn4), 4, "CapsAgn4");
    const int aiHeaps[2] = { kH16.miHeap, kH4.miHeap };
    const unsigned int auAlign[2] = { 0x10, 4 };
    for (int h = 0; h < 2; ++h)
    {
        const int iH = aiHeaps[h];
        const unsigned int uAlign = auAlign[h];
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
        CHECK(CapsStats(iH).uUsed == uSize - uAlign);
        CHECK(RedGetHeapFree(iH) == uAlign);
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

TEST_CASE_NAMED(FHeapCapsTest8, "Unit::HeapCaps::raised heap cap does not change the free value", "[EngineFilter]")
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsRaised");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    void* p = RedAllocFromHeap(iH, 1000, false);
    REQUIRE(p);
    const unsigned int uFree = RedGetHeapFree(iH);
    const unsigned int uCap = gRedHeapSlots[iH].cap;
    RedRaiseHeapCap(iH, uCap * 2);
    REQUIRE(gRedHeapSlots[iH].cap == uCap * 2);
    CHECK(RedGetHeapFree(iH) == uFree);
    RedFreeToHeap(iH, p, 0);
}

TEST_CASE_NAMED(FHeapCapsTest9, "Unit::HeapCaps::heap free never wraps and is zero for a destroyed heap", "[EngineFilter]")
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

TEST_CASE_NAMED(FHeapCapsTest10, "Unit::HeapCaps::heap realloc charges like allocate copy free", "[EngineFilter]")
{
    static const unsigned int kauSizes[8] = { 64, 100, 90, 40, 4000, 4000, 20, 512 };
    ScratchHeap kReal(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsRealloc");
    ScratchHeap kTwin(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsTwin");
    REQUIRE(kReal.miHeap >= 0);
    REQUIRE(kTwin.miHeap >= 0);
    void* pReal = RedAllocFromHeap(kReal.miHeap, kauSizes[0], false);
    void* pTwin = RedAllocFromHeap(kTwin.miHeap, kauSizes[0], false);
    REQUIRE(pReal);
    REQUIRE(pTwin);
    for (int i = 1; i < 8; ++i)
    {
        CAPTURE(i);
        const unsigned int uOld = kauSizes[i - 1];
        const unsigned int uNew = kauSizes[i];
        const unsigned int uAllocs = CapsStats(kReal.miHeap).uAllocs;
        pReal = RedReallocInHeap(kReal.miHeap, pReal, uOld, uNew);
        void* pNext = RedAllocFromHeap(kTwin.miHeap, uNew, false);
        REQUIRE(pReal);
        REQUIRE(pNext);
        memcpy(pNext, pTwin, uOld < uNew ? uOld : uNew);
        RedFreeToHeap(kTwin.miHeap, pTwin, 0);
        pTwin = pNext;
        const RedHeapStats kReals = CapsStats(kReal.miHeap);
        const RedHeapStats kTwins = CapsStats(kTwin.miHeap);
        CHECK(kReals.uAllocs == uAllocs + 1);
        CHECK(kReals.uUsed == kTwins.uUsed);
        CHECK(kReals.uPeak == kTwins.uPeak);
        CHECK(kReals.uAllocs == kTwins.uAllocs);
        CHECK(kReals.uFailed == kTwins.uFailed);
    }
    RedFreeToHeap(kReal.miHeap, pReal, 0);
    RedFreeToHeap(kTwin.miHeap, pTwin, 0);
    CHECK(CapsStats(kReal.miHeap).uUsed == 0);
}

TEST_CASE_NAMED(FHeapCapsTest11, "Unit::HeapCaps::heap realloc at the cap fails and leaves the block", "[EngineFilter]")
{
    ScratchHeap kHeap(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsReallocCap");
    REQUIRE(kHeap.miHeap >= 0);
    unsigned char* p = (unsigned char*)RedAllocFromHeap(kHeap.miHeap, 100, false);
    REQUIRE(p);
    memset(p, 0x77, 100);
    const RedHeapStats kBefore = CapsStats(kHeap.miHeap);
    gRedHeapSlots[kHeap.miHeap].cap = kBefore.uUsed + 8;
    FailureCapture kFail;
    CHECK(!RedReallocInHeap(kHeap.miHeap, p, 100, 200));
    const RedHeapStats kAfter = CapsStats(kHeap.miHeap);
    CHECK(siFailCalls == 1);
    CHECK(siFailHeap == kHeap.miHeap);
    CHECK(suFailSize == 200);
    CHECK(kAfter.uUsed == kBefore.uUsed);
    CHECK(kAfter.uAllocs == kBefore.uAllocs);
    CHECK(kAfter.uFailed == kBefore.uFailed + 1);
    for (int i = 0; i < 100; ++i)
    {
        CHECK(p[i] == 0x77);
    }
    RedFreeToHeap(kHeap.miHeap, p, 0);
    CHECK(CapsStats(kHeap.miHeap).uUsed == 0);
}

TEST_CASE_NAMED(FHeapCapsTest12, "Unit::HeapCaps::heap cap raise moves only cap", "[EngineFilter]")
{
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsRaiseOnly");
    const int iH = kHeap.miHeap;
    REQUIRE(iH >= 0);
    FailureCapture kFail;
    const unsigned int uDefault = gRedHeapSlots[iH].defaultCap;
    const unsigned int uFree = RedGetHeapFree(iH);
    CHECK(!RedAllocFromHeap(iH, uDefault + 0x1000, false));
    RedRaiseHeapCap(iH, uDefault * 4);
    CHECK(gRedHeapSlots[iH].cap == uDefault * 4);
    CHECK(gRedHeapSlots[iH].defaultCap == uDefault);
    CHECK(RedGetHeapFree(iH) == uFree);
    CHECK(RedGetHeapSize(iH) == uDefault);
    void* p = RedAllocFromHeap(iH, uDefault + 0x1000, false);
    CHECK(p);
    if (p)
    {
        RedFreeToHeap(iH, p, 0);
    }
    RedRaiseHeapCap(-1, uDefault * 4);
    RedRaiseHeapCap(maxheaps, uDefault * 4);
}

TEST_CASE_NAMED(FHeapCapsTest13, "Unit::HeapCaps::heap cap is clamped to the per-heap ceiling by the parser", "[EngineFilter]")
{
    int iKnob = -1;
    unsigned int uBytes = 0;
    bool bClamped = false;
    const char* pcHuge = "Temp=512M";
    REQUIRE(RedParseHeapCapArg(pcHuge, (int)strlen(pcHuge), &iKnob, &uBytes, &bClamped));
    CHECK(uBytes == RedScriptLimits::kHeapCapMax);
    CHECK_FALSE(bClamped);
    const char* pcOver = "Temp=513M";
    REQUIRE(RedParseHeapCapArg(pcOver, (int)strlen(pcOver), &iKnob, &uBytes, &bClamped));
    CHECK(uBytes == RedScriptLimits::kHeapCapMax);
    CHECK(bClamped);
    const char* pcSaturate = "Temp=99999999999999999999999M";
    REQUIRE(RedParseHeapCapArg(pcSaturate, (int)strlen(pcSaturate), &iKnob, &uBytes, &bClamped));
    CHECK(uBytes == RedScriptLimits::kHeapCapMax);
    CHECK(bClamped);
}

TEST_CASE_NAMED(FHeapCapsTest14, "Unit::HeapCaps::heap cap total is limited", "[EngineFilter]")
{
    ScratchHeap kA(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsTotalA");
    ScratchHeap kB(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsTotalB");
    const int iA = kA.miHeap;
    const int iB = kB.miHeap;
    REQUIRE(iA >= 0);
    REQUIRE(iB >= 0);
    const unsigned int uDefault = gRedHeapSlots[iA].defaultCap;
    const unsigned int uOthers = OtherCaps(iA, iB);
    {
        LogCapture capture;
        gRedHeapSlots[iB].cap = RedScriptLimits::kHeapCapTotalMax - uOthers - (uDefault + 0x1000);
        RedRaiseHeapCap(iA, uDefault * 4);
        CHECK(gRedHeapSlots[iA].cap == uDefault + 0x1000);
        CHECK(capture.Count("heap cap for CapsTotalA reduced to 69632, the total of all caps is limited") == 1);
    }
    {
        LogCapture capture;
        gRedHeapSlots[iB].cap = RedScriptLimits::kHeapCapTotalMax - uOthers - 0x1000;
        RedRaiseHeapCap(iA, uDefault * 4);
        CHECK(gRedHeapSlots[iA].cap == uDefault + 0x1000);
        CHECK(capture.Count("heap cap for CapsTotalA stays at 69632, request 4096 is below the default") == 1);
    }
    {
        LogCapture capture;
        gRedHeapSlots[iB].cap = RedScriptLimits::kHeapCapTotalMax;
        RedRaiseHeapCap(iA, uDefault * 4);
        CHECK(gRedHeapSlots[iA].cap == uDefault + 0x1000);
        CHECK(capture.Count("request 0 is below the default") == 1);
    }
    gRedHeapSlots[iB].cap = gRedHeapSlots[iB].defaultCap;
}

TEST_CASE_NAMED(FHeapCapsTest15, "Unit::HeapCaps::heap cap total holds when a slot is created at the limit", "[EngineFilter]")
{
    HeapCapScope kScope;
    ScratchHeap kLow(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsTrimLow");
    ScratchHeap kHigh(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsTrimHigh");
    const int iLow = kLow.miHeap;
    const int iHigh = kHigh.miHeap;
    REQUIRE(iLow >= 0);
    REQUIRE(iHigh > iLow);
    const unsigned int uDefault = gRedHeapSlots[iHigh].defaultCap;
    gRedHeapSlots[iLow].cap = uDefault * 2;
    const unsigned int uHigh = RedScriptLimits::kHeapCapTotalMax - OtherCaps(iHigh, -1);
    gRedHeapSlots[iHigh].cap = uHigh;
    REQUIRE(OtherCaps(-1, -1) == RedScriptLimits::kHeapCapTotalMax);
    {
        LogCapture capture;
        ScratchHeap kLate(sacCapsSecond, sacCapsSecond + sizeof(sacCapsSecond), 0x10, "CapsTrimLate");
        REQUIRE(kLate.miHeap > iHigh);
        const unsigned int uLate = gRedHeapSlots[kLate.miHeap].defaultCap;
        CHECK(gRedHeapSlots[kLate.miHeap].cap == uLate);
        CHECK(gRedHeapSlots[iHigh].cap == uHigh - uLate);
        CHECK(gRedHeapSlots[iLow].cap == uDefault * 2);
        CHECK(OtherCaps(-1, -1) == RedScriptLimits::kHeapCapTotalMax);
        CHECK(capture.Count("heap cap for CapsTrimHigh reduced to") == 1);
    }
    gRedHeapSlots[iHigh].cap = uHigh;
    {
        RedNoteHeapCap(Knob("HUDEdit=1M"), 0x20000);
        ScratchHeap kLate(sacCapsAgn4, sacCapsAgn4 + sizeof(sacCapsAgn4), 0x10, "HUDEditHeap");
        REQUIRE(kLate.miHeap > iHigh);
        CHECK(gRedHeapSlots[kLate.miHeap].cap == gRedHeapSlots[kLate.miHeap].defaultCap);
        CHECK(OtherCaps(-1, -1) == RedScriptLimits::kHeapCapTotalMax);
    }
    gRedHeapSlots[iHigh].cap = uDefault;
    gRedHeapSlots[iLow].cap = uDefault;
}

TEST_CASE_NAMED(FHeapCapsTest16, "Unit::HeapCaps::heap cap trim stops at the default and at used", "[EngineFilter]")
{
    ScratchHeap kUsed(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsFloorUsed");
    ScratchHeap kRoom(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsFloorRoom");
    const int iUsed = kUsed.miHeap;
    const int iRoom = kRoom.miHeap;
    REQUIRE(iUsed >= 0);
    REQUIRE(iRoom > iUsed);
    const unsigned int uDefault = gRedHeapSlots[iRoom].defaultCap;
    gRedHeapSlots[iRoom].cap = uDefault + 0x100;
    const unsigned int uCap = RedScriptLimits::kHeapCapTotalMax - OtherCaps(iUsed, -1);
    gRedHeapSlots[iUsed].cap = uCap;
    gRedHeapSlots[iUsed].used = (long)(uCap - 0x80);
    {
        ScratchHeap kLate(sacCapsAgn4, sacCapsAgn4 + sizeof(sacCapsAgn4), 0x10, "CapsFloorLate");
        REQUIRE(kLate.miHeap > iRoom);
        CHECK(gRedHeapSlots[iRoom].cap == uDefault);
        CHECK(gRedHeapSlots[iUsed].cap == uCap - 0x7F);
    }
    gRedHeapSlots[iUsed].used = 0;
    gRedHeapSlots[iUsed].cap = gRedHeapSlots[iUsed].defaultCap;
    gRedHeapSlots[iRoom].cap = uDefault;
}

TEST_CASE_NAMED(FHeapCapsTest17, "Unit::HeapCaps::heap cap window open fits notes after dropping stale raises", "[EngineFilter]")
{
    HeapCapScope kScope;
    RedNoteHeapCap(Knob("Debug=1M"), 0x40000);
    ScratchHeap kLow(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Debug");
    ScratchHeap kHigh(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsStale");
    const int iLow = kLow.miHeap;
    const int iHigh = kHigh.miHeap;
    REQUIRE(iLow >= 0);
    REQUIRE(iHigh > iLow);
    gRedHeapSlots[iLow].cap = gRedHeapSlots[iLow].defaultCap;
    gRedHeapSlots[iHigh].cap = RedScriptLimits::kHeapCapTotalMax - OtherCaps(iHigh, -1);
    LogCapture capture;
    RedHeapCapOpenWindow();
    CHECK(gRedHeapSlots[iHigh].cap == gRedHeapSlots[iHigh].defaultCap);
    CHECK(gRedHeapSlots[iLow].cap == 0x40000);
    CHECK(capture.Count("heap cap for") == 0);
}

TEST_CASE_NAMED(FHeapCapsTest18, "Unit::HeapCaps::heap cap below the default changes nothing", "[EngineFilter]")
{
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsLower");
    const int iH = kHeap.miHeap;
    REQUIRE(iH >= 0);
    const unsigned int uDefault = gRedHeapSlots[iH].defaultCap;
    {
        LogCapture capture;
        RedRaiseHeapCap(iH, uDefault - 1);
        CHECK(gRedHeapSlots[iH].cap == uDefault);
        CHECK(capture.Count("heap cap for CapsLower stays at 65536, request 65535 is below the default") == 1);
    }
    RedRaiseHeapCap(iH, uDefault * 2);
    {
        LogCapture capture;
        RedRaiseHeapCap(iH, uDefault / 2);
        CHECK(gRedHeapSlots[iH].cap == uDefault * 2);
        CHECK(capture.Count("heap cap for CapsLower stays at 131072, request 32768 is below the default") == 1);
    }
    CHECK(gRedHeapSlots[iH].defaultCap == uDefault);
}

TEST_CASE_NAMED(FHeapCapsTest19, "Unit::HeapCaps::heap cap request below the raised cap leaves it with one warning", "[EngineFilter]")
{
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsRaiseOnce");
    const int iH = kHeap.miHeap;
    REQUIRE(iH >= 0);
    const unsigned int uDefault = gRedHeapSlots[iH].defaultCap;
    RedRaiseHeapCap(iH, uDefault * 4);
    LogCapture capture;
    RedRaiseHeapCap(iH, uDefault * 2);
    CHECK(gRedHeapSlots[iH].cap == uDefault * 4);
    CHECK(capture.Count("heap cap for CapsRaiseOnce stays at 262144, request 131072 is below it") == 1);
    CHECK(capture.Count("heap cap for") == 1);
    RedRaiseHeapCap(iH, uDefault * 4);
    CHECK(gRedHeapSlots[iH].cap == uDefault * 4);
    CHECK(capture.Count("heap cap for") == 1);
}

TEST_CASE_NAMED(FHeapCapsTest20, "Unit::HeapCaps::heap cap note and script raise combine as the higher value", "[EngineFilter]")
{
    HeapCapScope kScope;
    const int iKnob = Knob("Temp=1M");
    RedNoteHeapCap(iKnob, 0x20000);
    {
        ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
        REQUIRE(kHeap.miHeap >= 0);
        RedRaiseHeapCap(kHeap.miHeap, 0x40000);
        CHECK(gRedHeapSlots[kHeap.miHeap].cap == 0x40000);
    }
    RedNoteHeapCap(iKnob, 0x40000);
    {
        ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
        REQUIRE(kHeap.miHeap >= 0);
        LogCapture capture;
        RedRaiseHeapCap(kHeap.miHeap, 0x20000);
        CHECK(gRedHeapSlots[kHeap.miHeap].cap == 0x40000);
        CHECK(capture.Count("heap cap for Temp stays at 262144, request 131072 is below it") == 1);
    }
}

TEST_CASE_NAMED(FHeapCapsTest21, "Unit::HeapCaps::heap cap note is applied when its slot is created", "[EngineFilter]")
{
    HeapCapScope kScope;
    const int iKnob = Knob("Temp=1M");
    RedNoteHeapCap(iKnob, 0x20000);
    {
        ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
        REQUIRE(kHeap.miHeap >= 0);
        CHECK(gRedHeapSlots[kHeap.miHeap].cap == 0x20000);
        CHECK(gRedHeapSlots[kHeap.miHeap].defaultCap == 0x10000);
    }
    RedNoteHeapCap(iKnob, 0x30000);
    {
        ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
        REQUIRE(kHeap.miHeap >= 0);
        CHECK(gRedHeapSlots[kHeap.miHeap].cap == 0x30000);
    }
    RedNoteHeapCap(iKnob, 0);
    {
        ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
        REQUIRE(kHeap.miHeap >= 0);
        CHECK(gRedHeapSlots[kHeap.miHeap].cap == 0x10000);
    }
}

TEST_CASE_NAMED(FHeapCapsTest22, "Unit::HeapCaps::heap cap knob HUDEdit reaches the HUDEditHeap slot", "[EngineFilter]")
{
    HeapCapScope kScope;
    CHECK(RedHeapCapSlot("HUDEdit") == -1);
    RedNoteHeapCap(Knob("HUDEdit=1M"), 0x20000);
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "HUDEditHeap");
    REQUIRE(kHeap.miHeap >= 0);
    CHECK(gRedHeapSlots[kHeap.miHeap].cap == 0x20000);
    CHECK(RedHeapCapSlot("HUDEdit") == kHeap.miHeap);
    CHECK(RedHeapCapSlot("HUDEditHeap") == -1);
    CHECK(RedHeapCapSlot("Bogus") == -1);
}

TEST_CASE_NAMED(FHeapCapsTest23, "Unit::HeapCaps::heap cap knob names match case-insensitively", "[EngineFilter]")
{
    CHECK(Knob("runtime=1M") == Knob("Runtime=1M"));
    CHECK(Knob("RUNTIME=1M") == Knob("Runtime=1M"));
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
    REQUIRE(kHeap.miHeap >= 0);
    CHECK(RedHeapCapSlot("Temp") == kHeap.miHeap);
    CHECK(RedHeapCapSlot("temp") == kHeap.miHeap);
    CHECK(RedHeapCapSlot("TEMP") == kHeap.miHeap);
}

TEST_CASE_NAMED(FHeapCapsTest24, "Unit::HeapCaps::heap cap window open resets live slots to the note or the default", "[EngineFilter]")
{
    HeapCapScope kScope;
    static const char* const kapcNames[3] = { "Application", "Gamespy", "Debug" };
    for (int i = 0; i < 3; ++i)
    {
        CAPTURE(i);
        ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, kapcNames[i]);
        const int iH = kHeap.miHeap;
        REQUIRE(iH >= 0);
        RedRaiseHeapCap(iH, 0x40000);
        REQUIRE(gRedHeapSlots[iH].cap == 0x40000);
        LogCapture capture;
        RedHeapCapOpenWindow();
        CHECK(RedHeapCapWindowOpen());
        CHECK(gRedHeapSlots[iH].cap == gRedHeapSlots[iH].defaultCap);
        CHECK(capture.Count("heap cap for") == 0);
        RedHeapCapCloseWindow();
        CHECK_FALSE(RedHeapCapWindowOpen());
    }
    LogCapture kCreate;
    RedNoteHeapCap(Knob("Debug=1M"), 0x8000);
    ScratchHeap kBelow(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Debug");
    REQUIRE(kBelow.miHeap >= 0);
    CHECK(kCreate.Count("heap cap for Debug stays at 65536, request 32768 is below the default") == 1);
    RedRaiseHeapCap(kBelow.miHeap, 0x40000);
    REQUIRE(gRedHeapSlots[kBelow.miHeap].cap == 0x40000);
    LogCapture capture;
    RedHeapCapOpenWindow();
    CHECK(gRedHeapSlots[kBelow.miHeap].cap == 0x10000);
    CHECK(capture.Count("below the default") == 1);
}

TEST_CASE_NAMED(FHeapCapsTest25, "Unit::HeapCaps::heap cap note warns once across heap builds", "[EngineFilter]")
{
    HeapCapScope kScope;
    const int iKnob = Knob("Temp=1M");
    RedNoteHeapCap(iKnob, 0x8000);
    {
        LogCapture capture;
        for (int i = 0; i < 3; ++i)
        {
            ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
            REQUIRE(kHeap.miHeap >= 0);
            RedHeapCapOpenWindow();
        }
        CHECK(capture.Count("heap cap for Temp stays at 65536, request 32768 is below the default") == 1);
    }
    RedHeapCapCloseWindow();
    RedNoteHeapCap(iKnob, 0x4000);
    {
        LogCapture capture;
        ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
        REQUIRE(kHeap.miHeap >= 0);
        CHECK(capture.Count("request 16384 is below the default") == 1);
    }
}

TEST_CASE_NAMED(FHeapCapsTest26, "Unit::HeapCaps::heap cap reduced note warns once across heap builds", "[EngineFilter]")
{
    HeapCapScope kScope;
    ScratchHeap kFill(sacCapsAgn16, sacCapsAgn16 + sizeof(sacCapsAgn16), 0x10, "CapsWarnFill");
    const int iFill = kFill.miHeap;
    REQUIRE(iFill >= 0);
    gRedHeapSlots[iFill].cap = RedScriptLimits::kHeapCapTotalMax - OtherCaps(iFill, -1) - 0x11000;
    RedNoteHeapCap(Knob("Temp=1M"), 0x40000);
    {
        LogCapture capture;
        for (int i = 0; i < 3; ++i)
        {
            ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
            REQUIRE(kHeap.miHeap >= 0);
            CHECK(gRedHeapSlots[kHeap.miHeap].cap == 0x11000);
        }
        CHECK(capture.Count("heap cap for Temp reduced to 69632, the total of all caps is limited") == 1);
    }
    gRedHeapSlots[iFill].cap = gRedHeapSlots[iFill].defaultCap;
}

TEST_CASE_NAMED(FHeapCapsTest27, "Unit::HeapCaps::heap cap trim warns again after a fresh script raise", "[EngineFilter]")
{
    HeapCapScope kScope;
    RedNoteHeapCap(Knob("Temp=1M"), 0x8000);
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Temp");
    const int iH = kHeap.miHeap;
    REQUIRE(iH >= 0);
    RedRaiseHeapCap(iH, 0x40000);
    REQUIRE(gRedHeapSlots[iH].cap == 0x40000);
    gRedHeapSlots[iH].cap = RedScriptLimits::kHeapCapTotalMax - OtherCaps(iH, -1);
    {
        LogCapture capture;
        ScratchHeap kLate(sacCapsAgn4, sacCapsAgn4 + sizeof(sacCapsAgn4), 0x10, "CapsTrimLate");
        REQUIRE(kLate.miHeap > iH);
        CHECK(capture.Count("heap cap for Temp reduced to") == 1);
    }
    gRedHeapSlots[iH].cap = gRedHeapSlots[iH].defaultCap;
}

TEST_CASE_NAMED(FHeapCapsTest28, "Unit::HeapCaps::heap cap window open re-applies the note", "[EngineFilter]")
{
    HeapCapScope kScope;
    RedNoteHeapCap(Knob("Debug=1M"), 0x20000);
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "Debug");
    const int iH = kHeap.miHeap;
    REQUIRE(iH >= 0);
    RedRaiseHeapCap(iH, 0x40000);
    REQUIRE(gRedHeapSlots[iH].cap == 0x40000);
    LogCapture capture;
    RedHeapCapOpenWindow();
    CHECK(gRedHeapSlots[iH].cap == 0x20000);
    CHECK(capture.Count("heap cap for") == 0);
    gRedHeapSlots[iH].cap = gRedHeapSlots[iH].defaultCap;
    RedHeapCapOpenWindow();
    CHECK(gRedHeapSlots[iH].cap == 0x20000);
    CHECK(capture.Count("heap cap for") == 0);
    gRedHeapSlots[iH].used = 0x18000;
    RedHeapCapOpenWindow();
    CHECK(gRedHeapSlots[iH].cap == 0x20000);
    CHECK(capture.Count("heap cap for") == 0);
    gRedHeapSlots[iH].used = 0;
}

TEST_CASE_NAMED(FHeapCapsTest29, "Unit::HeapCaps::heap cap stays when its target is below used", "[EngineFilter]")
{
    HeapCapScope kScope;
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsBelowUsed");
    const int iH = kHeap.miHeap;
    REQUIRE(iH >= 0);
    const unsigned int uDefault = gRedHeapSlots[iH].defaultCap;
    RedRaiseHeapCap(iH, uDefault * 4);
    gRedHeapSlots[iH].used = (long)(uDefault * 2);
    {
        LogCapture capture;
        RedHeapCapOpenWindow();
        CHECK(gRedHeapSlots[iH].cap == uDefault * 4);
        CHECK(capture.Count("heap cap for CapsBelowUsed stays at 262144, target 65536 is below used 131072") == 1);
    }
    {
        LogCapture capture;
        RedRaiseHeapCap(iH, uDefault + 1);
        CHECK(gRedHeapSlots[iH].cap == uDefault * 4);
        CHECK(capture.Count("heap cap for CapsBelowUsed stays at 262144, request 65537 is below it") == 1);
    }
    gRedHeapSlots[iH].used = (long)uDefault;
    {
        LogCapture capture;
        RedHeapCapOpenWindow();
        CHECK(gRedHeapSlots[iH].cap == uDefault * 4);
        CHECK(capture.Count("heap cap for CapsBelowUsed stays at 262144, target 65536 is below used 65536") == 1);
    }
    void* p = RedAllocFromHeap(iH, 64, false);
    CHECK(p);
    if (p)
    {
        RedFreeToHeap(iH, p, 0);
    }
    gRedHeapSlots[iH].used = 0;
}

TEST_CASE_NAMED(FHeapCapsTest30, "Unit::HeapCaps::heap cap at used on a heap never raised logs nothing", "[EngineFilter]")
{
    HeapCapScope kScope;
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsNeverRaised");
    const int iH = kHeap.miHeap;
    REQUIRE(iH >= 0);
    const unsigned int uDefault = gRedHeapSlots[iH].defaultCap;
    gRedHeapSlots[iH].used = (long)uDefault;
    LogCapture capture;
    RedHeapCapOpenWindow();
    CHECK(gRedHeapSlots[iH].cap == uDefault);
    RedRaiseHeapCap(iH, uDefault);
    CHECK(gRedHeapSlots[iH].cap == uDefault);
    CHECK(capture.Count("heap cap for") == 0);
    gRedHeapSlots[iH].used = 0;
}

TEST_CASE_NAMED(FHeapCapsTest31, "Unit::HeapCaps::heap cap parser accepts sizes and rejects malformed values", "[EngineFilter]")
{
    struct Good
    {
        const char* pcSpec;
        unsigned int uBytes;
    };
    static const Good kaGood[] = {
        { "Runtime=96M", 100663296 },
        { "Temp=4K", 4096 },
        { "Temp=4096", 4096 },
    };
    for (size_t i = 0; i < sizeof(kaGood) / sizeof(kaGood[0]); ++i)
    {
        CAPTURE(i);
        int iKnob = -1;
        unsigned int uBytes = 0;
        bool bClamped = true;
        REQUIRE(RedParseHeapCapArg(kaGood[i].pcSpec, (int)strlen(kaGood[i].pcSpec), &iKnob, &uBytes, &bClamped));
        CHECK(iKnob >= 0);
        CHECK(uBytes == kaGood[i].uBytes);
        CHECK_FALSE(bClamped);
    }
    int iKnob = -1;
    unsigned int uBytes = 0;
    bool bClamped = false;
    CHECK(RedParseHeapCapArg("Temp=1M trailing", 7, &iKnob, &uBytes, &bClamped));
    CHECK(uBytes == 0x100000);
    static const char* const kapcBad[] = {
        "Temp", "", "=1M", "Temp=", "Temp=K", "Temp=12x", "Temp=12KK", "Temp=1M2", "Bogus=1M", "Temp=0", "Temp=0M", "Temp=-1", "HUDEditHeap=1M",
    };
    for (size_t i = 0; i < sizeof(kapcBad) / sizeof(kapcBad[0]); ++i)
    {
        CAPTURE(i);
        uBytes = 7;
        CHECK_FALSE(RedParseHeapCapArg(kapcBad[i], (int)strlen(kapcBad[i]), &iKnob, &uBytes, &bClamped));
        CHECK(uBytes == 7);
    }
}

TEST_CASE_NAMED(FHeapCapsTest32, "Unit::HeapCaps::raised heap cap does not change a heap move", "[EngineFilter]")
{
    ScratchHeap kLow(sacCapsExact, sacCapsExact + 0x1000, 0x10, "CapsMoveLow");
    ScratchHeap kHigh(sacCapsExact + 0x1000, sacCapsExact + 0x2000, 0x10, "CapsMoveHigh");
    const int iL = kLow.miHeap;
    REQUIRE(iL >= 0);
    REQUIRE(kHigh.miHeap == iL + 1);
    gRedHeapSlots[iL].used = 0xF00;
    RedRaiseHeapCap(iL, 0x8000);
    REQUIRE(gRedHeapSlots[iL].cap == 0x8000);
    RedMoveHeapEnd(iL, -0x800);
    CHECK(gRedHeapSlots[iL].defaultCap == 0x1000);
    CHECK(gRedHeapSlots[iL].cap == 0x8000);
    CHECK(gRedHeapSlots[iL + 1].defaultCap == 0x1000);
    gRedHeapSlots[iL].used = 0;
}

TEST_CASE_NAMED(FHeapCapsTest33, "Unit::HeapCaps::raised heap cap does not change the heap size reading", "[EngineFilter]")
{
    ScratchHeap kHeap(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsSize");
    const int iH = kHeap.miHeap;
    REQUIRE(iH >= 0);
    const unsigned int uSize = RedGetHeapSize(iH);
    RedRaiseHeapCap(iH, uSize * 3);
    REQUIRE(gRedHeapSlots[iH].cap == uSize * 3);
    CHECK(RedGetHeapSize(iH) == uSize);
}

TEST_CASE_NAMED(FHeapCapsTest34, "Unit::HeapCaps::heap try allocation fails without the failure handler", "[EngineFilter]")
{
    ScratchHeap kScratch(sacCapsArena, sacCapsArena + sizeof(sacCapsArena), 0x10, "CapsTry");
    const int iH = kScratch.miHeap;
    REQUIRE(iH >= 0);
    FailureCapture kCapture;
    const unsigned int uCap = gRedHeapSlots[iH].cap;
    REQUIRE(CapsStats(iH).uUsed == 0);
    gRedHeapSlots[iH].cap = 256;
    REQUIRE(Need(240, 0x10) == 256);
    REQUIRE(Need(241, 0x10) == 272);
    const unsigned int uFailed = CapsStats(iH).uFailed;
    void* pExact = RedTryAllocFromHeap(iH, 240);
    CHECK(pExact != nullptr);
    CHECK(((uintptr_t)pExact & 0xF) == 0);
    CHECK(CapsStats(iH).uUsed == 256);
    if (pExact)
    {
        RedFreeToHeap(iH, pExact, 0);
    }
    void* pOver = RedTryAllocFromHeap(iH, 241);
    CHECK(pOver == nullptr);
    CHECK(CapsStats(iH).uFailed == uFailed + 1);
    CHECK(siFailCalls == 0);
    void* pZero = RedTryAllocFromHeap(iH, 0);
    CHECK(pZero != nullptr);
    CHECK(((uintptr_t)pZero & 0xF) == 0);
    if (pZero)
    {
        RedFreeToHeap(iH, pZero, 0);
    }
    gRedHeapSlots[iH].cap = uCap;
    for (unsigned int u = 1; u <= 48; ++u)
    {
        CAPTURE(u);
        void* p = RedTryAllocFromHeap(iH, u);
        CHECK(p != nullptr);
        CHECK(((uintptr_t)p & 0xF) == 0);
        if (p)
        {
            RedFreeToHeap(iH, p, 0);
        }
    }
    CHECK(CapsStats(iH).uUsed == 0);
    CHECK(siFailCalls == 0);
    gRedHeapSlots[iH].cap = 256;
    void* pReported = RedAllocFromHeap(iH, 241, false);
    gRedHeapSlots[iH].cap = uCap;
    CHECK(pReported == nullptr);
    CHECK(siFailCalls == 1);
    CHECK(suFailSize == 241);
}
