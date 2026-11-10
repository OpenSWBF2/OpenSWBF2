#pragma once

#include <stddef.h>
#include <stdint.h>

const int kRedHeapSlots = 16;
const unsigned int kRedTagBytes = 12;
const unsigned int kRedHeapObjBytes = 36;
const int kRedOwnerStale = -2;

struct RedHeapSlot
{
    const char* name;
    unsigned int size;
    unsigned int align;
    unsigned int cap;
    unsigned int defaultCap;
    volatile long used;
    volatile long peak;
    volatile long allocs;
    volatile long failed;
    bool bLive;
};

extern RedHeapSlot gRedHeapSlots[kRedHeapSlots];

namespace RedMemory
{
    extern int failHeap;
    extern unsigned int failSize;
}

struct RedMiPrefix
{
    unsigned int uSize;
    unsigned int uCharge;
    unsigned int uSlotGen;
    unsigned int uCookie;
};

enum RedMiBlockKind
{
    kRedMiValid,
    kRedMiBadPrefix
};

void* RedMiInit(void* start, int maxheaps);
size_t RedMiCreate(int heap, void* start, void* end, unsigned int align);
void RedMiDestroy(int heap);
void RedMiFailHeapCreate(bool bFail);
void* RedMiAlloc(int heap, size_t size, size_t* pReserved);
size_t RedMiFree(void* block);
int RedMiOwner(const void* block);
int RedMiMoveEnd(int iHeap, int iDelta, bool* pbAllocFailed);
bool RedMiVisit(int heap, bool (*pfnBlock)(void* pUser, size_t uSize, void* pArg), void* pArg);
void RedMiTrim();
void* RedMiRealloc(int heap, void* block, size_t oldSize, size_t size, size_t* pReserved, size_t* pOldCharge);
RedMiBlockKind RedMiClassify(const void* pUser);
void RedMiConfigure(long lPurgeMs);
void RedMiRegisterOutput();
bool RedMiFormatOptions(char* pcOut, unsigned int uSize);
void RedMiOutputLines(const char* pcMsg, void (*pfnLine)(const char* pcLine, int iLen, void* pArg), void* pArg);
bool RedMiFirstArena(uintptr_t* puBase, size_t* puSize);
void RedMiRecordReservation(const void* p);
void RedMiNoteOutput(const char* pcMsg);
bool RedMiTakeErrorFatal(int err);
