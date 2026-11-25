#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string>

#include "testutil.h"

TEST_CASE_NAMED(FWeaponCannonFireTest1, "Unit::WeaponCannonFire::weapon cannon fire compiles with fp strict", "[EngineFilter]")
{
    std::string sText;
    REQUIRE(ReadRepoText("BattlefrontII.vcxproj", sText));
    const std::string sItem = "<ClCompile Include=\"src\\Battlefront2\\Source\\WeaponCannon.cpp\">\n"
                              "      <FloatingPointModel>Strict</FloatingPointModel>\n"
                              "    </ClCompile>";
    const size_t uiItem = sText.find(sItem);
    CHECK(uiItem != std::string::npos);
    CHECK(sText.find(sItem, uiItem + 1) == std::string::npos);
    CHECK(sText.find("<ClCompile Include=\"src\\Battlefront2\\Source\\WeaponCannon.cpp\" />") == std::string::npos);
}
