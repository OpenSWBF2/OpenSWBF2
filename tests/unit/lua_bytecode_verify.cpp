#include "third_party/doctest/doctest.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "testutil.h"
#include "LuaFL/include/lua.h"
#include "LuaFL/include/lauxlib.h"
#include "LuaFL/include/lualib.h"
#include "lopcodes.h"
#include "lobject.h"
#include "lstate.h"

namespace
{

const unsigned char kVerifyHeader[18] = {
    0x1B, 0x4C, 0x75, 0x61, 0x50, 0x01, 0x04, 0x04, 0x04,
    0x06, 0x08, 0x09, 0x09, 0x04, 0x3B, 0xAF, 0xEF, 0x4B
};

struct FakeStr
{
    FakeStr() : bNull(true) {}
    FakeStr(const char* pcText) : s(pcText), bNull(false) {}
    std::string s;
    bool bNull;
};

struct FakeLocal
{
    FakeStr kName;
    int iStart;
    int iEnd;
};

struct FakeConst
{
    int iType;
    float fValue;
    FakeStr kText;
};

struct FakeProto
{
    FakeProto() : iLineDefined(0), iNups(0), iParams(0), iVararg(0), iMaxStack(2) {}
    FakeStr kSource;
    int iLineDefined;
    int iNups;
    int iParams;
    int iVararg;
    int iMaxStack;
    std::vector<int> aiLines;
    std::vector<FakeLocal> akLocals;
    std::vector<FakeStr> akUpNames;
    std::vector<FakeConst> akConsts;
    std::vector<FakeProto> akProtos;
    std::vector<Instruction> auiCode;
};

void AppendU8(std::string& sOut, int iValue)
{
    sOut.push_back((char)(unsigned char)iValue);
}

void AppendU32(std::string& sOut, unsigned int uiValue)
{
    sOut.append((const char*)&uiValue, 4);
}

void AppendI32(std::string& sOut, int iValue)
{
    AppendU32(sOut, (unsigned int)iValue);
}

void AppendFloat(std::string& sOut, float fValue)
{
    sOut.append((const char*)&fValue, 4);
}

void AppendStr(std::string& sOut, const FakeStr& kStr)
{
    if (kStr.bNull)
    {
        AppendU32(sOut, 0);
        return;
    }
    AppendU32(sOut, (unsigned int)kStr.s.size() + 1);
    sOut.append(kStr.s);
    sOut.push_back('\0');
}

void AppendProto(std::string& sOut, const FakeProto& kProto)
{
    AppendStr(sOut, kProto.kSource);
    AppendI32(sOut, kProto.iLineDefined);
    AppendU8(sOut, kProto.iNups);
    AppendU8(sOut, kProto.iParams);
    AppendU8(sOut, kProto.iVararg);
    AppendU8(sOut, kProto.iMaxStack);
    AppendI32(sOut, (int)kProto.aiLines.size());
    for (size_t i = 0; i < kProto.aiLines.size(); ++i)
    {
        AppendI32(sOut, kProto.aiLines[i]);
    }
    AppendI32(sOut, (int)kProto.akLocals.size());
    for (size_t i = 0; i < kProto.akLocals.size(); ++i)
    {
        AppendStr(sOut, kProto.akLocals[i].kName);
        AppendI32(sOut, kProto.akLocals[i].iStart);
        AppendI32(sOut, kProto.akLocals[i].iEnd);
    }
    AppendI32(sOut, (int)kProto.akUpNames.size());
    for (size_t i = 0; i < kProto.akUpNames.size(); ++i)
    {
        AppendStr(sOut, kProto.akUpNames[i]);
    }
    AppendI32(sOut, (int)kProto.akConsts.size());
    for (size_t i = 0; i < kProto.akConsts.size(); ++i)
    {
        const FakeConst& kConst = kProto.akConsts[i];
        AppendU8(sOut, kConst.iType);
        if (kConst.iType == LUA_TNUMBER)
        {
            AppendFloat(sOut, kConst.fValue);
        }
        else if (kConst.iType == LUA_TSTRING)
        {
            AppendStr(sOut, kConst.kText);
        }
    }
    AppendI32(sOut, (int)kProto.akProtos.size());
    for (size_t i = 0; i < kProto.akProtos.size(); ++i)
    {
        AppendProto(sOut, kProto.akProtos[i]);
    }
    AppendI32(sOut, (int)kProto.auiCode.size());
    for (size_t i = 0; i < kProto.auiCode.size(); ++i)
    {
        AppendU32(sOut, kProto.auiCode[i]);
    }
}

std::string BuildHeader()
{
    return std::string((const char*)kVerifyHeader, sizeof(kVerifyHeader));
}

std::string BuildChunk(const FakeProto& kMain)
{
    std::string sOut = BuildHeader();
    AppendProto(sOut, kMain);
    return sOut;
}

Instruction OpReturn()
{
    return CREATE_ABC(OP_RETURN, 0, 1, 0);
}

FakeProto MinimalProto()
{
    FakeProto kProto;
    kProto.kSource = FakeStr("=evil");
    kProto.auiCode.push_back(OpReturn());
    return kProto;
}

FakeProto NestedProto()
{
    FakeProto kChild;
    kChild.auiCode.push_back(OpReturn());
    FakeProto kMain = MinimalProto();
    kMain.akProtos.push_back(kChild);
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_CLOSURE, 0, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_CALL, 0, 1, 1));
    kMain.auiCode.push_back(OpReturn());
    return kMain;
}

FakeConst MakeConst(int iType, float fValue, const FakeStr& kText)
{
    FakeConst kConst;
    kConst.iType = iType;
    kConst.fValue = fValue;
    kConst.kText = kText;
    return kConst;
}

FakeLocal MakeLocal(const FakeStr& kName, int iStart, int iEnd)
{
    FakeLocal kLocal;
    kLocal.kName = kName;
    kLocal.iStart = iStart;
    kLocal.iEnd = iEnd;
    return kLocal;
}

FakeProto RichProto()
{
    FakeProto kChild;
    kChild.iNups = 1;
    kChild.akUpNames.push_back(FakeStr("u"));
    kChild.akLocals.push_back(MakeLocal(FakeStr("x"), 0, 1));
    kChild.akConsts.push_back(MakeConst(LUA_TNIL, 0.0f, FakeStr()));
    kChild.akConsts.push_back(MakeConst(LUA_TNUMBER, 2.5f, FakeStr()));
    kChild.akConsts.push_back(MakeConst(LUA_TSTRING, 0.0f, FakeStr("name")));
    kChild.aiLines.push_back(1);
    kChild.auiCode.push_back(OpReturn());
    FakeProto kMain = MinimalProto();
    kMain.akProtos.push_back(kChild);
    kMain.akConsts.push_back(MakeConst(LUA_TSTRING, 0.0f, FakeStr("k")));
    kMain.akLocals.push_back(MakeLocal(FakeStr("f"), 0, 3));
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_CLOSURE, 0, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_MOVE, 0, 1, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_CALL, 0, 1, 1));
    kMain.auiCode.push_back(OpReturn());
    return kMain;
}

int LoadAndRun(const std::string& sChunk, std::string* psMsg, int iDepth = 0)
{
    lua_State* L = lua_open();
    REQUIRE(L);
    REQUIRE(lua_checkstack(L, iDepth + 1));
    for (int i = 0; i < iDepth; ++i)
    {
        lua_pushnil(L);
    }
    int iStatus = luaL_loadbuffer(L, sChunk.data(), sChunk.size(), "=evil");
    if (iStatus == 0)
    {
        lua_setgcthreshold(L, 0);
        lua_pcall(L, 0, 0, 0);
    }
    else if (psMsg && lua_isstring(L, -1))
    {
        *psMsg = lua_tostring(L, -1);
    }
    lua_close(L);
    return iStatus;
}

}

TEST_CASE("lua chunk builder output loads" * doctest::test_suite("regression"))
{
    std::string sMsg;
    INFO(sMsg);
    CHECK(LoadAndRun(BuildChunk(MinimalProto()), &sMsg) == 0);
    CHECK(LoadAndRun(BuildChunk(NestedProto()), &sMsg) == 0);
    CHECK(LoadAndRun(BuildChunk(RichProto()), &sMsg) == 0);
}

TEST_CASE("lua loadstring refuses binary chunks" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    luaopen_base(L);
    luaopen_string(L);
    lua_settop(L, 0);
    const char* pcScript =
        "local f, e = loadstring(string.dump(function() end))\n"
        "BinNil = (f == nil)\n"
        "BinMsg = e\n"
        "local g = loadstring('return 1')\n"
        "SrcResult = g and g()\n";
    REQUIRE(luaL_loadbuffer(L, pcScript, strlen(pcScript), "=t") == 0);
    REQUIRE(lua_pcall(L, 0, 0, 0) == 0);
    lua_getglobal(L, "BinNil");
    CHECK(lua_toboolean(L, -1) != 0);
    lua_getglobal(L, "BinMsg");
    REQUIRE(lua_isstring(L, -1));
    std::string sMsg = lua_tostring(L, -1);
    INFO(sMsg);
    CHECK(sMsg.find("binary") != std::string::npos);
    lua_getglobal(L, "SrcResult");
    CHECK(lua_tonumber(L, -1) == 1);
    lua_close(L);
}

TEST_CASE("lua refused chunk logs one alert" * doctest::test_suite("regression"))
{
    std::string sChunk = BuildChunk(MinimalProto());
    sChunk.resize(sChunk.size() - 2);
    lua_State* L = lua_open();
    REQUIRE(L);
    luaopen_base(L);
    lua_settop(L, 0);
    {
        LogCapture capture;
        CHECK(lua_dobuffer(L, sChunk.data(), sChunk.size(), "=evil") != 0);
        CHECK(capture.Count("OPENSWBF2 LUA error") == 1);
        CHECK(capture.Contains("evil"));
    }
    lua_close(L);
}

namespace
{

std::string ForgedCountChunk(int iField, unsigned int uiCount, const std::string& sTail = std::string(256, '\0'))
{
    std::string sOut = BuildHeader();
    AppendStr(sOut, FakeStr("=evil"));
    AppendI32(sOut, 0);
    AppendU8(sOut, 0);
    AppendU8(sOut, 0);
    AppendU8(sOut, 0);
    AppendU8(sOut, 2);
    for (int i = 0; i < iField; ++i)
    {
        AppendI32(sOut, 0);
    }
    AppendU32(sOut, uiCount);
    sOut.append(sTail);
    return sOut;
}

std::string LocalRecords(int iCount)
{
    std::string sOut;
    for (int i = 0; i < iCount; ++i)
    {
        AppendStr(sOut, FakeStr("a"));
        AppendI32(sOut, 0);
        AppendI32(sOut, 1);
    }
    return sOut;
}

std::string ProtoRecords(int iCount)
{
    FakeProto kChild;
    kChild.auiCode.push_back(OpReturn());
    std::string sOut;
    for (int i = 0; i < iCount; ++i)
    {
        AppendProto(sOut, kChild);
    }
    return sOut;
}

std::string ReturnRecords(int iCount)
{
    std::string sOut;
    for (int i = 0; i < iCount; ++i)
    {
        AppendU32(sOut, OpReturn());
    }
    return sOut;
}

std::string ForgedStringChunk(unsigned int uiSize)
{
    std::string sOut = BuildHeader();
    AppendU32(sOut, uiSize);
    sOut.append(256, 'a');
    return sOut;
}

struct BlockReader
{
    const std::string* psChunk;
    size_t uiSplit;
    int iCall;
    int iStateCalls;
};

const char* ReadBlocks(lua_State* L, void* pData, size_t* puiSize)
{
    BlockReader* pReader = static_cast<BlockReader*>(pData);
    if (L)
    {
        ++pReader->iStateCalls;
    }
    int iCall = pReader->iCall++;
    if (iCall == 0)
    {
        *puiSize = pReader->uiSplit;
        return pReader->psChunk->data();
    }
    if (iCall == 1 && pReader->uiSplit < pReader->psChunk->size())
    {
        *puiSize = pReader->psChunk->size() - pReader->uiSplit;
        return pReader->psChunk->data() + pReader->uiSplit;
    }
    return 0;
}

int LoadInBlocks(const std::string& sChunk, size_t uiSplit, std::string* psMsg)
{
    lua_State* L = lua_open();
    REQUIRE(L);
    BlockReader kReader;
    kReader.psChunk = &sChunk;
    kReader.uiSplit = uiSplit;
    kReader.iCall = 0;
    kReader.iStateCalls = 0;
    int iStatus = lua_load(L, ReadBlocks, &kReader, "=evil");
    CHECK(kReader.iCall >= 2);
    CHECK(kReader.iStateCalls == 0);
    if (iStatus == 0)
    {
        lua_pcall(L, 0, 0, 0);
    }
    else if (psMsg && lua_isstring(L, -1))
    {
        *psMsg = lua_tostring(L, -1);
    }
    lua_close(L);
    return iStatus;
}

void CheckRefusedWith(const std::string& sChunk, const char* pcWhat, const char* pcWant)
{
    std::string sMsg;
    int iStatus = LoadAndRun(sChunk, &sMsg);
    INFO(pcWhat);
    INFO(sMsg);
    CHECK(iStatus != 0);
    CHECK(sMsg == pcWant);
}

void CheckRefusedWithBad(const std::string& sChunk, const char* pcWhat)
{
    std::string sMsg;
    int iStatus = LoadAndRun(sChunk, &sMsg);
    INFO(pcWhat);
    INFO(sMsg);
    CHECK(iStatus != 0);
    CHECK(sMsg.find("bad") != std::string::npos);
}

}

TEST_CASE("lua undump refuses every truncation" * doctest::test_suite("regression"))
{
    const FakeProto akProtos[2] = { MinimalProto(), RichProto() };
    for (int iProto = 0; iProto < 2; ++iProto)
    {
        const std::string sChunk = BuildChunk(akProtos[iProto]);
        CHECK(LoadAndRun(sChunk, 0) == 0);
        int iAccepted = 0;
        for (size_t uiLen = 1; uiLen < sChunk.size(); ++uiLen)
        {
            if (LoadAndRun(sChunk.substr(0, uiLen), 0) == 0)
            {
                ++iAccepted;
            }
        }
        CHECK(iAccepted == 0);
    }
}

TEST_CASE("lua undump refuses oversized line counts" * doctest::test_suite("regression"))
{
    const char* pcWant = "bad lines count in evil";
    CheckRefusedWith(ForgedCountChunk(0, 0x40000001u, ReturnRecords(64)), "lines wrap", pcWant);
    CheckRefusedWith(ForgedCountChunk(0, 0x01000000u), "lines huge", pcWant);
}

TEST_CASE("lua undump refuses oversized local counts" * doctest::test_suite("regression"))
{
    const char* pcWant = "bad locals count in evil";
    CheckRefusedWith(ForgedCountChunk(1, 0x15555556u, LocalRecords(16)), "locals wrap", pcWant);
    CheckRefusedWith(ForgedCountChunk(1, 0x01000000u), "locals huge", pcWant);
}

TEST_CASE("lua undump refuses oversized constant counts" * doctest::test_suite("regression"))
{
    const char* pcWant = "bad constant count in evil";
    CheckRefusedWith(ForgedCountChunk(3, 0x20000001u), "constants wrap", pcWant);
    CheckRefusedWith(ForgedCountChunk(3, 0x01000000u), "constants huge", pcWant);
}

TEST_CASE("lua undump refuses oversized function counts" * doctest::test_suite("regression"))
{
    const char* pcWant = "bad function count in evil";
    CheckRefusedWith(ForgedCountChunk(4, 0x40000001u, ProtoRecords(8)), "functions wrap", pcWant);
    CheckRefusedWith(ForgedCountChunk(4, 0x01000000u), "functions huge", pcWant);
}

TEST_CASE("lua undump refuses oversized code counts" * doctest::test_suite("regression"))
{
    const char* pcWant = "bad code count in evil";
    CheckRefusedWith(ForgedCountChunk(5, 0x40000001u, ReturnRecords(64)), "code wrap", pcWant);
    CheckRefusedWith(ForgedCountChunk(5, 0x01000000u), "code huge", pcWant);
}

TEST_CASE("lua undump refuses oversized string sizes" * doctest::test_suite("regression"))
{
    const char* pcWant = "bad string size in evil";
    CheckRefusedWith(ForgedStringChunk(0xFFFFFFFFu), "string wrap", pcWant);
    CheckRefusedWith(ForgedStringChunk(0x01000000u), "string huge", pcWant);
}

TEST_CASE("lua undump refuses multi-block binary chunks" * doctest::test_suite("regression"))
{
    const std::string sChunk = BuildChunk(RichProto());
    std::string sMsg;
    INFO(sMsg);
    CHECK(LoadInBlocks(sChunk, sChunk.size() / 2, &sMsg) != 0);
    CHECK(sMsg.find("block") != std::string::npos);
    std::string sWhole;
    CHECK(LoadInBlocks(sChunk, sChunk.size(), &sWhole) == 0);
}

namespace
{

FakeProto NestedChain(int iDepth)
{
    FakeProto kProto;
    kProto.auiCode.push_back(OpReturn());
    for (int i = 1; i < iDepth; ++i)
    {
        FakeProto kParent;
        kParent.auiCode.push_back(OpReturn());
        kParent.akProtos.push_back(kProto);
        kProto = kParent;
    }
    kProto.kSource = FakeStr("=evil");
    return kProto;
}

}

TEST_CASE("lua undump refuses deep nesting" * doctest::test_suite("regression"))
{
    std::string sMsg;
    INFO(sMsg);
    CHECK(LoadAndRun(BuildChunk(NestedChain(20)), &sMsg) == 0);
    sMsg.clear();
    CHECK(LoadAndRun(BuildChunk(NestedChain(LUA_MAXPARSERLEVEL + 1)), &sMsg) != 0);
    CHECK(sMsg.find("too deep") != std::string::npos);
}

TEST_CASE("lua undump refuses main upvalues" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.iNups = 1;
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABC(OP_GETUPVAL, 0, 0, 0));
    kMain.auiCode.push_back(OpReturn());
    CheckRefusedWithBad(BuildChunk(kMain), "main upvalues");
}

TEST_CASE("lua undump refuses null strings" * doctest::test_suite("regression"))
{
    FakeProto kConst = MinimalProto();
    kConst.akConsts.push_back(MakeConst(LUA_TSTRING, 0.0f, FakeStr()));
    CheckRefusedWithBad(BuildChunk(kConst), "null string constant");

    FakeProto kLocal = MinimalProto();
    kLocal.akLocals.push_back(MakeLocal(FakeStr(), 0, 1));
    CheckRefusedWithBad(BuildChunk(kLocal), "null local name");

    FakeProto kChild;
    kChild.iNups = 1;
    kChild.akUpNames.push_back(FakeStr());
    kChild.auiCode.push_back(OpReturn());
    FakeProto kUp = MinimalProto();
    kUp.akProtos.push_back(kChild);
    CheckRefusedWithBad(BuildChunk(kUp), "null upvalue name");

    FakeProto kSource = MinimalProto();
    kSource.kSource = FakeStr();
    CheckRefusedWithBad(BuildChunk(kSource), "null main source");
}

namespace
{

Instruction OpAsBx(int iOp, int iA, int iSbx)
{
    return CREATE_ABx(iOp, iA, iSbx + MAXARG_sBx);
}

const int kDepthNearStackEnd = 330;

std::string RefusedMessage(const FakeProto& kProto, int iDepth = 0)
{
    std::string sMsg;
    CHECK(LoadAndRun(BuildChunk(kProto), &sMsg, iDepth) != 0);
    return sMsg;
}

void CheckRefusedAt(const FakeProto& kProto, int iInstruction, int iDepth = 0)
{
    char acWant[64];
    sprintf(acWant, "bad code in evil: instruction %d", iInstruction);
    const std::string sMsg = RefusedMessage(kProto, iDepth);
    INFO(sMsg);
    CHECK(sMsg == acWant);
}

}

TEST_CASE("lua verifier names the bad instruction" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABC(OP_LOADNIL, 0, 0, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_MOVE, 5, 0, 0));
    kMain.auiCode.push_back(OpReturn());
    CheckRefusedAt(kMain, 2);
}

TEST_CASE("lua verifier refuses unknown opcodes" * doctest::test_suite("regression"))
{
    for (int iOp = NUM_OPCODES; iOp <= 63; ++iOp)
    {
        FakeProto kMain = MinimalProto();
        kMain.akConsts.push_back(MakeConst(LUA_TSTRING, 0.0f, FakeStr("k")));
        kMain.auiCode.clear();
        kMain.auiCode.push_back(CREATE_ABC(iOp, 0, 0, 0));
        kMain.auiCode.push_back(OpAsBx(OP_JMP, 0, 0));
        kMain.auiCode.push_back(OpReturn());
        INFO(iOp);
        CheckRefusedAt(kMain, 1);
    }
}

TEST_CASE("lua verifier refuses empty code" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.auiCode.clear();
    CHECK(RefusedMessage(kMain) == "bad code in evil");
}

TEST_CASE("lua verifier refuses parameters beyond the frame" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.iParams = 3;
    kMain.iMaxStack = 2;
    CHECK(RefusedMessage(kMain) == "bad code in evil");
}

TEST_CASE("lua verifier refuses concat past the frame" * doctest::test_suite("regression"))
{
    for (int iC = 3; iC < MAXSTACK; ++iC)
    {
        FakeProto kMain = MinimalProto();
        kMain.iMaxStack = 3;
        kMain.auiCode.clear();
        kMain.auiCode.push_back(CREATE_ABC(OP_CONCAT, 0, 0, iC));
        kMain.auiCode.push_back(OpReturn());
        INFO(iC);
        CheckRefusedAt(kMain, 1, kDepthNearStackEnd);
    }
}

TEST_CASE("lua verifier refuses wild tforprep jumps" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.iMaxStack = 3;
    kMain.auiCode.clear();
    kMain.auiCode.push_back(OpAsBx(OP_TFORPREP, 0, 1));
    kMain.auiCode.push_back(OpReturn());
    CheckRefusedAt(kMain, 1);
}

TEST_CASE("lua vm keeps tforloop results inside the stack" * doctest::test_suite("regression"))
{
    const int kWideC = 120;
    FakeProto kChild;
    kChild.auiCode.push_back(OpReturn());
    FakeProto kMain = MinimalProto();
    kMain.iMaxStack = kWideC + 6;
    kMain.akProtos.push_back(kChild);
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_CLOSURE, 0, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_TFORLOOP, 0, 0, kWideC));
    kMain.auiCode.push_back(OpAsBx(OP_JMP, 0, 0));
    kMain.auiCode.push_back(OpReturn());
    const std::string sChunk = BuildChunk(kMain);
    lua_State* L = lua_open();
    REQUIRE(L);
    REQUIRE(lua_checkstack(L, kDepthNearStackEnd + 1));
    for (int i = 0; i < kDepthNearStackEnd; ++i)
    {
        lua_pushnil(L);
    }
    std::string sMsg;
    int iStatus = luaL_loadbuffer(L, sChunk.data(), sChunk.size(), "=evil");
    const int iFunc = (int)(L->top - L->stack) - 1;
    if (iStatus == 0)
    {
        iStatus = lua_pcall(L, 0, 0, 0);
    }
    if (iStatus != 0 && lua_isstring(L, -1))
    {
        sMsg = lua_tostring(L, -1);
    }
    const int iStackSize = L->stacksize;
    lua_close(L);
    INFO(sMsg);
    CHECK(iStatus == 0);
    CHECK(iStackSize > iFunc + 1 + 2 * kWideC + 7);
}

namespace
{

int DumpToString(lua_State*, const void* pData, size_t uiSize, void* pUser)
{
    static_cast<std::string*>(pUser)->append(static_cast<const char*>(pData), uiSize);
    return 1;
}

std::string ForInSource(int iVars, const char* pcIter, const char* pcBody)
{
    std::string sOut = "for v1";
    for (int i = 2; i <= iVars; ++i)
    {
        char acName[16];
        sprintf(acName, ", v%d", i);
        sOut += acName;
    }
    sOut += " in ";
    sOut += pcIter;
    sOut += " do ";
    sOut += pcBody;
    sOut += " end\n";
    return sOut;
}

bool FindTforLoop(const Proto* pProto, int* piA, int* piC)
{
    for (int i = 0; i < pProto->sizecode; ++i)
    {
        if (GET_OPCODE(pProto->code[i]) == OP_TFORLOOP)
        {
            *piA = GETARG_A(pProto->code[i]);
            *piC = GETARG_C(pProto->code[i]);
            return true;
        }
    }
    return false;
}

int sTforFinalized = 0;

int CountFinalized(lua_State* L)
{
    if (lua_isuserdata(L, 1))
    {
        ++sTforFinalized;
    }
    return 0;
}

}

TEST_CASE("lua vm tforloop stays inside the stack at the extra stack limit" * doctest::test_suite("regression"))
{
    const int aiVars[2] = { EXTRA_STACK + 1, EXTRA_STACK + 2 };
    for (int iCase = 0; iCase < 2; ++iCase)
    {
        const int iVars = aiVars[iCase];
        INFO(iVars);
        lua_State* L = lua_open();
        REQUIRE(L);
        luaopen_base(L);
        lua_settop(L, 0);
        lua_register(L, "mark", CountFinalized);
        const char* pcPrelude = "G = setmetatable({}, {__call = function(self, u) mark(u) end})\n";
        REQUIRE(luaL_loadbuffer(L, pcPrelude, strlen(pcPrelude), "=prelude") == 0);
        REQUIRE(lua_pcall(L, 0, 0, 0) == 0);
        FakeProto kIter = MinimalProto();
        kIter.iVararg = 1;
        kIter.iMaxStack = 1;
        const std::string sIter = BuildChunk(kIter);
        REQUIRE(luaL_loadbuffer(L, sIter.data(), sIter.size(), "=it") == 0);
        lua_setglobal(L, "it");
        const std::string sMain = ForInSource(iVars, "it", "");
        REQUIRE(luaL_loadbuffer(L, sMain.data(), sMain.size(), "=edge") == 0);
        const int iMainIdx = lua_gettop(L);
        const Proto* pMain = clvalue(L->top - 1)->l.p;
        int iA = 0;
        int iC = 0;
        REQUIRE(FindTforLoop(pMain, &iA, &iC));
        REQUIRE(iC + 1 == iVars);
        REQUIRE(pMain->maxstacksize == iA + iC + 6);
        lua_newuserdata(L, 1);
        lua_newtable(L);
        lua_pushstring(L, "__gc");
        lua_getglobal(L, "G");
        lua_settable(L, -3);
        lua_setmetatable(L, -2);
        lua_pop(L, 1);
        REQUIRE(L->stacksize == BASIC_STACK_SIZE + EXTRA_STACK);
        const int iFunc = (int)(L->stack_last - L->stack) - 2 - pMain->maxstacksize;
        const int iTop = (int)(L->top - L->stack);
        REQUIRE(iFunc >= iTop);
        REQUIRE(lua_checkstack(L, iFunc - iTop + 1));
        for (int i = iTop; i < iFunc; ++i)
        {
            lua_pushnil(L);
        }
        lua_pushvalue(L, iMainIdx);
        REQUIRE((int)(L->top - L->stack) == iFunc + 1);
        const int iHighest = iFunc + 1 + iA + 2 * iC + 7;
        sTforFinalized = 0;
        G(L)->GCthreshold = 0;
        int iStatus = lua_pcall(L, 0, 0, 0);
        std::string sMsg = (iStatus != 0 && lua_isstring(L, -1)) ? lua_tostring(L, -1) : "";
        const int iStackSize = L->stacksize;
        lua_close(L);
        INFO(sMsg);
        CHECK(iStatus == 0);
        CHECK(sTforFinalized == 1);
        CHECK(iStackSize > iHighest);
    }
}

TEST_CASE("lua verifier accepts generic for loops with many variables" * doctest::test_suite("regression"))
{
    const int aiVars[4] = { EXTRA_STACK + 2, EXTRA_STACK + 4, 30, 90 };
    for (int iCase = 0; iCase < 4; ++iCase)
    {
        const int iVars = aiVars[iCase];
        INFO(iVars);
        std::string sSource =
            "local t = {}\n"
            "for i = 1, 3 do t[i] = i end\n"
            "local function step(s, k) if k < 3 then return k + 1, s[k + 1] end end\n"
            "local sum = 0\n";
        sSource += ForInSource(iVars, "next, t", "sum = sum + v2");
        sSource += ForInSource(iVars, "step, t, 0", "sum = sum + v2");
        sSource += "return sum\n";
        lua_State* L = lua_open();
        REQUIRE(L);
        REQUIRE(luaL_loadbuffer(L, sSource.data(), sSource.size(), "=many") == 0);
        std::string sChunk;
        lua_dump(L, DumpToString, &sChunk);
        lua_close(L);
        REQUIRE(!sChunk.empty());
        L = lua_open();
        REQUIRE(L);
        luaopen_base(L);
        lua_settop(L, 0);
        REQUIRE(lua_checkstack(L, kDepthNearStackEnd + 1));
        for (int i = 0; i < kDepthNearStackEnd; ++i)
        {
            lua_pushnil(L);
        }
        int iStatus = luaL_loadbuffer(L, sChunk.data(), sChunk.size(), "=many");
        if (iStatus == 0)
        {
            lua_setgcthreshold(L, 0);
            iStatus = lua_pcall(L, 0, 1, 0);
        }
        std::string sMsg = (iStatus != 0 && lua_isstring(L, -1)) ? lua_tostring(L, -1) : "";
        const lua_Number fSum = iStatus == 0 ? lua_tonumber(L, -1) : 0;
        lua_close(L);
        INFO(sMsg);
        CHECK(iStatus == 0);
        CHECK(fSum == 12);
    }
}

TEST_CASE("lua verifier accepts a four variable for-in loop" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    const char* pcSource = "for a, b, c, d in f do end\n";
    REQUIRE(luaL_loadbuffer(L, pcSource, strlen(pcSource), "=four") == 0);
    std::string sChunk;
    lua_dump(L, DumpToString, &sChunk);
    lua_close(L);
    REQUIRE(!sChunk.empty());
    std::string sMsg;
    INFO(sMsg);
    CHECK(LoadAndRun(sChunk, &sMsg) == 0);
}

TEST_CASE("lua verifier refuses huge table sizes" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABC(OP_NEWTABLE, 0, (29 << 3) | 1, 0));
    kMain.auiCode.push_back(OpReturn());
    CheckRefusedAt(kMain, 1);

    kMain.auiCode[0] = CREATE_ABC(OP_NEWTABLE, 0, 17 << 3, 0);
    CheckRefusedAt(kMain, 1);
}

TEST_CASE("lua verifier accepts the largest table size" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABC(OP_NEWTABLE, 0, (16 << 3) | 7, 0));
    kMain.auiCode.push_back(OpReturn());
    const std::string sChunk = BuildChunk(kMain);
    lua_State* L = lua_open();
    REQUIRE(L);
    int iStatus = luaL_loadbuffer(L, sChunk.data(), sChunk.size(), "=evil");
    std::string sMsg = (iStatus != 0 && lua_isstring(L, -1)) ? lua_tostring(L, -1) : "";
    lua_close(L);
    INFO(sMsg);
    CHECK(iStatus == 0);
}

namespace
{

int LoadAndCall(const std::string& sChunk, std::string* psMsg)
{
    lua_State* L = lua_open();
    REQUIRE(L);
    REQUIRE(luaL_loadbuffer(L, sChunk.data(), sChunk.size(), "=evil") == 0);
    int iStatus = lua_pcall(L, 0, 1, 0);
    if (psMsg && lua_isstring(L, -1))
    {
        *psMsg = lua_tostring(L, -1);
    }
    lua_close(L);
    return iStatus;
}

}

TEST_CASE("lua vm raises on self with a register key" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.iMaxStack = 4;
    kMain.akConsts.push_back(MakeConst(LUA_TNUMBER, 1.0f, FakeStr()));
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_LOADK, 2, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_SELF, 0, 1, 2));
    kMain.auiCode.push_back(OpReturn());
    std::string sMsg;
    INFO(sMsg);
    CHECK(LoadAndCall(BuildChunk(kMain), &sMsg) == LUA_ERRRUN);
    CHECK(sMsg.find("bad code") != std::string::npos);
}

TEST_CASE("lua vm raises on setlist into a non-table" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.iMaxStack = 3;
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_SETLIST, 0, 0));
    kMain.auiCode.push_back(OpReturn());
    std::string sMsg;
    INFO(sMsg);
    CHECK(LoadAndCall(BuildChunk(kMain), &sMsg) == LUA_ERRRUN);
    CHECK(sMsg.find("bad code") != std::string::npos);
}

TEST_CASE("lua verifier refuses self with a non-string constant key" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.iMaxStack = 3;
    kMain.akConsts.push_back(MakeConst(LUA_TNUMBER, 1.0f, FakeStr()));
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABC(OP_SELF, 0, 1, MAXSTACK));
    kMain.auiCode.push_back(OpReturn());
    CheckRefusedAt(kMain, 1);
}

TEST_CASE("lua verifier refuses open calls past their producer" * doctest::test_suite("regression"))
{
    FakeProto kChild;
    kChild.auiCode.push_back(OpReturn());
    FakeProto kMain = MinimalProto();
    kMain.iMaxStack = 3;
    kMain.akProtos.push_back(kChild);
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_CLOSURE, 1, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_CALL, 1, 1, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_CALL, 2, 0, 1));
    kMain.auiCode.push_back(OpReturn());
    CheckRefusedAt(kMain, 2);

    kMain.auiCode[2] = CREATE_ABC(OP_RETURN, 2, 0, 0);
    kMain.auiCode.pop_back();
    CheckRefusedAt(kMain, 2);

    kMain.auiCode[2] = CREATE_ABx(OP_SETLISTO, 2, 0);
    kMain.auiCode.push_back(OpReturn());
    CheckRefusedAt(kMain, 2);
}

namespace
{

FakeProto OpenCallIntoProducer()
{
    FakeProto kChild;
    kChild.iNups = 1;
    kChild.akUpNames.push_back(FakeStr("u"));
    kChild.akConsts.push_back(MakeConst(LUA_TSTRING, 0.0f, FakeStr("collectgarbage")));
    kChild.auiCode.push_back(CREATE_ABx(OP_GETGLOBAL, 0, 0));
    kChild.auiCode.push_back(CREATE_ABC(OP_CALL, 0, 1, 1));
    kChild.auiCode.push_back(CREATE_ABC(OP_SETUPVAL, 0, 0, 0));
    kChild.auiCode.push_back(CREATE_ABC(OP_GETUPVAL, 1, 0, 0));
    kChild.auiCode.push_back(OpReturn());
    FakeProto kMain = MinimalProto();
    kMain.akProtos.push_back(kChild);
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_CLOSURE, 0, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_MOVE, 0, 1, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_CALL, 0, 1, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_CALL, 0, 0, 1));
    kMain.auiCode.push_back(OpReturn());
    return kMain;
}

}

TEST_CASE("lua verifier refuses an open call into its producer register" * doctest::test_suite("regression"))
{
    const std::string sChunk = BuildChunk(OpenCallIntoProducer());
    lua_State* L = lua_open();
    REQUIRE(L);
    luaopen_base(L);
    lua_settop(L, 0);
    int iStatus = luaL_loadbuffer(L, sChunk.data(), sChunk.size(), "=evil");
    std::string sMsg = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
    if (iStatus == 0)
    {
        lua_setgcthreshold(L, 0);
        lua_pcall(L, 0, 0, 0);
        lua_setgcthreshold(L, 0);
    }
    lua_close(L);
    INFO(sMsg);
    CHECK(iStatus != 0);
    CHECK(sMsg == "bad code in evil: instruction 3");

    FakeProto kChild;
    kChild.auiCode.push_back(OpReturn());
    FakeProto kTail = MinimalProto();
    kTail.akProtos.push_back(kChild);
    kTail.auiCode.clear();
    kTail.auiCode.push_back(CREATE_ABx(OP_CLOSURE, 1, 0));
    kTail.auiCode.push_back(CREATE_ABC(OP_CALL, 1, 1, 0));
    kTail.auiCode.push_back(CREATE_ABC(OP_TAILCALL, 1, 0, 0));
    kTail.auiCode.push_back(CREATE_ABC(OP_RETURN, 1, 0, 0));
    CheckRefusedAt(kTail, 2);
}

TEST_CASE("lua verifier accepts open returns and lists into their producer register" * doctest::test_suite("regression"))
{
    FakeProto kChild;
    kChild.auiCode.push_back(OpReturn());
    FakeProto kMain = MinimalProto();
    kMain.akProtos.push_back(kChild);
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_CLOSURE, 0, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_CALL, 0, 1, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_RETURN, 0, 0, 0));
    std::string sMsg;
    int iStatus = LoadAndCall(BuildChunk(kMain), &sMsg);
    INFO(sMsg);
    CHECK(iStatus == 0);

    kMain.auiCode[2] = CREATE_ABx(OP_SETLISTO, 0, 0);
    kMain.auiCode.push_back(OpReturn());
    sMsg.clear();
    iStatus = LoadAndRun(BuildChunk(kMain), &sMsg);
    CHECK(iStatus == 0);
}

TEST_CASE("lua verifier refuses non-boolean loadbool" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABC(OP_LOADBOOL, 0, 2, 0));
    kMain.auiCode.push_back(OpReturn());
    CheckRefusedAt(kMain, 1);
}

TEST_CASE("lua verifier accepts boolean loadbool" * doctest::test_suite("regression"))
{
    for (int iB = 0; iB <= 1; ++iB)
    {
        FakeProto kMain = MinimalProto();
        kMain.auiCode.clear();
        kMain.auiCode.push_back(CREATE_ABC(OP_LOADBOOL, 0, iB, 0));
        kMain.auiCode.push_back(CREATE_ABC(OP_RETURN, 0, 2, 0));
        std::string sMsg;
        INFO(iB);
        INFO(sMsg);
        CHECK(LoadAndCall(BuildChunk(kMain), &sMsg) == 0);
    }
}

TEST_CASE("lua verifier accepts open calls from the parser" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    const char* pcSource =
        "local function g() return 1, 2 end\n"
        "local function f(...) return arg end\n"
        "local t = {g()}\n"
        "f(g())\n"
        "local o = {m = function(self, ...) return arg end}\n"
        "o:m(g())\n"
        "local function h() return g() end\n"
        "local function k() return f(g()) end\n"
        "h()\n"
        "k()\n"
        "return t[1], g()\n";
    REQUIRE(luaL_loadbuffer(L, pcSource, strlen(pcSource), "=open") == 0);
    std::string sChunk;
    lua_dump(L, DumpToString, &sChunk);
    lua_close(L);
    REQUIRE(!sChunk.empty());
    std::string sMsg;
    INFO(sMsg);
    CHECK(LoadAndCall(sChunk, &sMsg) == 0);
}

namespace
{

const int kClaimedLocals = 600;
int sLocalNames = 0;
int sLocalSets = 0;
int sLocalTopDrift = 0;

int ProbeLocals(lua_State* L)
{
    lua_Debug ar;
    if (!lua_getstack(L, 1, &ar))
    {
        return 0;
    }
    const int iTop = lua_gettop(L);
    for (int n = 3; n <= kClaimedLocals; ++n)
    {
        if (lua_getlocal(L, &ar, n))
        {
            ++sLocalNames;
        }
        if (lua_gettop(L) != iTop)
        {
            ++sLocalTopDrift;
            lua_settop(L, iTop);
        }
        lua_pushnil(L);
        if (lua_setlocal(L, &ar, n))
        {
            ++sLocalSets;
        }
        if (lua_gettop(L) != iTop)
        {
            ++sLocalTopDrift;
            lua_settop(L, iTop);
        }
    }
    return 0;
}

}

TEST_CASE("lua debug locals stay inside the frame" * doctest::test_suite("regression"))
{
    FakeProto kMain = MinimalProto();
    kMain.iMaxStack = 2;
    kMain.akConsts.push_back(MakeConst(LUA_TSTRING, 0.0f, FakeStr("probe")));
    for (int i = 0; i < kClaimedLocals; ++i)
    {
        kMain.akLocals.push_back(MakeLocal(FakeStr("x"), 0, 3));
    }
    kMain.auiCode.clear();
    kMain.auiCode.push_back(CREATE_ABx(OP_GETGLOBAL, 0, 0));
    kMain.auiCode.push_back(CREATE_ABC(OP_CALL, 0, 1, 1));
    kMain.auiCode.push_back(OpReturn());
    const std::string sChunk = BuildChunk(kMain);
    sLocalNames = 0;
    sLocalSets = 0;
    sLocalTopDrift = 0;
    lua_State* L = lua_open();
    REQUIRE(L);
    lua_register(L, "probe", ProbeLocals);
    REQUIRE(luaL_loadbuffer(L, sChunk.data(), sChunk.size(), "=evil") == 0);
    CHECK(lua_pcall(L, 0, 0, 0) == 0);
    lua_close(L);
    CHECK(sLocalNames == 0);
    CHECK(sLocalSets == 0);
    CHECK(sLocalTopDrift == 0);
}

namespace
{

int UpvalueHolder(lua_State*)
{
    return 0;
}

void CheckUpvalueIndexRefused(lua_State* L)
{
    const int iTop = lua_gettop(L);
    CHECK(!lua_getupvalue(L, iTop, 0));
    CHECK(!lua_getupvalue(L, iTop, -1));
    CHECK(lua_gettop(L) == iTop);
    lua_pushnil(L);
    CHECK(!lua_setupvalue(L, iTop, 0));
    lua_settop(L, iTop);
    CHECK(lua_getupvalue(L, iTop, 1) != (const char*)0);
    lua_settop(L, iTop);
}

}

TEST_CASE("lua debug upvalue index zero is refused" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    const char* pcSource = "local x = 1 return function() return x end\n";
    REQUIRE(luaL_loadbuffer(L, pcSource, strlen(pcSource), "=up") == 0);
    REQUIRE(lua_pcall(L, 0, 1, 0) == 0);
    REQUIRE(lua_isfunction(L, -1));
    CheckUpvalueIndexRefused(L);
    lua_settop(L, 0);
    lua_pushnumber(L, 7);
    lua_pushcclosure(L, UpvalueHolder, 1);
    CheckUpvalueIndexRefused(L);
    lua_settop(L, 0);
    luaopen_base(L);
    luaopen_debug(L);
    lua_settop(L, 0);
    const char* pcScript =
        "local x = 1\n"
        "local function f() return x end\n"
        "local a, b = debug.getupvalue(f, 0)\n"
        "ZeroNil = (a == nil and b == nil)\n"
        "local c = debug.setupvalue(f, 0, 5)\n"
        "SetNil = (c == nil and x == 1)\n";
    REQUIRE(luaL_loadbuffer(L, pcScript, strlen(pcScript), "=dbg") == 0);
    CHECK(lua_pcall(L, 0, 0, 0) == 0);
    lua_getglobal(L, "ZeroNil");
    CHECK(lua_toboolean(L, -1) != 0);
    lua_getglobal(L, "SetNil");
    CHECK(lua_toboolean(L, -1) != 0);
    lua_close(L);
}
