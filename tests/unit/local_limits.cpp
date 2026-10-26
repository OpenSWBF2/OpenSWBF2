#include "third_party/doctest/doctest.h"

#include "Battlefront2/Source/LocalLimits.h"
#include <string.h>

TEST_CASE("command post index" * doctest::test_suite("regression"))
{
    CHECK(LocalLimits::ChoosePostIndex(0, -1, 3) == 0);
    CHECK(LocalLimits::ChoosePostIndex(15, -1, 3) == 15);
    CHECK(LocalLimits::ChoosePostIndex(16, -1, 3) == -1);
    CHECK(LocalLimits::ChoosePostIndex(-1, 4, 15) == 4);
    CHECK(LocalLimits::ChoosePostIndex(-1, -1, 15) == 15);
    CHECK(LocalLimits::ChoosePostIndex(-1, -1, 16) == -1);
}

TEST_CASE("net object cap" * doctest::test_suite("regression"))
{
    CHECK(LocalLimits::CanAddNdObject(255));
    CHECK(!LocalLimits::CanAddNdObject(256));
    CHECK(LocalLimits::IsValidNdId(64));
    CHECK(LocalLimits::IsValidNdId(319));
    CHECK(!LocalLimits::IsValidNdId(320));
    CHECK(!LocalLimits::IsValidNdId(63));
    CHECK(!LocalLimits::IsValidNdId(-1));
}

TEST_CASE("team slot bounds" * doctest::test_suite("regression"))
{
    CHECK(LocalLimits::CanAddTeamMember(0, 599));
    CHECK(LocalLimits::CanAddTeamMember(7, 599));
    CHECK(!LocalLimits::CanAddTeamMember(0, 600));
    CHECK(!LocalLimits::CanAddTeamMember(7, 600));
    CHECK(!LocalLimits::CanAddTeamMember(8, 0));
    CHECK(!LocalLimits::CanAddTeamMember(-1, 0));
    CHECK(!LocalLimits::CanAddTeamMember(-8, 0));
}

TEST_CASE("lobby copy helper truncates host name to 64" * doctest::test_suite("regression"))
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

TEST_CASE("lobby copy helper truncates tags to 32" * doctest::test_suite("regression"))
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

TEST_CASE("lobby copy helper exact fit is terminated" * doctest::test_suite("regression"))
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

TEST_CASE("lobby copy helper treats null as empty" * doctest::test_suite("regression"))
{
    char acDst[8];
    memset(acDst, 'Z', sizeof(acDst));
    LocalLimits::CopyLobbyString(acDst, 0, sizeof(acDst));
    CHECK(acDst[0] == '\0');
    CHECK(acDst[1] == 'Z');
}

TEST_CASE("uploads are off by default" * doctest::test_suite("regression"))
{
    CHECK(!LocalLimits::kUploadsDefault);
    CHECK(!LocalLimits::UploadsAllowed(LocalLimits::kUploadsDefault));
}

TEST_CASE("uploads allowed follows the switch" * doctest::test_suite("regression"))
{
    CHECK(LocalLimits::UploadsAllowed(true));
    CHECK(!LocalLimits::UploadsAllowed(false));
}

TEST_CASE("client of index accepts only one and two" * doctest::test_suite("regression"))
{
    CHECK(LocalLimits::IsValidClientOf(1));
    CHECK(LocalLimits::IsValidClientOf(2));
    CHECK(!LocalLimits::IsValidClientOf(0));
    CHECK(!LocalLimits::IsValidClientOf(3));
    CHECK(!LocalLimits::IsValidClientOf(-1));
    CHECK(!LocalLimits::IsValidClientOf(64));
    CHECK(!LocalLimits::IsValidClientOf(2147483647));
}
