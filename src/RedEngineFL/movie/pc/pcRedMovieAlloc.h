#pragma once

#include <stddef.h>

#include "RedEngineFL/movie/pc/pcRedMovieHeader.h"

const size_t kMovieAllocHeader = 16;
const size_t kMovieBundleBytesPerBlock = 640;
const size_t kMovieAllocCap =
    (size_t)((kMovieMaxWidth + 7) >> 3) * ((kMovieMaxHeight + 7) >> 3) * kMovieBundleBytesPerBlock;

void* __cdecl MovieAlloc(size_t uSize);
void* __cdecl MovieRealloc(void* pBlock, size_t uSize);
void __cdecl MovieFree(void* pBlock);
unsigned int MovieAllocFailures();

#ifdef OPENSWBF2_TEST
extern unsigned int gRedMovieLiveBlocks;
#endif

extern "C"
{
    void __cdecl ossf2_set_allocator(void* (__cdecl* pfnAlloc)(size_t), void* (__cdecl* pfnRealloc)(void*, size_t),
                                     void(__cdecl* pfnFree)(void*));
    void __cdecl av_max_alloc(size_t uMax);
}

inline void MovieAllocRegister()
{
    static bool sbRegistered = false;
    if (sbRegistered)
    {
        return;
    }
    ossf2_set_allocator(MovieAlloc, MovieRealloc, MovieFree);
    av_max_alloc(kMovieAllocCap);
    sbRegistered = true;
}
