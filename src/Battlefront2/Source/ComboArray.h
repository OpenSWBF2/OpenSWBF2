#pragma once

#include <new>
#include <string.h>

#include "RedEngineFL/Memory/RedMemoryPool.h"

template <class T>
T* ComboArrayAdd(MemoryPool& rPool, T*& raArray, unsigned int uiCount)
{
    const unsigned int uiBlock = rPool.GetSize();
    const unsigned int uiElement = (unsigned int)sizeof(T);
    const unsigned int uiHeld = (uiCount * uiElement + uiBlock - 1) / uiBlock;
    const unsigned int uiNeeded = ((uiCount + 1) * uiElement + uiBlock - 1) / uiBlock;
    if (uiNeeded == uiHeld)
    {
        return ::new (raArray + uiCount) T;
    }
    if (rPool.mUsed >= rPool.GetCount())
    {
        return 0;
    }
    if (uiHeld != 0 && uiNeeded == uiHeld + 1 && rPool.AllocateAt((char*)raArray + uiHeld * uiBlock, uiBlock) != 0)
    {
        return ::new (raArray + uiCount) T;
    }
    T* pRun = (T*)rPool.AllocateRun(uiNeeded, uiBlock);
    if (pRun == 0)
    {
        return 0;
    }
    if (uiHeld != 0)
    {
        memcpy((void*)pRun, (const void*)raArray, uiCount * uiElement);
        for (unsigned int i = 0; i < uiHeld; ++i)
        {
            rPool.Free((char*)raArray + i * uiBlock);
        }
    }
    raArray = pRun;
    return ::new (raArray + uiCount) T;
}
