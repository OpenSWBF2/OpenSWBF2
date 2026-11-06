#include "third_party/doctest/doctest.h"

#include <string.h>

#include "testutil.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Memory/RedMemoryReport.h"

TEST_CASE("report formats heap line" * doctest::test_suite("regression"))
{
    RedHeapStats kStats = {100, 40, 60, 7, 1};
    char acLine[256];
    REQUIRE(RedMemoryReport::FormatHeapLine("Runtime", kStats, acLine, sizeof(acLine)));
    CHECK(strcmp(acLine, "OPENSWBF2 MEMREPORT heap=Runtime cap=100 peak=60 used=40 allocs=7 failed=1") == 0);
    char acSmall[16];
    memset(acSmall, 'x', sizeof(acSmall));
    REQUIRE(RedMemoryReport::FormatHeapLine("Runtime", kStats, acSmall, sizeof(acSmall)));
    CHECK(acSmall[15] == 0);
    CHECK(strcmp(acSmall, "OPENSWBF2 MEMRE") == 0);
    CHECK(!RedMemoryReport::FormatHeapLine("Runtime", kStats, 0, 16));
    CHECK(!RedMemoryReport::FormatHeapLine("Runtime", kStats, acLine, 0));
}

TEST_CASE("report formats pool line" * doctest::test_suite("regression"))
{
    char acLine[256];
    REQUIRE(RedMemoryReport::FormatPoolLine("Weapon", 64, 48, 70, 2, acLine, sizeof(acLine)));
    CHECK(strcmp(acLine, "OPENSWBF2 MEMREPORT pool=Weapon count=64 size=48 peak=70 grown=2") == 0);
}

TEST_CASE("report formats pool full line" * doctest::test_suite("regression"))
{
    char acLine[256];
    REQUIRE(RedMemoryReport::FormatPoolFull("crashtestpool", 3, acLine, sizeof(acLine)));
    CHECK(strcmp(acLine, "Memory pool \"crashtestpool\" is full; raise count to at least 3") == 0);
}

TEST_CASE("report formats cloth line" * doctest::test_suite("regression"))
{
    char acLine[256];
    REQUIRE(RedMemoryReport::FormatCloth(300000, true, 131072, 215536, acLine, sizeof(acLine)));
    CHECK(strcmp(acLine, "OPENSWBF2 CLOTH free=300000 enabled=1 low=131072 high=215536") == 0);
    REQUIRE(RedMemoryReport::FormatCloth(-5, false, 0, 0, acLine, sizeof(acLine)));
    CHECK(strcmp(acLine, "OPENSWBF2 CLOTH free=-5 enabled=0 low=0 high=0") == 0);
}

TEST_CASE("report formats memfail line" * doctest::test_suite("regression"))
{
    RedMemoryReport::MemFailInfo kInfo = {"Runtime", 3, 777, 12, 45, 1024, 512, 900, 1000, 950, 65536};
    char acLine[256];
    REQUIRE(RedMemoryReport::FormatMemFail(kInfo, acLine, sizeof(acLine)));
    CHECK(strcmp(acLine, "alloc failure heap Runtime (3) size 777 free 12 load 45% phys 1024 MB virt 512 MB "
                         "used 900 cap 1000 peak 950 largestfree 65536") == 0);
}

TEST_CASE("report finds largest free region" * doctest::test_suite("regression"))
{
    CHECK(RedMemoryReport::LargestFreeRegion() > 0x10000u);
}

namespace
{

char sacReportHeap[0x1000];

}

TEST_CASE("report emits one line per heap and created pool" * doctest::test_suite("regression"))
{
    const int iHeap = RedCreateHeap(sacReportHeap, sacReportHeap + sizeof(sacReportHeap), 0x10, "T1rep");
    REQUIRE(iHeap >= 0);
    MemoryPool kPool("t1reppool", 16);
    kPool.Create(4, 16, 0);
    void* pBlock = kPool.Allocate(16);
    REQUIRE(pBlock);
    MemoryPool kUnused("t1repnone", 16);
    MemoryPool kReset("t1represet", 16);
    kReset.Create(2, 16, 0);
    void* pResetBlock = kReset.Allocate(16);
    REQUIRE(pResetBlock);
    kReset.Free(pResetBlock);
    kReset.Destroy();
    kReset.Reset();
    {
        LogCapture capture;
        RedMemoryReport::LogMemReport("exit");
        CHECK(capture.Contains("OPENSWBF2 MEMREPORT source=exit"));
        CHECK(capture.Contains("heap=T1rep "));
        CHECK(capture.Contains("pool=t1reppool count=4 size=16 peak=1 grown=0"));
        CHECK(capture.Contains("pool=t1represet count=0 size=16 peak=1 grown=0"));
        CHECK(capture.Count("t1repnone") == 0);
    }
    kPool.Free(pBlock);
    RedDestroyHeap(iHeap);
}
