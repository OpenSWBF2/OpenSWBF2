#include "third_party/doctest/doctest.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "testutil.h"
#include "LuaFL/include/lua.h"
#include "LuaFL/include/lauxlib.h"

namespace
{

const unsigned char kRetailHeader[18] = {
    0x1B, 0x4C, 0x75, 0x61, 0x50, 0x01, 0x04, 0x04, 0x04,
    0x06, 0x08, 0x09, 0x09, 0x04, 0x3B, 0xAF, 0xEF, 0x4B
};

int WriteToString(lua_State*, const void* p, size_t sz, void* ud)
{
    static_cast<std::string*>(ud)->append(static_cast<const char*>(p), sz);
    return 0;
}

std::string DumpSample(lua_State* L)
{
    std::string sOut;
    const char* pcSource = "return 1 + 2";
    if (luaL_loadbuffer(L, pcSource, strlen(pcSource), "=t") != 0)
    {
        return sOut;
    }
    lua_dump(L, WriteToString, &sOut);
    lua_pop(L, 1);
    return sOut;
}

}

TEST_CASE("lua bytecode header matches retail" * doctest::test_suite("unit"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    std::string sDump = DumpSample(L);
    REQUIRE(sDump.size() >= 25);
    CHECK(memcmp(sDump.data(), kRetailHeader, sizeof(kRetailHeader)) == 0);
    const unsigned char acLen[4] = { 0x03, 0x00, 0x00, 0x00 };
    CHECK(memcmp(sDump.data() + 18, acLen, 4) == 0);
    CHECK(memcmp(sDump.data() + 22, "=t\0", 3) == 0);
    lua_close(L);
}

TEST_CASE("lua bytecode round trip" * doctest::test_suite("unit"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    std::string sDump = DumpSample(L);
    REQUIRE(!sDump.empty());
    REQUIRE(luaL_loadbuffer(L, sDump.data(), sDump.size(), "=t") == 0);
    REQUIRE(lua_pcall(L, 0, 1, 0) == 0);
    CHECK(lua_tonumber(L, -1) == 3);
    lua_close(L);
}

TEST_CASE("lua bytecode rejects 8-byte size_t" * doctest::test_suite("unit"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    std::string sDump = DumpSample(L);
    REQUIRE(sDump.size() > 7);
    sDump[7] = 8;
    CHECK(luaL_loadbuffer(L, sDump.data(), sDump.size(), "=t") == LUA_ERRRUN);
    const char* pcMsg = lua_tostring(L, -1);
    REQUIRE(pcMsg);
    CHECK(std::string(pcMsg).find("size_t") != std::string::npos);
    lua_close(L);
}

TEST_CASE("lua bytecode loads retail shell chunk" * doctest::test_suite("unit"))
{
    REQUIRE_GAMEDATA();
    std::string sPath = std::string(FindGameDataRoot()) + "\\data\\_lvl_pc\\shell.lvl";
    FILE* pFile = fopen(sPath.c_str(), "rb");
    REQUIRE(pFile);
    std::string sData(0x10000, '\0');
    size_t uiRead = fread(&sData[0], 1, sData.size(), pFile);
    fclose(pFile);
    sData.resize(uiRead);
    size_t uiPos = sData.find("\x1bLua");
    REQUIRE(uiPos != std::string::npos);
    REQUIRE(uiPos >= 8);
    CHECK(sData.compare(uiPos - 8, 4, "BODY") == 0);
    unsigned int uiSize = 0;
    memcpy(&uiSize, sData.data() + uiPos - 4, 4);
    REQUIRE(uiSize > 0);
    REQUIRE(uiPos + uiSize <= sData.size());
    lua_State* L = lua_open();
    REQUIRE(L);
    CHECK(luaL_loadbuffer(L, sData.data() + uiPos, uiSize, "=shell_interface") == 0);
    lua_close(L);
}
