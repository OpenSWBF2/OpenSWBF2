#include "third_party/doctest/doctest.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "LuaFL/include/lua.h"
#include "LuaFL/include/lauxlib.h"
#include "LuaFL/include/lualib.h"
#include "lstate.h"
#include "lobject.h"
#include "RedEngineFL/Memory/RedMemory.h"

TEST_CASE("lua object sizes are pinned" * doctest::test_suite("regression"))
{
    CHECK(sizeof(Node) == 28);
    CHECK(sizeof(Table) == 40);
    CHECK(offsetof(Node, nextadded) == 20);
    CHECK(offsetof(Node, prevadded) == 24);
    CHECK(offsetof(Table, firstadded) == 32);
    CHECK(offsetof(Table, lastadded) == 36);
    CHECK(sizeof(Udata) == 16);
    CHECK(sizeof(Proto) == 72);
    CHECK(sizeof(UpVal) == 20);
    CHECK(sizeof(lua_State) == 88);
    CHECK(sizeof(CClosure) == 24);
    CHECK(sizeof(LClosure) == 28);
    CHECK(sizeof(TString) == 16);
    CHECK(offsetof(TString, tsv.hash) == 8);
}

namespace
{

struct InsertionOrderGuard
{
    explicit InsertionOrderGuard(int iInsertion)
    {
        lua_setinsertionorder(iInsertion);
    }
    ~InsertionOrderGuard()
    {
        lua_setinsertionorder(0);
    }
};

const char* const kOrderScript =
    "local keep = {}\n"
    "local t = {}\n"
    "local keys = {}\n"
    "local fn = false\n"
    "for i = 1, 64 do\n"
    "  if PAD then\n"
    "    keep[i] = string.rep('p', 11) .. i\n"
    "    keep[i + 64] = string.rep('q', 15) .. i\n"
    "    keep[i + 128] = string.rep('r', 19) .. i\n"
    "  end\n"
    "  local k\n"
    "  if fn then\n"
    "    k = function() return i end\n"
    "  else\n"
    "    k = {}\n"
    "  end\n"
    "  fn = not fn\n"
    "  keys[i] = k\n"
    "  t[k] = i\n"
    "end\n"
    "if SWITCH then SWITCH() end\n"
    "for i = 1, 64 do\n"
    "  if t[keys[i]] ~= i then return 'miss' end\n"
    "end\n"
    "local s = ''\n"
    "for k, v in pairs(t) do\n"
    "  s = s .. v .. ','\n"
    "end\n"
    "return s\n";

int SwitchToAddress(lua_State*)
{
    lua_setinsertionorder(0);
    return 0;
}

int KeyNoop(lua_State*)
{
    return 0;
}

std::string RunScript(int iInsertion, const char* pcScript, bool bPad, bool bSwitch)
{
    InsertionOrderGuard guard(iInsertion);
    lua_State* L = lua_open();
    if (!L)
    {
        return "open failed";
    }
    luaopen_base(L);
    luaopen_string(L);
    lua_settop(L, 0);
    lua_pushboolean(L, bPad ? 1 : 0);
    lua_setglobal(L, "PAD");
    if (bSwitch)
    {
        lua_pushcfunction(L, SwitchToAddress);
        lua_setglobal(L, "SWITCH");
    }
    std::string sOut;
    if (luaL_loadbuffer(L, pcScript, strlen(pcScript), "=order") != 0 || lua_pcall(L, 0, 1, 0) != 0)
    {
        const char* pcError = lua_tostring(L, -1);
        sOut = std::string("error: ") + (pcError ? pcError : "?");
    }
    else
    {
        const char* pcResult = lua_tostring(L, -1);
        sOut = pcResult ? pcResult : "nil";
    }
    lua_close(L);
    return sOut;
}

std::string Sequence(int iFirst, int iLast, const char* pcSep, bool bTrailing)
{
    std::string sOut;
    for (int i = iFirst; i <= iLast; ++i)
    {
        char acValue[16];
        sprintf(acValue, "%d", i);
        if (!sOut.empty() && !bTrailing)
        {
            sOut += pcSep;
        }
        sOut += acValue;
        if (bTrailing)
        {
            sOut += pcSep;
        }
    }
    return sOut;
}

std::string OrderedValues()
{
    return Sequence(1, 64, ",", true);
}

enum LudPattern
{
    kLudAscending,
    kLudDescending,
    kLudScattered
};

void* LudKey(LudPattern ePattern, int i, int n)
{
    const uintptr_t uBase = 0x10000000;
    if (ePattern == kLudAscending)
    {
        return (void*)(uBase + (uintptr_t)i * 16);
    }
    if (ePattern == kLudDescending)
    {
        return (void*)(uBase + (uintptr_t)(n - 1 - i) * 16);
    }
    return (void*)(uBase + (uintptr_t)((i * 7919) % 1009) * 16);
}

void SetLud(lua_State* L, void* pKey, int iValue)
{
    lua_pushlightuserdata(L, pKey);
    lua_pushnumber(L, iValue);
    lua_rawset(L, 1);
}

void ClearLud(lua_State* L, void* pKey)
{
    lua_pushlightuserdata(L, pKey);
    lua_pushnil(L);
    lua_rawset(L, 1);
}

std::string WalkValues(lua_State* L, int iTable)
{
    std::string sOut;
    lua_pushnil(L);
    while (lua_next(L, iTable) != 0)
    {
        char acValue[16];
        sprintf(acValue, "%d", (int)lua_tonumber(L, -1));
        if (!sOut.empty())
        {
            sOut += ' ';
        }
        sOut += acValue;
        lua_pop(L, 1);
    }
    return sOut;
}

std::string LudOrder(int iInsertion, LudPattern ePattern, int n, bool bMixed)
{
    InsertionOrderGuard guard(iInsertion);
    lua_State* L = lua_open();
    if (!L)
    {
        return "open failed";
    }
    const int iLudCount = bMixed ? n - 3 : n;
    int iLud = 0;
    lua_newtable(L);
    for (int i = 0; i < n; ++i)
    {
        if (bMixed && i == 2)
        {
            lua_pushstring(L, "a");
        }
        else if (bMixed && i == 5)
        {
            lua_pushnumber(L, 7.5);
        }
        else if (bMixed && i == 7)
        {
            lua_pushstring(L, "b");
        }
        else
        {
            lua_pushlightuserdata(L, LudKey(ePattern, iLud++, iLudCount));
        }
        lua_pushnumber(L, i + 1);
        lua_rawset(L, -3);
    }
    const std::string sOut = WalkValues(L, 1);
    lua_close(L);
    return sOut;
}

void PushMixedKey(lua_State* L, int i)
{
    if (i == 10)
    {
        lua_pushboolean(L, 1);
    }
    else if (i == 20)
    {
        lua_pushboolean(L, 0);
    }
    else if (i % 5 == 0)
    {
        lua_pushlightuserdata(L, LudKey(kLudScattered, i, 0));
    }
    else if (i % 5 == 1)
    {
        char acKey[16];
        sprintf(acKey, "k%d", i);
        lua_pushstring(L, acKey);
    }
    else if (i % 5 == 2)
    {
        lua_pushnumber(L, i + 0.5);
    }
    else if (i % 5 == 3)
    {
        lua_newtable(L);
    }
    else
    {
        lua_pushcfunction(L, KeyNoop);
    }
}

void FillMixed(lua_State* L, int iCount)
{
    lua_settop(L, 0);
    lua_newtable(L);
    lua_newtable(L);
    for (int i = 1; i <= iCount; ++i)
    {
        PushMixedKey(L, i);
        lua_pushvalue(L, -1);
        lua_rawseti(L, 2, i);
        lua_pushnumber(L, i);
        lua_rawset(L, 1);
    }
}

void SetKeyAt(lua_State* L, int i, bool bNil, int iValue)
{
    lua_rawgeti(L, 2, i);
    if (bNil)
    {
        lua_pushnil(L);
    }
    else
    {
        lua_pushnumber(L, iValue);
    }
    lua_rawset(L, 1);
}

}

TEST_CASE("lua hash keys iterate in insertion order under serial order" * doctest::test_suite("regression"))
{
    InsertionOrderGuard guard(1);
    lua_State* L = lua_open();
    REQUIRE(L);
    FillMixed(L, 40);
    CHECK(WalkValues(L, 1) == Sequence(1, 40, " ", false));
    SetKeyAt(L, 7, true, 0);
    SetKeyAt(L, 23, true, 0);
    lua_pushstring(L, "n41");
    lua_pushnumber(L, 41);
    lua_rawset(L, 1);
    SetLud(L, LudKey(kLudScattered, 42, 0), 42);
    const std::string sExpected = Sequence(1, 6, " ", false) + " " + Sequence(8, 22, " ", false) + " " +
        Sequence(24, 40, " ", false) + " 41 42";
    CHECK(WalkValues(L, 1) == sExpected);
    lua_close(L);
}

TEST_CASE("lua reset keys keep their place under serial order" * doctest::test_suite("regression"))
{
    const LudPattern aePatterns[] = { kLudAscending, kLudDescending, kLudScattered };
    for (unsigned int p = 0; p < sizeof(aePatterns) / sizeof(aePatterns[0]); ++p)
    {
        CAPTURE(p);
        InsertionOrderGuard guard(1);
        lua_State* L = lua_open();
        REQUIRE(L);
        lua_newtable(L);
        for (int i = 0; i < 4; ++i)
        {
            SetLud(L, LudKey(aePatterns[p], i, 5), i + 1);
        }
        ClearLud(L, LudKey(aePatterns[p], 1, 5));
        ClearLud(L, LudKey(aePatterns[p], 2, 5));
        const Table* pTable = hvalue(L->base);
        const int iNodes = sizenode(pTable);
        SetLud(L, LudKey(aePatterns[p], 4, 5), 5);
        SetLud(L, LudKey(aePatterns[p], 2, 5), 3);
        CHECK(sizenode(pTable) == iNodes);
        CHECK(WalkValues(L, 1) == "1 3 4 5");
        lua_close(L);
    }
}

namespace
{

struct DeadSlotResult
{
    std::string sOrder;
    int iNodes;
    int iArray;
};

DeadSlotResult DeadSlotOrder(int iInsertion, uintptr_t uThird)
{
    DeadSlotResult kResult;
    kResult.iNodes = -1;
    kResult.iArray = -1;
    InsertionOrderGuard guard(iInsertion);
    lua_State* L = lua_open();
    if (!L)
    {
        kResult.sOrder = "open failed";
        return kResult;
    }
    lua_newtable(L);
    SetLud(L, (void*)0x3000, 1);
    SetLud(L, (void*)0x3020, 2);
    lua_pushnumber(L, 10);
    lua_rawseti(L, 1, 1);
    ClearLud(L, (void*)0x3000);
    SetLud(L, (void*)uThird, 3);
    const Table* pTable = hvalue(L->base);
    kResult.iNodes = sizenode(pTable);
    kResult.iArray = pTable->sizearray;
    kResult.sOrder = WalkValues(L, 1);
    lua_close(L);
    return kResult;
}

const char* const kDoubleNilScript =
    "local t = {}\n"
    "for i = 1, 40 do t[{}] = i end\n"
    "for i = 1, 10 do t['s' .. i] = i end\n"
    "local n = 0\n"
    "for k in pairs(t) do\n"
    "  t[k] = nil\n"
    "  t[k] = nil\n"
    "  n = n + 1\n"
    "end\n"
    "return n .. ',' .. tostring(next(t) == nil)\n";

const char* const kCollectDuringTraversalScript =
    "local t = {}\n"
    "for i = 1, 40 do t[{}] = i end\n"
    "for i = 1, 10 do t['s' .. i] = i end\n"
    "for i = 1, 10 do t[i + 0.5] = i end\n"
    "local n = 0\n"
    "for k in pairs(t) do\n"
    "  t[k] = nil\n"
    "  collectgarbage()\n"
    "  n = n + 1\n"
    "end\n"
    "return n .. ',' .. tostring(next(t) == nil)\n";

std::string LudReviveOrder(int iInsertion, bool bCollect)
{
    InsertionOrderGuard guard(iInsertion);
    lua_State* L = lua_open();
    if (!L)
    {
        return "open failed";
    }
    lua_setgcthreshold(L, 100000);
    lua_newtable(L);
    SetLud(L, (void*)0x5000, 1);
    SetLud(L, (void*)0x5010, 2);
    SetLud(L, (void*)0x5020, 3);
    ClearLud(L, (void*)0x5010);
    if (bCollect)
    {
        lua_setgcthreshold(L, 0);
    }
    SetLud(L, (void*)0x5010, 4);
    const std::string sOut = WalkValues(L, 1);
    lua_close(L);
    return sOut;
}

std::string StringReviveScript(bool bCollect)
{
    return std::string("collectgarbage(100000)\n"
                       "local t = {}\n"
                       "t.a = 1\n"
                       "t[string.rep('b', 3)] = 2\n"
                       "t.c = 3\n"
                       "t[string.rep('b', 3)] = nil\n") +
        (bCollect ? "collectgarbage()\n" : "") +
        "t[string.rep('b', 3)] = 4\n"
        "local s = ''\n"
        "for k, v in pairs(t) do s = s .. v .. ',' end\n"
        "return s\n";
}

}

TEST_CASE("lua dead keys do not revive after a collection under serial order" * doctest::test_suite("regression"))
{
    CHECK(LudReviveOrder(1, false) == "1 4 3");
    CHECK(LudReviveOrder(1, true) == "1 3 4");
    CHECK(LudReviveOrder(0, true) == LudReviveOrder(0, false));

    const std::string sKept = StringReviveScript(false);
    const std::string sCollected = StringReviveScript(true);
    CHECK(RunScript(1, sKept.c_str(), false, false) == "1,4,3,");
    CHECK(RunScript(1, sCollected.c_str(), false, false) == "1,3,4,");
    CHECK(RunScript(0, sCollected.c_str(), false, false) == RunScript(0, sKept.c_str(), false, false));
}

TEST_CASE("lua clearing keys with a collection during traversal visits every key" * doctest::test_suite("regression"))
{
    CHECK(RunScript(1, kCollectDuringTraversalScript, false, false) == "60,true");
    CHECK(RunScript(0, kCollectDuringTraversalScript, false, false) == "60,true");
}

TEST_CASE("lua weak key tables skip dead keys under serial order" * doctest::test_suite("regression"))
{
    InsertionOrderGuard guard(1);
    lua_State* L = lua_open();
    REQUIRE(L);
    lua_setgcthreshold(L, 100000);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "__mode");
    lua_pushstring(L, "k");
    lua_rawset(L, -3);
    lua_setmetatable(L, 1);
    SetLud(L, (void*)0x10, 1);
    SetLud(L, (void*)0x20, 2);
    ClearLud(L, (void*)0x10);
    lua_setgcthreshold(L, 0);
    lua_setgcthreshold(L, 0);
    CHECK(WalkValues(L, 1) == "2");
    lua_close(L);
}

TEST_CASE("lua dead slot reuse does not move the resize under serial order" * doctest::test_suite("regression"))
{
    const DeadSlotResult kHit = DeadSlotOrder(1, 0x3030);
    const DeadSlotResult kMiss = DeadSlotOrder(1, 0x3040);
    CHECK(kHit.sOrder == "10 2 3");
    CHECK(kMiss.sOrder == kHit.sOrder);
    CHECK(kMiss.iNodes == kHit.iNodes);
    CHECK(kMiss.iArray == kHit.iArray);
    CHECK(kHit.iArray == 1);

    const DeadSlotResult kAddressHit = DeadSlotOrder(0, 0x3030);
    const DeadSlotResult kAddressMiss = DeadSlotOrder(0, 0x3040);
    CHECK(kAddressHit.sOrder == "3 10 2");
    CHECK(kAddressHit.iArray == 0);
    CHECK(kAddressMiss.iArray == 1);
    CHECK(kAddressMiss.sOrder != kAddressHit.sOrder);
}

TEST_CASE("lua clearing a key twice during traversal visits every key" * doctest::test_suite("regression"))
{
    CHECK(RunScript(1, kDoubleNilScript, false, false) == "50,true");
    CHECK(RunScript(0, kDoubleNilScript, false, false) == "50,true");
}

TEST_CASE("lua lightuserdata order depends only on insertion sequence" * doctest::test_suite("regression"))
{
    CHECK(LudOrder(1, kLudAscending, 3, false) == "1 2 3");
    CHECK(LudOrder(1, kLudAscending, 8, false) == "1 2 3 4 5 6 7 8");
    const int aiCounts[] = { 3, 8, 20 };
    for (unsigned int c = 0; c < sizeof(aiCounts) / sizeof(aiCounts[0]); ++c)
    {
        CAPTURE(aiCounts[c]);
        const std::string sAscending = LudOrder(1, kLudAscending, aiCounts[c], false);
        CHECK(sAscending == Sequence(1, aiCounts[c], " ", false));
        CHECK(sAscending == LudOrder(1, kLudDescending, aiCounts[c], false));
        CHECK(sAscending == LudOrder(1, kLudScattered, aiCounts[c], false));
    }
    const std::string sMixed = LudOrder(1, kLudAscending, 9, true);
    CHECK(sMixed == "1 2 3 4 5 6 7 8 9");
    CHECK(sMixed == LudOrder(1, kLudDescending, 9, true));
    CHECK(sMixed == LudOrder(1, kLudScattered, 9, true));
}

TEST_CASE("lua next order ignores allocation padding under serial order" * doctest::test_suite("regression"))
{
    const std::string sPlain = RunScript(1, kOrderScript, false, false);
    const std::string sPadded = RunScript(1, kOrderScript, true, false);
    REQUIRE(sPlain.find("error") == std::string::npos);
    REQUIRE(sPlain != "miss");
    CHECK(sPlain == sPadded);
    CHECK(sPlain == OrderedValues());
}

TEST_CASE("lua next order is latched at open" * doctest::test_suite("regression"))
{
    const std::string sSerial = RunScript(1, kOrderScript, false, false);
    const std::string sSwitched = RunScript(1, kOrderScript, false, true);
    REQUIRE(sSerial.find("error") == std::string::npos);
    CHECK(sSwitched != "miss");
    CHECK(sSwitched == sSerial);
    CHECK(sSerial == OrderedValues());
}

TEST_CASE("lua next order belongs to each open state" * doctest::test_suite("regression"))
{
    lua_State* pSerial = NULL;
    lua_State* pAddress = NULL;
    {
        InsertionOrderGuard guard(1);
        pSerial = lua_open();
    }
    {
        InsertionOrderGuard guard(0);
        pAddress = lua_open();
    }
    REQUIRE(pSerial);
    REQUIRE(pAddress);
    lua_State* apStates[] = { pSerial, pAddress };
    std::string asOrder[2];
    for (int s = 0; s < 2; ++s)
    {
        lua_State* L = apStates[s];
        lua_newtable(L);
        for (int i = 0; i < 8; ++i)
        {
            SetLud(L, LudKey(kLudDescending, i, 8), i + 1);
        }
        asOrder[s] = WalkValues(L, 1);
    }
    CHECK(asOrder[0] == "1 2 3 4 5 6 7 8");
    CHECK(asOrder[1] != asOrder[0]);
    lua_close(pAddress);
    lua_close(pSerial);
}

#ifdef OPENSWBF2_TEST
TEST_CASE("lua next order is stable across perturbheap modes" * doctest::test_suite("regression"))
{
    const std::string sReference = RunScript(1, kOrderScript, false, false);
    REQUIRE(sReference.find("error") == std::string::npos);
    CHECK(sReference == OrderedValues());
    const unsigned int auModes[] = { kRedPerturbPad, kRedPerturbTop, kRedPerturbFill, kRedPerturbPool };
    const unsigned int auSeeds[] = { 1, 2 };
    for (unsigned int m = 0; m < sizeof(auModes) / sizeof(auModes[0]); ++m)
    {
        for (unsigned int s = 0; s < sizeof(auSeeds) / sizeof(auSeeds[0]); ++s)
        {
            CAPTURE(auModes[m]);
            CAPTURE(auSeeds[s]);
            RedPerturbSet(auSeeds[s], auModes[m]);
            const std::string sPerturbed = RunScript(1, kOrderScript, false, false);
            RedPerturbSet(0, 0);
            CHECK(sPerturbed == sReference);
        }
    }
}
#endif

TEST_CASE("lua next order survives deletion during traversal under serial order" * doctest::test_suite("regression"))
{
    InsertionOrderGuard guard(1);
    lua_State* L = lua_open();
    REQUIRE(L);
    FillMixed(L, 40);
    int iVisited = 0;
    lua_pushnil(L);
    while (lua_next(L, 1) != 0 && iVisited <= 40)
    {
        ++iVisited;
        lua_pop(L, 1);
        lua_pushvalue(L, -1);
        lua_pushnil(L);
        lua_rawset(L, 1);
    }
    CHECK(iVisited == 40);
    lua_settop(L, 2);
    lua_pushnil(L);
    CHECK(lua_next(L, 1) == 0);
    lua_settop(L, 2);
    for (int i = 0; i < 5; ++i)
    {
        SetLud(L, LudKey(kLudScattered, 100 + i, 0), i + 1);
    }
    CHECK(WalkValues(L, 1) == "1 2 3 4 5");
    lua_close(L);
}

TEST_CASE("lua address order keeps the slot walk" * doctest::test_suite("regression"))
{
    CHECK(LudOrder(0, kLudAscending, 8, false) != LudOrder(0, kLudDescending, 8, false));
}

namespace
{

const char* const kDumpChunk =
    "local a, b = 1.5, 'str'\n"
    "local function outer(x)\n"
    "  local c = x * 2\n"
    "  return function(y) return a + c + y .. b end\n"
    "end\n"
    "local t = { n = 3, s = 'k', [4] = false }\n"
    "return outer(t.n)\n";

int DumpToString(lua_State*, const void* p, size_t sz, void* ud)
{
    static_cast<std::string*>(ud)->append(static_cast<const char*>(p), sz);
    return 0;
}

std::string DumpChunk(int iInsertion, std::string& sRedump)
{
    InsertionOrderGuard guard(iInsertion);
    std::string sDump;
    lua_State* L = lua_open();
    if (!L)
    {
        return sDump;
    }
    if (luaL_loadbuffer(L, kDumpChunk, strlen(kDumpChunk), "=dump") == 0)
    {
        lua_dump(L, DumpToString, &sDump);
        lua_settop(L, 0);
        if (luaL_loadbuffer(L, sDump.data(), sDump.size(), "=dump") == 0)
        {
            lua_dump(L, DumpToString, &sRedump);
        }
    }
    lua_close(L);
    return sDump;
}

}

TEST_CASE("lua dump bytes are the same in both orders" * doctest::test_suite("regression"))
{
    std::string sAddressRedump;
    std::string sSerialRedump;
    const std::string sAddress = DumpChunk(0, sAddressRedump);
    const std::string sSerial = DumpChunk(1, sSerialRedump);
    REQUIRE(!sAddress.empty());
    CHECK(sAddress == sSerial);
    CHECK(sAddressRedump == sAddress);
    CHECK(sSerialRedump == sSerial);
}
