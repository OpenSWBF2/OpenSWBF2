#pragma once

#include <new>
#include <string.h>

#include "RedEngineFL/Memory/RedMemoryPool.h"

template <class T>
T* ComboArrayAdd(MemoryPool& rPool, T*& raArray, unsigned int uiCount)
{
    if (rPool.mUsed >= rPool.GetCount())
    {
        return 0;
    }
    void* pBlock = uiCount != 0 ? rPool.AllocateAt(raArray + uiCount, sizeof(T)) : rPool.AllocateRun(1, sizeof(T));
    if (pBlock == 0 && uiCount != 0)
    {
        T* pRun = (T*)rPool.AllocateRun(uiCount + 1, sizeof(T));
        if (pRun == 0)
        {
            return 0;
        }
        memcpy((void*)pRun, (const void*)raArray, uiCount * sizeof(T));
        for (unsigned int i = 0; i < uiCount; ++i)
        {
            rPool.Free(raArray + i);
        }
        raArray = pRun;
        pBlock = pRun + uiCount;
    }
    if (pBlock == 0)
    {
        return 0;
    }
    if (uiCount == 0)
    {
        raArray = (T*)pBlock;
    }
    return ::new (pBlock) T;
}
