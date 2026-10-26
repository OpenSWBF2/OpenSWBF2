#include "third_party/doctest/doctest.h"

#include <string.h>

#include "testutil.h"
#include "Battlefront2/Source/MemFailLog.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Util/RedWarning.h"

extern "C" int maxheaps;
extern "C" HeapObj* heapobj;

namespace
{

HeapObj sFakeHeaps[4];

void SetupFakeHeaps()
{
    memset(sFakeHeaps, 0, sizeof(sFakeHeaps));
    sFakeHeaps[0].size = 777;
    sFakeHeaps[3].size = 777;
    sFakeHeaps[2].name = "Runtime";
    sFakeHeaps[2].size = 1234;
    heapobj = sFakeHeaps + 1;
    maxheaps = 2;
}

}

TEST_CASE("alloc failure handler logs and suppresses" * doctest::test_suite("regression"))
{
    ExpectLog expect("alloc failure heap");
    SetupFakeHeaps();
    {
        LogCapture capture;
        MemFail::OnAllocFailure(1, 777);
        CHECK(capture.Contains("alloc failure heap Runtime (1) size 777 free 1234"));
        CHECK(capture.Contains("free 1234 load "));
        CHECK(capture.Contains(" phys "));
        CHECK(capture.Contains(" virt "));
    }
    {
        LogCapture capture;
        for (int i = 0; i < 14; ++i)
        {
            MemFail::OnAllocFailure(1, 1);
        }
        CHECK(capture.Contains("alloc failure heap"));
        CHECK(!capture.Contains("suppressed"));
    }
    {
        LogCapture capture;
        MemFail::OnAllocFailure(1, 2);
        CHECK(capture.Contains("alloc failure heap"));
        CHECK(!capture.Contains("suppressed"));
    }
    {
        LogCapture capture;
        MemFail::OnAllocFailure(1, 3);
        CHECK(capture.Contains("further allocation failures suppressed"));
        CHECK(!capture.Contains("alloc failure heap"));
    }
    {
        LogCapture capture;
        MemFail::OnAllocFailure(1, 4);
        CHECK(!capture.Contains("alloc failure heap"));
        CHECK(!capture.Contains("suppressed"));
    }
}

TEST_CASE("alloc failure handler tolerates bad heap id" * doctest::test_suite("regression"))
{
    ExpectLog expect("alloc failure heap");
    SetupFakeHeaps();
    LogCapture capture;
    MemFail::OnAllocFailure(-1, 5);
    MemFail::OnAllocFailure(2, 6);
    CHECK(capture.Contains("alloc failure heap ? (-1) size 5 free 0"));
    CHECK(capture.Contains("alloc failure heap ? (2) size 6 free 0"));
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
    SetupFakeHeaps();
    LogCapture capture;
    RedWarning::g_fnLogCallback = ReenterSink;
    MemFail::OnAllocFailure(1, 8);
    CHECK(sSinkCalls == 1);
}
