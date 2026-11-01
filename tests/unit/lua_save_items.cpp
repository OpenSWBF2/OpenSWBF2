#include "third_party/doctest/doctest.h"

#include <string.h>
#include <vector>

#include "Battlefront2/Source/LuaSaveItems.h"
#include "LuaFL/include/lua.h"

namespace
{

int SaveItemsTestCFunction(lua_State*)
{
    return 0;
}

double GetFieldNumber(lua_State* L, int iTable, const char* pcKey)
{
    lua_pushstring(L, pcKey);
    lua_rawget(L, iTable);
    double dValue = lua_type(L, -1) == LUA_TNUMBER ? lua_tonumber(L, -1) : -1.0;
    lua_pop(L, 1);
    return dValue;
}

int GetFieldType(lua_State* L, int iTable, const char* pcKey)
{
    lua_pushstring(L, pcKey);
    lua_rawget(L, iTable);
    int iType = lua_type(L, -1);
    lua_pop(L, 1);
    return iType;
}

int CountPairs(lua_State* L, int iTable)
{
    int iCount = 0;
    lua_pushnil(L);
    while (lua_next(L, iTable))
    {
        ++iCount;
        lua_pop(L, 1);
    }
    return iCount;
}

void CheckReadRejected(lua_State* L, const unsigned char* pData, unsigned int uSize)
{
    int iTop = lua_gettop(L);
    CHECK(LuaSaveItems::Read(L, pData, uSize) == 0);
    CHECK(lua_gettop(L) == iTop);
}

}

TEST_CASE("lua save items round trip and drop light userdata" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);

    lua_pushcfunction(L, SaveItemsTestCFunction);
    lua_setglobal(L, "SaveItemsGlobalFn");

    int iLocal = 5;
    lua_pushboolean(L, 1);
    lua_pushnumber(L, 2.5);
    lua_pushstring(L, "planet");
    lua_newtable(L);
    lua_pushstring(L, "res");
    lua_pushnumber(L, 40.0);
    lua_rawset(L, -3);
    lua_pushstring(L, "nested");
    lua_newtable(L);
    lua_pushstring(L, "deep");
    lua_pushnumber(L, 7.0);
    lua_rawset(L, -3);
    lua_rawset(L, -3);
    lua_pushlightuserdata(L, &iLocal);
    lua_pushnumber(L, 9.0);
    lua_rawset(L, -3);
    lua_pushstring(L, "handle");
    lua_pushlightuserdata(L, &iLocal);
    lua_rawset(L, -3);
    lua_pushstring(L, "fn");
    lua_getglobal(L, "SaveItemsGlobalFn");
    lua_rawset(L, -3);
    lua_pushlightuserdata(L, &iLocal);
    REQUIRE(lua_gettop(L) == 5);

    std::vector<unsigned char> vBuffer(0x10000);
    const unsigned int uWritten = LuaSaveItems::Write(L, &vBuffer[0], (unsigned int)vBuffer.size());
    REQUIRE(uWritten > 1);
    CHECK(lua_gettop(L) == 5);
    CHECK(vBuffer[0] == 5);

    lua_settop(L, 0);
    CHECK(LuaSaveItems::Read(L, &vBuffer[0], uWritten) == 5);
    REQUIRE(lua_gettop(L) == 5);
    CHECK(lua_type(L, 1) == LUA_TBOOLEAN);
    CHECK(lua_toboolean(L, 1) != 0);
    CHECK(lua_tonumber(L, 2) == 2.5);
    CHECK(strcmp(lua_tostring(L, 3), "planet") == 0);
    REQUIRE(lua_type(L, 4) == LUA_TTABLE);
    CHECK(GetFieldNumber(L, 4, "res") == 40.0);
    CHECK(GetFieldType(L, 4, "handle") == LUA_TNIL);
    CHECK(GetFieldType(L, 4, "fn") == LUA_TFUNCTION);
    CHECK(CountPairs(L, 4) == 3);
    lua_pushstring(L, "nested");
    lua_rawget(L, 4);
    REQUIRE(lua_type(L, -1) == LUA_TTABLE);
    CHECK(GetFieldNumber(L, lua_gettop(L), "deep") == 7.0);
    lua_pop(L, 1);
    CHECK(lua_type(L, 5) == LUA_TNIL);

    for (unsigned int uSize = 0; uSize < uWritten; ++uSize)
    {
        INFO(uSize);
        CheckReadRejected(L, &vBuffer[0], uSize);
    }

    lua_close(L);
}

TEST_CASE("lua save items write stays inside its buffer" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);

    std::vector<unsigned char> vBuffer(64, 0xCD);
    lua_pushstring(L, "0123456789012345678901234567890123456789");
    CHECK(LuaSaveItems::Write(L, &vBuffer[0], 16) == 0);
    CHECK(lua_gettop(L) == 1);
    for (size_t i = 16; i < vBuffer.size(); ++i)
    {
        INFO(i);
        CHECK(vBuffer[i] == 0xCD);
    }

    lua_settop(L, 0);
    lua_newtable(L);
    for (int i = 0; i < 64; ++i)
    {
        lua_pushnumber(L, (lua_Number)i);
        lua_pushstring(L, "padding-padding");
        lua_rawset(L, -3);
    }
    memset(&vBuffer[0], 0xCD, vBuffer.size());
    CHECK(LuaSaveItems::Write(L, &vBuffer[0], 32) == 0);
    CHECK(lua_gettop(L) == 1);
    for (size_t i = 32; i < vBuffer.size(); ++i)
    {
        INFO(i);
        CHECK(vBuffer[i] == 0xCD);
    }

    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushstring(L, "self");
    lua_pushvalue(L, 1);
    lua_rawset(L, 1);
    std::vector<unsigned char> vLarge(0x10000);
    CHECK(LuaSaveItems::Write(L, &vLarge[0], (unsigned int)vLarge.size()) == 0);
    CHECK(lua_gettop(L) == 1);

    lua_settop(L, 0);
    for (int i = 0; i < 256; ++i)
    {
        lua_pushboolean(L, 1);
    }
    CHECK(LuaSaveItems::Write(L, &vLarge[0], (unsigned int)vLarge.size()) == 0);
    CHECK(lua_gettop(L) == 256);

    lua_settop(L, 0);
    CHECK(LuaSaveItems::Write(L, &vLarge[0], (unsigned int)vLarge.size()) == 0);

    lua_close(L);
}

TEST_CASE("lua save items read rejects malformed buffers" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);
    lua_pushstring(L, "sentinel");

    const unsigned char acNoTerminator[] = { 1, LUA_TSTRING, 'a', 'b', 'c' };
    CheckReadRejected(L, acNoTerminator, sizeof(acNoTerminator));

    const unsigned char acFunctionNoTerminator[] = { 1, LUA_TFUNCTION, 'f', 'n' };
    CheckReadRejected(L, acFunctionNoTerminator, sizeof(acFunctionNoTerminator));

    const unsigned char acShortNumber[] = { 1, LUA_TNUMBER, 0, 0 };
    CheckReadRejected(L, acShortNumber, sizeof(acShortNumber));

    const unsigned char acShortPointer[] = { 1, LUA_TLIGHTUSERDATA, 1, 2 };
    CheckReadRejected(L, acShortPointer, sizeof(acShortPointer));

    const unsigned char acShortBool[] = { 1, LUA_TBOOLEAN };
    CheckReadRejected(L, acShortBool, sizeof(acShortBool));

    const unsigned char acOpenTable[] = { 1, LUA_TTABLE, LUA_TBOOLEAN, 1, LUA_TBOOLEAN, 1 };
    CheckReadRejected(L, acOpenTable, sizeof(acOpenTable));

    const unsigned char acMissingItem[] = { 3, LUA_TBOOLEAN, 1 };
    CheckReadRejected(L, acMissingItem, sizeof(acMissingItem));

    std::vector<unsigned char> vDeep(1 + 6000, (unsigned char)LUA_TTABLE);
    vDeep[0] = 1;
    vDeep.insert(vDeep.end(), 6000, 0);
    CheckReadRejected(L, &vDeep[0], (unsigned int)vDeep.size());

    const unsigned char acNone[] = { 2, 0xff, LUA_TBOOLEAN, 1 };
    const int iTop = lua_gettop(L);
    CHECK(LuaSaveItems::Read(L, acNone, sizeof(acNone)) == 2);
    CHECK(lua_gettop(L) == iTop + 2);
    CHECK(lua_type(L, -2) == LUA_TNIL);
    CHECK(lua_type(L, -1) == LUA_TBOOLEAN);
    lua_settop(L, iTop);

    float fNaN;
    const unsigned int uNaN = 0x7FC00000u;
    memcpy(&fNaN, &uNaN, sizeof(fNaN));
    std::vector<unsigned char> vKeys;
    vKeys.push_back(1);
    vKeys.push_back(LUA_TTABLE);
    vKeys.push_back(LUA_TLIGHTUSERDATA);
    vKeys.insert(vKeys.end(), sizeof(void*), 0x41);
    vKeys.push_back(LUA_TBOOLEAN);
    vKeys.push_back(1);
    vKeys.push_back(LUA_TNUMBER);
    vKeys.insert(vKeys.end(), (const unsigned char*)&fNaN, (const unsigned char*)&fNaN + sizeof(fNaN));
    vKeys.push_back(LUA_TBOOLEAN);
    vKeys.push_back(1);
    vKeys.push_back(LUA_TFUNCTION);
    const char acMissing[] = "SaveItemsMissingGlobal";
    vKeys.insert(vKeys.end(), acMissing, acMissing + sizeof(acMissing));
    vKeys.push_back(LUA_TBOOLEAN);
    vKeys.push_back(1);
    vKeys.push_back(LUA_TSTRING);
    vKeys.push_back('k');
    vKeys.push_back(0);
    vKeys.push_back(LUA_TBOOLEAN);
    vKeys.push_back(1);
    vKeys.push_back(0);
    CHECK(LuaSaveItems::Read(L, &vKeys[0], (unsigned int)vKeys.size()) == 1);
    REQUIRE(lua_gettop(L) == iTop + 1);
    REQUIRE(lua_type(L, -1) == LUA_TTABLE);
    CHECK(CountPairs(L, lua_gettop(L)) == 1);
    CHECK(GetFieldType(L, lua_gettop(L), "k") == LUA_TBOOLEAN);
    lua_settop(L, iTop);

    CHECK(lua_gettop(L) == 1);
    CHECK(strcmp(lua_tostring(L, 1), "sentinel") == 0);
    lua_close(L);
}
