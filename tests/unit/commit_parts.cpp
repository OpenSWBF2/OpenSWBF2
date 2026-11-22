#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <stdint.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"
#include "mimalloc/include/mimalloc.h"

#ifdef OPENSWBF2_TEST
namespace
{
DWORD WINAPI CommitPartsWaitProc(void* pArg)
{
    WaitForSingleObject((HANDLE)pArg, INFINITE);
    return 0;
}

struct FirstBlockArgs
{
    uintptr_t uSlice;
    unsigned int uAreas;
    unsigned int uSizes;
    unsigned int uMismatch;
    size_t auSize[8];
    unsigned int auOffset[8];
};

bool mi_cdecl CollectFirstBlock(const mi_heap_t*, const mi_heap_area_t* pArea, void* pBlock, size_t, void* pArg)
{
    if (pBlock != 0)
    {
        return true;
    }
    FirstBlockArgs* pArgs = (FirstBlockArgs*)pArg;
    ++pArgs->uAreas;
    const unsigned int uOffset = (unsigned int)((uintptr_t)pArea->blocks % pArgs->uSlice);
    for (unsigned int i = 0; i < pArgs->uSizes; ++i)
    {
        if (pArgs->auSize[i] == pArea->block_size)
        {
            if (pArgs->auOffset[i] != uOffset)
            {
                ++pArgs->uMismatch;
            }
            return true;
        }
    }
    if (pArgs->uSizes == 8)
    {
        ++pArgs->uMismatch;
        return true;
    }
    pArgs->auSize[pArgs->uSizes] = pArea->block_size;
    pArgs->auOffset[pArgs->uSizes] = uOffset;
    ++pArgs->uSizes;
    return true;
}
}

TEST_CASE("commit parts count only game-owned bytes" * doctest::test_suite("unit"))
{
    const size_t kPage = 4096;
    const size_t kMiB = 1024 * 1024;
    unsigned int uBefore = 0;
    REQUIRE(RedVmLedgerCommitted(0, 0, &uBefore));

    char* pForeign = (char*)VirtualAlloc(0, 2 * kMiB, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    REQUIRE(pForeign);
    pForeign[0] = 1;
    HANDLE hHeap = HeapCreate(0, 0, 0);
    REQUIRE(hHeap);
    void* pHeapBlock = HeapAlloc(hHeap, 0, kMiB);
    REQUIRE(pHeapBlock);
    HANDLE hRelease = CreateEventA(0, TRUE, FALSE, 0);
    REQUIRE(hRelease);
    HANDLE hThread = CreateThread(0, 0, CommitPartsWaitProc, hRelease, 0, 0);
    REQUIRE(hThread);

    char* pGame = (char*)VirtualAlloc(0, kMiB, MEM_RESERVE, PAGE_READWRITE);
    REQUIRE(pGame);
    REQUIRE(VirtualAlloc(pGame + 16 * kPage, 3 * kPage, MEM_COMMIT, PAGE_READWRITE));
    RedVmLedgerAdd(pGame + 17 * kPage);
    char* pArena = (char*)VirtualAlloc(0, kMiB, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    REQUIRE(pArena);
    RedVmLedgerAdd(pArena);
    char* pGone = (char*)VirtualAlloc(0, 16 * kPage, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    REQUIRE(pGone);
    RedVmLedgerAdd(pGone);
    RedVmLedgerRemove(pGone + kPage);

    unsigned int uAfter = 0;
    const bool bOk = RedVmLedgerCommitted(pArena, kMiB, &uAfter);

    SetEvent(hRelease);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hRelease);
    HeapFree(hHeap, 0, pHeapBlock);
    HeapDestroy(hHeap);
    RedVmLedgerRemove(pGame);
    RedVmLedgerRemove(pArena);
    VirtualFree(pGone, 0, MEM_RELEASE);
    VirtualFree(pArena, 0, MEM_RELEASE);
    VirtualFree(pGame, 0, MEM_RELEASE);
    VirtualFree(pForeign, 0, MEM_RELEASE);

    REQUIRE(bOk);
    CHECK(uAfter - uBefore == 12288u);
    unsigned int uEnd = 0;
    REQUIRE(RedVmLedgerCommitted(0, 0, &uEnd));
    CHECK(uEnd == uBefore);
    CHECK(!RedVmLedgerCommitted(0, 0, 0));
}

TEST_CASE("mimalloc pages start their first block at an address-independent offset" * doctest::test_suite("unit"))
{
    static const size_t kauSizes[] = {320, 384, 448, 640, 768, 896, 1280, 1792};
    const size_t uSlice = sizeof(void*) * 8192;
    mi_heap_t* pHeap = mi_heap_new();
    REQUIRE(pHeap);
    for (int s = 0; s < 8; ++s)
    {
        const size_t uCount = 2 * (uSlice / kauSizes[s]) + 1;
        for (size_t i = 0; i < uCount; ++i)
        {
            REQUIRE(mi_heap_malloc(pHeap, kauSizes[s] - 16));
        }
    }
    FirstBlockArgs kArgs = {};
    kArgs.uSlice = uSlice;
    mi_heap_visit_blocks(pHeap, false, CollectFirstBlock, &kArgs);
    mi_heap_destroy(pHeap);
    CHECK(kArgs.uAreas == 24u);
    CHECK(kArgs.uSizes == 8u);
    CHECK(kArgs.uMismatch == 0u);
}

TEST_CASE("mimalloc arena region stays inside one page-map range" * doctest::test_suite("unit"))
{
    const uintptr_t kRegion = 0x10000000;
    CHECK(RedMiArenaRegionStart(0x2C830000, kRegion) == 0x30000000);
    CHECK(RedMiArenaRegionStart(0x10000000, kRegion) == 0x10000000);
    CHECK(RedMiArenaRegionStart(0x10010000, kRegion) == 0x20000000);
    CHECK(RedMiArenaRegionStart(0x1BFF0000, kRegion) == 0x20000000);
    CHECK(RedMiArenaRegionStart(0x20010000, kRegion) == 0x30000000);
    CHECK(RedMiArenaRegionStart(0x00000000, kRegion) == 0x00000000);
    CHECK(RedMiArenaRegionStart(0x0FFF0000, 0x00020000) == 0x10000000);
    CHECK(RedMiArenaRegionStart(0x1FFE0000, 0x00020000) == 0x1FFE0000);

    RedMiConfigure(1000);
    uintptr_t uBase = 0;
    size_t uSize = 0;
    const bool bRegion = RedMiArenaRegion(&uBase, &uSize);
    if (sizeof(void*) > 4)
    {
        REQUIRE(bRegion);
        CHECK(uSize == (size_t)kRegion);
        CHECK((uBase >> 29) == ((uBase + uSize - 1) >> 29));
        CHECK((uBase & (kRegion - 1)) == 0);
        MEMORY_BASIC_INFORMATION kInfo;
        REQUIRE(VirtualQuery((const void*)uBase, &kInfo, sizeof(kInfo)) == sizeof(kInfo));
        CHECK((uintptr_t)kInfo.AllocationBase == uBase);
        CHECK(kInfo.State != MEM_FREE);
        RedMiConfigure(1000);
        uintptr_t uAgain = 0;
        size_t uAgainSize = 0;
        REQUIRE(RedMiArenaRegion(&uAgain, &uAgainSize));
        CHECK(uAgain == uBase);
    }
    else
    {
        CHECK(!bRegion);
    }
}
#endif
