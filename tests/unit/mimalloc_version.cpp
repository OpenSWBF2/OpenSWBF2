#include "third_party/doctest/doctest.h"

#include "mimalloc/include/mimalloc.h"

TEST_CASE("mimalloc version is 3.6.1" * doctest::test_suite("unit"))
{
    CHECK(mi_version() == 30601);
}
