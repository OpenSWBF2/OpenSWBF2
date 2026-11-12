#include "PblCrtHeap.h"

#include <intrin.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#ifdef OPENSWBF2_TEST
static volatile long sCrtLiveBytes;

static void NoteCrtBlock(void* p, bool bAlloc)
{
    if (p != 0)
    {
        const long lSize = (long)_msize(p);
        _InterlockedExchangeAdd(&sCrtLiveBytes, bAlloc ? lSize : -lSize);
    }
}

unsigned int PblCrtLiveBytes()
{
    return (unsigned int)sCrtLiveBytes;
}
#endif

void* PblCrtAlloc(size_t uSize)
{
    void* p = malloc(uSize);
#ifdef OPENSWBF2_TEST
    NoteCrtBlock(p, true);
#endif
    return p;
}

char* PblCrtStrdup(const char* pcText)
{
    const size_t uSize = strlen(pcText) + 1;
    char* pcCopy = (char*)PblCrtAlloc(uSize);
    if (pcCopy != 0)
    {
        memcpy(pcCopy, pcText, uSize);
    }
    return pcCopy;
}

void PblCrtFree(void* p)
{
#ifdef OPENSWBF2_TEST
    NoteCrtBlock(p, false);
#endif
    free(p);
}

void PblCrtFreeUncounted(void* p)
{
    free(p);
}
