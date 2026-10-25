#include "third_party/doctest/doctest.h"

#include <string.h>
#include "PebbleFL/Common/PblUnicode.h"

TEST_CASE("strlcpy treats a null source as empty" * doctest::test_suite("unit"))
{
    char acBuf[8];
    memset(acBuf, 'x', sizeof(acBuf));
    CHECK(strlcpy(acBuf, 0, sizeof(acBuf)) == 0);
    CHECK(acBuf[0] == '\0');
    CHECK(acBuf[1] == 'x');
}

TEST_CASE("strlcpy still copies and truncates normal input" * doctest::test_suite("unit"))
{
    char acBuf[4];
    CHECK(strlcpy(acBuf, "ab", sizeof(acBuf)) == 2);
    CHECK(strcmp(acBuf, "ab") == 0);
    strlcpy(acBuf, "abcdefgh", sizeof(acBuf));
    CHECK(strcmp(acBuf, "abc") == 0);
}
