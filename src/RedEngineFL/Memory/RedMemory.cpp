#include <intrin.h>
#include <stdlib.h>
#include <string.h>

#include "RedMemory.h"
#include "RedMemoryBackend.h"
#include "RedScriptLimits.h"
#include "RedEngineFL/Memory/engineAlloc.h"
#include "PebbleFL/Common/PblHandle.h"
#include "RedEngineFL/Util/RedLog.h"

#ifdef OPENSWBF2_TEST
#include <windows.h>
#endif

extern "C"
{
    // GLOBAL: SWBF2 0x0093eba1
    // isHeapObjInitialized
    bool isHeapObjInitialized;
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
}

RedHeapSlot gRedHeapSlots[kRedHeapSlots];
const RedHeapBackend* gRedHeapBackend = &kRedArenaBackend;

// FUNCTION: SWBF2 0x006c3a50
extern "C" unsigned int RedGetHeapFree(int heap)
{
    if (heap < 0 || heap >= maxheaps || !gRedHeapSlots[heap].bLive)
    {
        return 0;
    }
    const RedHeapSlot& rSlot = gRedHeapSlots[heap];
    return (unsigned int)rSlot.used < rSlot.defaultCap ? rSlot.defaultCap - (unsigned int)rSlot.used : 0;
}

extern "C" char const* RedGetHeapName(int heap)
{
    if (heap >= 0 && heap < maxheaps && gRedHeapSlots[heap].name)
    {
        return gRedHeapSlots[heap].name;
    }
    return "?";
}

static void HeapStatsAddUsed(int heap, long lDelta)
{
    RedHeapSlot& rSlot = gRedHeapSlots[heap];
    long lUsed = _InterlockedExchangeAdd(&rSlot.used, lDelta) + lDelta;
    long lPeak = rSlot.peak;
    while (lUsed > lPeak)
    {
        long lSeen = _InterlockedCompareExchange(&rSlot.peak, lUsed, lPeak);
        if (lSeen == lPeak)
        {
            break;
        }
        lPeak = lSeen;
    }
}

static void HeapStatsReset(RedHeapSlot& rSlot)
{
    _InterlockedExchange(&rSlot.used, 0);
    _InterlockedExchange(&rSlot.peak, 0);
    _InterlockedExchange(&rSlot.allocs, 0);
    _InterlockedExchange(&rSlot.failed, 0);
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

static unsigned int sPerturbModes;
static unsigned int sPerturbSeed;
static unsigned int sPerturbState[4];
static const char* const sapcPerturbNames[4] = { "pad", "top", "fill", "pool" };
static const unsigned int kPerturbAll = kRedPerturbPad | kRedPerturbTop | kRedPerturbFill | kRedPerturbPool;

static unsigned int PerturbMix(unsigned int x)
{
    x ^= x >> 16;
    x *= 0x85EBCA6Bu;
    x ^= x >> 13;
    x *= 0xC2B2AE35u;
    x ^= x >> 16;
    return x != 0 ? x : 1;
}

extern "C" void RedPerturbSet(unsigned int uSeed, unsigned int uModes)
{
    sPerturbSeed = uSeed;
    for (unsigned int i = 0; i < 4; ++i)
    {
        sPerturbState[i] = PerturbMix(uSeed * 0x9E3779B9u + (i + 1) * 0x7F4A7C15u);
    }
    sPerturbModes = uModes & kPerturbAll;
}

extern "C" unsigned int RedPerturbModes()
{
    return sPerturbModes;
}

extern "C" unsigned int RedPerturbSeed()
{
    return sPerturbSeed;
}

extern "C" bool RedPerturbDraw(unsigned int uMode, unsigned int* puOut)
{
    if (puOut == 0 || (sPerturbModes & uMode) == 0)
    {
        return false;
    }
    if (sMainTid != 0 && GetCurrentThreadId() != (DWORD)sMainTid)
    {
        return false;
    }
    for (unsigned int i = 0; i < 4; ++i)
    {
        if (uMode == (1u << i))
        {
            unsigned int x = sPerturbState[i];
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            sPerturbState[i] = x;
            *puOut = x;
            return true;
        }
    }
    return false;
}

extern "C" bool RedPerturbParse(const char* pcSpec, unsigned int* puSeed, unsigned int* puModes)
{
    if (pcSpec == 0 || puSeed == 0 || puModes == 0 || *pcSpec < '0' || *pcSpec > '9')
    {
        return false;
    }
    const char* pc = pcSpec;
    unsigned long long ullSeed = 0;
    while (*pc >= '0' && *pc <= '9')
    {
        ullSeed = ullSeed * 10 + (unsigned int)(*pc - '0');
        if (ullSeed > 0xFFFFFFFFull)
        {
            return false;
        }
        ++pc;
    }
    unsigned int uModes = kPerturbAll;
    if (*pc == ':')
    {
        uModes = 0;
        ++pc;
        for (;;)
        {
            const char* pcEnd = pc;
            while (*pcEnd != 0 && *pcEnd != ',')
            {
                ++pcEnd;
            }
            const unsigned int uLen = (unsigned int)(pcEnd - pc);
            unsigned int uBit = 0;
            for (unsigned int i = 0; i < 4; ++i)
            {
                if (uLen == strlen(sapcPerturbNames[i]) && _strnicmp(pc, sapcPerturbNames[i], uLen) == 0)
                {
                    uBit = 1u << i;
                }
            }
            if (uBit == 0)
            {
                return false;
            }
            uModes |= uBit;
            if (*pcEnd == 0)
            {
                break;
            }
            pc = pcEnd + 1;
        }
    }
    else if (*pc != 0)
    {
        return false;
    }
    *puSeed = (unsigned int)ullSeed;
    *puModes = uModes;
    return true;
}

extern "C" void RedPerturbFormatModes(unsigned int uModes, char* pcOut, unsigned int uSize)
{
    if (pcOut == 0 || uSize == 0)
    {
        return;
    }
    pcOut[0] = 0;
    unsigned int uLen = 0;
    for (unsigned int i = 0; i < 4; ++i)
    {
        if ((uModes & (1u << i)) == 0)
        {
            continue;
        }
        const unsigned int uName = (unsigned int)strlen(sapcPerturbNames[i]);
        if (uLen + uName + (uLen != 0 ? 1 : 0) >= uSize)
        {
            return;
        }
        if (uLen != 0)
        {
            pcOut[uLen++] = ',';
        }
        memcpy(pcOut + uLen, sapcPerturbNames[i], uName);
        uLen += uName;
        pcOut[uLen] = 0;
    }
}

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

extern "C" void Startup_RedInitHeap(int, int);

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

static unsigned int ReserveNeed(int heap, RedHeapSlot& rSlot, size_t size)
{
    const unsigned int uAlign = rSlot.align;
    const unsigned int uHdr = (uAlign + kRedTagBytes - 1) & ~(uAlign - 1);
    if (size > ~0u - (uAlign - 1 + uHdr))
    {
        if (sRecursionDepth <= 0)
        {
            RedMemory::failHeap = heap;
            RedMemory::failSize = ~0u;
        }
        _InterlockedIncrement(&rSlot.failed);
        return 0;
    }
    const unsigned int uSize = size != 0 ? (unsigned int)size : 1;
    const unsigned int uNeed = (uSize + uAlign - 1 + uHdr) & ~(uAlign - 1);
    long lUsed = rSlot.used;
    for (;;)
    {
        if ((unsigned int)lUsed > rSlot.cap || uNeed > rSlot.cap - (unsigned int)lUsed)
        {
            if (sRecursionDepth <= 0)
            {
                RedMemory::failHeap = heap;
                RedMemory::failSize = uNeed;
            }
            _InterlockedIncrement(&rSlot.failed);
            return 0;
        }
        const long lSeen = _InterlockedCompareExchange(&rSlot.used, lUsed + (long)uNeed, lUsed);
        if (lSeen == lUsed)
        {
            break;
        }
        lUsed = lSeen;
    }
    return uNeed;
}

static void RollbackNeed(RedHeapSlot& rSlot, unsigned int uNeed)
{
    _InterlockedExchangeAdd(&rSlot.used, -(long)uNeed);
    _InterlockedIncrement(&rSlot.failed);
}

static void* AllocFromSlot(int heap, size_t size, bool bTop)
{
    if (heap < 0 || heap >= maxheaps)
    {
        return 0;
    }
    RedHeapSlot& rSlot = gRedHeapSlots[heap];
    const unsigned int uNeed = ReserveNeed(heap, rSlot, size);
    if (uNeed == 0)
    {
        return 0;
    }
    size_t uReserved = 0;
    void* p = rSlot.pBackend->alloc(heap, size != 0 ? (unsigned int)size : 1, bTop, &uReserved);
    if (!p)
    {
        RollbackNeed(rSlot, uNeed);
        return 0;
    }
    HeapStatsAddUsed(heap, (long)uReserved - (long)uNeed);
    _InterlockedIncrement(&rSlot.allocs);
    return p;
}

#ifdef OPENSWBF2_TEST
static void NoteOffThreadAlloc(int heap, size_t size)
{
    DWORD uiTid = GetCurrentThreadId();
    if (sMainTid != 0 && uiTid != (DWORD)sMainTid)
    {
        RedNoteOffThread(heap, size, uiTid);
    }
}
#endif

// FUNCTION: SWBF2 0x006c38d0
void* RedAllocFromHeap(int heap, size_t size, bool bTop)
{
    if (!isHeapObjInitialized)
    {
        Startup_RedInitHeap(0x80, 0x80);
    }
#ifdef OPENSWBF2_TEST
    const unsigned int uiModes = sPerturbModes;
    unsigned int uiPad = 0;
    unsigned int uiDraw = 0;
    if ((uiModes & kRedPerturbPad) != 0 && size <= 0xFFFFFF00u && RedPerturbDraw(kRedPerturbPad, &uiDraw))
    {
        uiPad = 4 + 4 * (uiDraw % 15);
    }
    void* p = AllocFromSlot(heap, size + uiPad, bTop || (uiModes & kRedPerturbTop) != 0);
    if (p)
    {
        if (uiPad != 0)
        {
            PblAsanPoison((char*)p + size, uiPad);
        }
        if ((uiModes & kRedPerturbFill) != 0)
        {
            memset(p, 0xA5, size);
        }
    }
    NoteOffThreadAlloc(heap, size);
#else
    void* p = AllocFromSlot(heap, size, bTop);
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

void* RedReallocInHeap(int heap, void* block, size_t oldSize, size_t size)
{
    const int iOwner = RedHeapOwner(block);
    if (iOwner < 0 || gRedHeapSlots[iOwner].pBackend->realloc == 0)
    {
        void* pNew = RedAllocFromHeap(heap, size, false);
        if (pNew)
        {
            memcpy(pNew, block, oldSize < size ? oldSize : size);
            RedFreeToHeap(heap, block, 1);
        }
        return pNew;
    }
    RedHeapSlot& rSlot = gRedHeapSlots[iOwner];
#ifdef OPENSWBF2_TEST
    const unsigned int uiModes = sPerturbModes;
    unsigned int uiPad = 0;
    unsigned int uiDraw = 0;
    if ((uiModes & kRedPerturbPad) != 0 && size <= 0xFFFFFF00u && RedPerturbDraw(kRedPerturbPad, &uiDraw))
    {
        uiPad = 4 + 4 * (uiDraw % 15);
    }
    const size_t uRequest = size + uiPad;
    NoteOffThreadAlloc(iOwner, size);
#else
    const size_t uRequest = size;
#endif
    const unsigned int uNeed = ReserveNeed(iOwner, rSlot, uRequest);
    if (uNeed == 0)
    {
        if (OnFailure)
        {
            OnFailure(iOwner, size);
        }
        return 0;
    }
    size_t uReserved = 0;
    size_t uOldCharge = 0;
    void* p = rSlot.pBackend->realloc(iOwner, block, oldSize, uRequest != 0 ? uRequest : 1, &uReserved, &uOldCharge);
    if (!p)
    {
        RollbackNeed(rSlot, uNeed);
        if (OnFailure)
        {
            OnFailure(iOwner, size);
        }
        return 0;
    }
    HeapStatsAddUsed(iOwner, (long)uReserved - (long)uNeed);
    _InterlockedExchangeAdd(&rSlot.used, -(long)uOldCharge);
    _InterlockedIncrement(&rSlot.allocs);
#ifdef OPENSWBF2_TEST
    if (uiPad != 0)
    {
        PblAsanPoison((char*)p + size, uiPad);
    }
    if ((uiModes & kRedPerturbFill) != 0 && size > oldSize)
    {
        memset((char*)p + oldSize, 0xA5, size - oldSize);
    }
#endif
    return p;
}

struct HeapCapKnob
{
    const char* pcKnob;
    const char* pcSlot;
};

static const HeapCapKnob skaHeapCapKnobs[] = {
    { "Application", "Application" },
    { "Gamespy", "Gamespy" },
    { "Lua", "Lua" },
    { "Artist", "Artist" },
    { "Runtime", "Runtime" },
    { "Temp", "Temp" },
    { "Debug", "Debug" },
    { "HUDEdit", "HUDEditHeap" },
};
static const int kHeapCapKnobs = sizeof(skaHeapCapKnobs) / sizeof(skaHeapCapKnobs[0]);
static unsigned int suHeapCapNotes[kHeapCapKnobs];
static bool sabHeapCapWarned[kHeapCapKnobs];
static bool sbHeapCapWindow;

static int FindHeapCapKnob(const char* pcName, int iLen)
{
    for (int k = 0; k < kHeapCapKnobs; ++k)
    {
        if ((int)strlen(skaHeapCapKnobs[k].pcKnob) == iLen && _strnicmp(pcName, skaHeapCapKnobs[k].pcKnob, iLen) == 0)
        {
            return k;
        }
    }
    return -1;
}

static int HeapCapSlotKnob(const RedHeapSlot& rSlot)
{
    for (int k = 0; rSlot.name != 0 && k < kHeapCapKnobs; ++k)
    {
        if (strcmp(rSlot.name, skaHeapCapKnobs[k].pcSlot) == 0)
        {
            return k;
        }
    }
    return -1;
}

static unsigned int HeapCapNote(const RedHeapSlot& rSlot)
{
    const int iKnob = HeapCapSlotKnob(rSlot);
    return iKnob >= 0 ? suHeapCapNotes[iKnob] : 0;
}

static bool TakeHeapCapWarning(const RedHeapSlot& rSlot, bool bScript)
{
    const int iKnob = HeapCapSlotKnob(rSlot);
    if (bScript || iKnob < 0)
    {
        return true;
    }
    const bool bDue = !sabHeapCapWarned[iKnob];
    sabHeapCapWarned[iKnob] = true;
    return bDue;
}

static unsigned long long HeapCapsTotal(int iSkip)
{
    unsigned long long ullTotal = 0;
    for (int i = 0; i < maxheaps; ++i)
    {
        if (i != iSkip && gRedHeapSlots[i].bLive)
        {
            ullTotal += gRedHeapSlots[i].cap;
        }
    }
    return ullTotal;
}

static void SetSlotCap(RedHeapSlot& rSlot, unsigned int uTarget)
{
    const unsigned int uUsed = (unsigned int)rSlot.used;
    if (uTarget <= uUsed && uTarget < rSlot.cap)
    {
        if (TakeHeapCapWarning(rSlot, false))
        {
            LOG(LogMemory, Warning, "heap cap for %s stays at %u, target %u is below used %u", rSlot.name, rSlot.cap,
                uTarget, uUsed);
        }
        return;
    }
    rSlot.cap = uTarget;
}

static void RaiseHeapCap(int heap, unsigned int uBytes, bool bScript)
{
    if (heap < 0 || heap >= maxheaps || !gRedHeapSlots[heap].bLive)
    {
        return;
    }
    RedHeapSlot& rSlot = gRedHeapSlots[heap];
    const unsigned long long ullOthers = HeapCapsTotal(heap);
    const unsigned int uAvail =
        ullOthers < RedScriptLimits::kHeapCapTotalMax ? RedScriptLimits::kHeapCapTotalMax - (unsigned int)ullOthers : 0;
    const unsigned int uFit = uBytes < uAvail ? uBytes : uAvail;
    if (uFit < rSlot.defaultCap)
    {
        if (TakeHeapCapWarning(rSlot, bScript))
        {
            LOG(LogMemory, Warning, "heap cap for %s stays at %u, request %u is below the default", rSlot.name,
                rSlot.cap, uFit);
        }
        return;
    }
    if (uFit < rSlot.cap)
    {
        if (TakeHeapCapWarning(rSlot, bScript))
        {
            LOG(LogMemory, Warning, "heap cap for %s stays at %u, request %u is below it", rSlot.name, rSlot.cap, uFit);
        }
        return;
    }
    SetSlotCap(rSlot, uFit);
    const int iKnob = HeapCapSlotKnob(rSlot);
    if (bScript && iKnob >= 0)
    {
        sabHeapCapWarned[iKnob] = false;
    }
    if (uFit < uBytes && TakeHeapCapWarning(rSlot, bScript))
    {
        LOG(LogMemory, Warning, "heap cap for %s reduced to %u, the total of all caps is limited", rSlot.name, uFit);
    }
}

void RedRaiseHeapCap(int heap, unsigned int uBytes)
{
    RaiseHeapCap(heap, uBytes, true);
}

static void TrimHeapCaps()
{
    const unsigned long long ullTotal = HeapCapsTotal(-1);
    if (ullTotal <= RedScriptLimits::kHeapCapTotalMax)
    {
        return;
    }
    unsigned long long ullOver = ullTotal - RedScriptLimits::kHeapCapTotalMax;
    for (int i = maxheaps - 1; i >= 0 && ullOver != 0; --i)
    {
        RedHeapSlot& rSlot = gRedHeapSlots[i];
        const unsigned int uUsed = (unsigned int)rSlot.used;
        const unsigned int uFloor = uUsed < rSlot.defaultCap ? rSlot.defaultCap : uUsed + 1;
        if (!rSlot.bLive || rSlot.cap <= uFloor)
        {
            continue;
        }
        const unsigned int uRoom = rSlot.cap - uFloor;
        const unsigned int uCut = ullOver < uRoom ? (unsigned int)ullOver : uRoom;
        SetSlotCap(rSlot, rSlot.cap - uCut);
        ullOver -= uCut;
        if (TakeHeapCapWarning(rSlot, false))
        {
            LOG(LogMemory, Warning, "heap cap for %s reduced to %u, the total of all caps is limited", rSlot.name,
                rSlot.cap);
        }
    }
}

static void ApplyHeapCapNote(int i)
{
    const unsigned int uNote = HeapCapNote(gRedHeapSlots[i]);
    if (uNote != 0)
    {
        RaiseHeapCap(i, uNote, false);
    }
}

bool RedParseHeapCapArg(const char* pcVal, int iLen, int* piKnob, unsigned int* puBytes, bool* pbClamped)
{
    int iEq = 0;
    while (iEq < iLen && pcVal[iEq] != '=')
    {
        ++iEq;
    }
    if (iEq == iLen)
    {
        return false;
    }
    const int iKnob = FindHeapCapKnob(pcVal, iEq);
    if (iKnob < 0)
    {
        return false;
    }
    int i = iEq + 1;
    const int iDigits = i;
    unsigned __int64 ullValue = 0;
    while (i < iLen && pcVal[i] >= '0' && pcVal[i] <= '9')
    {
        ullValue = ullValue * 10 + (unsigned int)(pcVal[i] - '0');
        if (ullValue > 0xFFFFFFFFu)
        {
            ullValue = 0xFFFFFFFFu;
        }
        ++i;
    }
    if (i == iDigits)
    {
        return false;
    }
    if (i < iLen && (pcVal[i] == 'K' || pcVal[i] == 'M'))
    {
        ullValue *= pcVal[i] == 'K' ? 1024u : 1048576u;
        ++i;
    }
    if (i != iLen || ullValue == 0)
    {
        return false;
    }
    const unsigned __int64 ullMax = RedScriptLimits::kSetHeapCap.uMaxBytes;
    *pbClamped = ullValue > ullMax;
    *puBytes = (unsigned int)(*pbClamped ? ullMax : ullValue);
    *piKnob = iKnob;
    return true;
}

void RedNoteHeapCap(int iKnob, unsigned int uBytes)
{
    suHeapCapNotes[iKnob] = uBytes;
    sabHeapCapWarned[iKnob] = false;
}

int RedHeapCapSlot(const char* pcKnob)
{
    const int iKnob = FindHeapCapKnob(pcKnob, (int)strlen(pcKnob));
    for (int i = 0; iKnob >= 0 && i < maxheaps; ++i)
    {
        if (gRedHeapSlots[i].bLive && strcmp(gRedHeapSlots[i].name, skaHeapCapKnobs[iKnob].pcSlot) == 0)
        {
            return i;
        }
    }
    return -1;
}

void RedHeapCapOpenWindow()
{
    sbHeapCapWindow = true;
    for (int i = 0; i < maxheaps; ++i)
    {
        RedHeapSlot& rSlot = gRedHeapSlots[i];
        const unsigned int uNote = HeapCapNote(rSlot);
        const unsigned int uBase = uNote > rSlot.defaultCap ? uNote : rSlot.defaultCap;
        if (rSlot.bLive && uBase < rSlot.cap)
        {
            SetSlotCap(rSlot, uBase);
        }
    }
    for (int i = 0; i < maxheaps; ++i)
    {
        ApplyHeapCapNote(i);
    }
}

void RedHeapCapCloseWindow()
{
    sbHeapCapWindow = false;
}

bool RedHeapCapWindowOpen()
{
    return sbHeapCapWindow;
}

// FUNCTION: SWBF2 0x006c3900
int RedCreateHeap(void* start, void* end, unsigned int align, char const* name)
{
    for (int i = 0; i < maxheaps; ++i)
    {
        if (!gRedHeapSlots[i].bLive)
        {
            const size_t size = gRedHeapBackend->create(i, start, end, align, name);
            RedHeapSlot& rSlot = gRedHeapSlots[i];
            rSlot.pBackend = gRedHeapBackend;
            rSlot.name = name;
            rSlot.size = size;
            rSlot.align = align;
            rSlot.cap = size;
            rSlot.defaultCap = size;
            rSlot.bLive = size != 0;
            TrimHeapCaps();
            HeapStatsReset(rSlot);
            ApplyHeapCapNote(i);
            return i;
        }
    }
    gRedHeapBackend->create(-1, start, end, align, name);
    return -1;
}

// FUNCTION: SWBF2 0x006c39b0
void RedDestroyHeap(int heap)
{
    if (heap < 0 || heap >= maxheaps)
    {
        return;
    }
    RedHeapSlot& rSlot = gRedHeapSlots[heap];
    if (rSlot.pBackend)
    {
        rSlot.pBackend->destroy(heap);
    }
    rSlot.size = 0;
    rSlot.cap = 0;
    rSlot.defaultCap = 0;
    rSlot.align = 4;
    rSlot.name = 0;
    rSlot.bLive = false;
}

extern "C" int RedHeapOwner(const void* p)
{
    bool bStale = false;
    for (int i = 0; i < kRedHeapSlots; ++i)
    {
        const RedHeapBackend* pBackend = gRedHeapSlots[i].pBackend;
        if (pBackend == 0)
        {
            continue;
        }
        bool bSeen = false;
        for (int j = 0; j < i && !bSeen; ++j)
        {
            bSeen = gRedHeapSlots[j].pBackend == pBackend;
        }
        if (bSeen)
        {
            continue;
        }
        const int iOwner = pBackend->owner(p);
        if (iOwner >= 0 && iOwner < kRedHeapSlots && gRedHeapSlots[iOwner].bLive &&
            gRedHeapSlots[iOwner].pBackend == pBackend)
        {
            return iOwner;
        }
        bStale = bStale || iOwner == kRedOwnerStale;
    }
    return bStale ? kRedOwnerStale : -1;
}

// FUNCTION: SWBF2 0x006c3a00
void RedFreeToHeap(int heap, void* block, unsigned int flags)
{
    (void)flags;
    if (!isHeapObjInitialized)
    {
        return;
    }
    int iOwner = RedHeapOwner(block);
    if (iOwner < 0)
    {
        check(iOwner == -1);
        free(block);
        return;
    }
    size_t r = gRedHeapSlots[iOwner].pBackend->free(iOwner, block);
    _InterlockedExchangeAdd(&gRedHeapSlots[iOwner].used, -(long)r);
}

// FUNCTION: SWBF2 0x006c3aa0
void* RedInitHeap(void* start, void* end, int maxheaps)
{
    ::maxheaps = maxheaps < kRedHeapSlots ? maxheaps : kRedHeapSlots;
    for (int i = 0; i < kRedHeapSlots; ++i)
    {
        RedHeapSlot& rSlot = gRedHeapSlots[i];
        rSlot.name = 0;
        rSlot.size = 0;
        rSlot.align = 4;
        rSlot.cap = 0;
        rSlot.defaultCap = 0;
        rSlot.bLive = false;
        rSlot.pBackend = 0;
        HeapStatsReset(rSlot);
    }
    void* pNext = gRedHeapBackend->init(start, end, maxheaps);
#ifdef OPENSWBF2_TEST
    _InterlockedCompareExchange(&sMainTid, (long)GetCurrentThreadId(), 0);
#endif
    isHeapObjInitialized = true;
    return pNext;
}

// FUNCTION: SWBF2 0x006c3b20
void RedMarkHeapPermanent(int heap, bool permanent)
{
    (void)heap;
    (void)permanent;
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
    return gRedHeapSlots[heap].defaultCap;
}

// FUNCTION: SWBF2 0x006c3b40
void RedMoveHeapEnd(int iHeap, int iDelta)
{
    if (iHeap < 0 || iHeap >= maxheaps - 1)
    {
        return;
    }
    const RedHeapBackend* pBackend = gRedHeapSlots[iHeap].pBackend;
    if (pBackend == 0 || gRedHeapSlots[iHeap + 1].pBackend != pBackend)
    {
        return;
    }
    bool bAllocFailed = false;
    const int iMoved = pBackend->moveEnd(iHeap, iDelta, &bAllocFailed);
    const int iTarget = iDelta < 0 ? iHeap : iHeap + 1;
    if (bAllocFailed)
    {
        _InterlockedIncrement(&gRedHeapSlots[iTarget].failed);
    }
    if (iMoved != 0)
    {
        RedHeapSlot& rLow = gRedHeapSlots[iHeap];
        RedHeapSlot& rHigh = gRedHeapSlots[iHeap + 1];
        rLow.size += iMoved;
        rLow.cap += iMoved;
        rLow.defaultCap += iMoved;
        rHigh.size -= iMoved;
        rHigh.cap -= iMoved;
        rHigh.defaultCap -= iMoved;
        rLow.bLive = rLow.size != 0;
        rHigh.bLive = rHigh.size != 0;
    }
}

extern "C" bool RedGetHeapStats(int heap, RedHeapStats* pOut)
{
    if (pOut == 0 || heap < 0 || heap >= maxheaps || gRedHeapSlots[heap].size == 0)
    {
        return false;
    }
    const RedHeapSlot& rSlot = gRedHeapSlots[heap];
    pOut->uCap = rSlot.cap;
    pOut->uUsed = (unsigned int)rSlot.used;
    pOut->uPeak = (unsigned int)rSlot.peak;
    pOut->uAllocs = (unsigned int)rSlot.allocs;
    pOut->uFailed = (unsigned int)rSlot.failed;
    return true;
}

extern "C" bool RedVisitHeap(int heap, bool (*pfnBlock)(void* pUser, size_t uSize, void* pArg), void* pArg)
{
    if (pfnBlock == 0 || heap < 0 || heap >= maxheaps || !gRedHeapSlots[heap].bLive)
    {
        return false;
    }
    return gRedHeapSlots[heap].pBackend->visit(heap, pfnBlock, pArg);
}

extern "C" bool RedHeapBackendIsArena()
{
    return gRedHeapBackend == &kRedArenaBackend;
}

extern "C" void RedTrimHeaps()
{
    if (gRedHeapBackend->trim)
    {
        gRedHeapBackend->trim();
    }
}

const RedHeapBackend* RedParseAllocatorArg(const char* pcVal, int iLen, bool* pbRejected)
{
    if (pcVal == 0)
    {
        return &kRedMiBackend;
    }
    if (iLen == 5 && _strnicmp(pcVal, "arena", 5) == 0)
    {
        return &kRedArenaBackend;
    }
    if (iLen == 8 && _strnicmp(pcVal, "mimalloc", 8) == 0)
    {
        return &kRedMiBackend;
    }
    if (pbRejected)
    {
        *pbRejected = true;
    }
    return &kRedMiBackend;
}

long RedParseMiPurgeArg(const char* pcVal, int iLen, bool* pbRejected)
{
    if (pcVal == 0)
    {
        return 1000;
    }
    long lValue = 0;
    bool bDigits = iLen >= 1 && iLen <= 9;
    for (int i = 0; bDigits && i < iLen; ++i)
    {
        bDigits = pcVal[i] >= '0' && pcVal[i] <= '9';
        lValue = lValue * 10 + (pcVal[i] - '0');
    }
    if (bDigits)
    {
        return lValue;
    }
    if (pbRejected)
    {
        *pbRejected = true;
    }
    return 1000;
}

static bool HeapOwnsKey(const void* pKey, void* pArg)
{
    const int iOwner = RedHeapOwner(pKey);
    return iOwner == *(const int*)pArg || iOwner == kRedOwnerStale;
}

static bool KeyIsStale(const void* pKey, void* pArg)
{
    (void)pArg;
    return RedHeapOwner(pKey) == kRedOwnerStale;
}

unsigned int RedCountStaleHandles()
{
    return PblHandleRegistry::CountIf(KeyIsStale, 0);
}

void RedPurgeHeapHandles(int heap, const char* name)
{
    if (heap < 0 || heap >= maxheaps || !gRedHeapSlots[heap].bLive)
    {
        return;
    }
#ifdef OPENSWBF2_TEST
    const unsigned int uStale = RedCountStaleHandles();
    if (uStale != 0)
    {
        LOG(LogMemory, Inform, "OPENSWBF2 HANDLEREG purge heap=%s stale=%u", name ? name : "?", uStale);
    }
#endif
    PblHandleRegistry::PurgeIf(HeapOwnsKey, &heap);
#ifdef OPENSWBF2_TEST
    const unsigned int uLeft = PblHandleRegistry::CountIf(HeapOwnsKey, &heap);
    if (uLeft != 0)
    {
        LOG(LogMemory, Error, "OPENSWBF2 HANDLEREG audit heap=%s left=%u", name ? name : "?", uLeft);
        check(uLeft == 0);
    }
#else
    (void)name;
#endif
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
