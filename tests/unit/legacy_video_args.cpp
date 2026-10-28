#include "third_party/doctest/doctest.h"

#include "testutil.h"
#include "Battlefront2/Source/LuaVideoArgs.h"
#include "LuaFL/include/lua.h"

namespace
{

void PushCount(lua_State* L, int iCount)
{
    for (int i = 0; i < iCount; ++i)
    {
        lua_pushnumber(L, (lua_Number)(i + 1));
    }
}

void CheckRejected(lua_State* L, int iCount)
{
    lua_settop(L, 0);
    PushCount(L, iCount);
    float afOut[LuaVideoArgs::kLegacyVideoSetArgs];
    for (int i = 0; i < LuaVideoArgs::kLegacyVideoSetArgs; ++i)
    {
        afOut[i] = -7.0f;
    }
    CHECK_FALSE(LuaVideoArgs::ReadLegacyVideoSet(L, afOut));
    CHECK(lua_gettop(L) == iCount);
    for (int i = 0; i < LuaVideoArgs::kLegacyVideoSetArgs; ++i)
    {
        CHECK(afOut[i] == -7.0f);
    }
}

}

TEST_CASE("legacy video args arity" * doctest::test_suite("unit"))
{
    CHECK(LuaVideoArgs::kLegacyVideoSetArgs == 21);
    CHECK(LuaVideoArgs::kLegacyVideoGetValues == 19);

    lua_State* L = lua_open();
    REQUIRE(L);

    PushCount(L, 21);
    float afOut[LuaVideoArgs::kLegacyVideoSetArgs];
    for (int i = 0; i < LuaVideoArgs::kLegacyVideoSetArgs; ++i)
    {
        afOut[i] = -7.0f;
    }
    CHECK(LuaVideoArgs::ReadLegacyVideoSet(L, afOut));
    for (int i = 0; i < LuaVideoArgs::kLegacyVideoSetArgs; ++i)
    {
        CHECK(afOut[i] == (float)(i + 1));
    }

    CheckRejected(L, 20);
    CheckRejected(L, 22);
    CheckRejected(L, 0);

    lua_settop(L, 0);
    lua_pushnumber(L, 99);
    float afIn[LuaVideoArgs::kLegacyVideoGetValues];
    for (int i = 0; i < LuaVideoArgs::kLegacyVideoGetValues; ++i)
    {
        afIn[i] = (float)i * 0.5f + 1.0f;
    }
    CHECK(LuaVideoArgs::PushLegacyVideoGet(L, afIn) == 19);
    CHECK(lua_gettop(L) == 20);
    for (int i = 0; i < LuaVideoArgs::kLegacyVideoGetValues; ++i)
    {
        CHECK((float)lua_tonumber(L, i + 2) == afIn[i]);
    }

    lua_close(L);
}
