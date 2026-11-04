#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <malloc.h>
#include <new>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"
#include "Battlefront2/Source/LuaHandleArg.h"
#include "Battlefront2/Source/Factory.h"
#include "Battlefront2/Source/EventManager.h"
#include "PebbleFL/Common/PblPtrSet.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblRegion.h"
#include "LuaFL/include/lua.h"
#include "LuaFL/include/lauxlib.h"

namespace
{

void CheckHandleRejected(lua_State* L, int iIndex)
{
    int iTop = lua_gettop(L);
    CHECK(LuaHandleArg::Get(L, iIndex) == (const void*)0);
    CHECK(lua_gettop(L) == iTop);
}

int HandleTestCFunction(lua_State*)
{
    return 0;
}

}

TEST_CASE("lua handle argument accepts only light userdata" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);

    int iLocal = 7;
    lua_pushlightuserdata(L, &iLocal);
    CHECK(LuaHandleArg::Get(L, 1) == (const void*)&iLocal);
    CHECK(lua_gettop(L) == 1);

    lua_settop(L, 0);
    lua_pushlightuserdata(L, 0);
    CHECK(LuaHandleArg::Get(L, 1) == (const void*)0);
    CHECK(lua_gettop(L) == 1);

    lua_settop(L, 0);
    lua_pushnil(L);
    CheckHandleRejected(L, 1);
    CheckHandleRejected(L, 2);
    CheckHandleRejected(L, 8);

    lua_settop(L, 0);
    lua_pushboolean(L, 1);
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    lua_pushnumber(L, 42.0);
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    lua_pushstring(L, "handle");
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    lua_newtable(L);
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    const char acChunk[] = "return 1";
    REQUIRE(luaL_loadbuffer(L, acChunk, sizeof(acChunk) - 1, "chunk") == 0);
    CHECK(lua_type(L, 1) == LUA_TFUNCTION);
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    lua_pushcfunction(L, HandleTestCFunction);
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    lua_State* pThread = lua_newthread(L);
    REQUIRE(pThread != (lua_State*)0);
    CHECK(lua_type(L, 1) == LUA_TTHREAD);
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    void* pData = lua_newuserdata(L, 16);
    REQUIRE(pData != (void*)0);
    memset(pData, 0, 16);
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    pData = lua_newuserdata(L, 16);
    REQUIRE(pData != (void*)0);
    memset(pData, 0, 16);
    lua_newtable(L);
    REQUIRE(lua_setmetatable(L, 1) != 0);
    CheckHandleRejected(L, 1);

    lua_settop(L, 0);
    lua_pushlightuserdata(L, &iLocal);
    lua_pushnil(L);
    CHECK(LuaHandleArg::Get(L, -2) == (const void*)&iLocal);
    CHECK(lua_gettop(L) == 2);
    CheckHandleRejected(L, -1);

    lua_close(L);
}

namespace
{

PblPtrSet<16> sTestPtrSet;

const void* FakeAddress(unsigned int uIndex)
{
    return (const void*)(uintptr_t)(0x1000 + uIndex * 0x40);
}

}

TEST_CASE("pbl ptr set tracks membership by address" * doctest::test_suite("regression"))
{
    sTestPtrSet.Clear();
    CHECK(sTestPtrSet.GetCount() == 0);

    int iA = 1;
    int iB = 2;
    CHECK(sTestPtrSet.Insert(&iA));
    CHECK(sTestPtrSet.Contains(&iA));
    CHECK(!sTestPtrSet.Contains(&iB));
    CHECK(sTestPtrSet.Insert(&iA));
    CHECK(sTestPtrSet.GetCount() == 1);
    CHECK(sTestPtrSet.Remove(&iA));
    CHECK(!sTestPtrSet.Contains(&iA));
    CHECK(!sTestPtrSet.Remove(&iA));
    CHECK(sTestPtrSet.GetCount() == 0);

    CHECK(!sTestPtrSet.Insert(0));
    CHECK(!sTestPtrSet.Contains(0));
    CHECK(!sTestPtrSet.Remove(0));

    int* pFreed = new int(3);
    delete pFreed;
    CHECK(!sTestPtrSet.Contains((const void*)0x10));
    CHECK(!sTestPtrSet.Contains((const void*)(uintptr_t)0xFFFFFFF0));
    CHECK(!sTestPtrSet.Contains(pFreed));

    for (unsigned int i = 0; i < 15; ++i)
    {
        INFO(i);
        CHECK(sTestPtrSet.Insert(FakeAddress(i)));
    }
    CHECK(sTestPtrSet.GetCount() == 15);
    CHECK(!sTestPtrSet.Insert(FakeAddress(15)));
    CHECK(sTestPtrSet.GetCount() == 15);
    for (unsigned int i = 0; i < 15; ++i)
    {
        INFO(i);
        CHECK(sTestPtrSet.Contains(FakeAddress(i)));
    }
    CHECK(!sTestPtrSet.Contains(FakeAddress(15)));
    CHECK(!sTestPtrSet.Contains((const void*)0x10));

    const unsigned int auOrder[15] = { 7, 2, 11, 0, 14, 5, 9, 3, 12, 1, 8, 13, 4, 10, 6 };
    bool abAlive[15];
    for (unsigned int i = 0; i < 15; ++i)
    {
        abAlive[i] = true;
    }
    for (unsigned int uStep = 0; uStep < 15; ++uStep)
    {
        INFO(uStep);
        CHECK(sTestPtrSet.Remove(FakeAddress(auOrder[uStep])));
        abAlive[auOrder[uStep]] = false;
        CHECK(sTestPtrSet.GetCount() == 14 - uStep);
        for (unsigned int i = 0; i < 15; ++i)
        {
            INFO(i);
            CHECK(sTestPtrSet.Contains(FakeAddress(i)) == abAlive[i]);
        }
    }

    CHECK(sTestPtrSet.Insert(&iA));
    CHECK(sTestPtrSet.Insert(&iB));
    sTestPtrSet.Clear();
    CHECK(sTestPtrSet.GetCount() == 0);
    CHECK(!sTestPtrSet.Contains(&iA));
    CHECK(!sTestPtrSet.Contains(&iB));
}

namespace
{

PblHashTable<int, 8> sTestHashTable;

}

TEST_CASE("pbl hash table finds values by address" * doctest::test_suite("regression"))
{
    sTestHashTable.Clear();
    int iA = 1;
    int iB = 2;
    REQUIRE(HashStoreNew(sTestHashTable, 0x1234u, &iA));
    REQUIRE(HashStoreNew(sTestHashTable, 0x5678u, &iB));
    CHECK(sTestHashTable.HasValue(&iA));
    CHECK(sTestHashTable.HasValue(&iB));
    CHECK(!sTestHashTable.HasValue(0));
    CHECK(!sTestHashTable.HasValue((const int*)(uintptr_t)0x10));
    CHECK(!sTestHashTable.HasValue((const int*)(uintptr_t)0xFFFFFFF0));

    REQUIRE(HashRemoveOld(sTestHashTable, 0x1234u));
    static_assert(sizeof(sTestHashTable) == sizeof(int) + 16 * sizeof(unsigned int), "hash table layout");
    const unsigned int* puiSlots =
        reinterpret_cast<const unsigned int*>(reinterpret_cast<const char*>(&sTestHashTable) + sizeof(int));
    bool bStaleValue = false;
    for (int i = 0; i < 8; ++i)
    {
        bStaleValue = bStaleValue || (const int*)puiSlots[8 + i] == &iA;
    }
    CHECK(bStaleValue);
    CHECK(!sTestHashTable.HasValue(&iA));
    CHECK(sTestHashTable.HasValue(&iB));
    sTestHashTable.Clear();
}

TEST_CASE("pbl region list membership by address" * doctest::test_suite("regression"))
{
    PblRegion kRegion;
    void* pBlock = _aligned_malloc(sizeof(PblRegion), 16);
    REQUIRE(pBlock != (void*)0);
    PblRegion* pHeap = new (pBlock) PblRegion();
    CHECK(PblRegion::Contains(&kRegion));
    CHECK(PblRegion::Contains(pHeap));
    CHECK(!PblRegion::Contains((const PblRegion*)((const char*)&kRegion + 1)));
    CHECK(!PblRegion::Contains((const PblRegion*)(uintptr_t)0x10));
    CHECK(!PblRegion::Contains(0));
    const PblRegion* pDeleted = pHeap;
    pHeap->~PblRegion();
    _aligned_free(pBlock);
    CHECK(!PblRegion::Contains(pDeleted));
    CHECK(PblRegion::Contains(&kRegion));
}

namespace
{

struct TestObj
{
    int miValue;
};

struct TestDesc
{
    int miValue;
};

class TestClass : public Factory<TestObj, TestClass, TestDesc>
{
public:
    explicit TestClass(unsigned int uHash) : Factory<TestObj, TestClass, TestDesc>(uHash)
    {
    }

    virtual TestClass* Derive(unsigned int)
    {
        return 0;
    }

    virtual TestObj* Build(const TestDesc&)
    {
        return 0;
    }
};

}

TEST_CASE("factory list membership by address" * doctest::test_suite("regression"))
{
    TestClass* pA = new TestClass(0x11u);
    TestClass* pB = new TestClass(0x22u);
    TestClass* pC = new TestClass(0x33u);
    CHECK(TestClass::Contains(pA));
    CHECK(TestClass::Contains(pB));
    CHECK(TestClass::Contains(pC));
    const TestClass* pDeleted = pC;
    delete pC;
    CHECK(!TestClass::Contains(pDeleted));
    CHECK(TestClass::Contains(pA));
    CHECK(TestClass::Contains(pB));
    CHECK(!TestClass::Contains((const TestClass*)(uintptr_t)0x10));
    CHECK(!TestClass::Contains((const TestClass*)((const char*)pA + 4)));
    CHECK(!TestClass::Contains(0));
    delete pA;
    delete pB;
}

TEST_CASE("event node tree membership" * doctest::test_suite("regression"))
{
    EventManager::Node kRoot;
    EventManager::Node kFilter;
    EventManager::Node kCallback;
    EventManager::Node kSecond;
    EventManager::Node kDetached;

    kRoot.mChild = &kFilter;
    kFilter.mParent = &kRoot;
    kFilter.mChild = &kCallback;
    kCallback.mParent = &kFilter;
    kFilter.mSibling = &kSecond;
    kSecond.mParent = &kRoot;

    CHECK(kRoot.Contains(&kFilter));
    CHECK(kRoot.Contains(&kCallback));
    CHECK(kRoot.Contains(&kSecond));
    CHECK(!kRoot.Contains(&kRoot));
    CHECK(!kRoot.Contains(&kDetached));
    CHECK(!kRoot.Contains((const EventManager::Node*)(uintptr_t)0x10));
    CHECK(!kRoot.Contains(0));
    CHECK(kFilter.mChild != (EventManager::Node*)0);
    CHECK(kCallback.mChild == (EventManager::Node*)0);
    CHECK(kSecond.mChild == (EventManager::Node*)0);

    kRoot.mChild = 0;
    kFilter.mChild = 0;
    kFilter.mSibling = 0;
}

namespace
{

std::string HandleRepoRoot()
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

int HandleCountOf(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    size_t uLen = strlen(pcNeedle);
    for (size_t uPos = sText.find(pcNeedle); uPos != std::string::npos; uPos = sText.find(pcNeedle, uPos + uLen))
    {
        ++iCount;
    }
    return iCount;
}

void HandleListSources(const std::string& sDir, const char* pcPattern, std::vector<std::string>& vOut)
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

void SplitLines(const std::string& sText, std::vector<std::string>& vOut)
{
    size_t uStart = 0;
    while (uStart <= sText.size())
    {
        size_t uEnd = sText.find('\n', uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uStart, uEnd - uStart);
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        size_t uFirst = sLine.find_first_not_of(" \t");
        vOut.push_back(uFirst == std::string::npos ? std::string() : sLine.substr(uFirst));
        uStart = uEnd + 1;
    }
}

int CountLine(const std::string& sText, const char* pcLine)
{
    std::vector<std::string> vLines;
    SplitLines(sText, vLines);
    int iCount = 0;
    for (size_t i = 0; i < vLines.size(); ++i)
    {
        if (vLines[i] == pcLine)
        {
            ++iCount;
        }
    }
    return iCount;
}

std::string ReadSource(const char* pcRel)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRel, sText));
    return sText;
}

}

TEST_CASE("lua handle sites route through validation" * doctest::test_suite("regression"))
{
    const std::string sRel = "src\\Battlefront2\\Source\\";
    std::vector<std::string> vFiles;
    HandleListSources(HandleRepoRoot() + "\\" + sRel, "*.cpp", vFiles);
    HandleListSources(HandleRepoRoot() + "\\" + sRel, "*.h", vFiles);
    REQUIRE(vFiles.size() > 100);

    const char* const apcCompareOnly[] = {
        "return RegionMatches((RedRegion*)lua_topointer(LuaHelper::mState, iIndex));",
        "return ObjectMatches<char>((GameObject*)lua_topointer(LuaHelper::mState, 2));",
        "return ObjectMatches<DamageOwner>((GameObject*)lua_topointer(LuaHelper::mState, 2));",
        "return ObjectMatches<int>((GameObject*)lua_topointer(LuaHelper::mState, 2));",
        "return ObjectMatches<float>((GameObject*)lua_topointer(LuaHelper::mState, 2));",
    };
    const int iCompareOnly = (int)(sizeof(apcCompareOnly) / sizeof(apcCompareOnly[0]));
    int iUpvalue = 0;
    int iAccessor = 0;
    int iSaveItems = 0;
    int aiCompareOnly[5] = { 0, 0, 0, 0, 0 };
    for (size_t i = 0; i < vFiles.size(); ++i)
    {
        std::string sText;
        REQUIRE(ReadRepoText((sRel + vFiles[i]).c_str(), sText));
        std::vector<std::string> vLines;
        SplitLines(sText, vLines);
        const bool bAccessor = _stricmp(vFiles[i].c_str(), "LuaHandleArg.cpp") == 0;
        const bool bSaveItems = _stricmp(vFiles[i].c_str(), "LuaSaveItems.cpp") == 0;
        for (size_t j = 0; j < vLines.size(); ++j)
        {
            if (vLines[j].find("lua_topointer(") == std::string::npos)
            {
                continue;
            }
            if (vLines[j].find("lua_upvalueindex(1)") != std::string::npos)
            {
                ++iUpvalue;
                continue;
            }
            if (bAccessor)
            {
                ++iAccessor;
                continue;
            }
            if (bSaveItems)
            {
                ++iSaveItems;
                continue;
            }
            bool bCompare = false;
            for (int k = 0; k < iCompareOnly; ++k)
            {
                if (vLines[j] == apcCompareOnly[k])
                {
                    ++aiCompareOnly[k];
                    bCompare = true;
                }
            }
            INFO(vFiles[i]);
            INFO(vLines[j]);
            CHECK(bCompare);
        }
    }
    CHECK(iUpvalue == 24);
    CHECK(iAccessor == 1);
    CHECK(iSaveItems == 3);
    for (int k = 0; k < iCompareOnly; ++k)
    {
        INFO(apcCompareOnly[k]);
        CHECK(aiCompareOnly[k] == 1);
    }

    struct GetFile
    {
        const char* pcFile;
        int iGets;
    };
    const GetFile akGets[] = {
        { "LuaCallbacks_Mission.cpp", 6 },
        { "LuaCallbacks_IFace.cpp", 2 },
        { "ActiveRegion.cpp", 1 },
        { "EventManager_Timer.cpp", 2 },
        { "EventManager_Character.cpp", 1 },
        { "EventManager_CommandPost.cpp", 1 },
        { "EventManager_Flag.cpp", 1 },
        { "EventManager_LuaUser.cpp", 1 },
        { "EventManager_Object.cpp", 1 },
        { "EventManager_Region.cpp", 1 },
        { "EventManager_Team.cpp", 1 },
    };
    for (size_t i = 0; i < sizeof(akGets) / sizeof(akGets[0]); ++i)
    {
        INFO(akGets[i].pcFile);
        const std::string sText = ReadSource((sRel + akGets[i].pcFile).c_str());
        CHECK(HandleCountOf(sText, "LuaHandleArg::Get(") == akGets[i].iGets);
        CHECK(HandleCountOf(sText, "#include \"LuaHandleArg.h\"") == 1);
    }

    struct ExactLine
    {
        const char* pcFile;
        const char* pcLine;
    };
    const ExactLine akLines[] = {
        { "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", "if (!EntityEx::sLive.Contains(pEntity))" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", "if (!T::Contains(pClass))" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", "return Timer::sMap.HasValue(pTimer) ? pTimer : 0;" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", "return PblRegion::Contains(pRegion) ? (void*)pRegion : 0;" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", "if (pManager->mGoals[i] == pGoal)" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", "target.mGameObj = iArgs >= 5 && lua_type(LuaHelper::mState, 5) == LUA_TLIGHTUSERDATA ? _GetEntity<GameObject>(L, 5) : 0;" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", "if (!FLEffectObject::sLuaHandles.Contains(pEffect))" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", "(void)ensure(FLEffectObject::sLuaHandles.Insert(pEffect));" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "if (!RedInterfaceElement::sLuaHandles.Contains(pElement))" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "if (pAttachedElement && pAttachedElement->IsModel())" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "if (!RedModel::_HashTable.HasValue(pRedModel))" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "(void)ensure(RedInterfaceElement::sLuaHandles.Insert(pCurCreateObject));" },
        { "src\\Battlefront2\\Source\\ActiveRegion.cpp", "return PblRegion::Contains(pRegion) ? pRegion : 0;" },
        { "src\\Battlefront2\\Source\\EventManager_Timer.cpp", "if (!Timer::sMap.HasValue(pTimer))" },
        { "src\\Battlefront2\\Source\\EventManager_Character.cpp", "if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)" },
        { "src\\Battlefront2\\Source\\EventManager_CommandPost.cpp", "if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)" },
        { "src\\Battlefront2\\Source\\EventManager_Flag.cpp", "if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)" },
        { "src\\Battlefront2\\Source\\EventManager_LuaUser.cpp", "if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)" },
        { "src\\Battlefront2\\Source\\EventManager_Object.cpp", "if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)" },
        { "src\\Battlefront2\\Source\\EventManager_Region.cpp", "if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)" },
        { "src\\Battlefront2\\Source\\EventManager_Team.cpp", "if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)" },
        { "src\\Battlefront2\\Source\\EventManager_Timer.cpp", "if (pEvent->mRoot.Contains(pCallback) && !pCallback->mChild)" },
        { "src\\Battlefront2\\Source\\Entity.h", "EntityEx(const EntityEx&) = delete;" },
        { "src\\Battlefront2\\Source\\Entity.h", "static PblPtrSet<32768> sLive;" },
        { "src\\Battlefront2\\Source\\Entity.cpp", "(void)ensure(sLive.Insert(this));" },
        { "src\\Battlefront2\\Source\\Entity.cpp", "sLive.Remove(this);" },
        { "src\\Battlefront2\\Source\\GameState.cpp", "EntityEx::sLive.Clear();" },
        { "src\\Battlefront2\\Source\\FLEffect.cpp", "sLuaHandles.Remove(this);" },
        { "src\\Battlefront2\\Source\\FLEffect.cpp", "FLEffectObject::sLuaHandles.Clear();" },
        { "src\\RedEngineFL\\Interface\\RedInterfaceElement.cpp", "sLuaHandles.Remove(this);" },
        { "src\\RedEngineFL\\Interface\\RedInterfaceElement.cpp", "sLuaHandles.Clear();" },
        { "src\\Battlefront2\\Source\\LuaSaveItems.cpp", "pRead += sizeof(void*);" },
        { "src\\Battlefront2\\Source\\LuaSaveItems.cpp", "if (lua_isnil(L, -2))" },
    };
    for (size_t i = 0; i < sizeof(akLines) / sizeof(akLines[0]); ++i)
    {
        INFO(akLines[i].pcFile);
        INFO(akLines[i].pcLine);
        CHECK(CountLine(ReadSource(akLines[i].pcFile), akLines[i].pcLine) == 1);
    }

    const std::string sMission = ReadSource("src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp");
    CHECK(HandleCountOf(sMission, "GetEffect(") == 8);
    const std::string sIFace = ReadSource("src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp");
    CHECK(HandleCountOf(sIFace, "Util_GetLuaElement(") == 8);

    const size_t uDelete = sMission.find("int DeleteAIGoal(lua_State* L)");
    REQUIRE(uDelete != std::string::npos);
    size_t uDeleteEnd = sMission.find("// FUNCTION:", uDelete);
    if (uDeleteEnd == std::string::npos)
    {
        uDeleteEnd = sMission.size();
    }
    const std::string sDelete = sMission.substr(uDelete, uDeleteEnd - uDelete);
    const size_t uCheck = sDelete.find("pManager->mGoals[i] == pGoal");
    const size_t uUnAssign = sDelete.find("UnAssignAll");
    REQUIRE(uCheck != std::string::npos);
    REQUIRE(uUnAssign != std::string::npos);
    CHECK(uCheck < uUnAssign);

    const std::string sLoadSave = ReadSource("src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp");
    CHECK(HandleCountOf(sLoadSave, "lua_pushlightuserdata") == 0);
    CHECK(HandleCountOf(sLoadSave, "LuaSaveItems::Write(") == 1);
    CHECK(HandleCountOf(sLoadSave, "LuaSaveItems::Read(") == 1);
    const std::string sSaveItems = ReadSource("src\\Battlefront2\\Source\\LuaSaveItems.cpp");
    CHECK(HandleCountOf(sSaveItems, "lua_pushlightuserdata") == 0);
}
