#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <string.h>
#include <string>

#include "testutil.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

#ifdef OPENSWBF2_TEST
namespace
{

alignas(16) char sacPerturbArena[0x1000];

const unsigned int kPerturbPadSeed = 2;
const unsigned int kPerturbPoolSeed = 1;
const unsigned int kPerturbAll = kRedPerturbPad | kRedPerturbTop | kRedPerturbFill | kRedPerturbPool;

struct PerturbGuard
{
    PerturbGuard(unsigned int uSeed, unsigned int uModes)
    {
        RedPerturbSet(uSeed, uModes);
    }
    ~PerturbGuard()
    {
        RedPerturbSet(0, 0);
    }
};

PBL_ASAN_NO_SANITIZE bool BytesAre(const char* pc, unsigned int uCount, unsigned char uValue)
{
    const volatile unsigned char* pu = (const volatile unsigned char*)pc;
    for (unsigned int i = 0; i < uCount; ++i)
    {
        if (pu[i] != uValue)
        {
            return false;
        }
    }
    return true;
}

int CreatePerturbHeap()
{
    const int iHeap = RedCreateHeap(sacPerturbArena, sacPerturbArena + sizeof(sacPerturbArena), 0x10, "T1Perturb");
    REQUIRE(iHeap >= 0);
    return iHeap;
}

void AllocFour(int iHeap, char* apc[4])
{
    for (int i = 0; i < 4; ++i)
    {
        apc[i] = (char*)RedAllocFromHeap(iHeap, 24, false);
        REQUIRE(apc[i]);
    }
}

void CheckAbsentSequence(char* apc[4])
{
    CHECK(apc[0] == sacPerturbArena + 16);
    for (int i = 1; i < 4; ++i)
    {
        CHECK(apc[i] - apc[i - 1] == 48);
    }
    for (int i = 0; i < 4; ++i)
    {
        CHECK(BytesAre(apc[i], 24, 0));
    }
    CHECK(BytesAre(sacPerturbArena + 0x800, 0x800, 0));
}

void AllocEight(MemoryPool& rPool, char* apc[8])
{
    for (int i = 0; i < 8; ++i)
    {
        apc[i] = (char*)rPool.Allocate(16);
        REQUIRE(apc[i]);
    }
}

bool Ascending(char* const* apc, int iCount)
{
    for (int i = 1; i < iCount; ++i)
    {
        if (apc[i] <= apc[i - 1])
        {
            return false;
        }
    }
    return true;
}

void FreeEight(MemoryPool& rPool, char* apc[8])
{
    for (int i = 0; i < 8; ++i)
    {
        rPool.Free(apc[i]);
    }
}

void PoolOrder(int aiOrder[8])
{
    MemoryPool kPool("t1perturborder", 16);
    kPool.Create(8, 16, 0);
    char* apc[8];
    AllocEight(kPool, apc);
    for (int i = 0; i < 8; ++i)
    {
        aiOrder[i] = kPool.GetIndex(apc[i]);
    }
    FreeEight(kPool, apc);
    kPool.Destroy();
}

DWORD WINAPI CreatePoolProc(void* pv)
{
    ((MemoryPool*)pv)->Create(8, 16, 0);
    return 0;
}

}

TEST_CASE("perturb heap parses the seed and mode list" * doctest::test_suite("regression"))
{
    unsigned int uSeed = 99;
    unsigned int uModes = 99;
    CHECK(RedPerturbParse("1", &uSeed, &uModes));
    CHECK(uSeed == 1);
    CHECK(uModes == kPerturbAll);
    CHECK(RedPerturbParse("7:pad,pool", &uSeed, &uModes));
    CHECK(uSeed == 7);
    CHECK(uModes == (kRedPerturbPad | kRedPerturbPool));
    CHECK(RedPerturbParse("4294967295:TOP,Fill", &uSeed, &uModes));
    CHECK(uSeed == 4294967295u);
    CHECK(uModes == (kRedPerturbTop | kRedPerturbFill));

    static const char* const apcBad[] = { 0,    "",        "x",          "1:",         "1:bogus", "1:pad,",
                                          "-1", "1x",      "4294967296", "1:pad,,top", ":pad",    "1:padx",
                                          " 1", "1:pa",    "1;pad",      "1:,pad" };
    for (unsigned int i = 0; i < sizeof(apcBad) / sizeof(apcBad[0]); ++i)
    {
        CAPTURE(i);
        uSeed = 5;
        uModes = 6;
        CHECK_FALSE(RedPerturbParse(apcBad[i], &uSeed, &uModes));
        CHECK(uSeed == 5);
        CHECK(uModes == 6);
    }

    char acModes[32];
    RedPerturbFormatModes(kRedPerturbPad | kRedPerturbTop, acModes, sizeof(acModes));
    CHECK(std::string(acModes) == "pad,top");
    RedPerturbFormatModes(kPerturbAll, acModes, sizeof(acModes));
    CHECK(std::string(acModes) == "pad,top,fill,pool");
}

TEST_CASE("perturb heap absent keeps ascending zeroed blocks" * doctest::test_suite("regression"))
{
    PerturbGuard kGuard(0, 0);
    CHECK(RedPerturbModes() == 0);
    const int iHeap = CreatePerturbHeap();
    char* apc[4];
    AllocFour(iHeap, apc);
    CheckAbsentSequence(apc);
    RedDestroyHeap(iHeap);
}

TEST_CASE("perturb heap pad changes the address deltas and counts the pad" * doctest::test_suite("regression"))
{
    PerturbGuard kGuard(kPerturbPadSeed, kRedPerturbPad);
    CHECK(RedPerturbSeed() == kPerturbPadSeed);
    const int iHeap = CreatePerturbHeap();
    char* apc[4];
    AllocFour(iHeap, apc);
    int iChanged = 0;
    for (int i = 1; i < 4; ++i)
    {
        CHECK(apc[i] > apc[i - 1]);
        if (apc[i] - apc[i - 1] != 48)
        {
            ++iChanged;
        }
    }
    CHECK(apc[1] - apc[0] != 48);
    CHECK(iChanged >= 1);
    RedHeapStats kStats;
    memset(&kStats, 0, sizeof(kStats));
    REQUIRE(RedGetHeapStats(iHeap, &kStats));
    CHECK(kStats.uAllocs == 4);
    CHECK(apc[1] - apc[0] == 96);
    CHECK(apc[2] - apc[1] == 96);
    CHECK(apc[3] - apc[2] == 80);
    CHECK(kStats.uUsed == 96 + 96 + 80 + 80);
    CHECK(RedGetHeapSize(iHeap) - RedGetHeapFree(iHeap) == kStats.uUsed);
    RedDestroyHeap(iHeap);
}

TEST_CASE("perturb heap top places plain allocations top down" * doctest::test_suite("regression"))
{
    PerturbGuard kGuard(1, kRedPerturbTop);
    const int iHeap = CreatePerturbHeap();
    char* apc[4];
    AllocFour(iHeap, apc);
    CHECK(apc[1] < apc[0]);
    CHECK(apc[0] > sacPerturbArena + sizeof(sacPerturbArena) / 2);
    for (int i = 1; i < 4; ++i)
    {
        CHECK(apc[i - 1] - apc[i] == 48);
    }
    RedDestroyHeap(iHeap);
}

TEST_CASE("perturb heap fill writes 0xA5 over blocks and new heaps" * doctest::test_suite("regression"))
{
    PerturbGuard kGuard(1, kRedPerturbFill);
    const int iHeap = CreatePerturbHeap();
    CHECK(BytesAre(sacPerturbArena + 0x800, 0x800, 0xA5));
    char* apc[4];
    AllocFour(iHeap, apc);
    for (int i = 0; i < 4; ++i)
    {
        CHECK(BytesAre(apc[i], 24, 0xA5));
    }
    CHECK(apc[0] == sacPerturbArena + 16);
    RedDestroyHeap(iHeap);
}

TEST_CASE("perturb heap absent sequence repeats after the guard resets" * doctest::test_suite("regression"))
{
    char* apcBefore[4];
    {
        PerturbGuard kGuard(0, 0);
        const int iHeap = CreatePerturbHeap();
        AllocFour(iHeap, apcBefore);
        CheckAbsentSequence(apcBefore);
        RedDestroyHeap(iHeap);
    }
    {
        PerturbGuard kGuard(3, kPerturbAll);
        const int iHeap = CreatePerturbHeap();
        char* apc[4];
        AllocFour(iHeap, apc);
        CHECK(BytesAre(apc[0], 24, 0xA5));
        RedDestroyHeap(iHeap);
    }
    CHECK(RedPerturbModes() == 0);
    const int iHeap = CreatePerturbHeap();
    char* apcAfter[4];
    AllocFour(iHeap, apcAfter);
    CheckAbsentSequence(apcAfter);
    for (int i = 0; i < 4; ++i)
    {
        CHECK(apcAfter[i] == apcBefore[i]);
    }
    RedDestroyHeap(iHeap);
}
TEST_CASE("perturb heap pool absent keeps address order and sorted free" * doctest::test_suite("regression"))
{
    PerturbGuard kGuard(0, 0);
    MemoryPool kPool("t1perturb", 16);
    kPool.Create(8, 16, 0);
    char* apc[8];
    AllocEight(kPool, apc);
    CHECK(Ascending(apc, 8));
    for (int i = 0; i < 8; ++i)
    {
        CHECK(kPool.GetIndex(apc[i]) == i);
    }
    kPool.Free(apc[2]);
    kPool.Free(apc[5]);
    CHECK(kPool.Allocate(16) == apc[2]);
    CHECK(kPool.mUsed == 7);
    CHECK(kPool.GetPeak() == 8);
    CHECK(kPool.GetGrown() == 0);
    apc[5] = 0;
    FreeEight(kPool, apc);
    CHECK(kPool.mUsed == 0);
    kPool.Destroy();
}

TEST_CASE("perturb heap pool shuffles the free list and frees to the front" * doctest::test_suite("regression"))
{
    PerturbGuard kGuard(kPerturbPoolSeed, kRedPerturbPool);
    MemoryPool kPool("t1perturb", 16);
    kPool.Create(8, 16, 0);
    char* apc[8];
    AllocEight(kPool, apc);
    CHECK_FALSE(Ascending(apc, 8));
    unsigned int uSeen = 0;
    for (int i = 0; i < 8; ++i)
    {
        REQUIRE(kPool.IsPresent(apc[i]));
        uSeen |= 1u << kPool.GetIndex(apc[i]);
    }
    CHECK(uSeen == 0xFF);
    char* pcLow = apc[0] < apc[1] ? apc[0] : apc[1];
    char* pcHigh = apc[0] < apc[1] ? apc[1] : apc[0];
    kPool.Free(pcLow);
    kPool.Free(pcHigh);
    CHECK(kPool.Allocate(16) == pcHigh);
    CHECK(kPool.mUsed == 7);
    CHECK(kPool.GetPeak() == 8);
    CHECK(kPool.GetGrown() == 0);
    apc[0] = pcHigh;
    apc[1] = 0;
    FreeEight(kPool, apc);
    CHECK(kPool.mUsed == 0);
    kPool.Destroy();
}

TEST_CASE("perturb heap pool shuffles growth chunks" * doctest::test_suite("regression"))
{
    PerturbGuard kGuard(kPerturbPoolSeed, kRedPerturbPool);
    MemoryPool kPool("t1perturbgrow", 16);
    kPool.Create(2, 16, 8);
    char* apc[10];
    for (int i = 0; i < 10; ++i)
    {
        apc[i] = (char*)kPool.Allocate(16);
        REQUIRE(apc[i]);
    }
    CHECK(kPool.GetGrown() == 1);
    CHECK(kPool.GetPeak() == 10);
    CHECK_FALSE(Ascending(apc + 2, 8));
    char* pcMin = apc[2];
    char* pcMax = apc[2];
    for (int i = 3; i < 10; ++i)
    {
        pcMin = apc[i] < pcMin ? apc[i] : pcMin;
        pcMax = apc[i] > pcMax ? apc[i] : pcMax;
    }
    CHECK(pcMax - pcMin == 7 * 16);
    for (int i = 0; i < 10; ++i)
    {
        kPool.Free(apc[i]);
    }
    CHECK(kPool.mUsed == 0);
    kPool.Destroy();
}

TEST_CASE("perturb heap pool order repeats and ignores pools built off the main thread" * doctest::test_suite("regression"))
{
    int aiFirst[8];
    int aiSecond[8];
    {
        PerturbGuard kGuard(kPerturbPoolSeed, kRedPerturbPool);
        PoolOrder(aiFirst);
    }
    PerturbGuard kGuard(kPerturbPoolSeed, kRedPerturbPool);
    MemoryPool kThreadPool("t1perturbthread", 16);
    HANDLE hThread = CreateThread(0, 0, CreatePoolProc, &kThreadPool, 0, 0);
    REQUIRE(hThread);
    CHECK(WaitForSingleObject(hThread, 10000) == WAIT_OBJECT_0);
    CloseHandle(hThread);
    char* apc[8];
    AllocEight(kThreadPool, apc);
    CHECK(Ascending(apc, 8));
    FreeEight(kThreadPool, apc);
    kThreadPool.Destroy();
    PoolOrder(aiSecond);
    bool bIdentity = true;
    for (int i = 0; i < 8; ++i)
    {
        CHECK(aiSecond[i] == aiFirst[i]);
        bIdentity = bIdentity && aiFirst[i] == i;
    }
    CHECK_FALSE(bIdentity);
}

TEST_CASE("perturb heap pool fill writes 0xA5 over every allocated block" * doctest::test_suite("regression"))
{
    PerturbGuard kGuard(1, kRedPerturbFill);
    MemoryPool kPool("t1perturbfill", 16);
    kPool.Create(8, 16, 0);
    char* apc[8];
    AllocEight(kPool, apc);
    CHECK(Ascending(apc, 8));
    for (int i = 0; i < 8; ++i)
    {
        CHECK(BytesAre(apc[i], 16, 0xA5));
        memset(apc[i], 0, 16);
    }
    kPool.Free(apc[3]);
    char* pc = (char*)kPool.Allocate(16);
    CHECK(pc == apc[3]);
    CHECK(BytesAre(pc, 16, 0xA5));
    FreeEight(kPool, apc);
    kPool.Destroy();
}
#endif
