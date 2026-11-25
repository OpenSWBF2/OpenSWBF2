#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stdint.h>
#include <string.h>

#include "PebbleFL/Common/PblAsan.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/movie/pc/pcRedMovieAlloc.h"

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

extern "C"
{
    const char* __cdecl avutil_license(void);
    const char* __cdecl avcodec_license(void);
    const char* __cdecl avformat_license(void);
    void* __cdecl av_malloc(size_t uSize);
    void* __cdecl av_realloc(void* pBlock, size_t uSize);
    void __cdecl av_free(void* pBlock);
}

namespace
{

alignas(16) char sacMovieArena[0x1000000];

RedHeapStats MovieStats(int iHeap)
{
    RedHeapStats kStats;
    memset(&kStats, 0, sizeof(kStats));
    REQUIRE(RedGetHeapStats(iHeap, &kStats));
    return kStats;
}

class MovieHeapScope
{
public:
    MovieHeapScope() : miHeap(-1)
    {
        MovieAllocRegister();
        miHeap = RedCreateHeap(sacMovieArena, sacMovieArena + sizeof(sacMovieArena), 0x10, "MovieTest");
        REQUIRE(miHeap >= 0);
    }

    ~MovieHeapScope()
    {
        if (miHeap >= 0)
        {
            RedDestroyHeap(miHeap);
        }
    }

    void* Malloc(size_t uSize)
    {
        const int iSaved = RedSetCurrentHeap(miHeap);
        void* p = av_malloc(uSize);
        RedSetCurrentHeap(iSaved);
        return p;
    }

    void* Realloc(void* pBlock, size_t uSize)
    {
        const int iSaved = RedSetCurrentHeap(miHeap);
        void* p = av_realloc(pBlock, uSize);
        RedSetCurrentHeap(iSaved);
        return p;
    }

    void Free(void* pBlock)
    {
        const int iSaved = RedSetCurrentHeap(miHeap);
        av_free(pBlock);
        RedSetCurrentHeap(iSaved);
    }

    int miHeap;
};

class AllocatorRestore
{
public:
    ~AllocatorRestore()
    {
        ossf2_set_allocator(MovieAlloc, MovieRealloc, MovieFree);
    }
};

PBL_ASAN_NO_SANITIZE size_t StoredSize(void* p)
{
    const volatile unsigned char* pcHeader = (const volatile unsigned char*)p - 16;
    size_t uSize = 0;
    for (size_t i = 0; i < sizeof(uSize); ++i)
    {
        uSize |= (size_t)pcHeader[i] << (8 * i);
    }
    return uSize;
}

void Fill(void* p, size_t uSize, unsigned char ucSeed)
{
    for (size_t i = 0; i < uSize; ++i)
    {
        ((unsigned char*)p)[i] = (unsigned char)(ucSeed + i * 7);
    }
}

bool Matches(const void* p, size_t uSize, unsigned char ucSeed)
{
    for (size_t i = 0; i < uSize; ++i)
    {
        if (((const unsigned char*)p)[i] != (unsigned char)(ucSeed + i * 7))
        {
            return false;
        }
    }
    return true;
}

}

TEST_CASE_NAMED(FMovieFfmpegTest1, "Unit::MovieFfmpeg::movie ffmpeg libraries report the lgpl 2.1 or later licence", "[SmokeFilter]")
{
    CHECK(strcmp(avutil_license(), "LGPL version 2.1 or later") == 0);
    CHECK(strcmp(avcodec_license(), "LGPL version 2.1 or later") == 0);
    CHECK(strcmp(avformat_license(), "LGPL version 2.1 or later") == 0);
}

TEST_CASE_NAMED(FMovieFfmpegTest2, "Unit::MovieFfmpeg::movie ffmpeg allocation fails closed without a registered allocator", "[SmokeFilter]")
{
    MovieHeapScope kScope;
    const RedHeapStats kBefore = MovieStats(kScope.miHeap);
    {
        AllocatorRestore kRestore;
        ossf2_set_allocator(0, 0, 0);
        CHECK(kScope.Malloc(64) == nullptr);
        CHECK(kScope.Realloc(nullptr, 64) == nullptr);
    }
    CHECK(MovieStats(kScope.miHeap).uAllocs == kBefore.uAllocs);
    void* p = kScope.Malloc(64);
    CHECK(p != nullptr);
    kScope.Free(p);
}

TEST_CASE_NAMED(FMovieFfmpegTest3, "Unit::MovieFfmpeg::movie ffmpeg av_malloc lands on the current red heap with a 16 byte size header", "[SmokeFilter]")
{
    MovieHeapScope kScope;
    const size_t auSizes[6] = {1, 15, 16, 17, 4095, 65536};
    for (int i = 0; i < 6; ++i)
    {
        INFO("size " << auSizes[i]);
        const RedHeapStats kBefore = MovieStats(kScope.miHeap);
        void* p = kScope.Malloc(auSizes[i]);
        REQUIRE(p != nullptr);
        CHECK(((uintptr_t)p & 15) == 0);
        const RedHeapStats kAfter = MovieStats(kScope.miHeap);
        CHECK(kAfter.uAllocs == kBefore.uAllocs + 1);
        CHECK(kAfter.uUsed > kBefore.uUsed);
        const bool bOwned = RedHeapOwner((char*)p - 16) == kScope.miHeap;
        CHECK(bOwned);
        if (!bOwned)
        {
            continue;
        }
        CHECK(StoredSize(p) == auSizes[i]);
        kScope.Free(p);
        CHECK(MovieStats(kScope.miHeap).uUsed == kBefore.uUsed);
    }
}

TEST_CASE_NAMED(FMovieFfmpegTest4, "Unit::MovieFfmpeg::movie ffmpeg av_realloc keeps the leading bytes when it grows and shrinks", "[SmokeFilter]")
{
    MovieHeapScope kScope;
    const unsigned int uStart = MovieStats(kScope.miHeap).uUsed;
    void* p = kScope.Malloc(17);
    REQUIRE(p != nullptr);
    Fill(p, 17, 0x31);
    void* q = kScope.Realloc(p, 4095);
    REQUIRE(q != nullptr);
    CHECK(((uintptr_t)q & 15) == 0);
    CHECK(Matches(q, 17, 0x31));
    CHECK(StoredSize(q) == 4095);
    Fill(q, 4095, 0x52);
    void* r = kScope.Realloc(q, 5);
    REQUIRE(r != nullptr);
    CHECK(((uintptr_t)r & 15) == 0);
    CHECK(Matches(r, 5, 0x52));
    CHECK(StoredSize(r) == 5);
    kScope.Free(r);
    CHECK(MovieStats(kScope.miHeap).uUsed == uStart);
}

TEST_CASE_NAMED(FMovieFfmpegTest5, "Unit::MovieFfmpeg::movie ffmpeg av_malloc refuses one byte past the bundle cap", "[SmokeFilter]")
{
    MovieHeapScope kScope;
    const RedHeapStats kBefore = MovieStats(kScope.miHeap);
    CHECK(kScope.Malloc(7864321) == nullptr);
    const RedHeapStats kAfter = MovieStats(kScope.miHeap);
    CHECK(kAfter.uAllocs == kBefore.uAllocs);
    CHECK(kAfter.uFailed == kBefore.uFailed);
    void* p = kScope.Malloc(7864320);
    CHECK(p != nullptr);
    kScope.Free(p);
    CHECK(MovieStats(kScope.miHeap).uUsed == kBefore.uUsed);
}

#if defined(__SANITIZE_ADDRESS__)
TEST_CASE_NAMED(FMovieFfmpegTest6, "Unit::MovieFfmpeg::movie ffmpeg blocks keep their 16 byte size header poisoned under asan", "[SmokeFilter]")
{
    MovieHeapScope kScope;
    const size_t auSizes[4] = {1, 16, 4095, 65536};
    for (int i = 0; i < 4; ++i)
    {
        INFO("size " << auSizes[i]);
        unsigned char* p = (unsigned char*)kScope.Malloc(auSizes[i]);
        REQUIRE(p != nullptr);
        CHECK(__asan_address_is_poisoned(p - 1) != 0);
        CHECK(__asan_address_is_poisoned(p - 16) != 0);
        CHECK(__asan_region_is_poisoned(p - 16, 16) == p - 16);
        CHECK(__asan_region_is_poisoned(p, auSizes[i]) == nullptr);
        CHECK(StoredSize(p) == auSizes[i]);
        unsigned char* q = (unsigned char*)kScope.Realloc(p, auSizes[i] * 2);
        REQUIRE(q != nullptr);
        CHECK(__asan_address_is_poisoned(q - 1) != 0);
        CHECK(__asan_address_is_poisoned(q - 16) != 0);
        CHECK(__asan_region_is_poisoned(q, auSizes[i] * 2) == nullptr);
        CHECK(StoredSize(q) == auSizes[i] * 2);
        kScope.Free(q);
    }
}
#endif
