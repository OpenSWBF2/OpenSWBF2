#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "Battlefront2/Source/LocalLimits.h"
#include <string.h>

TEST_CASE_NAMED(FLocalLimitsTest1, "Unit::LocalLimits::command post index", "[EngineFilter]")
{
    CHECK(LocalLimits::ChoosePostIndex(0, -1, 3) == 0);
    CHECK(LocalLimits::ChoosePostIndex(15, -1, 3) == 15);
    CHECK(LocalLimits::ChoosePostIndex(16, -1, 3) == -1);
    CHECK(LocalLimits::ChoosePostIndex(-1, 4, 15) == 4);
    CHECK(LocalLimits::ChoosePostIndex(-1, -1, 15) == 15);
    CHECK(LocalLimits::ChoosePostIndex(-1, -1, 16) == -1);
}

TEST_CASE_NAMED(FLocalLimitsTest2, "Unit::LocalLimits::net object cap", "[EngineFilter]")
{
    CHECK(LocalLimits::CanAddNdObject(255));
    CHECK(!LocalLimits::CanAddNdObject(256));
    CHECK(LocalLimits::IsValidNdId(64));
    CHECK(LocalLimits::IsValidNdId(319));
    CHECK(!LocalLimits::IsValidNdId(320));
    CHECK(!LocalLimits::IsValidNdId(63));
    CHECK(!LocalLimits::IsValidNdId(-1));
}

TEST_CASE_NAMED(FLocalLimitsTest3, "Unit::LocalLimits::team slot bounds", "[EngineFilter]")
{
    CHECK(LocalLimits::CanAddTeamMember(0, 599));
    CHECK(LocalLimits::CanAddTeamMember(7, 599));
    CHECK(!LocalLimits::CanAddTeamMember(0, 600));
    CHECK(!LocalLimits::CanAddTeamMember(7, 600));
    CHECK(!LocalLimits::CanAddTeamMember(8, 0));
    CHECK(!LocalLimits::CanAddTeamMember(-1, 0));
    CHECK(!LocalLimits::CanAddTeamMember(-8, 0));
}

TEST_CASE_NAMED(FLocalLimitsTest4, "Unit::LocalLimits::lobby copy helper truncates host name to 64", "[EngineFilter]")
{
    char acSrc[512];
    memset(acSrc, 'A', 511);
    acSrc[511] = '\0';
    char acDst[80];
    memset(acDst, 'Z', sizeof(acDst));
    LocalLimits::CopyLobbyString(acDst, acSrc, LocalLimits::kLobbyNameSize);
    CHECK(strlen(acDst) == 63);
    CHECK(acDst[63] == '\0');
    CHECK(acDst[64] == 'Z');
}

TEST_CASE_NAMED(FLocalLimitsTest5, "Unit::LocalLimits::lobby copy helper truncates tags to 32", "[EngineFilter]")
{
    char acSrc[512];
    memset(acSrc, 'B', 511);
    acSrc[511] = '\0';
    char acDst[48];
    memset(acDst, 'Z', sizeof(acDst));
    LocalLimits::CopyLobbyString(acDst, acSrc, LocalLimits::kLobbyTagSize);
    CHECK(strlen(acDst) == 31);
    CHECK(acDst[32] == 'Z');
}

TEST_CASE_NAMED(FLocalLimitsTest6, "Unit::LocalLimits::lobby copy helper exact fit is terminated", "[EngineFilter]")
{
    char acSrc[33];
    memset(acSrc, 'C', 32);
    acSrc[32] = '\0';
    char acDst[40];
    memset(acDst, 'Z', sizeof(acDst));
    LocalLimits::CopyLobbyString(acDst, acSrc, 32);
    CHECK(strlen(acDst) == 31);
    CHECK(acDst[31] == '\0');
    CHECK(acDst[32] == 'Z');
}

TEST_CASE_NAMED(FLocalLimitsTest7, "Unit::LocalLimits::lobby copy helper treats null as empty", "[EngineFilter]")
{
    char acDst[8];
    memset(acDst, 'Z', sizeof(acDst));
    LocalLimits::CopyLobbyString(acDst, 0, sizeof(acDst));
    CHECK(acDst[0] == '\0');
    CHECK(acDst[1] == 'Z');
}

TEST_CASE_NAMED(FLocalLimitsTest8, "Unit::LocalLimits::uploads are off by default", "[EngineFilter]")
{
    CHECK(!LocalLimits::kUploadsDefault);
    CHECK(!LocalLimits::UploadsAllowed(LocalLimits::kUploadsDefault));
}

TEST_CASE_NAMED(FLocalLimitsTest9, "Unit::LocalLimits::uploads allowed follows the switch", "[EngineFilter]")
{
    CHECK(LocalLimits::UploadsAllowed(true));
    CHECK(!LocalLimits::UploadsAllowed(false));
}

TEST_CASE_NAMED(FLocalLimitsTest10, "Unit::LocalLimits::client of index accepts only one and two", "[EngineFilter]")
{
    CHECK(LocalLimits::IsValidClientOf(1));
    CHECK(LocalLimits::IsValidClientOf(2));
    CHECK(!LocalLimits::IsValidClientOf(0));
    CHECK(!LocalLimits::IsValidClientOf(3));
    CHECK(!LocalLimits::IsValidClientOf(-1));
    CHECK(!LocalLimits::IsValidClientOf(64));
    CHECK(!LocalLimits::IsValidClientOf(2147483647));
}


TEST_CASE_NAMED(FLocalLimitsTest11, "Unit::LocalLimits::net player and guest count bounds", "[EngineFilter]")
{
    for (int i = 0; i < 64; ++i)
    {
        CHECK(LocalLimits::IsValidNetPlayer(i));
    }
    CHECK(!LocalLimits::IsValidNetPlayer(-1));
    CHECK(!LocalLimits::IsValidNetPlayer(64));
    CHECK(!LocalLimits::IsValidNetPlayer(-2147483647 - 1));
    CHECK(!LocalLimits::IsValidNetPlayer(2147483647));
    CHECK(LocalLimits::IsValidGuestCount(0));
    CHECK(LocalLimits::IsValidGuestCount(1));
    CHECK(!LocalLimits::IsValidGuestCount(2));
    CHECK(!LocalLimits::IsValidGuestCount(7));
    CHECK(!LocalLimits::IsValidGuestCount(0xffffffffu));
}
