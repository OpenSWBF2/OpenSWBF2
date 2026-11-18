#pragma once

#include <stdint.h>

inline char* RedAlignUp(const void* p, uintptr_t uAlign)
{
    return (char*)(((uintptr_t)p + (uAlign - 1)) & ~(uAlign - 1));
}
