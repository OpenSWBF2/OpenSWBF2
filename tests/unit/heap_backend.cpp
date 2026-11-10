#include "third_party/doctest/doctest.h"

#include <string.h>
#include <new>

#include "PebbleFL/Common/PblHandle.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"

namespace
{

alignas(16) char sacBackendPurge[0x4000];
alignas(16) char sacBackendVisit[0x4000];
alignas(16) char sacBackendMove[0x4000];

class BoundHeap
{
public:
    BoundHeap(char* pStart, char* pEnd, unsigned int uAlign, const char* pcName)
        : miHeap(RedCreateHeap(pStart, pEnd, uAlign, pcName))
    {
    }

    ~BoundHeap()
    {
        if (miHeap >= 0)
        {
            RedDestroyHeap(miHeap);
        }
    }

    int miHeap;
};

struct PurgeProbe : PblHandled
{
};

bool OwnedBy(const void* pKey, void* pArg)
{
    return RedHeapOwner(pKey) == *(const int*)pArg;
}

PurgeProbe* BuildProbe(int iHeap)
{
    void* p = RedAllocFromHeap(iHeap, sizeof(PurgeProbe), false);
    REQUIRE(p);
    return new (p) PurgeProbe;
}

struct VisitLog
{
    int iCalls;
    int iStopAfter;
    void* apUser[8];
    size_t auSize[8];
};

bool RecordBlock(void* pUser, size_t uSize, void* pArg)
{
    VisitLog& rLog = *(VisitLog*)pArg;
    if (rLog.iCalls < 8)
    {
        rLog.apUser[rLog.iCalls] = pUser;
        rLog.auSize[rLog.iCalls] = uSize;
    }
    ++rLog.iCalls;
    return rLog.iStopAfter == 0 || rLog.iCalls < rLog.iStopAfter;
}

unsigned int Failed(int iHeap)
{
    return (unsigned int)gRedHeapSlots[iHeap].failed;
}

}

TEST_CASE("heap purge drops the handles owned by the heap" * doctest::test_suite("regression"))
{
    PurgeProbe* pKeep = BuildProbe(0);
    const unsigned int uKeepId = pKeep->GetHandleId();
    {
        BoundHeap kScratch(sacBackendPurge, sacBackendPurge + sizeof(sacBackendPurge), 0x10,
                           "T1Purge");
        int iHeap = kScratch.miHeap;
        REQUIRE(iHeap > 0);
        PurgeProbe* pA = BuildProbe(iHeap);
        PurgeProbe* pB = BuildProbe(iHeap);
        const unsigned int uA = pA->GetHandleId();
        const unsigned int uB = pB->GetHandleId();
        CHECK(RedHeapOwner(pA) == iHeap);
        CHECK(RedHeapOwner(pB) == iHeap);
        CHECK(RedHeapOwner(pKeep) == 0);
        CHECK(PblHandleRegistry::IsLive(pA, uA));
        CHECK(PblHandleRegistry::IsLive(pB, uB));
        CHECK(PblHandleRegistry::CountIf(OwnedBy, &iHeap) == 2);
        const unsigned int uCount = PblHandleRegistry::GetCount();
        RedPurgeHeapHandles(iHeap, "T1Purge");
        CHECK(!PblHandleRegistry::IsLive(pA, uA));
        CHECK(!PblHandleRegistry::IsLive(pB, uB));
        CHECK(PblHandleRegistry::IsLive(pKeep, uKeepId));
        CHECK(PblHandleRegistry::CountIf(OwnedBy, &iHeap) == 0);
        CHECK(PblHandleRegistry::GetCount() == uCount - 2);
    }
    pKeep->~PurgeProbe();
    RedFreeToHeap(0, pKeep, 0);
}

TEST_CASE("heap visit reports every live block" * doctest::test_suite("regression"))
{
    static const unsigned int kauSizes[5] = { 1, 13, 16, 100, 0x3FF };
    BoundHeap kScratch(sacBackendVisit, sacBackendVisit + sizeof(sacBackendVisit), 0x10,
                       "T1Visit");
    const int iHeap = kScratch.miHeap;
    REQUIRE(iHeap > 0);
    void* apBlock[5];
    for (int i = 0; i < 5; ++i)
    {
        apBlock[i] = RedAllocFromHeap(iHeap, kauSizes[i], (i & 1) != 0);
        REQUIRE(apBlock[i]);
    }
    RedFreeToHeap(iHeap, apBlock[1], 0);
    apBlock[1] = 0;
    VisitLog kLog;
    memset(&kLog, 0, sizeof(kLog));
    CHECK(RedVisitHeap(iHeap, RecordBlock, &kLog));
    CHECK(kLog.iCalls == 4);
    int iMatched = 0;
    for (int c = 0; c < kLog.iCalls && c < 8; ++c)
    {
        for (int i = 0; i < 5; ++i)
        {
            if (apBlock[i] != 0 && apBlock[i] == kLog.apUser[c])
            {
                CAPTURE(i);
                CHECK(kLog.auSize[c] >= kauSizes[i]);
                ++iMatched;
            }
        }
    }
    CHECK(iMatched == 4);
    VisitLog kStop;
    memset(&kStop, 0, sizeof(kStop));
    kStop.iStopAfter = 1;
    RedVisitHeap(iHeap, RecordBlock, &kStop);
    CHECK(kStop.iCalls == 1);
    CHECK(!RedVisitHeap(-1, RecordBlock, &kStop));
    CHECK(!RedVisitHeap(maxheaps, RecordBlock, &kStop));
    for (int i = 0; i < 5; ++i)
    {
        if (apBlock[i])
        {
            RedFreeToHeap(iHeap, apBlock[i], 0);
        }
    }
}

TEST_CASE("heap move end gives the same result" * doctest::test_suite("regression"))
{
    static const int kaiDelta[15] = {
        0, 16, 17, 32, 33, 48, 0x100, 0x1FD0, 0x1FE0, 0x1FE1, 0x1FF0, 0x1FFF, 0x2000, 0x2010, 0x3000,
    };
    const int iSavedFailHeap = RedMemory::failHeap;
    const unsigned int uSavedFailSize = RedMemory::failSize;
    for (int s = 0; s < 2; ++s)
    {
        for (int k = 0; k < 15; ++k)
        {
            const int iDelta = s == 0 ? kaiDelta[k] : -kaiDelta[k];
            CAPTURE(iDelta);
            BoundHeap kLow(sacBackendMove, sacBackendMove + 0x2000, 0x10, "T1Artist");
            BoundHeap kHigh(sacBackendMove + 0x2000, sacBackendMove + 0x4000, 0x10,
                            "T1Runtime");
            const int iLow = kLow.miHeap;
            const int iHigh = kHigh.miHeap;
            REQUIRE(iLow > 0);
            REQUIRE(iHigh == iLow + 1);
            const int iTarget = iDelta < 0 ? iLow : iHigh;
            const unsigned int uFree = RedGetHeapFree(iTarget);
            const unsigned int uMove = (unsigned int)(iDelta < 0 ? -iDelta : iDelta) & ~15u;
            const unsigned int uRem = uMove <= uFree ? uFree - uMove : 0;
            const bool bMoved = uMove > 16 && uMove <= uFree && (uRem == 0 || uRem >= 32);
            const bool bFailed = uMove > 16 && uMove > uFree;
            const unsigned int uLowSize = RedGetHeapSize(iLow);
            const unsigned int uHighSize = RedGetHeapSize(iHigh);
            const unsigned int uLowFailed = Failed(iLow);
            const unsigned int uHighFailed = Failed(iHigh);
            RedMoveHeapEnd(iLow, iDelta);
            const int iShift = !bMoved ? 0 : iDelta < 0 ? -(int)uMove : (int)uMove;
            CHECK(RedGetHeapSize(iLow) == uLowSize + iShift);
            CHECK(RedGetHeapSize(iHigh) == uHighSize - iShift);
            CHECK(Failed(iLow) == uLowFailed + (bFailed && iTarget == iLow ? 1 : 0));
            CHECK(Failed(iHigh) == uHighFailed + (bFailed && iTarget == iHigh ? 1 : 0));
        }
    }
    RedMemory::failHeap = iSavedFailHeap;
    RedMemory::failSize = uSavedFailSize;
}
