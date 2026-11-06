#include "third_party/doctest/doctest.h"

#include <stdio.h>

#include "testutil.h"
#include "Battlefront2/Source/MemFailLog.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Util/RedWarning.h"

namespace
{

char sacRuntime[0x1000];

int CreateRuntimeHeap()
{
    int iHeap = RedCreateHeap(sacRuntime, sacRuntime + sizeof(sacRuntime), 0x10, "Runtime");
    REQUIRE(iHeap >= 0);
    return iHeap;
}

}

TEST_CASE("alloc failure handler logs and suppresses" * doctest::test_suite("regression"))
{
    ExpectLog expect("alloc failure heap");
    const int iHeap = CreateRuntimeHeap();
    const unsigned int uiFree = RedGetHeapFree(iHeap);
    REQUIRE(uiFree > 0);
    char acLine[128];
    sprintf(acLine, "alloc failure heap Runtime (%d) size 777 free %u", iHeap, uiFree);
    char acFree[64];
    sprintf(acFree, "free %u load ", uiFree);
    {
        LogCapture capture;
        MemFail::OnAllocFailure(iHeap, 777);
        CHECK(capture.Contains(acLine));
        CHECK(capture.Contains(acFree));
        CHECK(capture.Contains(" phys "));
        CHECK(capture.Contains(" virt "));
        CHECK(capture.Contains(" MB used "));
        CHECK(capture.Contains(" cap "));
        CHECK(capture.Contains(" peak "));
        CHECK(capture.Contains(" largestfree "));
    }
    {
        LogCapture capture;
        for (int i = 0; i < 14; ++i)
        {
            MemFail::OnAllocFailure(iHeap, 1);
        }
        CHECK(capture.Contains("alloc failure heap"));
        CHECK(!capture.Contains("suppressed"));
    }
    {
        LogCapture capture;
        MemFail::OnAllocFailure(iHeap, 2);
        CHECK(capture.Contains("alloc failure heap"));
        CHECK(!capture.Contains("suppressed"));
    }
    {
        LogCapture capture;
        MemFail::OnAllocFailure(iHeap, 3);
        CHECK(capture.Contains("further allocation failures suppressed"));
        CHECK(!capture.Contains("alloc failure heap"));
    }
    {
        LogCapture capture;
        MemFail::OnAllocFailure(iHeap, 4);
        CHECK(!capture.Contains("alloc failure heap"));
        CHECK(!capture.Contains("suppressed"));
    }
}

TEST_CASE("alloc failure handler tolerates bad heap id" * doctest::test_suite("regression"))
{
    ExpectLog expect("alloc failure heap");
    CreateRuntimeHeap();
    LogCapture capture;
    MemFail::OnAllocFailure(-1, 5);
    MemFail::OnAllocFailure(8, 6);
    CHECK(capture.Contains("alloc failure heap ? (-1) size 5 free 0"));
    CHECK(capture.Contains("alloc failure heap ? (8) size 6 free 0"));
}

static int sSinkCalls;

static void __cdecl ReenterSink(char const* pcLine, ...)
{
    ++sSinkCalls;
    MemFail::OnAllocFailure(1, 99);
}

TEST_CASE("alloc failure handler ignores re-entry" * doctest::test_suite("regression"))
{
    ExpectLog expect("alloc failure heap");
    const int iHeap = CreateRuntimeHeap();
    LogCapture capture;
    RedWarning::g_fnLogCallback = ReenterSink;
    MemFail::OnAllocFailure(iHeap, 8);
    CHECK(sSinkCalls == 1);
}
