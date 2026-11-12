#include <stdio.h>
#include <string.h>

#include "LuaMemory.h"

#include "GameMemory.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Util/RedWarning.h"
#include "RedEngineFL/Util/RedLog.h"
#include "pcCrash.h"
#include "LuaFL/include/lua.h"

// GLOBAL: SWBF2 0x007e85d0
MemPoolInfo gGamePoolInfo[48] = {
    {  6933,   8, 0 }, {  3427,   6, 0 }, {  1764,  10, 0 }, {  1622,  18, 0 },
    {  1567,  80, 0 }, {  1405,   4, 0 }, {  1420,   7, 0 }, {   895, 160, 0 },
    {   750,  20, 0 }, {   895,   9, 0 }, {   733,  11, 0 }, {   728,  12, 0 },
    {   632,  40, 0 }, {   445,   5, 0 }, {   343,  13, 0 }, {   558,   3, 0 },
    {   284,  14, 0 }, {   349,   1, 0 }, {   184,  16, 0 }, {   146,  15, 0 },
    {   134,   2, 0 }, {   109,  24, 0 }, {    78,  28, 0 }, {    84,  22, 0 },
    {    90,  17, 0 }, {    73,  26, 0 }, {    59,  32, 0 }, {    60,  36, 0 },
    {    59,  30, 0 }, {    64,  21, 0 }, {    53,  34, 0 }, {    49,  19, 0 },
    {    43,  38, 0 }, {    33, 320, 0 }, {    34,  50, 0 }, {    34,  23, 0 },
    {    32,  42, 0 }, {    31,  29, 0 }, {    25,  44, 0 }, {    22,  46, 0 },
    {    21,  64, 0 }, {    18,  41, 0 }, {    18,  31, 0 }, {    16,  52, 0 },
    {    16,  68, 0 }, {    16,  56, 0 }, {    13,  58, 0 }, {    12,  53, 0 },
};

// GLOBAL: SWBF2 0x007e8750
MemPoolInfo gShellPoolInfo[48] = {
    {   743,   8, 0 }, {  1815,   6, 0 }, {  1203,   4, 0 }, {  4589,  10, 0 },
    {  1308,   7, 0 }, {  1046,  18, 0 }, {   739,   9, 0 }, {    18,  40, 0 },
    {    74,  20, 0 }, {   561,  11, 0 }, {   457,  12, 0 }, {     5,  80, 0 },
    {   261,   5, 0 }, {    25,   3, 0 }, {   616,  13, 0 }, {  1575,  14, 0 },
    {   144,   1, 0 }, {    57,  15, 0 }, {   199,  16, 0 }, {     3, 160, 0 },
    {   112,   2, 0 }, {   107,  24, 0 }, {  2102,  28, 0 }, {    53,  21, 0 },
    {   154,  22, 0 }, {    24,  34, 0 }, {    69,  26, 0 }, {    21,  36, 0 },
    {    79,  19, 0 }, {    66,  32, 0 }, {    25,  30, 0 }, {    87,  17, 0 },
    {    18,  38, 0 }, {    12,  64, 0 }, {    91,  23, 0 }, {    13,  60, 0 },
    {    13,  54, 0 }, {    10,  44, 0 }, {     7,  41, 0 }, {    12,  58, 0 },
    {     4,  53, 0 }, {     5,  68, 0 }, {    20,  46, 0 }, {     7,  66, 0 },
    {     2,  67, 0 }, {     0, 320, 0 }, {     4,  98, 0 }, {  2662,  56, 0 },
};

// GLOBAL: SWBF2 0x01f9cde0
MemPoolInfo gPoolInfo[48];
// GLOBAL: SWBF2 0x01f9cf60
MemoryPool* gPools[48];

// GLOBAL: SWBF2 0x01e579a4
unsigned int gLuaUsedMemory;
namespace LuaMemory
{
// GLOBAL: SWBF2 0x01e579a8
unsigned int gLuaNumBlocks;
}

#ifdef OPENSWBF2_TEST
namespace
{

unsigned int suLuaSpill[48];

void NoteLuaSpill(unsigned int uiSize)
{
    unsigned int uiBin = (uiSize + 3) >> 2;
    for (unsigned int i = 0; i < 48; ++i)
    {
        if ((unsigned int)gPoolInfo[i].mBin == uiBin && gPools[i])
        {
            ++suLuaSpill[i];
            return;
        }
    }
}

}

unsigned int LuaMemory::GetSpillCount(unsigned int uiBin)
{
    for (unsigned int i = 0; i < 48; ++i)
    {
        if ((unsigned int)gPoolInfo[i].mBin == uiBin && gPools[i])
        {
            return suLuaSpill[i];
        }
    }
    return 0;
}

void LuaMemory::LogMemReportLines()
{
    char acLine[64];
    for (unsigned int i = 0; i < 48; ++i)
    {
        if (gPools[i] && FormatLuaSpillLine(gPoolInfo[i].mBin, suLuaSpill[i], acLine, sizeof(acLine)))
        {
            LOG(LogMemory, Inform, "%s", acLine);
        }
    }
}

bool LuaMemory::FormatLuaSpillLine(int iBin, unsigned int uCount, char* pcOut, unsigned int uSize)
{
    if (!pcOut || uSize == 0)
    {
        return false;
    }
    int iLen = _snprintf(pcOut, uSize, "OPENSWBF2 MEMREPORT luaspill pool=Lua%d count=%u", iBin, uCount);
    pcOut[uSize - 1] = 0;
    return iLen >= 0 && (unsigned int)iLen < uSize;
}
#endif

// FUNCTION: SWBF2 0x005a15d0
void LuaMemory::CreatePools(bool bMission)
{
#ifdef OPENSWBF2_TEST
    memset(suLuaSpill, 0, sizeof(suLuaSpill));
#endif
    int iOldHeap = RedSetCurrentHeap(GameMemory::LuaHeap);
    RedAllocFromHeap(iOldHeap, 4, 0);

    MemPoolInfo* pInfo = bMission ? gGamePoolInfo : gShellPoolInfo;
    MemoryPool** ppPool = gPools;
    int iLeft = 48;
    int iIndex = 0;
    do
    {
        MemPoolInfo* pDest = &gPoolInfo[iIndex];
        *pDest = *pInfo;
        if (pDest->mCount != 0)
        {
            char acName[16];
            sprintf(acName, "Lua%d", pInfo->mBin);
            MemoryPool* pPool = new MemoryPool(acName, 0);
            *ppPool = pPool;
            pPool->Create(pInfo->mCount, pInfo->mBin * 4, 0);
        }
        else
        {
            *ppPool = 0;
        }
        ++pInfo;
        ++iIndex;
        ++ppPool;
    } while (--iLeft != 0);

    RedSetCurrentHeap(iOldHeap);
}

// FUNCTION: SWBF2 0x005a1760
MemoryPool* LuaMemory::LocatePool(unsigned int uiSize, void* pBlock)
{
    unsigned int uiBin = (uiSize + 3) >> 2;

    MemoryPool* pPool = 0;
    unsigned int i;
    for (i = 0; i < 48; ++i)
    {
        if ((unsigned int)gPoolInfo[i].mBin == uiBin)
        {
            pPool = gPools[i];
            break;
        }
    }

    if (pBlock)
    {
        if (pPool && pPool->IsPresent(pBlock))
        {
            return pPool;
        }
        return 0;
    }

    if (pPool)
    {
        return (pPool->mUsed < pPool->mCount) ? pPool : 0;
    }
    return 0;
}

// FUNCTION: SWBF2 0x005a17d0
void Lua_Red_Free(void* pBlock, unsigned int uiSize)
{
    if (!pBlock)
    {
        return;
    }

    MemoryPool* pPool = LuaMemory::LocatePool(uiSize, pBlock);
    if (pPool)
    {
        pPool->Free(pBlock);
    }
    else
    {
        RedFreeToHeap(GameMemory::LuaHeap, pBlock, 1);
    }
}

// FUNCTION: SWBF2 0x005a1810
void* Lua_Red_realloc(void* pBlock, unsigned int uiOldSize, unsigned int uiNewSize)
{
    MemoryPool* pPool = LuaMemory::LocatePool(uiNewSize, 0);
#ifdef OPENSWBF2_TEST
    if (!pPool)
    {
        NoteLuaSpill(uiNewSize);
    }
#endif
    bool bHeapToHeap = !pPool && pBlock && !LuaMemory::LocatePool(uiOldSize, pBlock);
    void* pNew = pPool ? pPool->Allocate(uiNewSize)
                 : bHeapToHeap ? RedReallocInHeap(GameMemory::LuaHeap, pBlock, uiOldSize, uiNewSize)
                               : RedAllocFromHeap(GameMemory::LuaHeap, uiNewSize, 0);

    if (!pNew)
    {
        LOG(LogLua, Error, "Lua out of memory (%d used in %d blocks, trying to alloc %d)",
            gLuaUsedMemory, LuaMemory::gLuaNumBlocks, uiNewSize);
        return 0;
    }

    if (pBlock)
    {
        if (bHeapToHeap)
        {
            return pNew;
        }
        if (uiNewSize <= uiOldSize)
        {
            memcpy(pNew, pBlock, uiNewSize);
        }
        else
        {
            memcpy(pNew, pBlock, uiOldSize);
        }
        Lua_Red_Free(pBlock, uiOldSize);
    }

    return pNew;
}

// FUNCTION: SWBF2 0x005a16e0
void LuaMemory::Reset()
{
    int iOldHeap = RedSetCurrentHeap(GameMemory::LuaHeap);
    unsigned int i;
    for (i = 0; i < 48; ++i)
    {
        delete gPools[i];
        gPools[i] = 0;
    }
    RedSetCurrentHeap(iOldHeap);

    gLuaUsedMemory = 0;
    LuaMemory::gLuaNumBlocks = 0;
    memset(gPoolInfo, 0, sizeof(gPoolInfo));
}
