#include "third_party/doctest/doctest.h"

#include <limits.h>
#include <string.h>

#include "testutil.h"
#include "Battlefront2/Source/GameMemory.h"
#include "Battlefront2/Source/LuaMemoryArgs.h"
#include "LuaFL/include/lua.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Memory/RedScriptLimits.h"
#include "RedEngineFL/Util/RedLog.h"

using namespace RedScriptLimits;

namespace
{

bool IsPowerOfTwo(unsigned int uValue)
{
    return uValue != 0 && (uValue & (uValue - 1)) == 0;
}

}

TEST_CASE("script clamp passes retail values unchanged" * doctest::test_suite("regression"))
{
    LogCapture capture;
    CHECK(ClampScriptSize(kSetupTempHeap, 0, 2 * 1024 * 1024, 1) == 2097152);
    CHECK(ClampScriptSize(kSetMemoryPoolSize, "SoldierAnimation", 1176, 48) == 1176);
    CHECK(capture.Count("clamped") == 0);
    CHECK(capture.Count("refused") == 0);
}

TEST_CASE("script clamp bounds negative and huge values" * doctest::test_suite("regression"))
{
    LogCapture capture;
    CHECK(ClampScriptSize(kSetHeapCap, "t1neg", INT_MIN, 16) == 0);
    CHECK(capture.Count("SetHeapCap(t1neg): script value -2147483648 clamped to 0") == 1);
    CHECK(ClampScriptSize(kSetMemoryPoolSize, "t1minus", -0x100000000LL, 16) == 0);
    CHECK(capture.Count("SetMemoryPoolSize(t1minus): script value -4294967296 clamped to 0") == 1);
    CHECK(ClampScriptSize(kSetMemoryPoolSize, "t1huge", 0x7FFFFFFF, 16) == kPoolBytesMax / 16);
    CHECK(capture.Count("SetMemoryPoolSize(t1huge): script value 2147483647 clamped to 1048576") == 1);
    CHECK(ClampScriptSize(kSetupTempHeap, "t1tmin", 0, 1) == kTempHeapMin);
    CHECK(capture.Count("SetupTempHeap(t1tmin): script value 0 clamped to 4096") == 1);
    CHECK(ClampScriptSize(kSetupTempHeap, "t1tmax", 0x7FFFFFFF, 1) == kTempHeapMax);
    CHECK(capture.Count("SetupTempHeap(t1tmax): script value 2147483647 clamped to 8388608") == 1);
    CHECK(capture.Count("clamped") == 5);
}

TEST_CASE("script clamp keeps the retail unsigned wrap for pool counts" * doctest::test_suite("regression"))
{
    LogCapture capture;
    CHECK(ClampScriptSize(kSetMemoryPoolSize, "t1wrap", -1, 16) == 0xFFFFFFFFu);
    CHECK(ClampScriptSize(kSetMemoryPoolSize, "t1wrap", INT_MIN, 16) == 0x80000000u);
    CHECK(ClampScriptSize(kAddWalkerType, "t1wrap", -2, 16) == 0xFFFFFFFEu);
    CHECK(capture.Count("t1wrap") == 0);
    CHECK(ClampScriptSize(kSetupTempHeap, "t1nowrap", -1, 1) == kTempHeapMin);
    CHECK(ClampScriptSize(kStealArtistHeap, "t1nowrap", -1, 1) == 0);
    CHECK(capture.Count("t1nowrap") == 2);
}

TEST_CASE("script clamp refuses outside range when configured" * doctest::test_suite("regression"))
{
    LogCapture capture;
    CHECK(ClampScriptSize(kStealArtistHeap, "t1steal", 0, 1) == 0);
    CHECK(capture.Count("StealArtistHeap(t1steal): script value 0 refused") == 1);
    CHECK(ClampScriptSize(kStealArtistHeap, "t1steal", 16, 1) == 0);
    CHECK(capture.Count("StealArtistHeap(t1steal): script value 16 refused") == 1);
    CHECK(ClampScriptSize(kStealArtistHeap, "t1steal", -64, 1) == 0);
    CHECK(capture.Count("StealArtistHeap(t1steal): script value -64 refused") == 1);
    CHECK(ClampScriptSize(kStealArtistHeap, "t1steal", 0x4000, 1) == 0);
    CHECK(capture.Count("StealArtistHeap(t1steal): script value 16384 refused") == 1);
    CHECK(ClampScriptSize(kStealArtistHeap, "t1steal", 64, 1) == 64);
    CHECK(capture.Count("refused") == 4);
}

TEST_CASE("script ceilings respect the runtime heap" * doctest::test_suite("regression"))
{
    const unsigned int uRuntime = 0x4000000 - GameMemory::kApplicationHeapSize - GameMemory::kGamespyHeapSize -
                                  GameMemory::kLuaHeapSize - GameMemory::kArtistHeapSize;
    CHECK(IsPowerOfTwo(kPoolBytesMax));
    CHECK(IsPowerOfTwo(kTempHeapMax));
    CHECK(kPoolBytesMax < uRuntime);
    CHECK(kTempHeapMax < uRuntime);
    CHECK(kTempHeapMin < kTempHeapMax);
    CHECK(kArtistStealMax == GameMemory::kArtistHeapSize);
}

namespace
{

alignas(16) char sacScriptRuntime[0x10000];
alignas(16) char sacScriptSmall[0x2000];
alignas(16) char sacScriptLoad[0x1000];
alignas(16) char sacScriptSteal[0x4000];
alignas(16) char sacScriptFill[16][0x100];

class VerboseMemoryLog
{
public:
    VerboseMemoryLog() { RedLog::ParseCmds("LogMemory Verbose"); }
    ~VerboseMemoryLog() { RedLog::ParseCmds("LogMemory Inform"); }
};

void CallSetPoolSize(lua_State* L, const char* pcName, float fCount, MemoryPool* pWeapon = 0,
                     MemoryPool* pAmmo = 0, MemoryPool* pEnergy = 0)
{
    lua_settop(L, 0);
    lua_pushstring(L, pcName);
    lua_pushnumber(L, fCount);
    LuaMemoryArgs::SetMemoryPoolSize(L, pWeapon, pAmmo, pEnergy);
    lua_settop(L, 0);
}

void CallSetPoolSize3(lua_State* L, const char* pcName, float fCount, float fOverride)
{
    lua_settop(L, 0);
    lua_pushstring(L, pcName);
    lua_pushnumber(L, fCount);
    lua_pushnumber(L, fOverride);
    LuaMemoryArgs::SetMemoryPoolSize(L, 0, 0, 0);
    lua_settop(L, 0);
}

void CallSetupTempHeap(lua_State* L, float fSize, int iRuntime, int& riTemp)
{
    lua_settop(L, 0);
    lua_pushnumber(L, fSize);
    LuaMemoryArgs::SetupTempHeap(L, iRuntime, riTemp);
    lua_settop(L, 0);
}

void CallStealArtistHeap(lua_State* L, float fSize, int iArtist)
{
    lua_settop(L, 0);
    lua_pushnumber(L, fSize);
    LuaMemoryArgs::StealArtistHeap(L, iArtist);
    lua_settop(L, 0);
}

void CallAddWalkerType(lua_State* L, float fLegPairs, float fCount, MemoryPool* pWalker, MemoryPool* pLegPair,
                       MemoryPool* pBlend, MemoryPool* pBlendUnit, MemoryPool* pDroideka)
{
    lua_settop(L, 0);
    lua_pushnumber(L, fLegPairs);
    lua_pushnumber(L, fCount);
    LuaMemoryArgs::AddWalkerType(L, pWalker, pLegPair, pBlend, pBlendUnit, pDroideka);
    lua_settop(L, 0);
}

lua_State* OpenScriptState()
{
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);
    return L;
}

int CreateScriptRuntime(char* pcStart, unsigned int uSize)
{
    const int iHeap = RedCreateHeap(pcStart, pcStart + uSize, 0x10, "T1Runtime");
    REQUIRE(iHeap > 0);
    return iHeap;
}

int NextFreeHeapSlot()
{
    const int iProbe = RedCreateHeap(sacScriptLoad, sacScriptLoad + sizeof(sacScriptLoad), 0x10, "T1Probe");
    if (iProbe >= 0)
    {
        RedDestroyHeap(iProbe);
    }
    return iProbe;
}

}

TEST_CASE("set memory pool size clamps script count" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    MemoryPool kPool("t1clamp", 16);
    lua_State* L = OpenScriptState();
    {
        LogCapture capture;
        CallSetPoolSize(L, "t1clamp", 2147483647.0f);
        CHECK(kPool.GetCount() == kPoolBytesMax / 16);
        CHECK(capture.Count("clamped") == 1);
    }
    {
        LogCapture capture;
        CallSetPoolSize(L, "t1clamp", -5.0f);
        CHECK(kPool.GetCount() == 0xFFFFFFFBu);
        CHECK(capture.Count("clamped") == 0);
        CHECK(capture.Count("label=t1clamp requested=-5 applied=4294967291 pool=t1clamp") == 1);
    }
    lua_close(L);
}

TEST_CASE("set memory pool size clamps with the rounded pool size" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1oddsize", 16);
    kPool.SetSize(13);
    lua_State* L = OpenScriptState();
    LogCapture capture;
    CallSetPoolSize(L, "t1oddsize", 2147483647.0f);
    CHECK(kPool.GetCount() == kPoolBytesMax / 16);
    CHECK(PoolBytesOk(kPool.GetCount(), (kPool.GetSize() + 3) & ~3u, kPoolBytesMax));
    CHECK(capture.Count("SetMemoryPoolSize(t1oddsize): script value 2147483647 clamped to 1048576") == 1);
    lua_close(L);
}

TEST_CASE("set memory pool size label abi" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    lua_State* L = OpenScriptState();
    {
        MemoryPool kColor("ParticleTransformer::ColorTransformer", 16);
        CHECK(strcmp(kColor.mLabel, "ParticleTransformer::ColorTrans") == 0);
        CallSetPoolSize(L, "ParticleTransformer::ColorTrans", 7.0f);
        CHECK(kColor.GetCount() == 7);
        CallSetPoolSize(L, "ParticleTransformer::ColorTransXYZ", 9.0f);
        CHECK(kColor.GetCount() == 9);
    }
    {
        MemoryPool kLight("EntityLight", 16);
        CallSetPoolSize(L, "EntityLight", 5.0f);
        CHECK(kLight.GetCount() == 65);
        CallSetPoolSize3(L, "EntityLight", 5.0f, 7.0f);
        CHECK(kLight.GetCount() == 7);
        const unsigned int uMax = kPoolBytesMax / 16;
        LogCapture capture;
        CallSetPoolSize(L, "EntityLight", (float)(uMax - 10));
        CHECK(kLight.GetCount() == uMax);
        CHECK(capture.Count("SetMemoryPoolSize(EntityLight): script value 1048626 clamped to 1048576") == 1);
    }
    {
        MemoryPool kWeapon("t1weapon", 16);
        MemoryPool kAmmo("t1ammo", 16);
        MemoryPool kEnergy("t1energy", 16);
        kAmmo.SetCount(10);
        kEnergy.SetCount(10);
        {
            LogCapture capture;
            CallSetPoolSize(L, "t1weapon", 50.0f, &kWeapon, &kAmmo, &kEnergy);
            CHECK(capture.Count("entry=SetMemoryPoolSize label=t1ammo requested=50 applied=50 pool=t1ammo") == 1);
            CHECK(capture.Count("entry=SetMemoryPoolSize label=t1energy requested=50 applied=50 pool=t1energy") ==
                  1);
        }
        CHECK(kWeapon.GetCount() == 50);
        CHECK(kAmmo.GetCount() == 50);
        CHECK(kEnergy.GetCount() == 50);
        kAmmo.SetCount(80);
        {
            LogCapture capture;
            CallSetPoolSize(L, "t1weapon", 60.0f, &kWeapon, &kAmmo, &kEnergy);
            CHECK(capture.Count("label=t1ammo") == 0);
            CHECK(capture.Count("entry=SetMemoryPoolSize label=t1energy requested=60 applied=60 pool=t1energy") ==
                  1);
        }
        CHECK(kWeapon.GetCount() == 60);
        CHECK(kAmmo.GetCount() == 80);
        CHECK(kEnergy.GetCount() == 60);
        CallSetPoolSize(L, "t1ammo", 20.0f, &kWeapon, &kAmmo, &kEnergy);
        CHECK(kAmmo.GetCount() == 20);
        CHECK(kEnergy.GetCount() == 60);
    }
    {
        MemoryPool kOther("t1other", 16);
        kOther.SetCount(3);
        LogCapture capture;
        CallSetPoolSize(L, "t1nosuchpool", 5.0f);
        CHECK(kOther.GetCount() == 3);
        CHECK(capture.Count(
                  "OPENSWBF2 SCRIPTMEM entry=SetMemoryPoolSize label=t1nosuchpool requested=5 applied=0 pool=none")
              == 1);
    }
    lua_close(L);
}

TEST_CASE("set memory pool size logs verbose line" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    MemoryPool kPool("t1clamp", 16);
    lua_State* L = OpenScriptState();
    LogCapture capture;
    CallSetPoolSize(L, "t1clamp", 12.0f);
    CHECK(capture.Count("OPENSWBF2 SCRIPTMEM entry=SetMemoryPoolSize label=t1clamp requested=12 applied=12 "
                        "pool=t1clamp")
          == 1);
    CHECK(capture.Count("clamped") == 0);
    lua_close(L);
}

TEST_CASE("setup temp heap clamps size" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    lua_State* L = OpenScriptState();
    const int iRuntime = CreateScriptRuntime(sacScriptRuntime, sizeof(sacScriptRuntime));
    const unsigned int uStart = RedGetHeapFree(iRuntime);
    int iTemp = 0;
    {
        LogCapture capture;
        CallSetupTempHeap(L, 100.0f, iRuntime, iTemp);
        CHECK(capture.Count("SetupTempHeap(): script value 100 clamped to 4096") == 1);
        CHECK(capture.Count("OPENSWBF2 SCRIPTMEM entry=SetupTempHeap label=- requested=100 applied=4096 pool=-")
              == 1);
    }
    REQUIRE(iTemp > 0);
    CHECK(RedGetHeapSize(iTemp) == kTempHeapMin - 256);
    CHECK(__RedTempHeap == iTemp);
    CHECK(RedGetHeapFree(iRuntime) < uStart);
    LuaMemoryArgs::ClearTempHeap(iRuntime, iTemp);
    CHECK(iTemp == 0);
    CHECK(__RedTempHeap == -1);
    CHECK(RedGetHeapFree(iRuntime) == uStart);
    RedDestroyHeap(iRuntime);
    lua_close(L);
}

TEST_CASE("setup temp heap null block leaves no temp heap" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    lua_State* L = OpenScriptState();
    const int iRuntime = CreateScriptRuntime(sacScriptSmall, sizeof(sacScriptSmall));
    const unsigned int uStart = RedGetHeapFree(iRuntime);
    const int iSlot = NextFreeHeapSlot();
    int iTemp = 0;
    {
        LogCapture capture;
        CallSetupTempHeap(L, 16384.0f, iRuntime, iTemp);
        CHECK(capture.Count("SetupTempHeap: block of 16384 bytes not allocated, no temp heap") == 1);
        CHECK(capture.Count("requested=16384 applied=0") == 1);
    }
    CHECK(iTemp == 0);
    CHECK(__RedTempHeap == -1);
    CHECK(NextFreeHeapSlot() == iSlot);
    CHECK(RedGetHeapFree(iRuntime) == uStart);
    LuaMemoryArgs::ClearTempHeap(iRuntime, iTemp);
    CHECK(RedGetHeapFree(iRuntime) == uStart);
    RedDestroyHeap(iRuntime);
    lua_close(L);
}

TEST_CASE("setup temp heap twice frees previous heap" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    lua_State* L = OpenScriptState();
    const int iRuntime = CreateScriptRuntime(sacScriptRuntime, sizeof(sacScriptRuntime));
    const unsigned int uStart = RedGetHeapFree(iRuntime);
    int iTemp = 0;
    CallSetupTempHeap(L, 8192.0f, iRuntime, iTemp);
    REQUIRE(iTemp > 0);
    const int iFirst = iTemp;
    const unsigned int uAfterFirst = RedGetHeapFree(iRuntime);
    const int iSlot = NextFreeHeapSlot();
    CallSetupTempHeap(L, 8192.0f, iRuntime, iTemp);
    CHECK(iTemp == iFirst);
    CHECK(RedGetHeapFree(iRuntime) == uAfterFirst);
    CHECK(NextFreeHeapSlot() == iSlot);
    LuaMemoryArgs::ClearTempHeap(iRuntime, iTemp);
    CHECK(iTemp == 0);
    CHECK(RedGetHeapFree(iRuntime) == uStart);
    RedDestroyHeap(iRuntime);
    lua_close(L);
}

TEST_CASE("setup temp heap forget drops stale block" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    lua_State* L = OpenScriptState();
    int iRuntime = CreateScriptRuntime(sacScriptRuntime, sizeof(sacScriptRuntime));
    int iTemp = 0;
    CallSetupTempHeap(L, 8192.0f, iRuntime, iTemp);
    REQUIRE(iTemp > 0);
    RedDestroyHeap(iTemp);
    RedSetTempHeap(-1);
    iTemp = 0;
    LuaMemoryArgs::ForgetTempHeap();
    RedDestroyHeap(iRuntime);
    iRuntime = CreateScriptRuntime(sacScriptRuntime, sizeof(sacScriptRuntime));
    const unsigned int uFresh = RedGetHeapFree(iRuntime);
    LuaMemoryArgs::ClearTempHeap(iRuntime, iTemp);
    CHECK(RedGetHeapFree(iRuntime) == uFresh);
    CallSetupTempHeap(L, 8192.0f, iRuntime, iTemp);
    REQUIRE(iTemp > 0);
    CHECK(RedGetHeapFree(iRuntime) < uFresh);
    LuaMemoryArgs::ClearTempHeap(iRuntime, iTemp);
    CHECK(iTemp == 0);
    CHECK(RedGetHeapFree(iRuntime) == uFresh);
    RedDestroyHeap(iRuntime);
    lua_close(L);
}

TEST_CASE("setup temp heap refuses while load heap is live" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    lua_State* L = OpenScriptState();
    const int iRuntime = CreateScriptRuntime(sacScriptRuntime, sizeof(sacScriptRuntime));
    const unsigned int uStart = RedGetHeapFree(iRuntime);
    const int iLoad = RedCreateHeap(sacScriptLoad, sacScriptLoad + sizeof(sacScriptLoad), 0x10, "T1Load");
    REQUIRE(iLoad > 0);
    int iTemp = iLoad;
    {
        LogCapture capture;
        CallSetupTempHeap(L, 8192.0f, iRuntime, iTemp);
        CHECK(capture.Count("SetupTempHeap: load temp heap still live, request 8192 ignored") == 1);
    }
    CHECK(iTemp == iLoad);
    CHECK(RedGetHeapFree(iRuntime) == uStart);
    CHECK(RedGetHeapSize(iLoad) != 0);
    RedDestroyHeap(iLoad);
    RedDestroyHeap(iRuntime);
    lua_close(L);
}

TEST_CASE("setup temp heap refuses when no heap slot is left" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    lua_State* L = OpenScriptState();
    const int iRuntime = CreateScriptRuntime(sacScriptRuntime, sizeof(sacScriptRuntime));
    const unsigned int uStart = RedGetHeapFree(iRuntime);
    int aiFill[16];
    int iFilled = 0;
    while (iFilled < 16)
    {
        char* pcFill = sacScriptFill[iFilled];
        const int iHeap = RedCreateHeap(pcFill, pcFill + sizeof(sacScriptFill[0]), 0x10, "T1Fill");
        if (iHeap < 0)
        {
            break;
        }
        aiFill[iFilled++] = iHeap;
    }
    REQUIRE(iFilled < 16);
    int iTemp = 0;
    {
        LogCapture capture;
        CallSetupTempHeap(L, 8192.0f, iRuntime, iTemp);
        CHECK(capture.Count("SetupTempHeap: no heap slot, no temp heap") == 1);
    }
    CHECK(iTemp == 0);
    CHECK(__RedTempHeap == -1);
    CHECK(RedGetHeapFree(iRuntime) == uStart);
    for (int i = 0; i < iFilled; ++i)
    {
        RedDestroyHeap(aiFill[i]);
    }
    RedDestroyHeap(iRuntime);
    lua_close(L);
}

TEST_CASE("steal artist heap refuses zero small and negative sizes" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    lua_State* L = OpenScriptState();
    const int iArtist = RedCreateHeap(sacScriptSteal, sacScriptSteal + 0x2000, 0x10, "T1Artist");
    const int iRuntime = RedCreateHeap(sacScriptSteal + 0x2000, sacScriptSteal + 0x4000, 0x10, "T1Runtime");
    REQUIRE(iArtist > 0);
    REQUIRE(iRuntime == iArtist + 1);
    {
        LogCapture capture;
        CallStealArtistHeap(L, 0.0f, iArtist);
        CallStealArtistHeap(L, 16.0f, iArtist);
        CallStealArtistHeap(L, -64.0f, iArtist);
        CHECK(capture.Count("refused") == 3);
        CallStealArtistHeap(L, 12288.0f, iArtist);
        CallStealArtistHeap(L, 819200.0f, iArtist);
        CHECK(capture.Count("refused") == 3);
        CHECK(capture.Count("requested=819200 applied=0") == 1);
        CHECK(capture.Count("StealArtistHeap(): script value -64 refused") == 1);
        CHECK(capture.Count("entry=StealArtistHeap label=- requested=16 applied=0 pool=-") == 1);
    }
    CHECK(RedGetHeapSize(iArtist) == 0x2000);
    CHECK(RedGetHeapSize(iRuntime) == 0x2000);
    CallStealArtistHeap(L, 256.0f, iArtist);
    CHECK(RedGetHeapSize(iArtist) == 0x2000 - 256);
    CHECK(RedGetHeapSize(iRuntime) == 0x2000 + 256);
    {
        LogCapture capture;
        CallStealArtistHeap(L, 40.0f, iArtist);
        CHECK(capture.Count("entry=StealArtistHeap label=- requested=40 applied=32 pool=-") == 1);
    }
    CHECK(RedGetHeapSize(iArtist) == 0x2000 - 288);
    void* pTop = RedAllocFromHeap(iArtist, 64, true);
    REQUIRE(pTop);
    {
        LogCapture capture;
        CallStealArtistHeap(L, 256.0f, iArtist);
        CHECK(capture.Count("entry=StealArtistHeap label=- requested=256 applied=0 pool=-") == 1);
    }
    CHECK(RedGetHeapSize(iArtist) == 0x2000 - 288);
    RedFreeToHeap(iArtist, pTop, 0);
    RedDestroyHeap(iRuntime);
    RedDestroyHeap(iArtist);
    lua_close(L);
}

TEST_CASE("add walker type clamps script counts" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    MemoryPool kWalker("t1walker", 16);
    MemoryPool kLegPair("t1legpair", 32);
    MemoryPool kBlend("t1blend", 64);
    MemoryPool kBlendUnit("t1blendunit", 8);
    MemoryPool kDroideka("t1droideka", 16);
    lua_State* L = OpenScriptState();
    {
        LogCapture capture;
        CallAddWalkerType(L, 2.0f, 3.0f, &kWalker, &kLegPair, &kBlend, &kBlendUnit, &kDroideka);
        CHECK(capture.Count("clamped") == 0);
        CHECK(capture.Count("OPENSWBF2 SCRIPTMEM entry=AddWalkerType label=t1blendunit requested=27 applied=27 "
                            "pool=t1blendunit")
              == 1);
    }
    CHECK(kWalker.GetCount() == 3);
    CHECK(kLegPair.GetCount() == 6);
    CHECK(kBlend.GetCount() == 9);
    CHECK(kBlendUnit.GetCount() == 27);
    CHECK(kDroideka.GetCount() == 0);
    {
        LogCapture capture;
        CallAddWalkerType(L, 1.0f, 268435456.0f, &kWalker, &kLegPair, &kBlend, &kBlendUnit, &kDroideka);
        CHECK(capture.Count("clamped") == 4);
        CHECK(capture.Count("OPENSWBF2 SCRIPTMEM entry=AddWalkerType label=t1walker requested=268435459 "
                            "applied=1048576 pool=t1walker")
              == 1);
    }
    CHECK(kWalker.GetCount() == kPoolBytesMax / 16);
    CHECK(kLegPair.GetCount() == kPoolBytesMax / 32);
    CHECK(kBlend.GetCount() == kPoolBytesMax / 64);
    CHECK(kBlendUnit.GetCount() == kPoolBytesMax / 8);
    CHECK(kDroideka.GetCount() == 0);
    CallAddWalkerType(L, 0.0f, 4.0f, &kWalker, &kLegPair, &kBlend, &kBlendUnit, &kDroideka);
    CHECK(kDroideka.GetCount() == 4);
    CHECK(kWalker.GetCount() == kPoolBytesMax / 16);
    CHECK(kBlendUnit.GetCount() == kPoolBytesMax / 8);
    lua_close(L);
}

TEST_CASE("add walker type keeps the retail unsigned wrap" * doctest::test_suite("regression"))
{
    VerboseMemoryLog kVerbose;
    MemoryPool kWalker("t1wrapwalker", 16);
    MemoryPool kLegPair("t1wraplegpair", 32);
    MemoryPool kBlend("t1wrapblend", 64);
    MemoryPool kBlendUnit("t1wrapblendunit", 8);
    MemoryPool kDroideka("t1wrapdroideka", 16);
    lua_State* L = OpenScriptState();
    {
        LogCapture capture;
        CallSetPoolSize(L, "t1wrapwalker", -1.0f);
        CHECK(kWalker.GetCount() == 0xFFFFFFFFu);
        CallAddWalkerType(L, 2.0f, 2.0f, &kWalker, &kLegPair, &kBlend, &kBlendUnit, &kDroideka);
        CHECK(kWalker.GetCount() == 1);
        CallAddWalkerType(L, 1.0f, 1.0f, &kWalker, &kLegPair, &kBlend, &kBlendUnit, &kDroideka);
        CHECK(kWalker.GetCount() == 2);
        CHECK(kLegPair.GetCount() == 5);
        CHECK(kBlend.GetCount() == 8);
        CHECK(kBlendUnit.GetCount() == 24);
        CHECK(capture.Count("clamped") == 0);
        CHECK(capture.Count("refused") == 0);
    }
    {
        LogCapture capture;
        CallSetPoolSize(L, "t1wrapdroideka", -2.0f);
        CallAddWalkerType(L, 0.0f, 1.0f, &kWalker, &kLegPair, &kBlend, &kBlendUnit, &kDroideka);
        CHECK(kDroideka.GetCount() == 0xFFFFFFFFu);
        void* pBlock = kDroideka.Allocate(16);
        REQUIRE(pBlock);
        CHECK(capture.Count("Memory pool \"t1wrapdroideka\" refused count 4294967295 size 16") == 1);
        CHECK(kDroideka.GetCount() == kDroideka.GetGrow());
        CHECK(!kDroideka.IsPresent((char*)kDroideka.mPool + kDroideka.GetGrow() * 16));
        kDroideka.Free(pBlock);
        kDroideka.Destroy();
    }
    lua_close(L);
}
