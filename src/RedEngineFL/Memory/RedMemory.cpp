#include <intrin.h>
#include <stdlib.h>
#include "PebbleFL/Common/PblMath.h"
#include <string.h>

#include "RedMemory.h"
#include "RedEngineFL/Memory/engineAlloc.h"
#include "PebbleFL/Common/PblAsan.h"

#ifdef OPENSWBF2_TEST
#include <windows.h>
#endif

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
    bool permanent;
};


extern "C"
{
    // GLOBAL: SWBF2 0x0093eba1
    // isHeapObjInitialized
    bool isHeapObjInitialized;
    // GLOBAL: SWBF2 0x0093eba4
    // heapobj
    HeapObj* heapobj;
    // GLOBAL: SWBF2 0x0093eba8
    // maxheaps
    int maxheaps;
    // GLOBAL: SWBF2 0x0093ebac
    // __RedCurrHeap
    int __RedCurrHeap;
    // GLOBAL: SWBF2 0x007dee30
    // __RedDebugHeap
    int __RedDebugHeap = -1;
    // GLOBAL: SWBF2 0x007dee34
    // __RedTempHeap
    int __RedTempHeap = -1;
    // GLOBAL: SWBF2 0x0093eb94
    // OnFailure
    void (*OnFailure)(int heap, unsigned int size);
    // GLOBAL: SWBF2 0x0093ebb0
    int sRecursionDepth;
}

// FUNCTION: SWBF2 0x006c3a50
extern "C" PBL_ASAN_NO_SANITIZE unsigned int RedGetHeapFree(int heap)
{
    unsigned int total = 0;
    for (Tag* tag = heapobj[heap].free.first; tag; tag = tag->next_)
    {
        total += tag->size;
    }
    return total;
}

extern "C" char const* RedGetHeapName(int heap)
{
    if (heap >= 0 && heap < maxheaps && heapobj[heap].name)
    {
        return heapobj[heap].name;
    }
    return "?";
}

static const int kStatsHeaps = 16;
static volatile long sHeapUsed[kStatsHeaps];
static volatile long sHeapPeak[kStatsHeaps];
static volatile long sHeapAllocs[kStatsHeaps];
static volatile long sHeapFailed[kStatsHeaps];

static bool HeapStatsSlot(int heap)
{
    return heap >= 0 && heap < kStatsHeaps && heap < maxheaps;
}

static void HeapStatsAddUsed(int heap, long lDelta)
{
    long lUsed = _InterlockedExchangeAdd(&sHeapUsed[heap], lDelta) + lDelta;
    long lPeak = sHeapPeak[heap];
    while (lUsed > lPeak)
    {
        long lSeen = _InterlockedCompareExchange(&sHeapPeak[heap], lUsed, lPeak);
        if (lSeen == lPeak)
        {
            break;
        }
        lPeak = lSeen;
    }
}

static void HeapStatsReset(int heap)
{
    if (HeapStatsSlot(heap))
    {
        _InterlockedExchange(&sHeapUsed[heap], 0);
        _InterlockedExchange(&sHeapPeak[heap], 0);
        _InterlockedExchange(&sHeapAllocs[heap], 0);
        _InterlockedExchange(&sHeapFailed[heap], 0);
    }
}

#ifdef OPENSWBF2_TEST
struct OffThreadSlot
{
    volatile long lKey;
    volatile long lReady;
    volatile long lCount;
    unsigned int uTid;
    int iHeap;
    const char* pcHeap;
    unsigned int uSize;
    unsigned int uFrames;
    void* apFrame[kRedOffThreadFrames];
    void* apModule[kRedOffThreadFrames];
    unsigned int auStamp[kRedOffThreadFrames];
    unsigned int auOffset[kRedOffThreadFrames];
};

static volatile long sMainTid;
static volatile long sOffCount;
static volatile long sOffDropped;
static OffThreadSlot sOffSources[kRedOffThreadSources];

extern "C" bool RedImageStamp(const void* pBase, unsigned int* puStamp)
{
    MEMORY_BASIC_INFORMATION kInfo;
    if (pBase == 0 || puStamp == 0 || VirtualQuery(pBase, &kInfo, sizeof(kInfo)) != sizeof(kInfo) ||
        kInfo.Type != MEM_IMAGE || kInfo.State != MEM_COMMIT || kInfo.AllocationBase != pBase ||
        (kInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 || kInfo.RegionSize < sizeof(IMAGE_DOS_HEADER))
    {
        return false;
    }
    const IMAGE_DOS_HEADER* pDos = (const IMAGE_DOS_HEADER*)pBase;
    if (pDos->e_magic != IMAGE_DOS_SIGNATURE || pDos->e_lfanew < 0 ||
        (SIZE_T)pDos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > kInfo.RegionSize)
    {
        return false;
    }
    const IMAGE_NT_HEADERS* pNt = (const IMAGE_NT_HEADERS*)((const char*)pBase + pDos->e_lfanew);
    if (pNt->Signature != IMAGE_NT_SIGNATURE)
    {
        return false;
    }
    *puStamp = pNt->FileHeader.TimeDateStamp;
    return true;
}

static bool OffThreadSlotMatches(const OffThreadSlot& rSlot, unsigned int uiTid, int heap, void* const* apv,
                                 unsigned int uFrames)
{
    if (rSlot.uTid != uiTid || rSlot.iHeap != heap || rSlot.uFrames != uFrames)
    {
        return false;
    }
    for (unsigned int f = 0; f < uFrames; ++f)
    {
        if (rSlot.apFrame[f] != apv[f])
        {
            return false;
        }
    }
    return true;
}

static void RedNoteOffThread(int heap, unsigned int size, unsigned int uiTid)
{
    void* apv[kRedOffThreadFrames];
    DWORD ulHash = 0;
    unsigned int uFrames = CaptureStackBackTrace(1, kRedOffThreadFrames, apv, &ulHash);
    long lKey = (long)(ulHash ^ uiTid ^ (unsigned int)heap);
    if (lKey == 0)
    {
        lKey = 1;
    }
    bool bRecorded = false;
    for (int i = 0; i < kRedOffThreadSources && !bRecorded; ++i)
    {
        OffThreadSlot& rSlot = sOffSources[i];
        long lSlotKey = rSlot.lKey;
        if (lSlotKey == 0)
        {
            lSlotKey = _InterlockedCompareExchange(&rSlot.lKey, lKey, 0);
        }
        if (lSlotKey != 0)
        {
            if (lSlotKey == lKey && rSlot.lReady != 0 && OffThreadSlotMatches(rSlot, uiTid, heap, apv, uFrames))
            {
                _InterlockedIncrement(&rSlot.lCount);
                bRecorded = true;
            }
        }
        else
        {
            rSlot.uTid = uiTid;
            rSlot.iHeap = heap;
            rSlot.pcHeap = RedGetHeapName(heap);
            rSlot.uSize = size;
            rSlot.uFrames = uFrames;
            for (unsigned int f = 0; f < uFrames; ++f)
            {
                MEMORY_BASIC_INFORMATION kInfo;
                unsigned int uiStamp = 0;
                const unsigned int uiAddr = (unsigned int)(UINT_PTR)apv[f];
                rSlot.apFrame[f] = apv[f];
                if (VirtualQuery(apv[f], &kInfo, sizeof(kInfo)) == sizeof(kInfo) && kInfo.Type == MEM_IMAGE &&
                    RedImageStamp(kInfo.AllocationBase, &uiStamp))
                {
                    rSlot.apModule[f] = kInfo.AllocationBase;
                    rSlot.auStamp[f] = uiStamp;
                    rSlot.auOffset[f] = uiAddr - (unsigned int)(UINT_PTR)kInfo.AllocationBase;
                }
                else
                {
                    rSlot.apModule[f] = 0;
                    rSlot.auStamp[f] = 0;
                    rSlot.auOffset[f] = uiAddr;
                }
            }
            _InterlockedIncrement(&rSlot.lCount);
            _InterlockedExchange(&rSlot.lReady, 1);
            bRecorded = true;
        }
    }
    if (!bRecorded)
    {
        _InterlockedIncrement(&sOffDropped);
    }
    _InterlockedIncrement(&sOffCount);
}
#endif

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

// FUNCTION: SWBF2 0x006c3540
void* __cdecl operator new(size_t size)
{
    return RedAllocFromHeap(__RedCurrHeap, size, 0);
}

// FUNCTION: SWBF2 0x006c3560
void __cdecl operator delete(void* block)
{
    if (block)
    {
        RedFreeToHeap(__RedCurrHeap, block, 0);
    }
}

// FUNCTION: SWBF2 0x006c3580
void* __cdecl operator new[](size_t size)
{
    return RedAllocFromHeap(__RedCurrHeap, size, 0);
}

// FUNCTION: SWBF2 0x006c35a0
void __cdecl operator delete[](void* block)
{
    if (block)
    {
        RedFreeToHeap(__RedCurrHeap, block, 0);
    }
}

void __cdecl operator delete(void* block, size_t)
{
    if (block)
    {
        RedFreeToHeap(__RedCurrHeap, block, 0);
    }
}

void __cdecl operator delete[](void* block, size_t)
{
    if (block)
    {
        RedFreeToHeap(__RedCurrHeap, block, 0);
    }
}

// FUNCTION: SWBF2 0x006c35c0
PBL_ASAN_NO_SANITIZE void* RedAllocFromHeapInternal(int heap, unsigned int size, bool bTop)
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
        if (HeapStatsSlot(heap))
        {
            _InterlockedIncrement(&sHeapFailed[heap]);
        }
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
    if (HeapStatsSlot(heap))
    {
        HeapStatsAddUsed(heap, (long)tag->size);
        _InterlockedIncrement(&sHeapAllocs[heap]);
    }
    TagAddList(&h.used, tag);
    PblAsanPoison(tag, need);
    void* pUser = (char*)tag + hdr;
    PblAsanUnpoison(pUser, size);
    return pUser;
}

// FUNCTION: SWBF2 0x006c3730
PBL_ASAN_NO_SANITIZE bool RedFreeToHeapInternal(int heap, void* block, unsigned int flags)
{
    (void)flags;
    if (!isHeapObjInitialized)
    {
        return true;
    }
    if (heap < 0 || heap >= maxheaps)
    {
        return false;
    }

    HeapObj& h = heapobj[heap];
    if (block < h.start || block > (char*)h.start + h.size)
    {
        return false;
    }
    unsigned int hdr = (h.align + sizeof(Tag) - 1) & ~(h.align - 1);
    Tag* tag = (Tag*)((char*)block - hdr);
    TagRemoveList(&h.used, tag);
    unsigned int uiBlockSize = tag->size;
    if (HeapStatsSlot(heap))
    {
        _InterlockedExchangeAdd(&sHeapUsed[heap], -(long)uiBlockSize);
    }
    TagMergeIntoList(&h.free, tag);
    PblAsanPoison(tag, uiBlockSize);
    return true;
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

// FUNCTION: SWBF2 0x006c38d0
void* RedAllocFromHeap(int heap, unsigned int size, bool bTop)
{
    void* p = RedAllocFromHeapInternal(heap, size, bTop);
#ifdef OPENSWBF2_TEST
    DWORD uiTid = GetCurrentThreadId();
    if (sMainTid != 0 && uiTid != (DWORD)sMainTid)
    {
        RedNoteOffThread(heap, size, uiTid);
    }
#endif
    if (!p)
    {
        if (OnFailure)
        {
            OnFailure(heap, size);
        }
        return 0;
    }
    return p;
}

// FUNCTION: SWBF2 0x006c3900
int RedCreateHeap(void* start, void* end, unsigned int align, char const* name)
{
    unsigned int mask = ~(align - 1);
    char* s = (char*)(((unsigned int)start + align - 1) & mask);
    char* e = (char*)((unsigned int)end & mask);
    unsigned int size = e - s;
    PblAsanUnpoison(s, size);
    if (size)
    {
        memset(s, 0, size);
    }

    for (int i = 0; i < maxheaps; ++i)
    {
        if (heapobj[i].size == 0)
        {
            Tag* tag = (Tag*)s;
            tag->size = size;
            tag->New();

            HeapObj& heap = heapobj[i];
            heap.start = s;
            heap.free.first = tag;
            heap.free.last = tag;
            heap.size = size;
            heap.used.first = 0;
            heap.used.last = 0;
            heap.align = align;
            heap.name = name;
            PblAsanPoison(s, size);
            heap.frozen = false;
            HeapStatsReset(i);
            return i;
        }
    }
    PblAsanPoison(s, size);
    return -1;
}

// FUNCTION: SWBF2 0x006c39b0
void RedDestroyHeap(int heap)
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

// FUNCTION: SWBF2 0x006c3a00
void RedFreeToHeap(int heap, void* block, unsigned int flags)
{
    if (RedFreeToHeapInternal(heap, block, flags))
    {
        return;
    }
    int numHeaps = maxheaps;
    for (int i = 0; i < numHeaps; ++i)
    {
        if (RedFreeToHeapInternal(i, block, flags))
        {
            return;
        }
    }
    free(block);
}

// FUNCTION: SWBF2 0x006c3aa0
void* RedInitHeap(void* start, void* end, int maxheaps)
{
    (void)end;
    ::maxheaps = maxheaps;
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
#ifdef OPENSWBF2_TEST
    _InterlockedCompareExchange(&sMainTid, (long)GetCurrentThreadId(), 0);
#endif
    isHeapObjInitialized = true;
    return pNext;
}

// FUNCTION: SWBF2 0x006c3b20
void RedMarkHeapPermanent(int heap, bool permanent)
{
    heapobj[heap].permanent = permanent;
}

// FUNCTION: SWBF2 0x006c3c10
int RedSetCurrentHeap(int heap)
{
    if (heap >= 0 && heap < maxheaps)
    {
        int old = __RedCurrHeap;
        __RedCurrHeap = heap;
        return old;
    }
    return __RedCurrHeap;
}

// FUNCTION: SWBF2 0x006c3c40
void RedSetDebugHeap(int heap)
{
    __RedDebugHeap = heap;
}

// FUNCTION: SWBF2 0x006c3c50
void RedSetTempHeap(int heap)
{
    __RedTempHeap = heap;
}

// FUNCTION: SWBF2 0x006c3a80
unsigned int RedGetHeapSize(int heap)
{
    return heapobj[heap].size;
}

// FUNCTION: SWBF2 0x006c3b40
PBL_ASAN_NO_SANITIZE void RedMoveHeapEnd(int iHeap, int iDelta)
{
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
        return;
    }
    unsigned int uAllocation = uMove - uTagSize;
    long lTargetPeak = HeapStatsSlot(iTarget) ? sHeapPeak[iTarget] : 0;
    void* pBlock = RedAllocFromHeapInternal(iTarget, uAllocation, iDelta < 0);
    if (pBlock == 0)
    {
        return;
    }
    HeapObj* heaps = heapobj;
    Tag* pTag = (Tag*)((char*)pBlock - uTagSize);
    char* pcBoundary = (char*)heaps[iHeap + 1].start;
    bool bAtBoundary = iDelta < 0 ? (char*)pTag + pTag->size == pcBoundary : (char*)pTag == pcBoundary;
    if (!bAtBoundary || pTag->size != uMove)
    {
        RedFreeToHeapInternal(iTarget, pBlock, 0);
        if (HeapStatsSlot(iTarget))
        {
            _InterlockedExchangeAdd(&sHeapAllocs[iTarget], -1);
            _InterlockedExchange(&sHeapPeak[iTarget], lTargetPeak);
        }
        return;
    }
    iDelta = iDelta < 0 ? -(int)uMove : (int)uMove;
    if (HeapStatsSlot(iTarget))
    {
        _InterlockedExchangeAdd(&sHeapUsed[iTarget], -(long)pTag->size);
        _InterlockedExchangeAdd(&sHeapAllocs[iTarget], -1);
        _InterlockedExchange(&sHeapPeak[iTarget], lTargetPeak);
    }
    if (HeapStatsSlot(iSource))
    {
        _InterlockedExchangeAdd(&sHeapUsed[iSource], (long)pTag->size);
    }
    heaps[iHeap].size += iDelta;
    heaps[iHeap + 1].start = (char*)heaps[iHeap + 1].start + iDelta;
    heaps[iHeap + 1].size -= iDelta;
    TagRemoveList(&heaps[iTarget].used, pTag);
    TagAddList(&heaps[iSource].used, pTag);
    RedFreeToHeapInternal(iSource, pBlock, 0);
}

extern "C" bool RedGetHeapStats(int heap, RedHeapStats* pOut)
{
    if (pOut == 0 || !HeapStatsSlot(heap) || heapobj[heap].size == 0)
    {
        return false;
    }
    pOut->uCap = RedGetHeapSize(heap);
    pOut->uUsed = (unsigned int)sHeapUsed[heap];
    pOut->uPeak = (unsigned int)sHeapPeak[heap];
    pOut->uAllocs = (unsigned int)sHeapAllocs[heap];
    pOut->uFailed = (unsigned int)sHeapFailed[heap];
    return true;
}

#ifdef OPENSWBF2_TEST
extern "C" void RedGetOffThreadTotals(unsigned int* puCount, unsigned int* puSources, unsigned int* puDropped)
{
    if (puCount)
    {
        *puCount = (unsigned int)sOffCount;
    }
    if (puSources)
    {
        unsigned int uSources = 0;
        for (int i = 0; i < kRedOffThreadSources; ++i)
        {
            if (sOffSources[i].lReady != 0)
            {
                ++uSources;
            }
        }
        *puSources = uSources;
    }
    if (puDropped)
    {
        *puDropped = (unsigned int)sOffDropped;
    }
}

extern "C" bool RedGetOffThreadSource(int i, RedOffThreadSource* pOut)
{
    if (pOut == 0 || i < 0 || i >= kRedOffThreadSources || sOffSources[i].lReady == 0)
    {
        return false;
    }
    const OffThreadSlot& rSlot = sOffSources[i];
    pOut->uTid = rSlot.uTid;
    pOut->pcHeap = rSlot.pcHeap;
    pOut->uSize = rSlot.uSize;
    pOut->uCount = (unsigned int)rSlot.lCount;
    pOut->uFrames = rSlot.uFrames;
    for (int f = 0; f < kRedOffThreadFrames; ++f)
    {
        pOut->apModule[f] = rSlot.apModule[f];
        pOut->auStamp[f] = rSlot.auStamp[f];
        pOut->auOffset[f] = rSlot.auOffset[f];
    }
    return true;
}

extern "C" void RedResetOffThreadStats()
{
    for (int i = 0; i < kRedOffThreadSources; ++i)
    {
        OffThreadSlot& rSlot = sOffSources[i];
        _InterlockedExchange(&rSlot.lReady, 0);
        _InterlockedExchange(&rSlot.lCount, 0);
        rSlot.uTid = 0;
        rSlot.iHeap = 0;
        rSlot.pcHeap = 0;
        rSlot.uSize = 0;
        rSlot.uFrames = 0;
        for (int f = 0; f < kRedOffThreadFrames; ++f)
        {
            rSlot.apFrame[f] = 0;
            rSlot.apModule[f] = 0;
            rSlot.auStamp[f] = 0;
            rSlot.auOffset[f] = 0;
        }
        _InterlockedExchange(&rSlot.lKey, 0);
    }
    _InterlockedExchange(&sOffCount, 0);
    _InterlockedExchange(&sOffDropped, 0);
}
#endif
