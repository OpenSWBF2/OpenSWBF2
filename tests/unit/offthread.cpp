#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <string.h>

#include "testutil.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryReport.h"

#ifdef OPENSWBF2_TEST
namespace
{

const int kCapThreads = kRedOffThreadSources + 1;

struct CapThreadArgs
{
    HANDLE hAllocated;
    HANDLE hRelease;
};

DWORD WINAPI OffThreadAllocProc(void*)
{
    void* p = RedAllocFromHeap(0, 24, 0);
    if (p)
    {
        RedFreeToHeap(0, p, 0);
    }
    return 0;
}

__declspec(noinline) void* OffThreadAllocSmall()
{
    return RedAllocFromHeap(0, 16, 0);
}

__declspec(noinline) void* OffThreadAllocLarge()
{
    return RedAllocFromHeap(0, 20, 0);
}

DWORD WINAPI OffThreadKeyProc(void*)
{
    for (int i = 0; i < 2; ++i)
    {
        void* p = OffThreadAllocSmall();
        if (p)
        {
            RedFreeToHeap(0, p, 0);
        }
    }
    void* p = OffThreadAllocLarge();
    if (p)
    {
        RedFreeToHeap(0, p, 0);
    }
    return 0;
}

CRITICAL_SECTION gCapLock;

void* CapAlloc()
{
    EnterCriticalSection(&gCapLock);
    void* p = RedAllocFromHeap(0, 32, 0);
    LeaveCriticalSection(&gCapLock);
    return p;
}

void CapFree(void* p)
{
    EnterCriticalSection(&gCapLock);
    RedFreeToHeap(0, p, 0);
    LeaveCriticalSection(&gCapLock);
}

DWORD WINAPI CapThreadProc(void* pArg)
{
    CapThreadArgs* pArgs = (CapThreadArgs*)pArg;
    void* p = CapAlloc();
    SetEvent(pArgs->hAllocated);
    WaitForSingleObject(pArgs->hRelease, INFINITE);
    if (p)
    {
        CapFree(p);
    }
    return 0;
}

bool FindSourceByTid(unsigned int uTid, RedOffThreadSource* pOut)
{
    for (int i = 0; i < kRedOffThreadSources; ++i)
    {
        if (RedGetOffThreadSource(i, pOut) && pOut->uTid == uTid)
        {
            return true;
        }
    }
    return false;
}

}

TEST_CASE("off-thread allocation is counted with tid heap and size" * doctest::test_suite("unit"))
{
    RedResetOffThreadStats();
    unsigned int uBefore = 0;
    RedGetOffThreadTotals(&uBefore, 0, 0);
    HANDLE hThread = CreateThread(0, 0, OffThreadAllocProc, 0, 0, 0);
    REQUIRE(hThread != (HANDLE)0);
    WaitForSingleObject(hThread, INFINITE);
    const unsigned int uTid = GetThreadId(hThread);
    CloseHandle(hThread);

    unsigned int uAfter = 0;
    RedGetOffThreadTotals(&uAfter, 0, 0);
    CHECK(uAfter == uBefore + 1);
    RedOffThreadSource kSource;
    REQUIRE(FindSourceByTid(uTid, &kSource));
    CHECK(strcmp(kSource.pcHeap, "T1") == 0);
    CHECK(kSource.uSize == 24);
    CHECK(kSource.uCount == 1);
    CHECK(kSource.uFrames >= 1);
    CHECK(kSource.apModule[0] == (void*)GetModuleHandleW(0));

    void* p = RedAllocFromHeap(0, 24, 0);
    REQUIRE(p);
    RedFreeToHeap(0, p, 0);
    unsigned int uMain = 0;
    RedGetOffThreadTotals(&uMain, 0, 0);
    CHECK(uMain == uAfter);
}

TEST_CASE("off-thread sources are keyed by tid heap and full stack" * doctest::test_suite("unit"))
{
    RedResetOffThreadStats();
    HANDLE hThread = CreateThread(0, 0, OffThreadKeyProc, 0, 0, 0);
    REQUIRE(hThread != (HANDLE)0);
    WaitForSingleObject(hThread, INFINITE);
    const unsigned int uTid = GetThreadId(hThread);
    CloseHandle(hThread);

    unsigned int uCount = 0;
    unsigned int uSources = 0;
    unsigned int uDropped = 0;
    RedGetOffThreadTotals(&uCount, &uSources, &uDropped);
    CHECK(uCount == 3);
    CHECK(uSources == 2);
    CHECK(uDropped == 0);
    unsigned int uSmall = 0;
    unsigned int uLarge = 0;
    for (int i = 0; i < kRedOffThreadSources; ++i)
    {
        RedOffThreadSource kSource;
        if (RedGetOffThreadSource(i, &kSource) && kSource.uTid == uTid)
        {
            if (kSource.uSize == 16)
            {
                uSmall += kSource.uCount;
            }
            else if (kSource.uSize == 20)
            {
                uLarge += kSource.uCount;
            }
        }
    }
    CHECK(uSmall == 2);
    CHECK(uLarge == 1);
}

TEST_CASE("off-thread source table caps at sixteen and counts dropped" * doctest::test_suite("unit"))
{
    RedResetOffThreadStats();
    InitializeCriticalSection(&gCapLock);
    HANDLE hRelease = CreateEventW(0, TRUE, FALSE, 0);
    REQUIRE(hRelease != (HANDLE)0);
    CapThreadArgs akArgs[kCapThreads];
    HANDLE ahAllocated[kCapThreads];
    HANDLE ahThreads[kCapThreads];
    int iStarted = 0;
    for (int i = 0; i < kCapThreads; ++i)
    {
        ahAllocated[i] = CreateEventW(0, TRUE, FALSE, 0);
        akArgs[i].hAllocated = ahAllocated[i];
        akArgs[i].hRelease = hRelease;
        ahThreads[i] = CreateThread(0, 0, CapThreadProc, &akArgs[i], 0, 0);
        if (ahThreads[i] != 0)
        {
            ++iStarted;
        }
    }
    const bool bAllStarted = iStarted == kCapThreads;
    DWORD uWait = WAIT_FAILED;
    if (bAllStarted)
    {
        uWait = WaitForMultipleObjects(kCapThreads, ahAllocated, TRUE, 30000);
    }
    SetEvent(hRelease);
    for (int i = 0; i < kCapThreads; ++i)
    {
        if (ahThreads[i] != 0)
        {
            WaitForSingleObject(ahThreads[i], INFINITE);
            CloseHandle(ahThreads[i]);
        }
        CloseHandle(ahAllocated[i]);
    }
    CloseHandle(hRelease);
    DeleteCriticalSection(&gCapLock);
    REQUIRE(bAllStarted);
    REQUIRE(uWait == WAIT_OBJECT_0);

    unsigned int uCount = 0;
    unsigned int uSources = 0;
    unsigned int uDropped = 0;
    RedGetOffThreadTotals(&uCount, &uSources, &uDropped);
    CHECK(uCount == (unsigned int)kCapThreads);
    CHECK(uSources == (unsigned int)kRedOffThreadSources);
    CHECK(uDropped == 1);
}

TEST_CASE("report formats off-thread total and source lines" * doctest::test_suite("regression"))
{
    char acLine[256];
    REQUIRE(RedMemoryReport::FormatOffThreadTotal(5, 2, 1, acLine, sizeof(acLine)));
    CHECK(strcmp(acLine, "OPENSWBF2 MEMREPORT offthread count=5 sources=2 dropped=1") == 0);

    RedOffThreadSource kSource;
    memset(&kSource, 0, sizeof(kSource));
    kSource.uTid = 7;
    kSource.pcHeap = "T1";
    kSource.uSize = 24;
    kSource.uCount = 3;
    kSource.uFrames = 2;
    kSource.apModule[0] = GetModuleHandleW(0);
    kSource.apModule[1] = GetModuleHandleW(0);
    REQUIRE(RedImageStamp(kSource.apModule[0], &kSource.auStamp[0]));
    kSource.auStamp[1] = kSource.auStamp[0];
    kSource.auOffset[0] = 0x10;
    kSource.auOffset[1] = 0x20;
    char acSource[768];
    REQUIRE(RedMemoryReport::FormatOffThreadSource(kSource, acSource, sizeof(acSource)));
    CHECK(strcmp(acSource, "OPENSWBF2 MEMREPORT offthread tid=7 heap=T1 size=24 count=3 "
                           "stack=OpenSWBF2Tests.exe+0x10,OpenSWBF2Tests.exe+0x20") == 0);

    kSource.auStamp[1] = kSource.auStamp[0] + 1;
    REQUIRE(RedMemoryReport::FormatOffThreadSource(kSource, acSource, sizeof(acSource)));
    CHECK(strcmp(acSource, "OPENSWBF2 MEMREPORT offthread tid=7 heap=T1 size=24 count=3 "
                           "stack=OpenSWBF2Tests.exe+0x10,stale+0x20") == 0);
    kSource.apModule[1] = 0;
    REQUIRE(RedMemoryReport::FormatOffThreadSource(kSource, acSource, sizeof(acSource)));
    CHECK(strcmp(acSource, "OPENSWBF2 MEMREPORT offthread tid=7 heap=T1 size=24 count=3 "
                           "stack=OpenSWBF2Tests.exe+0x10,abs+0x20") == 0);
    const char* pcFirst = "OPENSWBF2 MEMREPORT offthread tid=7 heap=T1 size=24 count=3 "
                          "stack=OpenSWBF2Tests.exe+0x10";
    REQUIRE(RedMemoryReport::FormatOffThreadSource(kSource, acSource, (unsigned int)strlen(pcFirst) + 5));
    CHECK(strcmp(acSource, pcFirst) == 0);
    unsigned int uStamp = 0;
    CHECK(!RedImageStamp(acSource, &uStamp));

    kSource.uFrames = 0;
    REQUIRE(RedMemoryReport::FormatOffThreadSource(kSource, acSource, sizeof(acSource)));
    CHECK(strcmp(acSource, "OPENSWBF2 MEMREPORT offthread tid=7 heap=T1 size=24 count=3 stack=-") == 0);

    kSource.uFrames = 2;
    char acSmall[16];
    memset(acSmall, 'x', sizeof(acSmall));
    REQUIRE(RedMemoryReport::FormatOffThreadSource(kSource, acSmall, sizeof(acSmall)));
    CHECK(acSmall[15] == 0);
    memset(acSmall, 'x', sizeof(acSmall));
    REQUIRE(RedMemoryReport::FormatOffThreadTotal(5, 2, 1, acSmall, sizeof(acSmall)));
    CHECK(acSmall[15] == 0);
    CHECK(!RedMemoryReport::FormatOffThreadSource(kSource, 0, 16));
    CHECK(!RedMemoryReport::FormatOffThreadTotal(5, 2, 1, 0, 16));
    CHECK(!RedMemoryReport::FormatOffThreadTotal(5, 2, 1, acLine, 0));

    LogCapture capture;
    RedMemoryReport::LogMemReport("exit");
    CHECK(capture.Contains("OPENSWBF2 MEMREPORT offthread count="));
}
#endif
