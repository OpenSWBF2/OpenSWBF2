#include "PblHandle.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "RedEngineFL/Util/RedLog.h"

#ifdef OPENSWBF2_TEST
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace
{

struct HandleSlot
{
    const PblHandled* pKey;
    unsigned int uId;
};

const unsigned int kHandleSlotsInitial = 4096;

HandleSlot* sSlots;
unsigned int sMask;
unsigned int sCount;
bool sGrowFailed;
#ifdef OPENSWBF2_TEST
unsigned long sOwnerThread;
bool sFailAlloc;
#endif

unsigned int HandleHome(const void* pKey, unsigned int uMask)
{
    uintptr_t uValue = (uintptr_t)pKey;
    unsigned int h = (unsigned int)uValue ^ (unsigned int)(uValue >> 16 >> 16);
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return h & uMask;
}

unsigned int HandleLocate(const PblHandled* pKey)
{
    unsigned int uSlot = HandleHome(pKey, sMask);
    while (sSlots[uSlot].pKey && sSlots[uSlot].pKey != pKey)
    {
        uSlot = (uSlot + 1) & sMask;
    }
    return uSlot;
}

HandleSlot* HandleAllocSlots(unsigned int uSlots)
{
#ifdef OPENSWBF2_TEST
    if (sFailAlloc)
    {
        return 0;
    }
#endif
    HandleSlot* pSlots = (HandleSlot*)malloc(uSlots * sizeof(HandleSlot));
    if (pSlots)
    {
        memset(pSlots, 0, uSlots * sizeof(HandleSlot));
    }
    return pSlots;
}

bool HandleGrow()
{
    const unsigned int uSlots = (sMask + 1) * 2;
    HandleSlot* pSlots = HandleAllocSlots(uSlots);
    if (!pSlots)
    {
        return false;
    }
    const unsigned int uMask = uSlots - 1;
    for (unsigned int i = 0; i <= sMask; ++i)
    {
        if (sSlots[i].pKey)
        {
            unsigned int uSlot = HandleHome(sSlots[i].pKey, uMask);
            while (pSlots[uSlot].pKey)
            {
                uSlot = (uSlot + 1) & uMask;
            }
            pSlots[uSlot] = sSlots[i];
        }
    }
    free(sSlots);
    sSlots = pSlots;
    sMask = uMask;
    return true;
}

void HandleRemoveAt(unsigned int uHole)
{
    sSlots[uHole].pKey = 0;
    sSlots[uHole].uId = 0;
    --sCount;
    for (unsigned int uNext = (uHole + 1) & sMask; sSlots[uNext].pKey; uNext = (uNext + 1) & sMask)
    {
        unsigned int uHome = HandleHome(sSlots[uNext].pKey, sMask);
        if (((uNext - uHome) & sMask) >= ((uNext - uHole) & sMask))
        {
            sSlots[uHole] = sSlots[uNext];
            sSlots[uNext].pKey = 0;
            sSlots[uNext].uId = 0;
            uHole = uNext;
        }
    }
}

void HandleCheckThread(bool bLatch)
{
#ifdef OPENSWBF2_TEST
    const unsigned long uThread = GetCurrentThreadId();
    if (sOwnerThread == 0)
    {
        if (bLatch)
        {
            sOwnerThread = uThread;
        }
        return;
    }
    if (uThread != sOwnerThread)
    {
        LOG(LogMemory, Error, "OPENSWBF2 HANDLEREG thread=%lu owner=%lu", uThread, sOwnerThread);
        check(uThread == sOwnerThread);
    }
#else
    (void)bLatch;
#endif
}

}

void PblHandleRegistry::Insert(const PblHandled* pKey, unsigned int uId)
{
    if (!pKey)
    {
        return;
    }
    HandleCheckThread(true);
    if (!sSlots)
    {
        sSlots = HandleAllocSlots(kHandleSlotsInitial);
        if (!sSlots)
        {
            LOG(LogMemory, Error, "OPENSWBF2 HANDLEREG alloc slots=%u failed", kHandleSlotsInitial);
            check(sSlots != 0);
            return;
        }
        sMask = kHandleSlotsInitial - 1;
    }
    unsigned int uSlot = HandleLocate(pKey);
    if (sSlots[uSlot].pKey == pKey)
    {
        sSlots[uSlot].uId = uId;
        return;
    }
    if ((sCount + 1) * 2 > sMask + 1 && !sGrowFailed)
    {
        if (HandleGrow())
        {
            uSlot = HandleLocate(pKey);
        }
        else
        {
            sGrowFailed = true;
            LOG(LogMemory, Warning, "OPENSWBF2 HANDLEREG grow slots=%u failed", (sMask + 1) * 2);
        }
    }
    if (sCount >= sMask)
    {
        LOG(LogMemory, Error, "OPENSWBF2 HANDLEREG full count=%u slots=%u", sCount, sMask + 1);
        check(sCount < sMask);
        return;
    }
    sSlots[uSlot].pKey = pKey;
    sSlots[uSlot].uId = uId;
    ++sCount;
}

void PblHandleRegistry::Erase(const PblHandled* pKey)
{
    if (!pKey || !sSlots)
    {
        return;
    }
    HandleCheckThread(false);
    unsigned int uSlot = HandleLocate(pKey);
    if (sSlots[uSlot].pKey == pKey)
    {
        HandleRemoveAt(uSlot);
    }
}

bool PblHandleRegistry::Lookup(const PblHandled* pKey, unsigned int* puId)
{
    if (!pKey || !sSlots)
    {
        return false;
    }
#ifdef OPENSWBF2_TEST
    HandleCheckThread(false);
#endif
    unsigned int uSlot = HandleLocate(pKey);
    if (sSlots[uSlot].pKey != pKey)
    {
        return false;
    }
    if (puId)
    {
        *puId = sSlots[uSlot].uId;
    }
    return true;
}

bool PblHandleRegistry::IsLive(const PblHandled* pKey, unsigned int uId)
{
    unsigned int uStored = 0;
    return Lookup(pKey, &uStored) && uStored == uId;
}

void PblHandleRegistry::PurgeIf(bool (*pfnMatch)(const void* pKey, void* pArg), void* pArg)
{
    if (!sSlots || !pfnMatch)
    {
        return;
    }
    const unsigned int uBefore = sCount;
    unsigned int uSlot = 0;
    while (uSlot <= sMask)
    {
        const void* pKey = sSlots[uSlot].pKey;
        if (pKey && pfnMatch(pKey, pArg))
        {
            HandleRemoveAt(uSlot);
        }
        else
        {
            ++uSlot;
        }
    }
    if (sCount < uBefore)
    {
        sGrowFailed = false;
    }
}

unsigned int PblHandleRegistry::CountIf(bool (*pfnMatch)(const void* pKey, void* pArg), void* pArg)
{
    if (!sSlots || !pfnMatch)
    {
        return 0;
    }
    unsigned int uFound = 0;
    for (unsigned int i = 0; i <= sMask; ++i)
    {
        const void* pKey = sSlots[i].pKey;
        if (pKey && pfnMatch(pKey, pArg))
        {
            ++uFound;
        }
    }
    return uFound;
}

unsigned int PblHandleRegistry::GetCount()
{
    return sCount;
}

unsigned int PblHandleRegistry::GetCapacity()
{
    return sSlots ? sMask + 1 : 0;
}

unsigned int PblHandleRegistry::HomeSlot(const void* pKey)
{
    return sSlots ? HandleHome(pKey, sMask) : 0;
}

#ifdef OPENSWBF2_TEST
void PblHandleRegistry::FailAllocForTest(bool bFail)
{
    sFailAlloc = bFail;
}
#endif

// GLOBAL: SWBF2 0x007e2c78
static unsigned int sNextHandleId = 1;

// FUNCTION: SWBF2 0x00727d20
PblHandled::PblHandled()
{
    mHandleId = sNextHandleId++;
    if (sNextHandleId == 0)
    {
        sNextHandleId = 1;
    }
    PblHandleRegistry::Insert(this, mHandleId);
}

PblHandled::PblHandled(const PblHandled& other) : mHandleId(other.mHandleId)
{
    PblHandleRegistry::Insert(this, mHandleId);
}

PblHandled& PblHandled::operator=(const PblHandled& other)
{
    mHandleId = other.mHandleId;
    PblHandleRegistry::Insert(this, mHandleId);
    return *this;
}

// FUNCTION: SWBF2 0x00727d40
unsigned int PblHandled::SetNextHandleId(unsigned int uId)
{
    unsigned int uOld = sNextHandleId;
    sNextHandleId = uId;
    return uOld;
}

unsigned int PblHandled::GetNextHandleId()
{
    return sNextHandleId;
}

unsigned int PblHandled::HandleIdOffset()
{
    return offsetof(PblHandled, mHandleId);
}

// FUNCTION: SWBF2 0x00423e90
unsigned int PblHandled::GetHandleId() const
{
    return mHandleId;
}
