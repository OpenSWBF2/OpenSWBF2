#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblAsan.h"

struct RedHeapBackend
{
    void* (*init)(void* start, void* end, int maxheaps);
    size_t (*create)(int heap, void* start, void* end, unsigned int align, const char* name);
    void (*destroy)(int heap);
    void* (*alloc)(int heap, size_t size, bool bTop, size_t* pReserved);
    size_t (*free)(int heap, void* block);
    int (*owner)(const void* block);
    int (*moveEnd)(int iHeap, int iDelta, bool* pbAllocFailed);
    bool (*visit)(int heap, bool (*pfnBlock)(void* pUser, size_t uSize, void* pArg), void* pArg);
    void (*trim)();
};

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
    const RedHeapBackend* pBackend;
};

extern "C" const RedHeapBackend kRedArenaBackend;
extern "C" const RedHeapBackend kRedMiBackend;
extern const RedHeapBackend* gRedHeapBackend;
extern RedHeapSlot gRedHeapSlots[kRedHeapSlots];
extern "C" int sRecursionDepth;

namespace RedMemory
{
    extern int failHeap;
    extern unsigned int failSize;
}

PBL_ASAN_NO_SANITIZE unsigned int RedArenaFreeTagSum(int heap);

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

RedMiBlockKind RedMiClassify(const void* pUser);
void RedMiConfigure(long lPurgeMs);
void RedMiRegisterOutput();
bool RedMiFormatOptions(char* pcOut, unsigned int uSize);
void RedMiOutputLines(const char* pcMsg, void (*pfnLine)(const char* pcLine, int iLen, void* pArg), void* pArg);
bool RedMiFirstArena(unsigned int* puBase, unsigned int* puSize);
void RedMiRecordReservation(const void* p);
void RedMiNoteOutput(const char* pcMsg);
bool RedMiTakeErrorFatal(int err);
