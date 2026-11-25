#include "RedEngineFL/movie/pc/pcRedMovieAlloc.h"

#include <stdint.h>
#include <string.h>

#include "PebbleFL/Common/PblAsan.h"
#include "RedEngineFL/Memory/RedMemory.h"

namespace
{

char* MovieBase(void* pBlock)
{
    return (char*)pBlock - kMovieAllocHeader;
}

size_t MovieSize(void* pBlock)
{
    size_t uSize = 0;
    PblAsanUnpoison(MovieBase(pBlock), kMovieAllocHeader);
    memcpy(&uSize, MovieBase(pBlock), sizeof(uSize));
    PblAsanPoison(MovieBase(pBlock), kMovieAllocHeader);
    return uSize;
}

}

#ifdef OPENSWBF2_TEST
unsigned int gRedMovieLiveBlocks;
#endif

static unsigned int suMovieAllocFailures;

unsigned int MovieAllocFailures()
{
    return suMovieAllocFailures;
}

void* __cdecl MovieAlloc(size_t uSize)
{
    if (uSize > (size_t)-1 - kMovieAllocHeader)
    {
        return 0;
    }
    char* pBase = (char*)RedTryAllocFromHeap(__RedCurrHeap, uSize + kMovieAllocHeader);
    if (!pBase)
    {
        ++suMovieAllocFailures;
        return 0;
    }
    if (((uintptr_t)pBase & (kMovieAllocHeader - 1)) != 0)
    {
        RedFreeToHeap(__RedCurrHeap, pBase, 0);
        return 0;
    }
    memcpy(pBase, &uSize, sizeof(uSize));
    PblAsanPoison(pBase, kMovieAllocHeader);
#ifdef OPENSWBF2_TEST
    ++gRedMovieLiveBlocks;
#endif
    return pBase + kMovieAllocHeader;
}

void* __cdecl MovieRealloc(void* pBlock, size_t uSize)
{
    if (!pBlock)
    {
        return MovieAlloc(uSize);
    }
    void* pNew = MovieAlloc(uSize);
    if (!pNew)
    {
        return 0;
    }
    const size_t uOld = MovieSize(pBlock);
    memcpy(pNew, pBlock, uOld < uSize ? uOld : uSize);
    MovieFree(pBlock);
    return pNew;
}

void __cdecl MovieFree(void* pBlock)
{
    if (pBlock)
    {
        PblAsanUnpoison(MovieBase(pBlock), kMovieAllocHeader);
        RedFreeToHeap(__RedCurrHeap, MovieBase(pBlock), 0);
#ifdef OPENSWBF2_TEST
        --gRedMovieLiveBlocks;
#endif
    }
}
