#include "third_party/doctest/doctest.h"

#include "RedEngineFL/Graphics/PC/pcRedPoolMath.h"

TEST_CASE("bone matrix pool" * doctest::test_suite("regression"))
{
    unsigned int uiOffset = 3000;
    unsigned int uiStart = 99;
    CHECK(PoolMath::PoolAlloc(3062, uiOffset, 100, uiStart));
    CHECK(uiStart == 0);
    CHECK(uiOffset == 100);

    uiOffset = 0;
    CHECK(PoolMath::PoolAlloc(3062, uiOffset, 3061, uiStart));
    CHECK(uiStart == 0);
    CHECK(uiOffset == 3061);

    uiOffset = 5;
    CHECK(!PoolMath::PoolAlloc(3062, uiOffset, 3062, uiStart));
    CHECK(uiOffset == 5);
    CHECK(!PoolMath::PoolAlloc(3062, uiOffset, 3063, uiStart));
    CHECK(uiOffset == 5);

    for (unsigned int o = 0; o < 3062; ++o)
    {
        for (unsigned int c = 0; c < 3071; ++c)
        {
            unsigned int uiOff = o;
            if (PoolMath::PoolAlloc(3062, uiOff, c, uiStart))
            {
                CHECK(uiOff < 3062);
                CHECK(uiStart + c <= 3061);
            }
        }
    }
}

TEST_CASE("render item pool" * doctest::test_suite("regression"))
{
    CHECK(PoolMath::RenderItemHasRoom(4747, 4748));
    CHECK(!PoolMath::RenderItemHasRoom(4748, 4748));
}
