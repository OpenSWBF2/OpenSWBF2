#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string.h>
#include <string>

#include "testutil.h"
#include "Battlefront2/Source/SideNames.h"

TEST_CASE_NAMED(FSideNamesTest1, "Unit::SideNames::side name prefix is bounded", "[EngineFilter]")
{
    REQUIRE(SideNamePrefix(-1) == SideNamePrefix(6));
    REQUIRE(SideNamePrefix(6) == SideNamePrefix(7));
    REQUIRE(SideNamePrefix(7) == SideNamePrefix(8));
    CHECK(SideNamePrefix(6)[0] == '\0');
    CHECK(strcmp(SideNamePrefix(0), "Neu") == 0);
    CHECK(strcmp(SideNamePrefix(1), "All") == 0);
    CHECK(strcmp(SideNamePrefix(2), "Imp") == 0);
    CHECK(strcmp(SideNamePrefix(3), "Rep") == 0);
    CHECK(strcmp(SideNamePrefix(4), "CIS") == 0);
    CHECK(strcmp(SideNamePrefix(5), "Loc") == 0);
}

TEST_CASE_NAMED(FSideNamesTest2, "Unit::SideNames::final score uses the bounded side name", "[EngineFilter]")
{
    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\GameLoop.cpp", sText));
    CHECK(sText.find("SideNamePrefix(iSide)") != std::string::npos);
    CHECK(sText.find("sSideNamePrefixes[") == std::string::npos);
}
