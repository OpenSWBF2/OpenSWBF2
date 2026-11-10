#include "third_party/doctest/doctest.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "PebbleFL/Common/PblHandle.h"

#include "testutil.h"

#ifdef OPENSWBF2_TEST
extern int gCheckCalls;
#endif

namespace
{

struct RegistryProbe : PblHandled
{
};

struct OffsetBase
{
    virtual ~OffsetBase() {}
    int mPad[3];
};

struct OffsetProbe : OffsetBase, PblHandled
{
};

struct ProbeBuffer
{
    explicit ProbeBuffer(unsigned int uCount) : mCount(uCount)
    {
        mBytes = (char*)calloc(uCount, sizeof(RegistryProbe));
    }

    ~ProbeBuffer() { free(mBytes); }

    RegistryProbe* At(unsigned int i) const { return (RegistryProbe*)(mBytes + i * sizeof(RegistryProbe)); }

    RegistryProbe* Build(unsigned int i) const { return new (At(i)) RegistryProbe; }

    char* mBytes;
    unsigned int mCount;
};

struct KeyRange
{
    const char* pcStart;
    const char* pcEnd;
};

bool KeyInRange(const void* pKey, void* pArg)
{
    const KeyRange& rRange = *(const KeyRange*)pArg;
    return (const char*)pKey >= rRange.pcStart && (const char*)pKey < rRange.pcEnd;
}

void PurgeKeysInRange(const void* pStart, const void* pEnd)
{
    KeyRange kRange = { (const char*)pStart, (const char*)pEnd };
    PblHandleRegistry::PurgeIf(KeyInRange, &kRange);
}

unsigned int IdOf(const PblHandled* pObject)
{
    unsigned int uId = 0;
    PblHandleRegistry::Lookup(pObject, &uId);
    return uId;
}

}

TEST_CASE("handle registry resolves a freed and overwritten block to null" * doctest::test_suite("regression"))
{
    ProbeBuffer kBuf(1);
    RegistryProbe* p = kBuf.Build(0);
    const unsigned int uOldId = p->GetHandleId();
    PblHandle<RegistryProbe> h(p);
    CHECK((RegistryProbe*)h == p);
    CHECK(h.IsLive());
    CHECK(PblHandleRegistry::IsLive(p, uOldId));

    char acSnapshot[sizeof(RegistryProbe)];
    memcpy(acSnapshot, p, sizeof(acSnapshot));
    p->~RegistryProbe();
    memcpy(p, acSnapshot, sizeof(acSnapshot));

    CHECK(p->GetHandleId() == uOldId);
    CHECK(!PblHandleRegistry::IsLive(p, uOldId));
    CHECK(!h.IsLive());
    CHECK(!(RegistryProbe*)h);
    CHECK(h.GetHandleId() == 0);
}

TEST_CASE("handle ids follow the stored create destroy rewind sequence" * doctest::test_suite("regression"))
{
    const unsigned int uSaved = PblHandled::SetNextHandleId(1000);
    ProbeBuffer kBuf(5);
    unsigned int auIds[5];

    RegistryProbe* pA = kBuf.Build(0);
    auIds[0] = pA->GetHandleId();
    RegistryProbe* pB = kBuf.Build(1);
    auIds[1] = pB->GetHandleId();
    pA->~RegistryProbe();
    const unsigned int uRewound = PblHandled::SetNextHandleId(7);
    RegistryProbe* pC = kBuf.Build(2);
    auIds[2] = pC->GetHandleId();
    PblHandled::SetNextHandleId(0xFFFFFFFFu);
    RegistryProbe* pD = kBuf.Build(3);
    auIds[3] = pD->GetHandleId();
    RegistryProbe* pE = kBuf.Build(4);
    auIds[4] = pE->GetHandleId();

    const unsigned int auExpected[5] = {1000, 1001, 7, 0xFFFFFFFFu, 1};
    CHECK(uRewound == 1002);
    for (int i = 0; i < 5; ++i)
    {
        CHECK(auIds[i] == auExpected[i]);
    }
    CHECK(!PblHandleRegistry::Lookup(pA, 0));
    CHECK(IdOf(pB) == 1001);
    CHECK(IdOf(pC) == 7);
    CHECK(IdOf(pD) == 0xFFFFFFFFu);
    CHECK(IdOf(pE) == 1);

    pB->~RegistryProbe();
    pC->~RegistryProbe();
    pD->~RegistryProbe();
    pE->~RegistryProbe();
    PblHandled::SetNextHandleId(uSaved);
}

TEST_CASE("handle registry keeps one entry per address through copies and re-construction" * doctest::test_suite("regression"))
{
    const unsigned int uStart = PblHandleRegistry::GetCount();
    ProbeBuffer kBuf(3);

    RegistryProbe* pSource = kBuf.Build(0);
    const unsigned int uSourceId = pSource->GetHandleId();
    RegistryProbe* pCopy = new (kBuf.At(1)) RegistryProbe(*pSource);
    CHECK(pCopy->GetHandleId() == uSourceId);
    CHECK(IdOf(pCopy) == uSourceId);
    CHECK(PblHandleRegistry::GetCount() == uStart + 2);

    RegistryProbe* pTarget = kBuf.Build(2);
    PblHandle<RegistryProbe> hTarget(pTarget);
    PblHandle<RegistryProbe> hSource(pSource);
    CHECK(PblHandleRegistry::GetCount() == uStart + 3);
    *pTarget = *pSource;
    CHECK(PblHandleRegistry::GetCount() == uStart + 3);
    CHECK(pTarget->GetHandleId() == uSourceId);
    CHECK(IdOf(pTarget) == uSourceId);
    CHECK(!hTarget.IsLive());
    CHECK(hSource.IsLive());
    CHECK((RegistryProbe*)hSource == pSource);

    PblHandle<RegistryProbe> hOld(pSource);
    RegistryProbe* pReborn = new (kBuf.At(0)) RegistryProbe;
    CHECK(PblHandleRegistry::GetCount() == uStart + 3);
    CHECK(IdOf(pReborn) == pReborn->GetHandleId());
    CHECK(pReborn->GetHandleId() != uSourceId);
    CHECK(!hOld.IsLive());

    pReborn->~RegistryProbe();
    pCopy->~RegistryProbe();
    pTarget->~RegistryProbe();
    CHECK(PblHandleRegistry::GetCount() == uStart);
}

TEST_CASE("handle registry purge removes exactly the entries in a destroyed range" * doctest::test_suite("regression"))
{
    const unsigned int uStart = PblHandleRegistry::GetCount();
    const unsigned int kMany = 5000;
    const unsigned int kFew = 10;
    ProbeBuffer kMain(kMany);
    ProbeBuffer kSide(kFew);
    PblHandle<RegistryProbe>* pHandles = new PblHandle<RegistryProbe>[kMany + kFew];

    for (unsigned int i = 0; i < kMany; ++i)
    {
        pHandles[i] = kMain.Build(i);
    }
    for (unsigned int i = 0; i < kFew; ++i)
    {
        pHandles[kMany + i] = kSide.Build(i);
    }
    CHECK(PblHandleRegistry::GetCount() == uStart + kMany + kFew);
    const unsigned int uCapacity = PblHandleRegistry::GetCapacity();
    CHECK(uCapacity >= 16384);

    PurgeKeysInRange(kMain.At(1000), kMain.At(3000));
    CHECK(PblHandleRegistry::GetCount() == uStart + kMany + kFew - 2000);
    CHECK(PblHandleRegistry::GetCapacity() == uCapacity);

    unsigned int uLive = 0;
    unsigned int uDead = 0;
    for (unsigned int i = 0; i < kMany + kFew; ++i)
    {
        bool bInRange = i >= 1000 && i < 3000;
        RegistryProbe* pExpected = i < kMany ? kMain.At(i) : kSide.At(i - kMany);
        if (bInRange)
        {
            uDead += pHandles[i].IsLive() ? 0 : 1;
        }
        else
        {
            uLive += (RegistryProbe*)pHandles[i] == pExpected ? 1 : 0;
        }
    }
    CHECK(uDead == 2000);
    CHECK(uLive == kMany + kFew - 2000);

    for (unsigned int i = 0; i < kMany; ++i)
    {
        kMain.At(i)->~RegistryProbe();
    }
    for (unsigned int i = 0; i < kFew; ++i)
    {
        kSide.At(i)->~RegistryProbe();
    }
    CHECK(PblHandleRegistry::GetCount() == uStart);
    CHECK(PblHandleRegistry::GetCapacity() == uCapacity);
    delete[] pHandles;
}

TEST_CASE("handle registry purge keeps a cluster that wraps past the last slot" * doctest::test_suite("regression"))
{
    ProbeBuffer kInit(1);
    RegistryProbe* pInit = kInit.Build(0);
    const unsigned int uCapacity = PblHandleRegistry::GetCapacity();
    REQUIRE(uCapacity >= 4096);
    const unsigned int uMask = uCapacity - 1;
    const unsigned int uStart = PblHandleRegistry::GetCount();
    const unsigned int kCandidates = uCapacity * 64;
    const unsigned int uHalf = kCandidates / 2;
    ProbeBuffer kBuf(kCandidates);

    unsigned int auLow[2];
    unsigned int uLow = 0;
    for (unsigned int i = 0; i < uHalf && uLow < 2; ++i)
    {
        if (PblHandleRegistry::HomeSlot(kBuf.At(i)) == uMask - 1)
        {
            auLow[uLow++] = i;
        }
    }
    unsigned int auLast[2];
    unsigned int uLast = 0;
    unsigned int uZero = kCandidates;
    for (unsigned int i = uHalf; i < kCandidates && (uLast < 2 || uZero == kCandidates); ++i)
    {
        const unsigned int uHome = PblHandleRegistry::HomeSlot(kBuf.At(i));
        if (uHome == uMask && uLast < 2)
        {
            auLast[uLast++] = i;
        }
        else if (uHome == 0 && uZero == kCandidates)
        {
            uZero = i;
        }
    }
    REQUIRE(uLow == 2);
    REQUIRE(uLast == 2);
    REQUIRE(uZero < kCandidates);

    RegistryProbe* pA = kBuf.Build(auLow[0]);
    RegistryProbe* pB = kBuf.Build(auLast[0]);
    RegistryProbe* pC = kBuf.Build(uZero);
    RegistryProbe* pD = kBuf.Build(auLow[1]);
    RegistryProbe* pE = kBuf.Build(auLast[1]);
    PblHandle<RegistryProbe> hA(pA);
    PblHandle<RegistryProbe> hB(pB);
    PblHandle<RegistryProbe> hC(pC);
    PblHandle<RegistryProbe> hD(pD);
    PblHandle<RegistryProbe> hE(pE);
    CHECK(PblHandleRegistry::GetCount() == uStart + 5);

    PurgeKeysInRange(kBuf.At(0), kBuf.At(uHalf));
    CHECK(PblHandleRegistry::GetCount() == uStart + 3);
    CHECK(!hA.IsLive());
    CHECK(!hD.IsLive());
    CHECK((RegistryProbe*)hB == pB);
    CHECK((RegistryProbe*)hC == pC);
    CHECK((RegistryProbe*)hE == pE);
    CHECK(IdOf(pE) == pE->GetHandleId());

    PurgeKeysInRange(pB, kBuf.At(auLast[0] + 1));
    CHECK(PblHandleRegistry::GetCount() == uStart + 2);
    CHECK(!hB.IsLive());
    CHECK((RegistryProbe*)hC == pC);
    CHECK((RegistryProbe*)hE == pE);
    CHECK(PblHandleRegistry::GetCapacity() == uCapacity);

    pA->~RegistryProbe();
    pB->~RegistryProbe();
    pC->~RegistryProbe();
    pD->~RegistryProbe();
    pE->~RegistryProbe();
    CHECK(PblHandleRegistry::GetCount() == uStart);
    pInit->~RegistryProbe();
}

#ifdef OPENSWBF2_TEST
TEST_CASE("handle registry growth failure warns once and a full table refuses inserts" * doctest::test_suite("regression"))
{
    ProbeBuffer kInit(1);
    RegistryProbe* pInit = kInit.Build(0);
    const unsigned int uCapacity = PblHandleRegistry::GetCapacity();
    const unsigned int uStart = PblHandleRegistry::GetCount();
    REQUIRE(uStart < uCapacity / 2);
    const unsigned int kFill = uCapacity - 1 - uStart;
    ProbeBuffer kBuf(kFill + 1);

    PblHandleRegistry::FailAllocForTest(true);
    {
        LogCapture kLog;
        ExpectLog kFull("OPENSWBF2 HANDLEREG full");
        for (unsigned int i = 0; i < kFill; ++i)
        {
            kBuf.Build(i);
        }
        CHECK(PblHandleRegistry::GetCapacity() == uCapacity);
        CHECK(PblHandleRegistry::GetCount() == uCapacity - 1);
        CHECK(kLog.Count("OPENSWBF2 HANDLEREG grow") == 1);
        const int iChecks = gCheckCalls;
        RegistryProbe* pLast = kBuf.Build(kFill);
        CHECK(gCheckCalls == iChecks + 1);
        CHECK(!PblHandleRegistry::Lookup(pLast, 0));
        CHECK(PblHandleRegistry::GetCount() == uCapacity - 1);
    }
    PblHandleRegistry::FailAllocForTest(false);

    PurgeKeysInRange(kBuf.At(0), kBuf.At(kFill + 1));
    CHECK(PblHandleRegistry::GetCount() == uStart);
    const unsigned int kMore = uCapacity / 2 + 1;
    ProbeBuffer kGrow(kMore);
    for (unsigned int i = 0; i < kMore; ++i)
    {
        kGrow.Build(i);
    }
    CHECK(PblHandleRegistry::GetCapacity() == uCapacity * 2);

    for (unsigned int i = 0; i < kMore; ++i)
    {
        kGrow.At(i)->~RegistryProbe();
    }
    for (unsigned int i = 0; i <= kFill; ++i)
    {
        kBuf.At(i)->~RegistryProbe();
    }
    pInit->~RegistryProbe();
    CHECK(PblHandleRegistry::GetCount() == uStart - 1);
}
#endif

TEST_CASE("handle registry resolves a handle whose PblHandled base is at an offset" * doctest::test_suite("regression"))
{
    void* pBytes = calloc(1, sizeof(OffsetProbe));
    REQUIRE(pBytes);
    OffsetProbe* p = new (pBytes) OffsetProbe;
    CHECK((void*)static_cast<PblHandled*>(p) != (void*)p);
    PblHandle<OffsetProbe> h(p);
    CHECK((OffsetProbe*)h == p);
    CHECK(h.IsLive());

    char acSnapshot[sizeof(OffsetProbe)];
    memcpy(acSnapshot, p, sizeof(acSnapshot));
    p->~OffsetProbe();
    memcpy(p, acSnapshot, sizeof(acSnapshot));
    CHECK(!h.IsLive());
    CHECK(!(OffsetProbe*)h);
    free(pBytes);
}

TEST_CASE("handle registry and handle layout pins hold" * doctest::test_suite("regression"))
{
    CHECK(sizeof(PblHandle<OffsetProbe>) == 8);
    CHECK(offsetof(PblHandle<OffsetProbe>, mObject) == 0);
}

TEST_CASE("handle registry lookups stay within the time budget" * doctest::test_suite("regression"))
{
    const unsigned int kLive = 2048;
    const unsigned int kLookups = 1000000;
    ProbeBuffer kBuf(kLive);
    const PblHandled** ppKeys = (const PblHandled**)malloc(kLive * sizeof(const PblHandled*));
    unsigned int* puIds = (unsigned int*)malloc(kLive * sizeof(unsigned int));
    REQUIRE(ppKeys);
    REQUIRE(puIds);
    for (unsigned int i = 0; i < kLive; ++i)
    {
        RegistryProbe* p = kBuf.Build(i);
        ppKeys[i] = p;
        puIds[i] = p->GetHandleId();
    }

    volatile unsigned int uHits = 0;
    LARGE_INTEGER kFreq;
    LARGE_INTEGER kBegin;
    LARGE_INTEGER kEnd;
    QueryPerformanceFrequency(&kFreq);
    QueryPerformanceCounter(&kBegin);
    unsigned int uSum = 0;
    for (unsigned int i = 0; i < kLookups; ++i)
    {
        unsigned int uSlot = i & (kLive - 1);
        uSum += PblHandleRegistry::IsLive(ppKeys[uSlot], puIds[uSlot]) ? 1 : 0;
    }
    QueryPerformanceCounter(&kEnd);
    uHits = uSum;
    const double ms = (double)(kEnd.QuadPart - kBegin.QuadPart) * 1000.0 / (double)kFreq.QuadPart;
    printf("OPENSWBF2 HANDLEBENCH lookups=%u live=%u ms=%.3f\n", kLookups, kLive, ms);
    fflush(stdout);
    CHECK(uHits == kLookups);
#ifdef OPENSWBF2_TESTS_DEBUG
    CHECK(ms <= 100.0);
#endif

    for (unsigned int i = 0; i < kLive; ++i)
    {
        kBuf.At(i)->~RegistryProbe();
    }
    free(puIds);
    free(ppKeys);
}
