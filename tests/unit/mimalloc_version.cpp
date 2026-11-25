#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "mimalloc/include/mimalloc.h"

TEST_CASE_NAMED(FMimallocVersionTest1, "Unit::MimallocVersion::mimalloc version is 3.6.1", "[SmokeFilter]")
{
    CHECK(mi_version() == 30601);
}
