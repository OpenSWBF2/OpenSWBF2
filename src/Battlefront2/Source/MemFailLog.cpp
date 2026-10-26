#include <windows.h>

#include "MemFailLog.h"

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Util/RedWarning.h"
#include "RedEngineFL/Util/RedLog.h"

extern "C" int maxheaps;

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
        bool bValid = iHeap >= 0 && iHeap < maxheaps;
        MEMORYSTATUSEX kStatus;
        memset(&kStatus, 0, sizeof(kStatus));
        kStatus.dwLength = sizeof(kStatus);
        if (!GlobalMemoryStatusEx(&kStatus))
        {
            memset(&kStatus, 0, sizeof(kStatus));
        }
        LOG(LogMemory, Error, "alloc failure heap %s (%d) size %u free %u load %u%% phys %u MB virt %u MB",
            bValid ? RedGetHeapName(iHeap) : "?", iHeap, uiSize, bValid ? RedGetHeapFree(iHeap) : 0u,
            (unsigned int)kStatus.dwMemoryLoad, (unsigned int)(kStatus.ullAvailPhys >> 20),
            (unsigned int)(kStatus.ullAvailVirtual >> 20));
    }
    else if (siCount == 16)
    {
        ++siCount;
        LOG(LogMemory, Warning, "further allocation failures suppressed");
    }
    sbInHandler = false;
}
