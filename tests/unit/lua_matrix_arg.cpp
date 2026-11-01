#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"
#include "Battlefront2/Source/LuaMatrixArg.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "LuaFL/include/lua.h"

namespace
{

void CheckRejected(lua_State* L, int iIndex)
{
    int iTop = lua_gettop(L);
    CHECK(LuaMatrixArg::Get(L, iIndex) == (const PblMatrix*)0);
    CHECK(lua_gettop(L) == iTop);
}

}

TEST_CASE("lua matrix argument accepts only pushed matrices" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);

    float afPattern[16];
    for (int i = 0; i < 16; ++i)
    {
        afPattern[i] = (float)(i * 3 + 1);
    }

    void* pMatrixData = lua_newuserdata(L, sizeof(PblMatrix));
    REQUIRE(pMatrixData != (void*)0);
    memcpy(pMatrixData, afPattern, sizeof(afPattern));
    CHECK(lua_gettop(L) == 1);
    const PblMatrix* pGot = LuaMatrixArg::Get(L, 1);
    CHECK(lua_gettop(L) == 1);
    CHECK((const void*)pGot == lua_touserdata(L, 1));
    if (pGot)
    {
        float afRead[16];
        memcpy(afRead, pGot, sizeof(afRead));
        CHECK(memcmp(afRead, afPattern, sizeof(afPattern)) == 0);
    }

    lua_settop(L, 0);
    lua_pushnil(L);
    CheckRejected(L, 1);
    CheckRejected(L, 2);
    CheckRejected(L, 8);

    lua_settop(L, 0);
    lua_pushnumber(L, 42.0);
    CheckRejected(L, 1);

    lua_settop(L, 0);
    char acBytes[sizeof(PblMatrix)];
    memset(acBytes, 0x41, sizeof(acBytes));
    lua_pushlstring(L, acBytes, sizeof(acBytes));
    CheckRejected(L, 1);

    lua_settop(L, 0);
    lua_newtable(L);
    CheckRejected(L, 1);

    lua_settop(L, 0);
    PblMatrix kReal;
    memcpy(&kReal, afPattern, sizeof(afPattern));
    lua_pushlightuserdata(L, &kReal);
    CheckRejected(L, 1);

    const size_t auSizes[] = { 0, 4, sizeof(PblMatrix) - 1, sizeof(PblMatrix) + 1, 0x100 };
    for (size_t i = 0; i < sizeof(auSizes) / sizeof(auSizes[0]); ++i)
    {
        INFO(auSizes[i]);
        lua_settop(L, 0);
        void* pData = lua_newuserdata(L, auSizes[i]);
        REQUIRE(pData != (void*)0);
        if (auSizes[i])
        {
            memset(pData, 0x5A, auSizes[i]);
        }
        CheckRejected(L, 1);
    }

    const size_t auMetaSizes[] = { 4, sizeof(PblMatrix) };
    for (size_t i = 0; i < sizeof(auMetaSizes) / sizeof(auMetaSizes[0]); ++i)
    {
        INFO(auMetaSizes[i]);
        lua_settop(L, 0);
        void* pData = lua_newuserdata(L, auMetaSizes[i]);
        REQUIRE(pData != (void*)0);
        memset(pData, 0, auMetaSizes[i]);
        lua_newtable(L);
        REQUIRE(lua_setmetatable(L, 1) != 0);
        CHECK(lua_gettop(L) == 1);
        CheckRejected(L, 1);
    }

    lua_settop(L, 0);
    lua_newuserdata(L, sizeof(PblMatrix));
    lua_pushnil(L);
    CHECK(LuaMatrixArg::Get(L, -2) == (const PblMatrix*)lua_touserdata(L, 1));
    CHECK(lua_gettop(L) == 2);
    CheckRejected(L, -1);

    lua_close(L);
}

namespace
{

std::string RepoRoot()
{
    char acExe[MAX_PATH];
    DWORD uLen = GetModuleFileNameA(0, acExe, sizeof(acExe));
    REQUIRE(uLen > 0);
    REQUIRE(uLen < sizeof(acExe));
    std::string sPath(acExe, uLen);
    for (int i = 0; i < 4; ++i)
    {
        size_t uiSlash = sPath.find_last_of("\\/");
        REQUIRE(uiSlash != std::string::npos);
        sPath.erase(uiSlash);
    }
    return sPath;
}

int CountOf(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    size_t uLen = strlen(pcNeedle);
    for (size_t uPos = sText.find(pcNeedle); uPos != std::string::npos; uPos = sText.find(pcNeedle, uPos + uLen))
    {
        ++iCount;
    }
    return iCount;
}

void ListSources(const std::string& sDir, const char* pcPattern, std::vector<std::string>& vOut)
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + pcPattern).c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if (!(kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            vOut.push_back(kFind.cFileName);
        }
    } while (FindNextFileA(hFind, &kFind));
    FindClose(hFind);
}

}

TEST_CASE("lua matrix sites route through the accessor" * doctest::test_suite("regression"))
{
    const std::string sRel = "src\\Battlefront2\\Source\\";
    std::vector<std::string> vFiles;
    ListSources(RepoRoot() + "\\" + sRel, "*.cpp", vFiles);
    ListSources(RepoRoot() + "\\" + sRel, "*.h", vFiles);
    REQUIRE(vFiles.size() > 100);

    bool bSawAccessor = false;
    for (size_t i = 0; i < vFiles.size(); ++i)
    {
        INFO(vFiles[i]);
        std::string sText;
        REQUIRE(ReadRepoText((sRel + vFiles[i]).c_str(), sText));
        const bool bAccessor = _stricmp(vFiles[i].c_str(), "LuaMatrixArg.cpp") == 0;
        bSawAccessor = bSawAccessor || bAccessor;
        CHECK(CountOf(sText, "lua_touserdata") == (bAccessor ? 1 : 0));
        CHECK(CountOf(sText, "lua_isuserdata") == 0);
    }
    CHECK(bSawAccessor);

    struct SiteFile
    {
        const char* pcFile;
        int iGets;
    };
    const SiteFile akSites[] = {
        { "LuaCallbacks_Mission.cpp", 9 },
        { "LuaCallbacks.cpp", 3 },
    };
    for (size_t i = 0; i < sizeof(akSites) / sizeof(akSites[0]); ++i)
    {
        INFO(akSites[i].pcFile);
        std::string sText;
        REQUIRE(ReadRepoText((sRel + akSites[i].pcFile).c_str(), sText));
        CHECK(CountOf(sText, "LuaMatrixArg::Get(") == akSites[i].iGets);
        CHECK(CountOf(sText, "#include \"LuaMatrixArg.h\"") == 1);
        CHECK(CountOf(sText, "LUA_TUSERDATA") == 0);
    }
}
