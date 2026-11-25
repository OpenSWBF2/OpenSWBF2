#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "Battlefront2/Source/SimDigestCore.h"

TEST_CASE_NAMED(FSimDigestCoreTest1, "Unit::SimDigestCore::sim digest core fnv1a empty", "[SmokeFilter]")
{
    CHECK(SimDigestCore::AddBytes(SimDigestCore::kOffsetBasis, "", 0) == 0x811C9DC5u);
}

TEST_CASE_NAMED(FSimDigestCoreTest2, "Unit::SimDigestCore::sim digest core fnv1a a", "[SmokeFilter]")
{
    CHECK(SimDigestCore::AddBytes(SimDigestCore::kOffsetBasis, "a", 1) == 0xE40C292Cu);
}

TEST_CASE_NAMED(FSimDigestCoreTest3, "Unit::SimDigestCore::sim digest core fnv1a foobar", "[SmokeFilter]")
{
    CHECK(SimDigestCore::AddBytes(SimDigestCore::kOffsetBasis, "foobar", 6) == 0xBF9CF968u);
}

TEST_CASE_NAMED(FSimDigestCoreTest4, "Unit::SimDigestCore::sim digest core float bits", "[SmokeFilter]")
{
    unsigned int uFromU32 = SimDigestCore::AddU32(SimDigestCore::kOffsetBasis, 0x3F800000u);
    unsigned int uFromFloat = SimDigestCore::AddFloat(SimDigestCore::kOffsetBasis, 1.0f);
    CHECK(uFromU32 == uFromFloat);
    CHECK(uFromFloat == 0x1B587698u);
}

TEST_CASE_NAMED(FSimDigestCoreTest5, "Unit::SimDigestCore::sim digest core signed zero", "[SmokeFilter]")
{
    unsigned int uPos = SimDigestCore::AddFloat(SimDigestCore::kOffsetBasis, 0.0f);
    unsigned int uNeg = SimDigestCore::AddFloat(SimDigestCore::kOffsetBasis, -0.0f);
    CHECK(uPos == 0x4B95F515u);
    CHECK(uNeg == 0xCB952B95u);
    CHECK(uPos != uNeg);
}

TEST_CASE_NAMED(FSimDigestCoreTest6, "Unit::SimDigestCore::sim digest core int bits", "[SmokeFilter]")
{
    CHECK(SimDigestCore::AddInt(SimDigestCore::kOffsetBasis, -1) == 0xE3160FB1u);
    CHECK(SimDigestCore::AddInt(SimDigestCore::kOffsetBasis, -1) == SimDigestCore::AddU32(SimDigestCore::kOffsetBasis, 0xFFFFFFFFu));
}
