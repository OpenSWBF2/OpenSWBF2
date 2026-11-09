#include <stddef.h>
#include <string.h>
#include "PebbleFL/Common/PblMath.h"

#include "RedMemory.h"
#include "RedMemoryBackend.h"
#include "PebbleFL/Common/PblAsan.h"

struct Tag
{
    Tag* prev_;
    Tag* next_;
    unsigned int size;

    void New() { prev_ = next_ = 0; }
};


struct TagList
{
    Tag* first;
    Tag* last;
};


struct HeapObj
{
    void* start;
    TagList free;
    TagList used;
    unsigned int size;
    const char* name;
    unsigned int align;
    bool frozen;
};

static_assert(sizeof(HeapObj) == kRedHeapObjBytes, "HeapObj must stay 36 bytes");
static_assert(sizeof(Tag) == kRedTagBytes, "Tag must match kRedTagBytes");


extern "C"
{
    extern bool isHeapObjInitialized;
    // GLOBAL: SWBF2 0x0093eba4
    // heapobj
    HeapObj* heapobj;
    // GLOBAL: SWBF2 0x0093ebb0
    int sRecursionDepth;
}

namespace RedMemory
{
    // GLOBAL: SWBF2 0x0093eb98
    int failHeap;
    // GLOBAL: SWBF2 0x0093eb9c
    unsigned int failSize;
}

extern "C" void Startup_RedInitHeap(int, int);

PBL_ASAN_NO_SANITIZE void TagAddList(TagList* list, Tag* tag);
PBL_ASAN_NO_SANITIZE void TagMergeIntoList(TagList* list, Tag* tag);
PBL_ASAN_NO_SANITIZE void TagRemoveList(TagList* list, Tag* tag);

// FUNCTION: SWBF2 0x006c35c0
PBL_ASAN_NO_SANITIZE void* RedAllocFromHeapInternal(int heap, size_t size, bool bTop, size_t* pReserved)
{
    if (!isHeapObjInitialized)
    {
        Startup_RedInitHeap(0x80, 0x80);
    }
    if (heap < 0 || heap >= maxheaps)
    {
        return 0;
    }

    HeapObj& h = heapobj[heap];
    unsigned int align = h.align;
    unsigned int mask = ~(align - 1);
    unsigned int hdr = (align + sizeof(Tag) - 1) & mask;
    if (size == 0)
    {
        size = 1;
    }
    unsigned int need = (size + align - 1 + hdr) & mask;

    Tag* tag;
    if (bTop)
    {
        for (tag = h.free.last; tag; tag = tag->prev_)
        {
            if (need <= tag->size)
            {
                break;
            }
        }
    }
    else
    {
        for (tag = h.free.first; tag; tag = tag->next_)
        {
            if (need <= tag->size)
            {
                break;
            }
        }
    }
    if (!tag)
    {
        if (sRecursionDepth <= 0)
        {
            RedMemory::failHeap = heap;
            RedMemory::failSize = need;
        }
        return 0;
    }

    unsigned int rest = tag->size - need;
    if (rest < h.align + hdr)
    {
        TagRemoveList(&h.free, tag);
    }
    else if (bTop)
    {
        tag->size = rest;
        tag = (Tag*)((char*)tag + rest);
        tag->next_ = 0;
        tag->prev_ = 0;
        tag->size = need;
    }
    else
    {
        Tag* split = (Tag*)((char*)tag + need);
        split->next_ = 0;
        split->prev_ = 0;
        split->size = 0;
        split->size = tag->size - need;
        split->prev_ = tag->prev_;
        split->next_ = tag->next_;
        tag->size = need;
        if (split->next_)
        {
            split->next_->prev_ = split;
        }
        else
        {
            h.free.last = split;
        }
        if (split->prev_)
        {
            split->prev_->next_ = split;
        }
        else
        {
            h.free.first = split;
        }
    }
    *pReserved = tag->size;
    TagAddList(&h.used, tag);
    PblAsanPoison(tag, need);
    void* pUser = (char*)tag + hdr;
    PblAsanUnpoison(pUser, size);
    return pUser;
}

// FUNCTION: SWBF2 0x006c3730
PBL_ASAN_NO_SANITIZE size_t RedFreeToHeapInternal(int heap, void* block)
{
    if (!isHeapObjInitialized)
    {
        return 0;
    }
    if (heap < 0 || heap >= maxheaps)
    {
        return 0;
    }

    HeapObj& h = heapobj[heap];
    if (block < h.start || block > (char*)h.start + h.size)
    {
        return 0;
    }
    unsigned int hdr = (h.align + sizeof(Tag) - 1) & ~(h.align - 1);
    Tag* tag = (Tag*)((char*)block - hdr);
    TagRemoveList(&h.used, tag);
    unsigned int uiBlockSize = tag->size;
    TagMergeIntoList(&h.free, tag);
    PblAsanPoison(tag, uiBlockSize);
    return uiBlockSize;
}

// FUNCTION: SWBF2 0x006c37b0
PBL_ASAN_NO_SANITIZE void TagAddList(TagList* list, Tag* tag)
{
    if (list->first)
    {
        list->first->prev_ = tag;
    }
    else
    {
        list->last = tag;
    }
    tag->prev_ = 0;
    tag->next_ = list->first;
    list->first = tag;
}

// FUNCTION: SWBF2 0x006c37f0
PBL_ASAN_NO_SANITIZE void TagMergeIntoList(TagList* list, Tag* tag)
{
    Tag* prev = 0;
    Tag* next = list->first;
    while (next && tag > next)
    {
        prev = next;
        next = next->next_;
    }
    tag->next_ = next;
    tag->prev_ = prev;

    if (next)
    {
        next->prev_ = tag;
        if ((char*)tag + tag->size == (char*)next)
        {
            tag->size += next->size;
            next = next->next_;
            tag->next_ = next;
            if (next)
            {
                next->prev_ = tag;
            }
            else
            {
                list->last = tag;
            }
        }
    }
    else
    {
        list->last = tag;
    }

    if (prev)
    {
        prev->next_ = tag;
        if ((char*)prev + prev->size == (char*)tag)
        {
            prev->size += tag->size;
            prev->next_ = next;
            if (next)
            {
                next->prev_ = prev;
            }
            else
            {
                list->last = prev;
            }
        }
    }
    else
    {
        list->first = tag;
    }
}

// FUNCTION: SWBF2 0x006c3880
PBL_ASAN_NO_SANITIZE void TagRemoveList(TagList* list, Tag* tag)
{
    if (tag->next_)
    {
        tag->next_->prev_ = tag->prev_;
    }
    else
    {
        list->last = tag->prev_;
    }
    if (tag->prev_)
    {
        tag->prev_->next_ = tag->next_;
    }
    else
    {
        list->first = tag->next_;
    }
    tag->prev_ = 0;
    tag->next_ = 0;
}

static void* RedArenaInit(void* start, void* end, int maxheaps)
{
    (void)end;
    HeapObj* heaps = (HeapObj*)(((unsigned int)start + 3) & ~3);
    heapobj = heaps;
    char* pNext = (char*)(((unsigned int)heaps + maxheaps * sizeof(HeapObj) + 3) & ~3);
    for (int i = 0; i < maxheaps; ++i)
    {
        HeapObj& h = heaps[i];
        h.start = 0;
        h.free.first = 0;
        h.free.last = 0;
        h.used.first = 0;
        h.used.last = 0;
        h.size = 0;
        h.align = 4;
        h.frozen = false;
        h.name = 0;
    }
    return pNext;
}

static PBL_ASAN_NO_SANITIZE char* ArenaClear(void* start, void* end, unsigned int align, unsigned int* puSize)
{
    unsigned int mask = ~(align - 1);
    char* s = (char*)(((unsigned int)start + align - 1) & mask);
    char* e = (char*)((unsigned int)end & mask);
    unsigned int size = e - s;
    PblAsanUnpoison(s, size);
    if (size)
    {
#ifdef OPENSWBF2_TEST
        memset(s, (RedPerturbModes() & kRedPerturbFill) != 0 ? 0xA5 : 0, size);
#else
        memset(s, 0, size);
#endif
    }
    *puSize = size;
    return s;
}

static PBL_ASAN_NO_SANITIZE size_t RedArenaCreate(int heap, void* start, void* end, unsigned int align, const char* name)
{
    unsigned int size = 0;
    char* s = ArenaClear(start, end, align, &size);
    if (heap < 0)
    {
        PblAsanPoison(s, size);
        return 0;
    }
    Tag* tag = (Tag*)s;
    tag->size = size;
    tag->New();

    HeapObj& h = heapobj[heap];
    h.start = s;
    h.free.first = tag;
    h.free.last = tag;
    h.size = size;
    h.used.first = 0;
    h.used.last = 0;
    h.align = align;
    h.name = name;
    PblAsanPoison(s, size);
    h.frozen = false;
    return size;
}

static void RedArenaDestroy(int heap)
{
    HeapObj& h = heapobj[heap];
    if (h.size)
    {
        PblAsanPoison(h.start, h.size);
    }
    h.free.first = 0;
    h.free.last = 0;
    h.used.first = 0;
    h.used.last = 0;
    h.size = 0;
    h.name = 0;
    h.align = 4;
    h.frozen = false;
}

static int RedArenaOwner(const void* block)
{
    int iOwner = -1;
    if (heapobj == 0)
    {
        return iOwner;
    }
    for (int i = 0; i < maxheaps; ++i)
    {
        const HeapObj& h = heapobj[i];
        if (h.size != 0 && block > h.start && block <= (const char*)h.start + h.size &&
            (iOwner < 0 || h.size < heapobj[iOwner].size))
        {
            iOwner = i;
        }
    }
    return iOwner;
}

PBL_ASAN_NO_SANITIZE unsigned int RedArenaFreeTagSum(int heap)
{
    unsigned int total = 0;
    for (Tag* tag = heapobj[heap].free.first; tag; tag = tag->next_)
    {
        total += tag->size;
    }
    return total;
}

static PBL_ASAN_NO_SANITIZE int RedArenaMoveHeapEnd(int iHeap, int iDelta, bool* pbAllocFailed)
{
    *pbAllocFailed = false;
    int iSource = iHeap;
    int iTarget = iHeap + 1;
    unsigned int uAmount = iDelta;
    if (iDelta < 0)
    {
        iSource = iHeap + 1;
        iTarget = iHeap;
        uAmount = -iDelta;
    }
    int sourceAlign = heapobj[iSource].align;
    int targetAlign = heapobj[iTarget].align;
    unsigned int uTagSize = (targetAlign + sizeof(Tag) - 1) & ~(targetAlign - 1);
    int align = Max(sourceAlign, targetAlign);
    unsigned int uMove = uAmount & ~(align - 1);
    if (uMove <= uTagSize)
    {
        return 0;
    }
    unsigned int uAllocation = uMove - uTagSize;
    size_t uReserved = 0;
    void* pBlock = RedAllocFromHeapInternal(iTarget, uAllocation, iDelta < 0, &uReserved);
    if (pBlock == 0)
    {
        *pbAllocFailed = true;
        return 0;
    }
    HeapObj* heaps = heapobj;
    Tag* pTag = (Tag*)((char*)pBlock - uTagSize);
    char* pcBoundary = (char*)heaps[iHeap + 1].start;
    bool bAtBoundary = iDelta < 0 ? (char*)pTag + pTag->size == pcBoundary : (char*)pTag == pcBoundary;
    if (!bAtBoundary || pTag->size != uMove)
    {
        RedFreeToHeapInternal(iTarget, pBlock);
        return 0;
    }
    iDelta = iDelta < 0 ? -(int)uMove : (int)uMove;
    heaps[iHeap].size += iDelta;
    heaps[iHeap + 1].start = (char*)heaps[iHeap + 1].start + iDelta;
    heaps[iHeap + 1].size -= iDelta;
    TagRemoveList(&heaps[iTarget].used, pTag);
    TagAddList(&heaps[iSource].used, pTag);
    RedFreeToHeapInternal(iSource, pBlock);
    return iDelta;
}

static PBL_ASAN_NO_SANITIZE bool RedArenaVisit(int heap, bool (*pfnBlock)(void* pUser, size_t uSize, void* pArg),
                                               void* pArg)
{
    if (heapobj == 0 || heap < 0 || heap >= maxheaps || heapobj[heap].size == 0)
    {
        return false;
    }
    const HeapObj& h = heapobj[heap];
    unsigned int hdr = (h.align + sizeof(Tag) - 1) & ~(h.align - 1);
    for (Tag* tag = h.used.first; tag; tag = tag->next_)
    {
        if (!pfnBlock((char*)tag + hdr, tag->size - hdr, pArg))
        {
            break;
        }
    }
    return true;
}

static void RedArenaTrim()
{
}

extern "C" const RedHeapBackend kRedArenaBackend = {
    RedArenaInit,
    RedArenaCreate,
    RedArenaDestroy,
    RedAllocFromHeapInternal,
    RedFreeToHeapInternal,
    RedArenaOwner,
    RedArenaMoveHeapEnd,
    RedArenaVisit,
    RedArenaTrim,
};
