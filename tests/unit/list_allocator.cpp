#include "third_party/doctest/doctest.h"

#include <string.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryBackend.h"
#include "RedEngineFL/Util/ext/ListAllocator.h"

namespace
{

const int kLaPages = 8;
const unsigned int kLaPageBytes = 0x2000;
alignas(16) char sacLaPool[kLaPages * kLaPageBytes];
alignas(16) char sacLaScratch[0x4000];
unsigned int suLaScratchUsed;
char* sapLaGiven[kLaPages];
int siLaGiven;
bool sbLaDescending;
int siLaHeap = -1;

void* LaInit(void* start, void* end, int iMax)
{
    return kRedArenaBackend.init(start, end, iMax);
}

size_t LaCreate(int heap, void* start, void* end, unsigned int align, const char* name)
{
    (void)heap;
    (void)align;
    (void)name;
    return (char*)end - (char*)start;
}

void LaDestroy(int heap)
{
    (void)heap;
}

void* LaAlloc(int heap, size_t size, bool bTop, size_t* pReserved)
{
    (void)heap;
    (void)bTop;
    if (size < kLaPageBytes)
    {
        const unsigned int uSize = ((unsigned int)size + 15) & ~15u;
        if (suLaScratchUsed + uSize > sizeof(sacLaScratch))
        {
            return 0;
        }
        char* pScratch = sacLaScratch + suLaScratchUsed;
        suLaScratchUsed += uSize;
        *pReserved = size;
        return pScratch;
    }
    if (size > kLaPageBytes || siLaGiven >= kLaPages)
    {
        return 0;
    }
    const int iSlot = sbLaDescending ? kLaPages - 1 - siLaGiven : siLaGiven;
    char* p = sacLaPool + iSlot * kLaPageBytes;
    sapLaGiven[siLaGiven++] = p;
    *pReserved = size;
    return p;
}

size_t LaFree(int heap, void* block)
{
    (void)heap;
    (void)block;
    return kLaPageBytes;
}

int LaOwner(const void* block)
{
    const char* p = (const char*)block;
    const bool bPool = p >= sacLaPool && p < sacLaPool + sizeof(sacLaPool);
    const bool bScratch = p >= sacLaScratch && p < sacLaScratch + sizeof(sacLaScratch);
    return (bPool || bScratch) ? siLaHeap : -1;
}

int LaMoveEnd(int iHeap, int iDelta, bool* pbAllocFailed)
{
    (void)iHeap;
    (void)iDelta;
    *pbAllocFailed = true;
    return 0;
}

bool LaVisit(int heap, bool (*pfnBlock)(void* pUser, size_t uSize, void* pArg), void* pArg)
{
    (void)heap;
    (void)pfnBlock;
    (void)pArg;
    return false;
}

const RedHeapBackend kLaBackend = { LaInit, LaCreate, LaDestroy, LaAlloc, LaFree, LaOwner, LaMoveEnd, LaVisit, 0 };

struct LaPick
{
    int iPage;
    int iOffset;
};

LaPick LaLocate(void* p)
{
    LaPick kPick = { -1, -1 };
    for (int i = 0; i < siLaGiven; ++i)
    {
        if ((char*)p >= sapLaGiven[i] && (char*)p < sapLaGiven[i] + kLaPageBytes)
        {
            kPick.iPage = i;
            kPick.iOffset = (int)((char*)p - sapLaGiven[i]);
        }
    }
    return kPick;
}

void LaRun(bool bDescending, LaPick* pSmall, LaPick* pLarge)
{
    sbLaDescending = bDescending;
    siLaGiven = 0;
    suLaScratchUsed = 0;
    const RedHeapBackend* pSaved = gRedHeapBackend;
    gRedHeapBackend = &kLaBackend;
    siLaHeap = RedCreateHeap(sacLaPool, sacLaPool + sizeof(sacLaPool), 0x10, "T1ListAlloc");
    gRedHeapBackend = pSaved;
    REQUIRE(siLaHeap > 0);
    const int iOldHeap = RedSetCurrentHeap(siLaHeap);
    util::ListAllocator* pAlloc = util::ListAllocator::Create(kLaPageBytes, kLaPages * kLaPageBytes);
    REQUIRE(pAlloc);
    util::Buffer akBuf[3];
    for (int i = 0; i < 3; ++i)
    {
        akBuf[i] = util::Buffer(0, 0x1800);
        pAlloc->Allocate(akBuf[i]);
        REQUIRE(akBuf[i].data_);
    }
    CHECK(siLaGiven == 3);
    CHECK(LaLocate(akBuf[0].data_).iPage == 0);
    CHECK(LaLocate(akBuf[1].data_).iPage == 1);
    CHECK(LaLocate(akBuf[2].data_).iPage == 2);
    for (int i = 0; i < 3; ++i)
    {
        pAlloc->Deallocate(akBuf[i]);
    }
    util::Buffer kSmall(0, 0x400);
    pAlloc->Allocate(kSmall);
    REQUIRE(kSmall.data_);
    *pSmall = LaLocate(kSmall.data_);
    util::Buffer kLarge(0, 0x3000);
    pAlloc->Allocate(kLarge);
    *pLarge = LaLocate(kLarge.data_);
    if (kLarge.data_)
    {
        pAlloc->Deallocate(kLarge);
    }
    pAlloc->Deallocate(kSmall);
    delete pAlloc;
    RedSetCurrentHeap(iOldHeap);
    RedDestroyHeap(siLaHeap);
    siLaHeap = -1;
}

}

TEST_CASE("cloth list allocator picks the same block whatever order its pages have in memory" *
          doctest::test_suite("regression"))
{
    LaPick kAscSmall;
    LaPick kAscLarge;
    LaPick kDescSmall;
    LaPick kDescLarge;
    LaRun(false, &kAscSmall, &kAscLarge);
    LaRun(true, &kDescSmall, &kDescLarge);
    CHECK(kAscSmall.iPage == 0);
    CHECK(kDescSmall.iPage == kAscSmall.iPage);
    CHECK(kDescSmall.iOffset == kAscSmall.iOffset);
    CHECK(kAscLarge.iPage == -1);
    CHECK(kDescLarge.iPage == -1);
}
