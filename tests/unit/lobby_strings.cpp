#include "third_party/doctest/doctest.h"

#include <string.h>
#include "PebbleFL/Common/PblUnicode.h"

static void CheckTruncate(unsigned int uiSize)
{
    char acSource[512];
    char acDest[512];
    memset(acSource, 'A', 511);
    acSource[511] = 0;
    memset(acDest, 0x7F, sizeof(acDest));
    strlcpy(acDest, acSource, uiSize);
    CHECK(strlen(acDest) == uiSize - 1);
    CHECK(acDest[uiSize - 1] == 0);
    CHECK(acDest[uiSize] == 0x7F);
}

TEST_CASE("lobby host name truncates to 64 and terminates" * doctest::test_suite("regression"))
{
    CheckTruncate(64);
}

TEST_CASE("lobby map type truncates to 32 and terminates" * doctest::test_suite("regression"))
{
    CheckTruncate(32);
}

TEST_CASE("lobby map name truncates to 32 and terminates" * doctest::test_suite("regression"))
{
    CheckTruncate(32);
}

TEST_CASE("lobby string exact fit is truncated and terminated" * doctest::test_suite("regression"))
{
    char acDest[32];
    char acSource[33];
    memset(acSource, 'B', 32);
    acSource[32] = 0;
    memset(acDest, 0x7F, sizeof(acDest));
    strlcpy(acDest, acSource, sizeof(acDest));
    CHECK(strlen(acDest) == 31);
    CHECK(acDest[31] == 0);
}
