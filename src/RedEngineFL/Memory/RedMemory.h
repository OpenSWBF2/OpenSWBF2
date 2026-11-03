#pragma once

struct Tag;
struct TagList;
struct HeapObj;

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

    unsigned int RedGetHeapFree(int heap);
    bool RedMoveHeapEnd(int heap, int offset);

    extern int __RedCurrHeap;
    extern int __RedDebugHeap;
    extern int __RedTempHeap;
}
