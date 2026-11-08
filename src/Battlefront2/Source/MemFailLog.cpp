#include <windows.h>

#include "MemFailLog.h"

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryReport.h"
#include "RedEngineFL/Util/RedWarning.h"
#include "RedEngineFL/Util/RedLog.h"

static bool sbInHandler;
static int siCount;

void MemFail::OnAllocFailure(int iHeap, unsigned int uiSize)
{
    if (sbInHandler)
    {
        return;
    }
    sbInHandler = true;
    if (siCount < 16)
    {
        ++siCount;
        MEMORYSTATUSEX kStatus;
        memset(&kStatus, 0, sizeof(kStatus));
        kStatus.dwLength = sizeof(kStatus);
        if (!GlobalMemoryStatusEx(&kStatus))
        {
            memset(&kStatus, 0, sizeof(kStatus));
        }
        RedMemoryReport::MemFailInfo kInfo;
        memset(&kInfo, 0, sizeof(kInfo));
        kInfo.pcHeap = RedGetHeapName(iHeap);
        kInfo.iHeap = iHeap;
        kInfo.uSize = uiSize;
        kInfo.uFree = RedGetHeapFree(iHeap);
        kInfo.uLoad = (unsigned int)kStatus.dwMemoryLoad;
        kInfo.uPhysMB = (unsigned int)(kStatus.ullAvailPhys >> 20);
        kInfo.uVirtMB = (unsigned int)(kStatus.ullAvailVirtual >> 20);
        RedHeapStats kStats;
        if (RedGetHeapStats(iHeap, &kStats))
        {
            kInfo.uUsed = kStats.uUsed;
            kInfo.uCap = kStats.uCap;
            kInfo.uPeak = kStats.uPeak;
        }
        kInfo.uLargestFree = RedMemoryReport::LargestFreeRegion();
        char acLine[512];
        RedMemoryReport::FormatMemFail(kInfo, acLine, sizeof(acLine));
        LOG(LogMemory, Error, "%s", acLine);
    }
    else if (siCount == 16)
    {
        ++siCount;
        LOG(LogMemory, Warning, "further allocation failures suppressed");
    }
    sbInHandler = false;
}
