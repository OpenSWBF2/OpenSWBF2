#include "MemFailLog.h"

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Util/RedWarning.h"

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
        RedWarning::SetLogData(RedWarning::RED_SEVERITY_INFORM);
        RedWarning::LogMessage("alloc failure heap %s (%d) size %u free %u\n",
            bValid ? RedGetHeapName(iHeap) : "?", iHeap, uiSize, bValid ? RedGetHeapFree(iHeap) : 0u);
    }
    else if (siCount == 16)
    {
        ++siCount;
        RedWarning::SetLogData(RedWarning::RED_SEVERITY_INFORM);
        RedWarning::LogMessage("further allocation failures suppressed\n");
    }
    sbInHandler = false;
}
