#include "third_party/doctest/doctest.h"

#include "Battlefront2/Source/LocalLimits.h"

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
