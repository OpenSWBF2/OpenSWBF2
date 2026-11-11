#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblAsan.h"

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
const unsigned int kRedPerturbPad = 1;
const unsigned int kRedPerturbFill = 4;
const unsigned int kRedPerturbPool = 8;

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
    void* RedAllocFromHeap(int heap, size_t size, bool bTop);
    void* RedReallocInHeap(int heap, void* block, size_t oldSize, size_t size);
    void RedFreeToHeap(int heap, void* block, unsigned int flags);
    void RedMarkHeapPermanent(int heap, bool permanent);
    void RedSetTempHeap(int heap);

    unsigned int RedGetHeapFree(int heap);
    char const* RedGetHeapName(int heap);
    extern void (*OnFailure)(int heap, unsigned int size);
    void RedMoveHeapEnd(int heap, int offset);
    bool RedGetHeapStats(int heap, RedHeapStats* pOut);
    int RedHeapOwner(const void* p);
    bool RedVisitHeap(int heap, bool (*pfnBlock)(void* pUser, size_t uSize, void* pArg), void* pArg);
    void RedTrimHeaps();
#ifdef OPENSWBF2_TEST
    void RedGetOffThreadTotals(unsigned int* puCount, unsigned int* puSources, unsigned int* puDropped);
    bool RedGetOffThreadSource(int i, RedOffThreadSource* pOut);
    void RedResetOffThreadStats();
    bool RedImageStamp(const void* pBase, unsigned int* puStamp);
    void RedPerturbSet(unsigned int uSeed, unsigned int uModes);
    unsigned int RedPerturbModes();
    unsigned int RedPerturbSeed();
    bool RedPerturbParse(const char* pcSpec, unsigned int* puSeed, unsigned int* puModes);
    bool RedPerturbDraw(unsigned int uMode, unsigned int* puOut);
    void RedPerturbFormatModes(unsigned int uModes, char* pcOut, unsigned int uSize);
    bool RedProcessHeapUsage(void* hHeap, unsigned int* puBusy, unsigned int* puCommitted, unsigned int* puLarge);
    bool RedRangeCommitted(const void* pStart, unsigned int uSize, unsigned int* puCommitted);
    bool RedPrivateCommitted(unsigned int* puCommitted);
    bool RedHeapsCommitted(unsigned int* puCommitted);
    bool RedThreadsCommitted(unsigned int* puCommitted, unsigned int* puThreads);
#endif

    extern int maxheaps;
    extern int __RedCurrHeap;
    extern int __RedDebugHeap;
    extern int __RedTempHeap;
}

unsigned int RedGetHeapSize(int heap);
void RedPurgeHeapHandles(int heap, const char* name);
unsigned int RedCountStaleHandles();
long RedParseMiPurgeArg(const char* pcVal, int iLen, bool* pbRejected);
void RedRaiseHeapCap(int heap, unsigned int uBytes);
bool RedParseHeapCapArg(const char* pcVal, int iLen, int* piKnob, unsigned int* puBytes, bool* pbClamped);
void RedNoteHeapCap(int iKnob, unsigned int uBytes);
int RedHeapCapSlot(const char* pcKnob);
void RedHeapCapOpenWindow();
void RedHeapCapCloseWindow();
bool RedHeapCapWindowOpen();
