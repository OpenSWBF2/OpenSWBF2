#pragma once

#include <stdint.h>

namespace pcAddrSpace
{
    uintptr_t HogLimit();
    uintptr_t MinRegion();
    unsigned int ReserveBelow(uintptr_t uLimit, uintptr_t uMinRegion);
    unsigned int FreeRegionsBelow(uintptr_t uLimit, uintptr_t uMinSize, bool bGranuleAligned);
}
