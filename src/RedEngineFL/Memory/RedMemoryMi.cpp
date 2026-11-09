#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "RedMemory.h"
#include "RedMemoryBackend.h"
#include "PebbleFL/Common/PblAsan.h"
#include "RedEngineFL/Util/RedLog.h"
#include "mimalloc/include/mimalloc.h"

static const unsigned int kMiPrefixBytes = sizeof(RedMiPrefix);
static const unsigned int kMiCookie = 0x5EA1C0DEu;

static_assert(sizeof(RedMiPrefix) == 16, "RedMiPrefix must stay 16 bytes");

static mi_heap_t* sMiHeap[kRedHeapSlots];
static unsigned int sMiGen[kRedHeapSlots];
static volatile LONG salMiResBits[0x10000 / 32];
static volatile LONG slMiFirstBase;
static unsigned int suMiFirstSize;
#if defined(__SANITIZE_ADDRESS__)
static const int kMiAreasFirst = 64;

struct MiArea
{
    void* pBlocks;
    size_t uBytes;
};

static MiArea* spMiArea;
static int siMiAreaCap;
static int siMiAreas;
static int siMiAreasLost;
#endif
static __declspec(thread) bool stbMiExhaustionText;

struct MiVisitArg
{
    int iHeap;
    bool (*pfnBlock)(void* pUser, size_t uSize, void* pArg);
    void* pArg;
};

static PBL_ASAN_NO_SANITIZE void MiReadPrefix(const char* pBase, RedMiPrefix* pOut)
{
    const volatile unsigned int* puWords = (const volatile unsigned int*)pBase;
    pOut->uSize = puWords[0];
    pOut->uCharge = puWords[1];
    pOut->uSlotGen = puWords[2];
    pOut->uCookie = puWords[3];
}

static unsigned int MiCookie(const char* pBase, unsigned int uSize, unsigned int uCharge)
{
    return (unsigned int)pBase ^ kMiCookie ^ uCharge ^ (uSize * 0x9E3779B1u);
}

static bool MiInReservation(const void* p)
{
    const unsigned int uIdx = (unsigned int)p >> 16;
    return (salMiResBits[uIdx >> 5] & (1u << (uIdx & 31))) != 0;
}

static bool MiIsCrtBlock(const void* p)
{
#if defined(__SANITIZE_ADDRESS__)
    (void)p;
    return false;
#else
    HANDLE hHeap = GetProcessHeap();
    if (!HeapLock(hHeap))
    {
        return false;
    }
    PROCESS_HEAP_ENTRY kEntry;
    kEntry.lpData = 0;
    bool bFound = false;
    while (!bFound && HeapWalk(hHeap, &kEntry))
    {
        bFound = (kEntry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0 && kEntry.lpData == p;
    }
    HeapUnlock(hHeap);
    return bFound;
#endif
}

void RedMiRecordReservation(const void* p)
{
    if (MiInReservation(p))
    {
        return;
    }
    MEMORY_BASIC_INFORMATION kInfo;
    if (VirtualQuery(p, &kInfo, sizeof(kInfo)) != sizeof(kInfo) || kInfo.AllocationBase == 0)
    {
        return;
    }
    const unsigned int uBase = (unsigned int)kInfo.AllocationBase;
    unsigned int uAt = uBase;
    while (VirtualQuery((const void*)uAt, &kInfo, sizeof(kInfo)) == sizeof(kInfo) &&
           (unsigned int)kInfo.AllocationBase == uBase && kInfo.RegionSize != 0 && uAt + kInfo.RegionSize > uAt)
    {
        uAt += kInfo.RegionSize;
    }
    const unsigned int uFirst = uBase >> 16;
    const unsigned int uLast = (uAt - 1) >> 16;
    for (unsigned int uIdx = uFirst; uIdx <= uLast; ++uIdx)
    {
        InterlockedBitTestAndSet(&salMiResBits[uIdx >> 5], uIdx & 31);
    }
    if (InterlockedCompareExchange(&slMiFirstBase, (LONG)uBase, 0) == 0)
    {
        suMiFirstSize = uAt - uBase;
    }
}

#if defined(__SANITIZE_ADDRESS__)
static bool MiGrowAreas()
{
    const int iCap = siMiAreaCap ? siMiAreaCap * 2 : kMiAreasFirst;
    MiArea* pNew = (MiArea*)VirtualAlloc(0, iCap * sizeof(MiArea), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (pNew == 0)
    {
        return false;
    }
    if (spMiArea)
    {
        memcpy(pNew, spMiArea, siMiAreas * sizeof(MiArea));
        VirtualFree(spMiArea, 0, MEM_RELEASE);
    }
    spMiArea = pNew;
    siMiAreaCap = iCap;
    return true;
}

static bool mi_cdecl MiPoisonArea(const mi_heap_t*, const mi_heap_area_t* pArea, void* pBlock, size_t, void*)
{
    if (pBlock != 0)
    {
        return true;
    }
    if (siMiAreas == siMiAreaCap && !MiGrowAreas())
    {
        ++siMiAreasLost;
        return true;
    }
    spMiArea[siMiAreas].pBlocks = pArea->blocks;
    spMiArea[siMiAreas].uBytes = pArea->committed;
    ++siMiAreas;
    return true;
}
#endif

static void* MiInit(void* start, void* end, int maxheaps)
{
    (void)end;
    const unsigned int uHeaps = ((unsigned int)start + 3) & ~3u;
    return (void*)((uHeaps + maxheaps * kRedHeapObjBytes + 3) & ~3u);
}

static void MiDestroy(int i)
{
    if (i < 0 || i >= kRedHeapSlots || sMiHeap[i] == 0)
    {
        return;
    }
    mi_heap_t* pHeap = sMiHeap[i];
#if defined(__SANITIZE_ADDRESS__)
    siMiAreas = 0;
    siMiAreasLost = 0;
    mi_heap_visit_blocks(sMiHeap[i], false, MiPoisonArea, 0);
#endif
    mi_heap_destroy(pHeap);
#if defined(__SANITIZE_ADDRESS__)
    for (int a = 0; a < siMiAreas; ++a)
    {
        PblAsanPoison(spMiArea[a].pBlocks, spMiArea[a].uBytes);
    }
    if (siMiAreasLost != 0)
    {
        LOG(LogMemory, Warning, "OPENSWBF2 MIPOISON heap=%d lost=%d", i, siMiAreasLost);
    }
    siMiAreas = 0;
#endif
    sMiHeap[i] = 0;
    sMiGen[i] = (sMiGen[i] + 1) & 0xFFFFFFu;
}

static size_t MiCreate(int heap, void* start, void* end, unsigned int align, const char* name)
{
    (void)name;
    if (heap < 0 || heap >= kRedHeapSlots)
    {
        return 0;
    }
    const unsigned int uMask = ~(align - 1);
    const unsigned int uStart = ((unsigned int)start + align - 1) & uMask;
    const unsigned int uEnd = (unsigned int)end & uMask;
    MiDestroy(heap);
    sMiGen[heap] = (sMiGen[heap] + 1) & 0xFFFFFFu;
    sMiHeap[heap] = mi_heap_new();
    if (sMiHeap[heap] == 0)
    {
        return 0;
    }
    return uEnd - uStart;
}

static unsigned int MiNeed(int i, size_t size, unsigned int* puWant)
{
    const unsigned int uAlign = gRedHeapSlots[i].align;
    const unsigned int uHdr = (uAlign + kRedTagBytes - 1) & ~(uAlign - 1);
    const unsigned int uNeed = ((unsigned int)size + uAlign - 1 + uHdr) & ~(uAlign - 1);
    *puWant = uNeed + 16 - uHdr;
    return uNeed;
}

static PBL_ASAN_NO_SANITIZE void MiStamp(int i, char* pBase, size_t size, unsigned int uNeed, size_t* pReserved)
{
    volatile unsigned int* puWords = (volatile unsigned int*)pBase;
    puWords[0] = (unsigned int)size;
    puWords[1] = uNeed;
    puWords[2] = (unsigned int)i | sMiGen[i] << 8;
    puWords[3] = MiCookie(pBase, (unsigned int)size, uNeed);
#ifdef OPENSWBF2_TEST
    check(mi_heap_of(pBase) == sMiHeap[i]);
#endif
    PblAsanPoison(pBase, 16);
    PblAsanUnpoison(pBase + 16, mi_usable_size(pBase) - 16);
    if (mi_usable_size(pBase) > 16 + size)
    {
        PblAsanPoison(pBase + 16 + size, mi_usable_size(pBase) - 16 - size);
    }
    *pReserved = uNeed;
}

static void* MiAlloc(int i, size_t size, bool bTop, size_t* pReserved)
{
    (void)bTop;
    if (i < 0 || i >= kRedHeapSlots || sMiHeap[i] == 0)
    {
        return 0;
    }
    unsigned int uWant = 0;
    const unsigned int uNeed = MiNeed(i, size, &uWant);
    char* pBase = (char*)mi_heap_zalloc_aligned(sMiHeap[i], uWant, gRedHeapSlots[i].align);
    if (pBase == 0)
    {
        if (sRecursionDepth <= 0)
        {
            RedMemory::failHeap = i;
            RedMemory::failSize = uNeed;
        }
        return 0;
    }
    RedMiRecordReservation(pBase);
    MiStamp(i, pBase, size, uNeed, pReserved);
    return pBase + 16;
}

RedMiBlockKind RedMiClassify(const void* pUser)
{
    const char* pcUser = (const char*)pUser;
    const char* pBase = pcUser - kMiPrefixBytes;
    mi_heap_t* pHeap = mi_heap_of(pBase);
    if (pHeap == 0)
    {
        return kRedMiBadPrefix;
    }
    RedMiPrefix kPrefix;
    MiReadPrefix(pBase, &kPrefix);
    const unsigned int uSlot = kPrefix.uSlotGen & 0xFFu;
    if (uSlot >= (unsigned int)kRedHeapSlots || sMiHeap[uSlot] != pHeap)
    {
        return kRedMiBadPrefix;
    }
    if (((kPrefix.uSlotGen >> 8) & 0xFFFFFFu) != sMiGen[uSlot])
    {
        return kRedMiBadPrefix;
    }
    if (kPrefix.uCookie != MiCookie(pBase, kPrefix.uSize, kPrefix.uCharge))
    {
        return kRedMiBadPrefix;
    }
    return kRedMiValid;
}

static size_t MiFree(int heap, void* block)
{
    (void)heap;
    char* pcUser = (char*)block;
    char* pBase = pcUser - kMiPrefixBytes;
    const RedMiBlockKind eKind = RedMiClassify(block);
    if (eKind == kRedMiValid)
    {
        RedMiPrefix kPrefix;
        MiReadPrefix(pBase, &kPrefix);
        mi_free(pBase);
        return kPrefix.uCharge;
    }
    if (mi_heap_of(pBase) != 0)
    {
        mi_free(pBase);
    }
    LOG(LogMemory, Error, "OPENSWBF2 MIERROR kind=badprefix");
    check(eKind == kRedMiValid);
    return 0;
}

static void* MiRealloc(int i, void* block, size_t oldSize, size_t size, size_t* pReserved, size_t* pOldCharge)
{
    if (RedMiClassify(block) != kRedMiValid)
    {
        MiFree(i, block);
        return 0;
    }
    char* pBase = (char*)block - kMiPrefixBytes;
    unsigned int uWant = 0;
    const unsigned int uNeed = MiNeed(i, size, &uWant);
    if (uWant <= mi_usable_size(pBase) && uWant >= mi_usable_size(pBase) / 2)
    {
        RedMiPrefix kPrefix;
        MiReadPrefix(pBase, &kPrefix);
        *pOldCharge = kPrefix.uCharge;
        MiStamp(i, pBase, size, uNeed, pReserved);
        return block;
    }
    void* pNew = MiAlloc(i, size, false, pReserved);
    if (pNew == 0)
    {
        return 0;
    }
    memcpy(pNew, block, oldSize < size ? oldSize : size);
    *pOldCharge = MiFree(i, block);
    return pNew;
}

static int MiOwner(const void* p)
{
    mi_heap_t* pOwnerHeap = mi_heap_of(p);
    if (pOwnerHeap != 0)
    {
        for (int i = 0; i < kRedHeapSlots; ++i)
        {
            if (sMiHeap[i] == pOwnerHeap)
            {
                return i;
            }
        }
        return kRedOwnerStale;
    }
    return MiInReservation(p) && !MiIsCrtBlock(p) ? kRedOwnerStale : -1;
}

static int MiMoveEnd(int iHeap, int iDelta, bool* pbAllocFailed)
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
    const RedHeapSlot& rTarget = gRedHeapSlots[iTarget];
    const unsigned int uSourceAlign = gRedHeapSlots[iSource].align;
    const unsigned int uAlign = rTarget.align;
    const unsigned int uHdr = (uAlign + kRedTagBytes - 1) & ~(uAlign - 1);
    const unsigned int uMoveAlign = uSourceAlign > uAlign ? uSourceAlign : uAlign;
    const unsigned int uMove = uAmount & ~(uMoveAlign - 1);
    if (uMove <= uHdr)
    {
        return 0;
    }
    const unsigned int uUsed = (unsigned int)rTarget.used;
    const unsigned int uHeadroom = uUsed < rTarget.defaultCap ? rTarget.defaultCap - uUsed : 0;
    if (uMove > uHeadroom)
    {
        if (sRecursionDepth <= 0)
        {
            RedMemory::failHeap = iTarget;
            RedMemory::failSize = uMove;
        }
        *pbAllocFailed = true;
        return 0;
    }
    const unsigned int uRem = uHeadroom - uMove;
    if (uRem != 0 && uRem < uAlign + uHdr)
    {
        return 0;
    }
    return iDelta < 0 ? -(int)uMove : (int)uMove;
}

static bool mi_cdecl MiVisitBlock(const mi_heap_t*, const mi_heap_area_t*, void* pBlock, size_t, void* pArg)
{
    if (pBlock == 0)
    {
        return true;
    }
    const MiVisitArg& rArg = *(const MiVisitArg*)pArg;
    const unsigned int uAlign = gRedHeapSlots[rArg.iHeap].align;
    char* pBase = (char*)(((unsigned int)pBlock + uAlign - 1) & ~(uAlign - 1));
    RedMiPrefix kPrefix;
    MiReadPrefix(pBase, &kPrefix);
    return rArg.pfnBlock(pBase + 16, kPrefix.uSize, rArg.pArg);
}

static bool MiVisit(int heap, bool (*pfnBlock)(void* pUser, size_t uSize, void* pArg), void* pArg)
{
    if (heap < 0 || heap >= kRedHeapSlots || sMiHeap[heap] == 0)
    {
        return false;
    }
    MiVisitArg kArg;
    kArg.iHeap = heap;
    kArg.pfnBlock = pfnBlock;
    kArg.pArg = pArg;
    mi_heap_visit_blocks(sMiHeap[heap], true, MiVisitBlock, &kArg);
    return true;
}

void RedMiNoteOutput(const char* pcMsg)
{
    static const char* const kapcExhaustion[] = {
        "unable to allocate memory for a thread local heap",
        "unable to dynamically create a thread local for a heap",
        "unable to allocate thread local variables",
        "aligned allocation request is too large",
    };
    bool bFound = false;
    for (int i = 0; i < 4; ++i)
    {
        bFound = bFound || (pcMsg != 0 && strstr(pcMsg, kapcExhaustion[i]) != 0);
    }
    stbMiExhaustionText = bFound;
}

bool RedMiTakeErrorFatal(int err)
{
    const bool bExhaustion = stbMiExhaustionText;
    stbMiExhaustionText = false;
    if (err == ENOMEM || err == EOVERFLOW)
    {
        return false;
    }
    return !(bExhaustion && (err == EFAULT || err == EINVAL));
}

static void mi_cdecl MiError(int err, void* arg)
{
    (void)arg;
    if (!RedMiTakeErrorFatal(err))
    {
        LOG(LogMemory, Warning, "OPENSWBF2 MIERROR err=%d", err);
        return;
    }
    LOG(LogMemory, Error, "OPENSWBF2 MIERROR err=%d", err);
    check(err == 0);
}

void RedMiConfigure(long lPurgeMs)
{
    mi_option_set(mi_option_purge_delay, lPurgeMs);
    mi_option_set(mi_option_retry_on_oom, 0);
    mi_option_set(mi_option_arena_reserve, 64 * 1024);
    mi_option_set(mi_option_arena_eager_commit, 0);
    mi_option_set(mi_option_page_full_retain, -1);
    mi_option_set(mi_option_show_errors, 1);
    mi_option_set(mi_option_max_errors, -1);
    mi_register_error(MiError, 0);
}

static void MiLogLine(const char* pcLine, int iLen, void* pArg)
{
    (void)pArg;
    LOG(LogMemory, Inform, "%.*s", iLen, pcLine);
}

static void mi_cdecl MiOutput(const char* msg, void* arg)
{
    (void)arg;
    RedMiNoteOutput(msg);
    RedMiOutputLines(msg, MiLogLine, 0);
}

void RedMiRegisterOutput()
{
    mi_register_output(MiOutput, 0);
}

bool RedMiFormatOptions(char* pcOut, unsigned int uSize)
{
    if (pcOut == 0 || uSize == 0)
    {
        return false;
    }
    const int iLen = _snprintf(pcOut, uSize,
                               "OPENSWBF2 MIOPTIONS purge_delay=%ld retry_on_oom=%ld arena_reserve=%u "
                               "arena_eager_commit=%ld reserve_os_memory=%u page_full_retain=%ld",
                               mi_option_get(mi_option_purge_delay), mi_option_get(mi_option_retry_on_oom),
                               (unsigned int)mi_option_get_size(mi_option_arena_reserve),
                               mi_option_get(mi_option_arena_eager_commit),
                               (unsigned int)mi_option_get_size(mi_option_reserve_os_memory),
                               mi_option_get(mi_option_page_full_retain));
    if (iLen < 0 || (unsigned int)iLen >= uSize)
    {
        pcOut[uSize - 1] = 0;
        return false;
    }
    return true;
}

void RedMiOutputLines(const char* pcMsg, void (*pfnLine)(const char* pcLine, int iLen, void* pArg), void* pArg)
{
    if (pcMsg == 0 || pfnLine == 0)
    {
        return;
    }
    const char* pcStart = pcMsg;
    for (;;)
    {
        const char* pcEnd = strchr(pcStart, '\n');
        const int iLen = pcEnd ? (int)(pcEnd - pcStart) : (int)strlen(pcStart);
        if (iLen > 0)
        {
            pfnLine(pcStart, iLen, pArg);
        }
        if (pcEnd == 0)
        {
            return;
        }
        pcStart = pcEnd + 1;
    }
}

bool RedMiFirstArena(unsigned int* puBase, unsigned int* puSize)
{
    if (slMiFirstBase == 0 || puBase == 0 || puSize == 0)
    {
        return false;
    }
    *puBase = (unsigned int)slMiFirstBase;
    *puSize = suMiFirstSize;
    return true;
}

static void MiTrim()
{
    mi_collect(true);
}

extern "C" const RedHeapBackend kRedMiBackend = { MiInit, MiCreate, MiDestroy, MiAlloc, MiFree, MiOwner, MiMoveEnd, MiVisit, MiTrim, MiRealloc };
