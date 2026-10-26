#include "third_party/doctest/doctest.h"

#include "testutil.h"
#include "nettest.h"
#include "Battlefront2/Source/LuaMemory.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "LuaFL/include/lua.h"

extern "C" unsigned int gFakeHeapBudget;

namespace
{

struct BudgetGuard
{
    BudgetGuard(unsigned int uiBudget) { gFakeHeapBudget = uiBudget; }
    ~BudgetGuard() { gFakeHeapBudget = 0xFFFFFFFFu; }
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

int RaiseWithHeap(lua_State* L)
{
    RedSetCurrentHeap(7);
    lua_pushstring(L, "boom");
    lua_error(L);
    return 0;
}

}

TEST_CASE("lua oom is raised as LUA_ERRMEM" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    {
        LogCapture capture;
        BudgetGuard guard(0x100000);
        CHECK(lua_cpcall(L, AllocHuge, 0) == LUA_ERRMEM);
        CHECK(capture.Contains("Lua out of memory"));
    }
    CHECK(lua_cpcall(L, AllocSmall, 0) == 0);
    lua_close(L);
}

TEST_CASE("lua failed shrink keeps the old block" * doctest::test_suite("regression"))
{
    void* pBlock = Lua_Red_realloc(0, 0, 100);
    REQUIRE(pBlock);
    {
        BudgetGuard guard(0);
        CHECK(!Lua_Red_realloc(pBlock, 100, 50));
    }
    Lua_Red_Free(pBlock, 100);
}

TEST_CASE("lua error restores the current heap" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    RedSetCurrentHeap(2);
    CHECK(lua_cpcall(L, RaiseWithHeap, 0) == LUA_ERRRUN);
    CHECK(__RedCurrHeap == 2);
    lua_close(L);
}

TEST_CASE("lua panic handler exits with code 4" * doctest::test_suite("regression"))
{
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
