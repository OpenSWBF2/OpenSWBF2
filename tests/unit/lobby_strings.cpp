#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

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

TEST_CASE_NAMED(FLobbyStringsTest1, "Unit::LobbyStrings::lobby host name truncates to 64 and terminates", "[EngineFilter]")
{
    CheckTruncate(64);
}

TEST_CASE_NAMED(FLobbyStringsTest2, "Unit::LobbyStrings::lobby map type truncates to 32 and terminates", "[EngineFilter]")
{
    CheckTruncate(32);
}

TEST_CASE_NAMED(FLobbyStringsTest3, "Unit::LobbyStrings::lobby map name truncates to 32 and terminates", "[EngineFilter]")
{
    CheckTruncate(32);
}

TEST_CASE_NAMED(FLobbyStringsTest4, "Unit::LobbyStrings::lobby string exact fit is truncated and terminated", "[EngineFilter]")
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
