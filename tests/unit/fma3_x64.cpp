#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <intrin.h>
#include <math.h>
#include <stdio.h>

TEST_CASE_NAMED(FFma3X64Test1, "Unit::Fma3X64::x64 start-up turns FMA3 off", "[EngineFilter]")
{
    int aiRegs[4] = {0, 0, 0, 0};
    __cpuid(aiRegs, 1);
    printf("fma3_cpu=%d\n", (aiRegs[2] >> 12) & 1);
    CHECK(_get_FMA3_enable() == 0);
}
