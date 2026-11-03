#pragma once

#include "PebbleFL/Common/PblAsan.h"

class RedStack
{
public:
    RedStack() : mMemIdx(sMemIdx)
    {
#if defined(__SANITIZE_ADDRESS__)
        PblAsanPoison(&sMem[sMemIdx], sizeof(sMem) - sMemIdx);
#endif
    }

    ~RedStack()
    {
#if defined(__SANITIZE_ADDRESS__)
        PblAsanPoison(&sMem[mMemIdx], sMemIdx - mMemIdx);
#endif
        sMemIdx = mMemIdx;
    }

    void* Alloc(int iSize)
    {
#if defined(__SANITIZE_ADDRESS__)
        PblAsanUnpoison(&sMem[sMemIdx], iSize);
#endif
        if (iSize & 0xf)
        {
            iSize = (iSize & ~0xf) + 0x10;
        }
        void* p = &sMem[sMemIdx];
        sMemIdx += iSize;
        return p;
    }

    int mMemIdx;

    static char sMem[40960];
    static int sMemIdx;
};
 
