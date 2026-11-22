#include "third_party/doctest/doctest.h"

#include <string.h>

#include "testutil.h"
#include "nettest.h"
#include "Battlefront2/Source/LuaMemory.h"
#include "Battlefront2/Source/GameMemory.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "LuaFL/include/lua.h"
#include "lobject.h"

namespace
{

struct FillGuard
{
    FillGuard(int iHeap, unsigned int uiLeave) : miHeap(iHeap), mpFiller(0)
    {
        unsigned int uiLow = 0;
        unsigned int uiHigh = 0x7FFFFFFFu;
        while (uiLow < uiHigh)
        {
            unsigned int uiMid = uiLow + (uiHigh - uiLow + 1) / 2;
            void* pProbe = RedAllocFromHeap(iHeap, uiMid, 0);
            if (pProbe)
            {
                RedFreeToHeap(iHeap, pProbe, 0);
                uiLow = uiMid;
            }
            else
            {
                uiHigh = uiMid - 1;
            }
        }
        if (uiLow > uiLeave)
        {
            mpFiller = RedAllocFromHeap(iHeap, uiLow - uiLeave, 0);
        }
    }

    ~FillGuard()
    {
        if (mpFiller)
        {
            RedFreeToHeap(miHeap, mpFiller, 0);
        }
    }

    int miHeap;
    void* mpFiller;
};

int AllocHuge(lua_State* L)
{
    lua_newuserdata(L, 0x200000);
    return 0;
}

int AllocSmall(lua_State* L)
{
    lua_newuserdata(L, 16);
    return 0;
}

alignas(16) char sacLuaMiRange[0x10000];
class MiLuaHeap
{
public:
    MiLuaHeap() : miSavedHeap(GameMemory::LuaHeap)
    {
        miHeap = RedCreateHeap(sacLuaMiRange, sacLuaMiRange + sizeof(sacLuaMiRange), 4, "T1LuaMi");
        gRedHeapSlots[miHeap].cap = 0x1000000;
        GameMemory::LuaHeap = miHeap;
        LuaMemory::CreatePools(true);
    }

    ~MiLuaHeap()
    {
        LuaMemory::Reset();
        GameMemory::LuaHeap = miSavedHeap;
        RedDestroyHeap(miHeap);
    }

    int miSavedHeap;
    int miHeap;
};

#ifdef OPENSWBF2_TEST
class PerturbScope
{
public:
    PerturbScope(unsigned int uSeed, unsigned int uModes)
    {
        RedPerturbSet(uSeed, uModes);
    }

    ~PerturbScope()
    {
        RedPerturbSet(0, 0);
    }
};

unsigned int LuaNeed(unsigned int uSize)
{
    return (uSize + 3 + 12) & ~3u;
}
#endif

int RaiseWithHeap(lua_State* L)
{
    RedSetCurrentHeap(0);
    lua_pushstring(L, "boom");
    lua_error(L);
    return 0;
}

}

TEST_CASE("lua oom is raised as LUA_ERRMEM" * doctest::test_suite("regression"))
{
    ExpectLog expect("Lua out of memory");
    lua_State* L = lua_open();
    REQUIRE(L);
    {
        LogCapture capture;
        FillGuard guard(GameMemory::LuaHeap, 0x100000);
        CHECK(lua_cpcall(L, AllocHuge, 0) == LUA_ERRMEM);
        CHECK(capture.Contains("Lua out of memory"));
    }
    CHECK(lua_cpcall(L, AllocSmall, 0) == 0);
    lua_close(L);
}

TEST_CASE("lua userdata header is four pointer widths" * doctest::test_suite("unit"))
{
    CHECK(sizeof(Udata) == 4 * sizeof(void*));
}

TEST_CASE("lua failed shrink keeps the old block" * doctest::test_suite("regression"))
{
    ExpectLog expect("Lua out of memory");
    void* pBlock = Lua_Red_realloc(0, 0, 100);
    REQUIRE(pBlock);
    {
        FillGuard guard(GameMemory::LuaHeap, 0);
        const unsigned int uiFull = RedGetHeapFree(GameMemory::LuaHeap);
        CHECK(!Lua_Red_realloc(pBlock, 100, 50));
        CHECK(RedGetHeapFree(GameMemory::LuaHeap) == uiFull);
    }
    const unsigned int uiFree = RedGetHeapFree(GameMemory::LuaHeap);
    Lua_Red_Free(pBlock, 100);
    CHECK(RedGetHeapFree(GameMemory::LuaHeap) > uiFree);
}

TEST_CASE("lua error restores the current heap" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    RedSetCurrentHeap(GameMemory::LuaHeap);
    CHECK(lua_cpcall(L, RaiseWithHeap, 0) == LUA_ERRRUN);
    CHECK(__RedCurrHeap == GameMemory::LuaHeap);
    lua_close(L);
}

TEST_CASE("lua panic handler exits with code 4" * doctest::test_suite("regression"))
{
    ExpectLog expect("Lua panic");
    lua_State* L = lua_open();
    REQUIRE(L);
    int iCalls = gExitCalls;
    {
        LogCapture capture;
        lua_pushstring(L, "bad thing");
        Lua_Red_Panic(L);
        CHECK(capture.Contains("Lua panic: bad thing"));
    }
    CHECK(gExitCalls == iCalls + 1);
    CHECK(gLastExitCode == 4);
    lua_settop(L, 0);
    Lua_Red_Panic(L);
    lua_pushnumber(L, 1);
    Lua_Red_Panic(L);
    CHECK(gExitCalls == iCalls + 3);
    CHECK(gLastExitCode == 4);
    lua_close(L);
}

TEST_CASE("lua realloc keeps a shrunk heap block in place and bin blocks on their path" * doctest::test_suite("regression"))
{
    MiLuaHeap kHeap;
    unsigned char* pHeap = (unsigned char*)Lua_Red_realloc(0, 0, 2000);
    REQUIRE(pHeap);
    memset(pHeap, 0x3C, 2000);
    unsigned char* pShrunk = (unsigned char*)Lua_Red_realloc(pHeap, 2000, 1500);
    REQUIRE(pShrunk);
    CHECK(pShrunk == pHeap);
    CHECK(pShrunk[1499] == 0x3C);
    unsigned char* pGrown = (unsigned char*)Lua_Red_realloc(pShrunk, 1500, 3000);
    REQUIRE(pGrown);
    CHECK(pGrown[1499] == 0x3C);
    unsigned char* pBin = (unsigned char*)Lua_Red_realloc(0, 0, 40);
    REQUIRE(pBin);
    CHECK(LuaMemory::LocatePool(40, pBin));
    memset(pBin, 0x4D, 40);
    unsigned char* pFromBin = (unsigned char*)Lua_Red_realloc(pBin, 40, 2000);
    REQUIRE(pFromBin);
    CHECK(pFromBin[39] == 0x4D);
    unsigned char* pToBin = (unsigned char*)Lua_Red_realloc(pFromBin, 2000, 40);
    REQUIRE(pToBin);
    CHECK(pToBin[39] == 0x4D);
    CHECK(LuaMemory::LocatePool(40, pToBin));
    unsigned char* pBinToBin = (unsigned char*)Lua_Red_realloc(pToBin, 40, 80);
    REQUIRE(pBinToBin);
    CHECK(LuaMemory::LocatePool(80, pBinToBin));
    CHECK(pBinToBin[39] == 0x4D);
    Lua_Red_Free(pBinToBin, 80);
    Lua_Red_Free(pGrown, 3000);
}

#ifdef OPENSWBF2_TEST
TEST_CASE("lua realloc keeps the fill test hook on the heap path" * doctest::test_suite("regression"))
{
    MiLuaHeap kHeap;
    PerturbScope kPerturb(1, kRedPerturbFill);
    unsigned char* p = (unsigned char*)Lua_Red_realloc(0, 0, 2000);
    REQUIRE(p);
    memset(p, 0x01, 2000);
    unsigned char* q = (unsigned char*)Lua_Red_realloc(p, 2000, 3000);
    REQUIRE(q);
    int iOldBad = 0;
    int iGrownBad = 0;
    for (int i = 0; i < 3000; ++i)
    {
        if (i < 2000 && q[i] != 0x01)
        {
            ++iOldBad;
        }
        if (i >= 2000 && q[i] != 0xA5)
        {
            ++iGrownBad;
        }
    }
    CHECK(iOldBad == 0);
    CHECK(iGrownBad == 0);
    Lua_Red_Free(q, 3000);
}

TEST_CASE("lua realloc keeps the pad test hook on the heap path" * doctest::test_suite("regression"))
{
    MiLuaHeap kHeap;
    PerturbScope kPerturb(7, kRedPerturbPad);
    const long lBefore = gRedHeapSlots[kHeap.miHeap].used;
    char* p = (char*)Lua_Red_realloc(0, 0, 2000);
    REQUIRE(p);
    char* q = (char*)Lua_Red_realloc(p, 2000, 3000);
    REQUIRE(q);
    const unsigned int uCharge = (unsigned int)(gRedHeapSlots[kHeap.miHeap].used - lBefore);
    CHECK(uCharge > LuaNeed(3000));
    CHECK(uCharge <= LuaNeed(3000 + 60));
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(q + 3000));
#endif
    Lua_Red_Free(q, 3000);
}
#endif

TEST_CASE("lua mission bin table is the re-tuned one" * doctest::test_suite("regression"))
{
    LuaMemory::CreatePools(false);
    const unsigned int auBins86[4] = { 8, 56, 10, 28 };
    const unsigned int auCounts86[4] = { 743, 2662, 4589, 2102 };
    const unsigned int auBins64[48] = {
        20, 224, 112, 8, 56, 448, 12, 28, 30, 9, 11, 10,
        15, 13, 14, 7, 16, 4, 24, 22, 18, 23, 17, 26,
        21, 1, 19, 6, 25, 32, 5, 27, 2, 64, 52, 3,
        44, 40, 48, 36, 60, 68, 72, 92, 80, 96, 76, 108
    };
    const unsigned int auCounts64[48] = {
        2945, 719, 899, 1730, 472, 574, 1682, 139, 876, 743, 661, 774,
        254, 453, 437, 175, 200, 71, 99, 52, 69, 46, 49, 71,
        42, 93, 50, 40, 34, 63, 37, 41, 23, 49, 37, 22,
        33, 32, 32, 32, 24, 17, 18, 16, 15, 14, 15, 12
    };
    const bool bWide = sizeof(void*) == 8;
    const unsigned int* puBins = bWide ? auBins64 : auBins86;
    const unsigned int* puCounts = bWide ? auCounts64 : auCounts86;
    const int iEntries = bWide ? 48 : 4;
    for (int i = 0; i < iEntries; ++i)
    {
        CAPTURE(puBins[i]);
        MemoryPool* pPool = LuaMemory::LocatePool(puBins[i] * 4, 0);
        REQUIRE(pPool);
        CHECK(pPool->mCount == puCounts[i]);
    }
    CHECK(!LuaMemory::LocatePool(320 * 4, 0));
    LuaMemory::Reset();
}

TEST_CASE("lua shell bin table is unchanged" * doctest::test_suite("regression"))
{
    LuaMemory::CreatePools(true);
    MemoryPool* pEight = LuaMemory::LocatePool(8 * 4, 0);
    REQUIRE(pEight);
    CHECK(pEight->mCount == 6933);
    MemoryPool* pLast = LuaMemory::LocatePool(320 * 4, 0);
    REQUIRE(pLast);
    CHECK(pLast->mCount == 33);
    LuaMemory::Reset();
}

#ifdef OPENSWBF2_TEST
extern MemPoolInfo gGamePoolInfo[48];
extern MemPoolInfo gShellPoolInfo[48];

TEST_CASE("lua demand histogram bound covers twice every bin" * doctest::test_suite("regression"))
{
    CHECK(LuaMemory::kLuaHistMaxBin == 640);
    CHECK(LuaMemory::kLuaHistRows == 642);
    for (int i = 0; i < 48; ++i)
    {
        CAPTURE(i);
        CHECK(2u * gGamePoolInfo[i].mBin <= LuaMemory::kLuaHistMaxBin);
        CHECK(2u * gShellPoolInfo[i].mBin <= LuaMemory::kLuaHistMaxBin);
    }
    CHECK(LuaMemory::LuaHistRow(0) == 0);
    CHECK(LuaMemory::LuaHistRow(1) == 1);
    CHECK(LuaMemory::LuaHistRow(4) == 1);
    CHECK(LuaMemory::LuaHistRow(5) == 2);
    CHECK(LuaMemory::LuaHistRow(2557) == 640);
    CHECK(LuaMemory::LuaHistRow(2560) == 640);
    CHECK(LuaMemory::LuaHistRow(2561) == 641);
    CHECK(LuaMemory::LuaHistRow(0xFFFFFFFEu) == 641);
    CHECK(LuaMemory::LuaHistRow(0xFFFFFFFFu) == 641);
}

TEST_CASE("lua demand histogram counts peak and calls per bin" * doctest::test_suite("regression"))
{
    ExpectLog expect("Lua out of memory");
    MiLuaHeap kHeap;
    {
        FillGuard guard(GameMemory::LuaHeap, 0);
        CHECK(!Lua_Red_realloc(0, 0, 2000));
    }
    void* pA = Lua_Red_realloc(0, 0, 32);
    void* pB = Lua_Red_realloc(0, 0, 29);
    void* pC = Lua_Red_realloc(0, 0, 30);
    REQUIRE(pA);
    REQUIRE(pB);
    REQUIRE(pC);
    CHECK(LuaMemory::LocatePool(32, pA));
    Lua_Red_Free(pA, 32);
    void* pB2 = Lua_Red_realloc(pB, 29, 40);
    REQUIRE(pB2);
    CHECK(LuaMemory::LocatePool(40, pB2));
    void* pH = Lua_Red_realloc(0, 0, 2000);
    REQUIRE(pH);
    CHECK(!LuaMemory::LocatePool(2000, pH));
    void* pH2 = Lua_Red_realloc(pH, 2000, 1500);
    REQUIRE(pH2);
    void* pH3 = Lua_Red_realloc(pH2, 1500, 2560);
    REQUIRE(pH3);
    void* pH4 = Lua_Red_realloc(pH3, 2560, 2561);
    REQUIRE(pH4);
    void* pP = Lua_Red_realloc(0, 0, 2000);
    REQUIRE(pP);
    void* pP2 = Lua_Red_realloc(pP, 2000, 32);
    REQUIRE(pP2);
    CHECK(LuaMemory::LocatePool(32, pP2));
    void* pQ = Lua_Red_realloc(0, 0, 2000);
    void* pQ2 = Lua_Red_realloc(0, 0, 1999);
    REQUIRE(pQ);
    REQUIRE(pQ2);
    Lua_Red_Free(pQ, 2000);
    Lua_Red_Free(pQ2, 1999);
    Lua_Red_Free(pC, 30);
    Lua_Red_Free(pP2, 32);
    Lua_Red_Free(pB2, 40);
    Lua_Red_Free(pH4, 2561);
    LuaMemory::NoteHistAlloc(0xFFFFFFFEu, false, 0);
    LuaMemory::NoteHistFree(0xFFFFFFFEu);
    void* pLast = Lua_Red_realloc(0, 0, 31);
    REQUIRE(pLast);
    Lua_Red_Free(pLast, 31);

    const unsigned int auRow[6] = { 8, 10, 375, 500, 640, 641 };
    const unsigned int auPeak[6] = { 3, 1, 1, 2, 1, 1 };
    const unsigned int auCalls[6] = { 5, 1, 1, 4, 1, 2 };
    for (int i = 0; i < 6; ++i)
    {
        CAPTURE(auRow[i]);
        CHECK(LuaMemory::GetHistPeak(auRow[i]) == auPeak[i]);
        CHECK(LuaMemory::GetHistCalls(auRow[i]) == auCalls[i]);
    }
    unsigned int uRows = 0;
    unsigned int uCalls = 0;
    for (unsigned int i = 0; i < 642; ++i)
    {
        if (LuaMemory::GetHistCalls(i) != 0)
        {
            ++uRows;
        }
        uCalls += LuaMemory::GetHistCalls(i);
    }
    CHECK(uRows == 6);
    CHECK(uCalls == 14);
    CHECK(LuaMemory::GetHistCalls(0) == 0);
    CHECK(LuaMemory::GetHistCalls(642) == 0);
    {
        LogCapture capture;
        LuaMemory::LogMemReportLines();
        CHECK(capture.Contains("OPENSWBF2 MEMREPORT luahist bin=8 peak=3 calls=5"));
        CHECK(capture.Contains("OPENSWBF2 MEMREPORT luahist bin=500 peak=2 calls=4"));
        CHECK(capture.Contains("OPENSWBF2 MEMREPORT luahist bin=over peak=1 calls=2"));
        CHECK(capture.Count("OPENSWBF2 MEMREPORT luahist ") == 6);
    }
    LuaMemory::Reset();
    LuaMemory::CreatePools(true);
    CHECK(LuaMemory::GetHistCalls(8) == 0);
    CHECK(LuaMemory::GetHistPeak(8) == 0);
}
#endif
