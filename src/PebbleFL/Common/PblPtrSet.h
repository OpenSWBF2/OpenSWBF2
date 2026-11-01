#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

template <int SLOTS>
class PblPtrSet
{
public:
    static_assert(SLOTS >= 2 && (SLOTS & (SLOTS - 1)) == 0, "PblPtrSet size must be a power of two");

    static unsigned int Home(const void* pPtr)
    {
        uintptr_t uValue = (uintptr_t)pPtr;
        unsigned int h = (unsigned int)uValue ^ (unsigned int)(uValue >> 16 >> 16);
        h ^= h >> 16;
        h *= 0x7FEB352Du;
        h ^= h >> 15;
        h *= 0x846CA68Bu;
        h ^= h >> 16;
        return h & (SLOTS - 1);
    }

    unsigned int Locate(const void* pPtr) const
    {
        unsigned int uSlot = Home(pPtr);
        while (mSlots[uSlot] && mSlots[uSlot] != pPtr)
        {
            uSlot = (uSlot + 1) & (SLOTS - 1);
        }
        return uSlot;
    }

    bool Contains(const void* pPtr) const
    {
        return pPtr && mSlots[Locate(pPtr)] == pPtr;
    }

    bool Insert(const void* pPtr)
    {
        if (!pPtr)
        {
            return false;
        }
        unsigned int uSlot = Locate(pPtr);
        if (mSlots[uSlot] == pPtr)
        {
            return true;
        }
        if (mCount >= SLOTS - 1)
        {
            return false;
        }
        mSlots[uSlot] = pPtr;
        ++mCount;
        return true;
    }

    bool Remove(const void* pPtr)
    {
        if (!pPtr)
        {
            return false;
        }
        unsigned int uHole = Locate(pPtr);
        if (mSlots[uHole] != pPtr)
        {
            return false;
        }
        mSlots[uHole] = 0;
        --mCount;
        for (unsigned int uNext = (uHole + 1) & (SLOTS - 1); mSlots[uNext]; uNext = (uNext + 1) & (SLOTS - 1))
        {
            unsigned int uHome = Home(mSlots[uNext]);
            if (((uNext - uHome) & (SLOTS - 1)) >= ((uNext - uHole) & (SLOTS - 1)))
            {
                mSlots[uHole] = mSlots[uNext];
                mSlots[uNext] = 0;
                uHole = uNext;
            }
        }
        return true;
    }

    void Clear()
    {
        memset(mSlots, 0, sizeof(mSlots));
        mCount = 0;
    }

    unsigned int GetCount() const
    {
        return mCount;
    }

    const void* mSlots[SLOTS];
    unsigned int mCount;
};
