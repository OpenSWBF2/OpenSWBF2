#include "third_party/doctest/doctest.h"

#include <stdlib.h>

#if defined(__SANITIZE_ADDRESS__)
TEST_CASE("asan canary heap overflow is reported" * doctest::test_suite("asan_canary"))
{
    volatile char* p = (volatile char*)malloc(8);
    p[8] = 1;
    free((void*)p);
}
#endif
