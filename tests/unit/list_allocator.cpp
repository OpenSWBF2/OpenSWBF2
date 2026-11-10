#include "third_party/doctest/doctest.h"

#include <string.h>

#include "RedEngineFL/Util/ext/ListAllocator.h"

namespace
{

const int kLaPages = 8;
const unsigned int kLaPageBytes = 0x2000;
alignas(16) char sacLaPool[kLaPages * kLaPageBytes];
char* sapLaGiven[kLaPages];
int siLaGiven;
bool sbLaDescending;

class LaPages : public util::IAllocator
{
public:
    virtual void Allocate(util::Buffer& rBuf)
    {
        if (rBuf.size_ > kLaPageBytes || siLaGiven >= kLaPages)
        {
            rBuf.data_ = 0;
            return;
        }
        const int iSlot = sbLaDescending ? kLaPages - 1 - siLaGiven : siLaGiven;
        rBuf.data_ = sacLaPool + iSlot * kLaPageBytes;
        sapLaGiven[siLaGiven++] = (char*)rBuf.data_;
    }

    virtual void Deallocate(util::Buffer&)
    {
    }
};

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
    LaPages kPages;
    util::ListAllocator* pAlloc = util::ListAllocator::Create(kLaPageBytes, kLaPages * kLaPageBytes, &kPages);
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
