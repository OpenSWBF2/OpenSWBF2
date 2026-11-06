#pragma once

#include "RedEngineFL/Memory/RedMemory.h"

namespace RedMemoryReport
{

struct MemFailInfo
{
    const char* pcHeap;
    int iHeap;
    unsigned int uSize;
    unsigned int uFree;
    unsigned int uLoad;
    unsigned int uPhysMB;
    unsigned int uVirtMB;
    unsigned int uUsed;
    unsigned int uCap;
    unsigned int uPeak;
    unsigned int uLargestFree;
};

bool FormatHeapLine(const char* pcName, const RedHeapStats& rStats, char* pcOut, unsigned int uSize);
bool FormatPoolLine(const char* pcLabel, unsigned int uCount, unsigned int uBlockSize, unsigned int uPeak,
                    unsigned int uGrown, char* pcOut, unsigned int uSize);
bool FormatPoolFull(const char* pcLabel, unsigned int uPeak, char* pcOut, unsigned int uSize);
bool FormatCloth(int iFree, bool bEnabled, int iLow, int iHigh, char* pcOut, unsigned int uSize);
bool FormatMemFail(const MemFailInfo& rInfo, char* pcOut, unsigned int uSize);
#ifdef OPENSWBF2_TEST
bool FormatOffThreadTotal(unsigned int uCount, unsigned int uSources, unsigned int uDropped, char* pcOut,
                          unsigned int uSize);
bool FormatOffThreadSource(const RedOffThreadSource& rSource, char* pcOut, unsigned int uSize);
#endif
unsigned int LargestFreeRegion();
void LogMemReport(const char* pcSource);

}
