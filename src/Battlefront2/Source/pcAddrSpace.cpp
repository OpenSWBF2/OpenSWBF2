#include <windows.h>

#include "pcAddrSpace.h"

uintptr_t pcAddrSpace::HogLimit()
{
    return sizeof(void*) == 8 ? (uintptr_t)0x100000000ull : (uintptr_t)0x80000000u;
}

uintptr_t pcAddrSpace::MinRegion()
{
    return 0x100000u;
}

unsigned int pcAddrSpace::ReserveBelow(uintptr_t uLimit, uintptr_t uMinRegion)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const uintptr_t uGran = si.dwAllocationGranularity;
    uintptr_t uAt = (uintptr_t)si.lpMinimumApplicationAddress;
    unsigned int uReserved = 0;
    int iRetry = 0;
    while (uAt < uLimit)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void*)uAt, &mbi, sizeof(mbi)))
        {
            break;
        }
        uintptr_t uBase = (uintptr_t)mbi.BaseAddress;
        uintptr_t uEnd = uBase + mbi.RegionSize;
        if (uEnd < uBase || uEnd > uLimit)
        {
            uEnd = uLimit;
        }
        if (mbi.State == MEM_FREE)
        {
            uintptr_t uFrom = (uBase + uGran - 1) & ~(uGran - 1);
            if (uEnd > uFrom && (uEnd - uFrom >= uMinRegion || uEnd == uLimit))
            {
                if (VirtualAlloc((void*)uFrom, uEnd - uFrom, MEM_RESERVE, PAGE_NOACCESS))
                {
                    uReserved++;
                }
                else if (iRetry < 16)
                {
                    iRetry++;
                    continue;
                }
            }
        }
        if (uEnd == uLimit)
        {
            break;
        }
        uAt = uEnd;
    }
    return uReserved;
}

unsigned int pcAddrSpace::FreeRegionsBelow(uintptr_t uLimit, uintptr_t uMinSize, bool bGranuleAligned)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const uintptr_t uGran = si.dwAllocationGranularity;
    uintptr_t uAt = (uintptr_t)si.lpMinimumApplicationAddress;
    unsigned int uFree = 0;
    while (uAt < uLimit)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void*)uAt, &mbi, sizeof(mbi)))
        {
            break;
        }
        uintptr_t uBase = (uintptr_t)mbi.BaseAddress;
        uintptr_t uEnd = uBase + mbi.RegionSize;
        if (uEnd < uBase || uEnd > uLimit)
        {
            uEnd = uLimit;
        }
        if (mbi.State == MEM_FREE)
        {
            uintptr_t uFrom = (uBase + uGran - 1) & ~(uGran - 1);
            uintptr_t uTo = bGranuleAligned ? (uEnd & ~(uGran - 1)) : uEnd;
            if (uTo > uFrom && uTo - uFrom >= uMinSize && (!bGranuleAligned || uTo - uFrom >= uGran))
            {
                uFree++;
            }
        }
        if (uEnd == uLimit)
        {
            break;
        }
        uAt = uEnd;
    }
    return uFree;
}
