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

HeapObj sFakeHeaps[2];

void SetupFakeHeaps()
{
    memset(sFakeHeaps, 0, sizeof(sFakeHeaps));
    sFakeHeaps[1].name = "Runtime";
    sFakeHeaps[1].size = 1234;
    heapobj = sFakeHeaps;
    maxheaps = 2;
}

}

TEST_CASE("alloc failure handler logs and suppresses" * doctest::test_suite("regression"))
{
    SetupFakeHeaps();
    {
        LogCapture capture;
        MemFail::OnAllocFailure(1, 777);
        CHECK(capture.Contains("alloc failure heap Runtime (1) size 777 free 1234"));
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
    SetupFakeHeaps();
    LogCapture capture;
    MemFail::OnAllocFailure(-1, 5);
    MemFail::OnAllocFailure(2, 6);
    CHECK(capture.Contains("alloc failure heap ? (-1) size 5 free 0"));
}

static void __cdecl ReenterSink(char const* pcLine, ...)
{
    MemFail::OnAllocFailure(1, 99);
}

TEST_CASE("alloc failure handler ignores re-entry" * doctest::test_suite("regression"))
{
    SetupFakeHeaps();
    void(__cdecl* pOld)(char const*, ...) = RedWarning::g_fnLogCallback;
    RedWarning::g_fnLogCallback = ReenterSink;
    MemFail::OnAllocFailure(1, 8);
    RedWarning::g_fnLogCallback = pOld;
    CHECK(true);
}
