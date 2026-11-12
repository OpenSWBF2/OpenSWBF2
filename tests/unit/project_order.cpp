#include "third_party/doctest/doctest.h"

#include <string>

#include "testutil.h"

namespace
{

size_t ItemAt(const std::string& sText, const char* pcInclude)
{
    const std::string sItem = std::string("<ClCompile Include=\"") + pcInclude + "\"";
    const size_t uiAt = sText.find(sItem);
    REQUIRE_MESSAGE(uiAt != std::string::npos, pcInclude);
    CHECK_MESSAGE(sText.find(sItem, uiAt + 1) == std::string::npos, pcInclude);
    return uiAt;
}

}

TEST_CASE("identity matrix initializer runs before every game initializer" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("BattlefrontII.vcxproj", sText));
    const size_t uiMatrix = ItemAt(sText, "src\\PebbleFL\\pc\\pcPblMatrix.cpp");
    const size_t uiGame = sText.find("<ClCompile Include=\"src\\Battlefront2\\");
    REQUIRE(uiGame != std::string::npos);
    CHECK(uiMatrix < uiGame);
    CHECK(uiMatrix < ItemAt(sText, "src\\Battlefront2\\Source\\NetGame.cpp"));
}

TEST_CASE("deprecated winsock calls are allowed only in nonport" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("BattlefrontII.vcxproj", sText));
    const std::string sDefine = "_WINSOCK_DEPRECATED_NO_WARNINGS";
    const size_t uiDefine = sText.find(sDefine);
    REQUIRE(uiDefine != std::string::npos);
    CHECK(sText.find(sDefine, uiDefine + 1) == std::string::npos);
    const size_t uiItem = ItemAt(sText, "src\\GameSpyFL\\nonport.c");
    CHECK(sText.rfind("<ClCompile Include=\"", uiDefine) == uiItem);
    CHECK(sText.find("</ClCompile>", uiItem) > uiDefine);
}
