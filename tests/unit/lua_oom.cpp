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
    const unsigned int auBins[4] = { 8, 56, 10, 28 };
    const unsigned int auCounts[4] = { 743, 2662, 4589, 2102 };
    for (int i = 0; i < 4; ++i)
    {
        CAPTURE(auBins[i]);
        MemoryPool* pPool = LuaMemory::LocatePool(auBins[i] * 4, 0);
        REQUIRE(pPool);
        CHECK(pPool->mCount == auCounts[i]);
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
