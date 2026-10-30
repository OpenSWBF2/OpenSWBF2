#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "Battlefront2/Source/LocalizedFormatArgs.h"

namespace
{

std::vector<unsigned short> BoundsWide(const char* pcText)
{
    std::vector<unsigned short> v;
    for (const char* p = pcText; *p; ++p)
    {
        v.push_back((unsigned short)(unsigned char)*p);
    }
    v.push_back(0);
    return v;
}

}

TEST_CASE("medal key stays inside key[68]" * doctest::test_suite("regression"))
{
    char acBlock[68 + 8];
    memset(acBlock, 0xCD, sizeof(acBlock));
    const std::string sPrefix(64, 'p');
    const std::string sName(64, 'n');
    strlcpy(acBlock, sPrefix.c_str(), 68);
    strlcat(acBlock, sName.c_str(), 68);
    CHECK(strlen(acBlock) == 67);
    for (unsigned int i = 68; i < sizeof(acBlock); ++i)
    {
        CHECK(acBlock[i] == (char)0xCD);
    }

    char acNew[68];
    strlcpy(acNew, "award.", sizeof(acNew));
    strlcat(acNew, "kills", sizeof(acNew));
    char acOld[68];
    strncpy(acOld, "award.", 64);
    strncat(acOld, "kills", 64);
    CHECK(strcmp(acNew, acOld) == 0);
    CHECK(strcmp(acNew, "award.kills") == 0);

    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\MedalsMgr.cpp", sText));
    CHECK(sText.find("strlcpy(key, pPrefix, sizeof(key));") != std::string::npos);
    CHECK(sText.find("strlcat(key, sMedalPoints_condition[iType].name, sizeof(key));") != std::string::npos);
    CHECK(sText.find("strncpy(key, pPrefix, 64);") == std::string::npos);
    CHECK(sText.find("strncat(key, sMedalPoints_condition[iType].name, 64);") == std::string::npos);
}

TEST_CASE("team random name clears the whole buffer" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\Team.cpp", sText));
    CHECK(sText.find("memset(pName, 0, 64 * sizeof(pName[0]));") != std::string::npos);
    CHECK(sText.find("memset(pName, 0, 64);") == std::string::npos);
}

TEST_CASE("key name copy honours the element count" * doctest::test_suite("regression"))
{
    wchar_t awName[260];
    for (unsigned int i = 0; i < 259; ++i)
    {
        awName[i] = (wchar_t)('A' + i % 26);
    }
    awName[259] = 0;
    unsigned short ausBlock[32 + 8];
    for (unsigned int i = 0; i < 32 + 8; ++i)
    {
        ausBlock[i] = 0xCDCD;
    }
    CHECK(PblUnicode::ustrlcpy(ausBlock, (const unsigned short*)awName, 32) == 32);
    CHECK(ausBlock[31] == 0);
    CHECK(PblUnicode::ustrlen(ausBlock) == 31);
    for (unsigned int i = 32; i < 32 + 8; ++i)
    {
        CHECK(ausBlock[i] == 0xCDCD);
    }

    std::string sText;
    REQUIRE(ReadRepoText("src\\RedEngineFL\\input\\pc\\pcRedKeyboard.cpp", sText));
    CHECK(sText.find("PblUnicode::ustrlcpy(pusName, (const unsigned short*)dips.wsz, uiSize);") != std::string::npos);
    CHECK(sText.find("memcpy(pusName, dips.wsz") == std::string::npos);
    CHECK(sText.find("(void)uiSize;") == std::string::npos);
}

TEST_CASE("localized format never reads past the passed args" * doctest::test_suite("regression"))
{
    struct Case
    {
        const char* pcFormat;
        int iArgs;
        bool bFits;
    };
    const Case kCases[] = {
        { "plain", 0, true },
        { "%s", 1, true },
        { "%s", 0, false },
        { "%s and %s", 1, false },
        { "%s and %s", 2, true },
        { "a%%b%s", 1, true },
        { "%%%%", 0, true },
        { "%d%s", 1, false },
        { "%c%f%d%s", 4, true },
        { "%s%q%s%s", 1, true },
        { "100%", 0, true },
        { "%s%s%s%s%s%s%s", 6, false },
    };
    for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i)
    {
        CAPTURE(kCases[i].pcFormat);
        CAPTURE(kCases[i].iArgs);
        const std::vector<unsigned short> fmt = BoundsWide(kCases[i].pcFormat);
        CHECK(LocalizedFormatArgsFit(&fmt[0], kCases[i].iArgs) == kCases[i].bFits);
    }

    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\LuaCallbacks.cpp", sText));
    CHECK(sText.find("switch (LocalizedFormatArgsFit(pusFormat, iTop - 1) ? iTop : 0)") != std::string::npos);
}
