#include "RedEngineFL/Memory/RedMemory.h"

extern "C"
{
    HeapObj* heapobj;
    int maxheaps;
    void (*OnFailure)(int heap, unsigned int size);

    unsigned int RedGetHeapFree(int heap)
    {
        return heapobj[heap].size;
    }

    char const* RedGetHeapName(int heap)
    {
        if (heap >= 0 && heap < maxheaps && heapobj[heap].name)
        {
            return heapobj[heap].name;
        }
        return "?";
    }
}

#include <stdlib.h>

extern "C"
{
    int __RedCurrHeap;
    unsigned int gFakeHeapBudget = 0xFFFFFFFFu;
    unsigned int gFakeHeapFrees;

    int RedSetCurrentHeap(int heap)
    {
        int iOld = __RedCurrHeap;
        __RedCurrHeap = heap;
        return iOld;
    }

    void* RedAllocFromHeap(int heap, unsigned int size, int bTop)
    {
        (void)heap;
        (void)bTop;
        return size <= gFakeHeapBudget ? malloc(size ? size : 1) : 0;
    }

    void RedFreeToHeap(int heap, void* block, int flags)
    {
        (void)heap;
        (void)flags;
        ++gFakeHeapFrees;
        free(block);
    }
}

namespace GameMemory { int LuaHeap = 3; }
