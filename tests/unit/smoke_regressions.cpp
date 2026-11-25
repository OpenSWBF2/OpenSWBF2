#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string.h>
#include "PebbleFL/Common/PblUnicode.h"

TEST_CASE_NAMED(FSmokeRegressionsTest1, "Unit::SmokeRegressions::strlcpy treats a null source as empty", "[SmokeFilter]")
{
    char acBuf[8];
    memset(acBuf, 'x', sizeof(acBuf));
    CHECK(strlcpy(acBuf, 0, sizeof(acBuf)) == 0);
    CHECK(acBuf[0] == '\0');
    CHECK(acBuf[1] == 'x');
}

TEST_CASE_NAMED(FSmokeRegressionsTest2, "Unit::SmokeRegressions::strlcpy still copies and truncates normal input", "[SmokeFilter]")
{
    char acBuf[4];
    CHECK(strlcpy(acBuf, "ab", sizeof(acBuf)) == 2);
    CHECK(strcmp(acBuf, "ab") == 0);
    strlcpy(acBuf, "abcdefgh", sizeof(acBuf));
    CHECK(strcmp(acBuf, "abc") == 0);
}
