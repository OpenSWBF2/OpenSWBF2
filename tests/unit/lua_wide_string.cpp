#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <string.h>
#include <string>

#include "testutil.h"
#include "Battlefront2/Source/LuaWideString.h"
#include "LuaFL/include/lua.h"

namespace
{

struct GuardPages
{
    char* pcBase;
    unsigned int uiPage;
};

bool OpenGuard(GuardPages& kGuard)
{
    SYSTEM_INFO kInfo;
    GetSystemInfo(&kInfo);
    kGuard.uiPage = kInfo.dwPageSize;
    kGuard.pcBase = (char*)VirtualAlloc(0, kGuard.uiPage * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!kGuard.pcBase)
    {
        return false;
    }
    DWORD dwOld = 0;
    return VirtualProtect(kGuard.pcBase + kGuard.uiPage, kGuard.uiPage, PAGE_NOACCESS, &dwOld) != 0;
}

void CloseGuard(GuardPages& kGuard)
{
    VirtualFree(kGuard.pcBase, 0, MEM_RELEASE);
}

char* PlaceAtEnd(GuardPages& kGuard, const char* pcBytes, unsigned int uiCount)
{
    char* pcDst = kGuard.pcBase + kGuard.uiPage - uiCount;
    memcpy(pcDst, pcBytes, uiCount);
    return pcDst;
}

int GuardedTerminator(const char* pcData, unsigned int uiLen)
{
    __try
    {
        return LuaWideString::HasWideTerminator(pcData, uiLen) ? 1 : 0;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return -1;
    }
}

int GuardedWideLen(const unsigned short* pusData)
{
    __try
    {
        int iLen = 0;
        while (pusData[iLen] != 0)
        {
            ++iLen;
        }
        return iLen;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return -1;
    }
}

int TerminatorAtEnd(GuardPages& kGuard, const char* pcBytes, unsigned int uiLen)
{
    char acBuf[16];
    memcpy(acBuf, pcBytes, uiLen);
    acBuf[uiLen] = 0;
    return GuardedTerminator(PlaceAtEnd(kGuard, acBuf, uiLen + 1), uiLen);
}

int SlotWideLenAtEnd(GuardPages& kGuard, lua_State* L, int iIndex)
{
    const char* pcData = lua_tostring(L, iIndex);
    unsigned int uiLen = (unsigned int)lua_strlen(L, iIndex);
    char* pcDst = PlaceAtEnd(kGuard, pcData, uiLen + 1);
    return GuardedWideLen((const unsigned short*)pcDst);
}

}

TEST_CASE("lua wide terminator scan stays inside the string" * doctest::test_suite("regression"))
{
    GuardPages kGuard;
    REQUIRE(OpenGuard(kGuard));

    const unsigned short ausHi[3] = { 'h', 'i', 0 };
    char acHi[6];
    memcpy(acHi, ausHi, 6);

    CHECK(TerminatorAtEnd(kGuard, "abc", 3) == 0);
    CHECK(TerminatorAtEnd(kGuard, "abcd", 4) == 0);
    CHECK(TerminatorAtEnd(kGuard, "", 0) == 0);
    CHECK(TerminatorAtEnd(kGuard, acHi, 6) == 1);
    CHECK(TerminatorAtEnd(kGuard, "a\0\0\0bc", 6) == 1);

    CloseGuard(kGuard);
}

TEST_CASE("lua wide accessor terminates inside the allocation" * doctest::test_suite("regression"))
{
    GuardPages kGuard;
    REQUIRE(OpenGuard(kGuard));

    lua_State* L = lua_open();
    REQUIRE(L);

    CHECK(GuardedWideLen((const unsigned short*)PlaceAtEnd(kGuard, "abc", 4)) == -1);

    struct MalformedCase
    {
        const char* pcBytes;
        unsigned int uiLen;
        int iWideLen;
    };
    const MalformedCase akCases[3] = { { "abc", 3, 2 }, { "abcd", 4, 2 }, { "", 0, 0 } };
    for (int c = 0; c < 3; ++c)
    {
        lua_settop(L, 0);
        lua_pushlstring(L, akCases[c].pcBytes, akCases[c].uiLen);
        lua_pushnumber(L, 5);
        int iTop = lua_gettop(L);
        const unsigned short* pusOut = LuaWideString::Check(L, 1);
        CHECK(lua_gettop(L) == iTop);
        CHECK(pusOut == (const unsigned short*)lua_tostring(L, 1));
        CHECK(lua_tonumber(L, 2) == 5);
        CHECK(SlotWideLenAtEnd(kGuard, L, 1) == akCases[c].iWideLen);
        if (c == 0)
        {
            CHECK(pusOut[0] == (unsigned short)('a' | ('b' << 8)));
            CHECK(pusOut[1] == (unsigned short)'c');
            CHECK(pusOut[2] == 0);
        }
    }

    lua_settop(L, 0);
    lua_pushlstring(L, "abc", 3);
    lua_pushlstring(L, "abcd", 4);
    const unsigned short* pusRel = LuaWideString::Check(L, -2);
    CHECK(lua_gettop(L) == 2);
    CHECK(pusRel == (const unsigned short*)lua_tostring(L, 1));
    CHECK(SlotWideLenAtEnd(kGuard, L, 1) == 2);
    CHECK(lua_strlen(L, 2) == 4);

    const unsigned short ausHi[3] = { 'h', 'i', 0 };
    lua_settop(L, 0);
    lua_pushlstring(L, (const char*)ausHi, 6);
    const char* pcBefore = lua_tostring(L, 1);
    CHECK(LuaWideString::Check(L, 1) == (const unsigned short*)pcBefore);
    CHECK(lua_gettop(L) == 1);
    CHECK(lua_strlen(L, 1) == 6);

    lua_settop(L, 0);
    CHECK(LuaWideString::Opt(L, 1) == (const unsigned short*)0);
    CHECK(lua_gettop(L) == 0);
    lua_pushnil(L);
    CHECK(LuaWideString::Opt(L, 1) == (const unsigned short*)0);
    CHECK(lua_gettop(L) == 1);

    lua_settop(L, 0);
    lua_pushlstring(L, "abc", 3);
    const unsigned short* pusOpt = LuaWideString::Opt(L, 1);
    CHECK(lua_gettop(L) == 1);
    CHECK(pusOpt == (const unsigned short*)lua_tostring(L, 1));
    CHECK(SlotWideLenAtEnd(kGuard, L, 1) == 2);

    lua_close(L);
    CloseGuard(kGuard);
}

namespace
{

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

}

TEST_CASE("lua wide string sites route through the accessor" * doctest::test_suite("regression"))
{
    struct SiteFile
    {
        const char* pcPath;
        int iCheck;
        int iOpt;
    };
    const SiteFile akFiles[4] = {
        { "src\\Battlefront2\\Source\\LuaCallbacks.cpp", 5, 0 },
        { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", 2, 0 },
        { "src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp", 10, 6 },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Net.cpp", 1, 0 },
    };
    const char* apcBanned[6] = {
        "short*)luaL_", "short const*)luaL_", "short*>(luaL_", "short*)pc", "short const*)pc", "short*>(pc",
    };
    for (int f = 0; f < 4; ++f)
    {
        std::string sText;
        REQUIRE(ReadRepoText(akFiles[f].pcPath, sText));
        INFO(akFiles[f].pcPath);
        for (int b = 0; b < 6; ++b)
        {
            INFO(apcBanned[b]);
            CHECK(sText.find(apcBanned[b]) == std::string::npos);
        }
        CHECK(CountOf(sText, "LuaWideString::Check(") == akFiles[f].iCheck);
        CHECK(CountOf(sText, "LuaWideString::Opt(") == akFiles[f].iOpt);
    }

    std::string sSaveFsm;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\SaveFsm.cpp", sSaveFsm));
    CHECK(sSaveFsm.find("if (pusFilename && mLoadQueueCnt < MAX_LOAD_QUEUE)") != std::string::npos);
}
