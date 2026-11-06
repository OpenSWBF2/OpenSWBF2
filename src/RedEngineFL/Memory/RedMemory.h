#pragma once

#include "PebbleFL/Common/PblAsan.h"

struct Tag;
struct TagList;
struct HeapObj;

struct RedHeapStats
{
    unsigned int uCap;
    unsigned int uUsed;
    unsigned int uPeak;
    unsigned int uAllocs;
    unsigned int uFailed;
};

#ifdef OPENSWBF2_TEST
const int kRedOffThreadSources = 16;
const int kRedOffThreadFrames = 8;

struct RedOffThreadSource
{
    unsigned int uTid;
    const char* pcHeap;
    unsigned int uSize;
    unsigned int uCount;
    unsigned int uFrames;
    void* apModule[kRedOffThreadFrames];
    unsigned int auStamp[kRedOffThreadFrames];
    unsigned int auOffset[kRedOffThreadFrames];
};
#endif

extern "C"
{
    void* RedInitHeap(void* start, void* end, int maxheaps);
    int RedCreateHeap(void* start, void* end, unsigned int align, char const* name);
    void RedDestroyHeap(int heap);
    int RedSetCurrentHeap(int heap);
    void RedSetDebugHeap(int heap);
    void* RedAllocFromHeap(int heap, unsigned int size, bool bTop);
    void RedFreeToHeap(int heap, void* block, unsigned int flags);
    void RedMarkHeapPermanent(int heap, bool permanent);
    void RedSetTempHeap(int heap);

    PBL_ASAN_NO_SANITIZE unsigned int RedGetHeapFree(int heap);
    char const* RedGetHeapName(int heap);
    extern void (*OnFailure)(int heap, unsigned int size);
    PBL_ASAN_NO_SANITIZE void RedMoveHeapEnd(int heap, int offset);
    bool RedGetHeapStats(int heap, RedHeapStats* pOut);
#ifdef OPENSWBF2_TEST
    void RedGetOffThreadTotals(unsigned int* puCount, unsigned int* puSources, unsigned int* puDropped);
    bool RedGetOffThreadSource(int i, RedOffThreadSource* pOut);
    void RedResetOffThreadStats();
    bool RedImageStamp(const void* pBase, unsigned int* puStamp);
#endif

    extern int maxheaps;
    extern int __RedCurrHeap;
    extern int __RedDebugHeap;
    extern int __RedTempHeap;
}

unsigned int RedGetHeapSize(int heap);
