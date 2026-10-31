#pragma once

#include <stddef.h>

inline intptr_t PblPtrOrderKey(const void* pPtr)
{
    return (intptr_t)((uintptr_t)pPtr ^ ((uintptr_t)1 << (sizeof(void*) * 8 - 1)));
}

inline bool PblPtrAfter(const void* pA, const void* pB)
{
    return (uintptr_t)pA > (uintptr_t)pB;
}
