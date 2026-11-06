#include <windows.h>
#include <stdio.h>
#ifdef OPENSWBF2_TEST
#include <stdarg.h>
#include <string.h>
#endif

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

#ifdef OPENSWBF2_TEST
bool FormatOffThreadTotal(unsigned int uCount, unsigned int uSources, unsigned int uDropped, char* pcOut,
                          unsigned int uSize)
{
    if (!pcOut || uSize == 0)
    {
        return false;
    }
    _snprintf(pcOut, uSize, "OPENSWBF2 MEMREPORT offthread count=%u sources=%u dropped=%u", uCount, uSources,
              uDropped);
    pcOut[uSize - 1] = 0;
    return true;
}

static unsigned int AppendText(char* pcOut, unsigned int uSize, unsigned int uLen, const char* pcFormat, ...)
{
    if (uLen + 1 >= uSize)
    {
        return uLen;
    }
    va_list args;
    va_start(args, pcFormat);
    const int iWritten = _vsnprintf(pcOut + uLen, uSize - uLen, pcFormat, args);
    va_end(args);
    if (iWritten < 0 || (unsigned int)iWritten >= uSize - uLen)
    {
        return uSize - 1;
    }
    return uLen + (unsigned int)iWritten;
}

static void ModuleBaseName(void* pModule, unsigned int uStamp, char* pcOut, unsigned int uSize)
{
    wchar_t awcModule[MAX_PATH];
    if (pModule == 0)
    {
        lstrcpynA(pcOut, "abs", (int)uSize);
        return;
    }
    unsigned int uCurrent = 0;
    if (!RedImageStamp(pModule, &uCurrent) || uCurrent != uStamp)
    {
        lstrcpynA(pcOut, "stale", (int)uSize);
        return;
    }
    lstrcpynA(pcOut, "unknown", (int)uSize);
    const DWORD uiLen = GetModuleFileNameW((HMODULE)pModule, awcModule, MAX_PATH);
    if (uiLen == 0 || uiLen >= MAX_PATH)
    {
        return;
    }
    wchar_t* pwcBase = awcModule;
    for (wchar_t* pwc = awcModule; *pwc != 0; ++pwc)
    {
        if (*pwc == L'\\' || *pwc == L'/')
        {
            pwcBase = pwc + 1;
        }
    }
    if (WideCharToMultiByte(CP_ACP, 0, pwcBase, -1, pcOut, (int)uSize, 0, 0) == 0)
    {
        lstrcpynA(pcOut, "unknown", (int)uSize);
    }
    for (char* pc = pcOut; *pc != 0; ++pc)
    {
        if (*pc == ' ' || *pc == ',')
        {
            *pc = '_';
        }
    }
}

bool FormatOffThreadSource(const RedOffThreadSource& rSource, char* pcOut, unsigned int uSize)
{
    if (!pcOut || uSize == 0)
    {
        return false;
    }
    pcOut[0] = 0;
    unsigned int uLen = AppendText(pcOut, uSize, 0,
                                   "OPENSWBF2 MEMREPORT offthread tid=%u heap=%s size=%u count=%u stack=",
                                   rSource.uTid, rSource.pcHeap ? rSource.pcHeap : "?", rSource.uSize,
                                   rSource.uCount);
    unsigned int uFrames = rSource.uFrames;
    if (uFrames > (unsigned int)kRedOffThreadFrames)
    {
        uFrames = kRedOffThreadFrames;
    }
    if (uFrames == 0)
    {
        uLen = AppendText(pcOut, uSize, uLen, "-");
    }
    for (unsigned int f = 0; f < uFrames; ++f)
    {
        char acModule[MAX_PATH];
        ModuleBaseName(rSource.apModule[f], rSource.auStamp[f], acModule, sizeof(acModule));
        char acFrame[MAX_PATH + 16];
        _snprintf(acFrame, sizeof(acFrame), "%s%s+0x%X", f == 0 ? "" : ",", acModule, rSource.auOffset[f]);
        acFrame[sizeof(acFrame) - 1] = 0;
        const unsigned int uFrameLen = (unsigned int)strlen(acFrame);
        if (uLen + uFrameLen >= uSize)
        {
            break;
        }
        memcpy(pcOut + uLen, acFrame, uFrameLen + 1);
        uLen += uFrameLen;
    }
    pcOut[uSize - 1] = 0;
    return true;
}
#endif

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
#ifdef OPENSWBF2_TEST
    unsigned int uCount = 0;
    unsigned int uSources = 0;
    unsigned int uDropped = 0;
    RedGetOffThreadTotals(&uCount, &uSources, &uDropped);
    if (FormatOffThreadTotal(uCount, uSources, uDropped, acLine, sizeof(acLine)))
    {
        LOG(LogMemory, Inform, "%s", acLine);
    }
    char acSource[768];
    for (int i = 0; i < kRedOffThreadSources; ++i)
    {
        RedOffThreadSource kSource;
        if (RedGetOffThreadSource(i, &kSource) && FormatOffThreadSource(kSource, acSource, sizeof(acSource)))
        {
            LOG(LogMemory, Inform, "%s", acSource);
        }
    }
#endif
}

}
