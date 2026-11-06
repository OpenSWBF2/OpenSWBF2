#include <windows.h>
#include <stdio.h>

#include "RedMemoryReport.h"

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Util/RedLog.h"

namespace RedMemoryReport
{

bool FormatHeapLine(const char* pcName, const RedHeapStats& rStats, char* pcOut, unsigned int uSize)
{
    if (!pcOut || uSize == 0)
    {
        return false;
    }
    _snprintf(pcOut, uSize, "OPENSWBF2 MEMREPORT heap=%s cap=%u peak=%u used=%u allocs=%u failed=%u",
              pcName ? pcName : "?", rStats.uCap, rStats.uPeak, rStats.uUsed, rStats.uAllocs, rStats.uFailed);
    pcOut[uSize - 1] = 0;
    return true;
}

bool FormatPoolLine(const char* pcLabel, unsigned int uCount, unsigned int uBlockSize, unsigned int uPeak,
                    unsigned int uGrown, char* pcOut, unsigned int uSize)
{
    if (!pcOut || uSize == 0)
    {
        return false;
    }
    _snprintf(pcOut, uSize, "OPENSWBF2 MEMREPORT pool=%s count=%u size=%u peak=%u grown=%u",
              pcLabel ? pcLabel : "?", uCount, uBlockSize, uPeak, uGrown);
    pcOut[uSize - 1] = 0;
    return true;
}

bool FormatPoolFull(const char* pcLabel, unsigned int uPeak, char* pcOut, unsigned int uSize)
{
    if (!pcOut || uSize == 0)
    {
        return false;
    }
    _snprintf(pcOut, uSize, "Memory pool \"%s\" is full; raise count to at least %u", pcLabel ? pcLabel : "?",
              uPeak);
    pcOut[uSize - 1] = 0;
    return true;
}

bool FormatCloth(int iFree, bool bEnabled, int iLow, int iHigh, char* pcOut, unsigned int uSize)
{
    if (!pcOut || uSize == 0)
    {
        return false;
    }
    _snprintf(pcOut, uSize, "OPENSWBF2 CLOTH free=%d enabled=%d low=%d high=%d", iFree, bEnabled ? 1 : 0, iLow,
              iHigh);
    pcOut[uSize - 1] = 0;
    return true;
}

bool FormatMemFail(const MemFailInfo& rInfo, char* pcOut, unsigned int uSize)
{
    if (!pcOut || uSize == 0)
    {
        return false;
    }
    _snprintf(pcOut, uSize,
              "alloc failure heap %s (%d) size %u free %u load %u%% phys %u MB virt %u MB used %u cap %u peak %u largestfree %u",
              rInfo.pcHeap ? rInfo.pcHeap : "?", rInfo.iHeap, rInfo.uSize, rInfo.uFree, rInfo.uLoad, rInfo.uPhysMB,
              rInfo.uVirtMB, rInfo.uUsed, rInfo.uCap, rInfo.uPeak, rInfo.uLargestFree);
    pcOut[uSize - 1] = 0;
    return true;
}

unsigned int LargestFreeRegion()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const unsigned int uiLimit = (unsigned int)si.lpMaximumApplicationAddress;
    unsigned int uiAt = (unsigned int)si.lpMinimumApplicationAddress;
    unsigned int uiLargest = 0;
    while (uiAt < uiLimit)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void*)uiAt, &mbi, sizeof(mbi)))
        {
            break;
        }
        const unsigned int uiBase = (unsigned int)mbi.BaseAddress;
        const unsigned int uiEnd = uiBase + (unsigned int)mbi.RegionSize;
        if (mbi.State == MEM_FREE && (unsigned int)mbi.RegionSize > uiLargest)
        {
            uiLargest = (unsigned int)mbi.RegionSize;
        }
        if (uiEnd <= uiBase)
        {
            break;
        }
        uiAt = uiEnd;
    }
    return uiLargest;
}

void LogMemReport(const char* pcSource)
{
    char acLine[256];
    LOG(LogMemory, Inform, "OPENSWBF2 MEMREPORT source=%s", pcSource ? pcSource : "?");
    for (int i = 0; i < maxheaps; ++i)
    {
        RedHeapStats kStats;
        if (RedGetHeapStats(i, &kStats) && FormatHeapLine(RedGetHeapName(i), kStats, acLine, sizeof(acLine)))
        {
            LOG(LogMemory, Inform, "%s", acLine);
        }
    }
    for (PblList<MemoryPool>::Itor itor(*GetMemoryPoolList()); itor; ++itor)
    {
        if (itor->mPool == 0 && itor->GetPeak() == 0)
        {
            continue;
        }
        if (FormatPoolLine(itor->mLabel, itor->GetCount(), itor->GetSize(), itor->GetPeak(), itor->GetGrown(),
                           acLine, sizeof(acLine)))
        {
            LOG(LogMemory, Inform, "%s", acLine);
        }
    }
}

}
