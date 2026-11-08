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
};

const int kRedHeapSlots = 16;
const unsigned int kRedTagBytes = 12;

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

extern "C" const RedHeapBackend kRedArenaBackend;
extern const RedHeapBackend* gRedHeapBackend;
extern RedHeapSlot gRedHeapSlots[kRedHeapSlots];
extern "C" int sRecursionDepth;

namespace RedMemory
{
    extern int failHeap;
    extern unsigned int failSize;
}

PBL_ASAN_NO_SANITIZE unsigned int RedArenaFreeTagSum(int heap);
bool RedArenaHeapRange(int heap, void** ppStart, unsigned int* puSize);
void RedArenaMarkPermanent(int heap, bool permanent);
PBL_ASAN_NO_SANITIZE int RedArenaMoveHeapEnd(int iHeap, int iDelta, bool* pbAllocFailed);
PBL_ASAN_NO_SANITIZE void RedArenaClearRange(void* start, void* end, unsigned int align);
