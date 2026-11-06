#include "LuaMemoryArgs.h"

#include "LuaFL/include/lauxlib.h"

#include <limits.h>
#include <string.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Memory/RedScriptLimits.h"
#include "RedEngineFL/Util/RedLog.h"

// GLOBAL: SWBF2 0x01e57724
static void* sTempHeapBlock;

namespace LuaMemoryArgs
{

static int ReadScriptInt(lua_State* L, int iArg)
{
    lua_Number fValue = luaL_checknumber(L, iArg);
    if (fValue >= 2147483648.0f)
    {
        return INT_MAX;
    }
    if (!(fValue > -2147483648.0f))
    {
        return INT_MIN;
    }
    return (int)fValue;
}

static unsigned int PoolUnit(const MemoryPool* pPool)
{
    return (pPool->GetSize() + 3) & ~3u;
}

static void LogWalker(MemoryPool* pPool, unsigned int uRequested)
{
    LOG(LogMemory, Verbose, "OPENSWBF2 SCRIPTMEM entry=AddWalkerType label=%s requested=%u applied=%u pool=%s",
        pPool->mLabel, uRequested, pPool->GetCount(), pPool->mLabel);
}

static void RaiseToWeapon(MemoryPool* pPool, MemoryPool* pWeapon)
{
    if (pPool->GetCount() < pWeapon->GetCount())
    {
        pPool->SetCount(RedScriptLimits::ClampScriptSize(RedScriptLimits::kSetMemoryPoolSize, pPool->mLabel,
                                                         (int)pWeapon->GetCount(), PoolUnit(pPool)));
        LOG(LogMemory, Verbose,
            "OPENSWBF2 SCRIPTMEM entry=SetMemoryPoolSize label=%s requested=%u applied=%u pool=%s", pPool->mLabel,
            pWeapon->GetCount(), pPool->GetCount(), pPool->mLabel);
    }
}

void SetMemoryPoolSize(lua_State* L, MemoryPool* pWeapon, MemoryPool* pAmmo, MemoryPool* pEnergy)
{
    int iArgs = lua_gettop(L);
    const char* pcName = luaL_checklstring(L, 1, 0);
    int iCount = ReadScriptInt(L, 2);
    int iRequested = iCount;
    MemoryPool* pPool = MemoryPool::Find(pcName);
    if (pPool)
    {
        if (iArgs >= 3)
        {
            iCount = ReadScriptInt(L, 3);
        }
        else if (!_stricmp(pcName, "EntityLight"))
        {
            iCount = (int)((unsigned int)iCount + 60u);
        }
        iRequested = iCount;
        unsigned int uApplied = RedScriptLimits::ClampScriptSize(RedScriptLimits::kSetMemoryPoolSize, pcName, iCount, PoolUnit(pPool));
        pPool->SetCount(uApplied);
    }
    LOG(LogMemory, Verbose, "OPENSWBF2 SCRIPTMEM entry=SetMemoryPoolSize label=%s requested=%d applied=%u pool=%s",
        pcName, iRequested, pPool ? pPool->GetCount() : 0, pPool ? pPool->mLabel : "none");
    if (pPool != 0 && pPool == pWeapon)
    {
        RaiseToWeapon(pAmmo, pWeapon);
        RaiseToWeapon(pEnergy, pWeapon);
    }
}

void ClearTempHeap(int iRuntimeHeap, int& riTempHeap)
{
    if (sTempHeapBlock)
    {
        if (riTempHeap)
        {
            RedDestroyHeap(riTempHeap);
            RedSetTempHeap(-1);
            riTempHeap = 0;
        }
        RedFreeToHeap(iRuntimeHeap, sTempHeapBlock, 0);
        sTempHeapBlock = 0;
    }
}

void ForgetTempHeap()
{
    sTempHeapBlock = 0;
}

static unsigned int CreateTempHeap(int iRuntimeHeap, int& riTempHeap, unsigned int uSize)
{
    if (sTempHeapBlock != 0)
    {
        ClearTempHeap(iRuntimeHeap, riTempHeap);
    }
    else if (riTempHeap != 0)
    {
        LOG(LogMemory, Warning, "SetupTempHeap: load temp heap still live, request %u ignored", uSize);
        return 0;
    }
    void* pBlock = RedAllocFromHeap(iRuntimeHeap, uSize, 1);
    if (pBlock == 0)
    {
        LOG(LogMemory, Warning, "SetupTempHeap: block of %u bytes not allocated, no temp heap", uSize);
        return 0;
    }
    int iHeap = RedCreateHeap(pBlock, (char*)pBlock + uSize - 256, 16, "Temp");
    if (iHeap < 0)
    {
        RedFreeToHeap(iRuntimeHeap, pBlock, 0);
        LOG(LogMemory, Warning, "SetupTempHeap: no heap slot, no temp heap");
        return 0;
    }
    sTempHeapBlock = pBlock;
    riTempHeap = iHeap;
    RedSetTempHeap(iHeap);
    return uSize;
}

void SetupTempHeap(lua_State* L, int iRuntimeHeap, int& riTempHeap)
{
    int iSize = ReadScriptInt(L, 1);
    unsigned int uSize = RedScriptLimits::ClampScriptSize(RedScriptLimits::kSetupTempHeap, 0, iSize, 1);
    unsigned int uApplied = CreateTempHeap(iRuntimeHeap, riTempHeap, uSize);
    LOG(LogMemory, Verbose, "OPENSWBF2 SCRIPTMEM entry=SetupTempHeap label=- requested=%d applied=%u pool=-", iSize,
        uApplied);
}

void StealArtistHeap(lua_State* L, int iArtistHeap)
{
    int iSize = ReadScriptInt(L, 1);
    unsigned int uApplied = 0;
    if (iSize < 0 || RedGetHeapSize(iArtistHeap) > (unsigned int)iSize)
    {
        unsigned int uSize = RedScriptLimits::ClampScriptSize(RedScriptLimits::kStealArtistHeap, 0, iSize, 1);
        if (uSize != 0)
        {
            const unsigned int uBefore = RedGetHeapSize(iArtistHeap);
            RedMoveHeapEnd(iArtistHeap, -(int)uSize);
            uApplied = uBefore - RedGetHeapSize(iArtistHeap);
        }
    }
    LOG(LogMemory, Verbose, "OPENSWBF2 SCRIPTMEM entry=StealArtistHeap label=- requested=%d applied=%u pool=-", iSize,
        uApplied);
}

void AddWalkerType(lua_State* L, MemoryPool* pWalker, MemoryPool* pLegPair, MemoryPool* pBlend,
                   MemoryPool* pBlendUnit, MemoryPool* pDroideka)
{
    int iLegPairs = ReadScriptInt(L, 1);
    int iCount = ReadScriptInt(L, 2);
    const unsigned int uCount = (unsigned int)iCount;
    const unsigned int uLegPairs = uCount * (unsigned int)iLegPairs;
    const unsigned int uBlends = ((unsigned int)iLegPairs + 1u) * uCount;
    if (iLegPairs > 0)
    {
        const unsigned int uWalker = pWalker->GetCount() + uCount;
        pWalker->SetCount(RedScriptLimits::ClampScriptSize(RedScriptLimits::kAddWalkerType, pWalker->mLabel, (int)uWalker, PoolUnit(pWalker)));
        LogWalker(pWalker, uWalker);
        const unsigned int uLegPair = pLegPair->GetCount() + uLegPairs;
        pLegPair->SetCount(RedScriptLimits::ClampScriptSize(RedScriptLimits::kAddWalkerType, pLegPair->mLabel,
                                                            (int)uLegPair, PoolUnit(pLegPair)));
        LogWalker(pLegPair, uLegPair);
        const unsigned int uBlend = pBlend->GetCount() + uBlends;
        pBlend->SetCount(RedScriptLimits::ClampScriptSize(RedScriptLimits::kAddWalkerType, pBlend->mLabel,
                                                          (int)uBlend, PoolUnit(pBlend)));
        LogWalker(pBlend, uBlend);
        const unsigned int uBlendUnit = pBlendUnit->GetCount() + uBlends * 3u;
        pBlendUnit->SetCount(RedScriptLimits::ClampScriptSize(RedScriptLimits::kAddWalkerType, pBlendUnit->mLabel,
                                                              (int)uBlendUnit, PoolUnit(pBlendUnit)));
        LogWalker(pBlendUnit, uBlendUnit);
    }
    else
    {
        const unsigned int uDroideka = pDroideka->GetCount() + uCount;
        pDroideka->SetCount(RedScriptLimits::ClampScriptSize(RedScriptLimits::kAddWalkerType, pDroideka->mLabel,
                                                             (int)uDroideka, PoolUnit(pDroideka)));
        LogWalker(pDroideka, uDroideka);
    }
}

}
