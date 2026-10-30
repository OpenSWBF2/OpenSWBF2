#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"
#include "RedEngineFL/Interface/RedMagicStrings.h"

namespace
{

const unsigned short kMagicCanary = 0xCDCD;
int sLookups = 0;
std::string sLastName;

unsigned short FakeFind(const char* pcName)
{
    ++sLookups;
    sLastName = pcName;
    return strcmp(pcName, "btn") == 0 ? (unsigned short)'Z' : (unsigned short)0;
}

std::vector<unsigned short> MagicWide(const std::string& s)
{
    std::vector<unsigned short> v;
    for (size_t i = 0; i < s.size(); ++i)
    {
        v.push_back((unsigned short)(unsigned char)s[i]);
    }
    v.push_back(0);
    return v;
}

bool HoldsText(const unsigned short* pusText, const char* pcExpected)
{
    const size_t uiLen = strlen(pcExpected);
    for (size_t i = 0; i < uiLen; ++i)
    {
        if (pusText[i] != (unsigned short)(unsigned char)pcExpected[i])
        {
            return false;
        }
    }
    return pusText[uiLen] == 0;
}

}

TEST_CASE("magic strings replace like retail" * doctest::test_suite("regression"))
{
    struct Case
    {
        const char* pcSource;
        const char* pcExpected;
        unsigned int uiCount;
    };
    const Case kCases[] = {
        { "a{btn}b", "aZb", 1 },
        { "{x}y", " y", 0 },
        { "plain", "plain", 0 },
        { "{open", "", 0 },
        { "{}", " ", 0 },
        { "{btn}{btn}", "ZZ", 2 },
        { "x{a{btn}y", "xZy", 1 },
    };
    for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i)
    {
        CAPTURE(kCases[i].pcSource);
        const std::vector<unsigned short> src = MagicWide(kCases[i].pcSource);
        unsigned short ausDest[64];
        CHECK(ReplaceMagicStrings(&src[0], ausDest, 64, FakeFind) == kCases[i].uiCount);
        CHECK(HoldsText(ausDest, kCases[i].pcExpected));
    }
}

TEST_CASE("magic strings drop an overlong name" * doctest::test_suite("regression"))
{
    std::string sOverlong = "{";
    sOverlong.append(300, 'k');
    sOverlong += "}";
    const std::vector<unsigned short> src = MagicWide(sOverlong);
    unsigned short ausDest[64];
    sLookups = 0;
    CHECK(ReplaceMagicStrings(&src[0], ausDest, 64, FakeFind) == 0);
    CHECK(sLookups == 0);
    CHECK(HoldsText(ausDest, " "));

    std::string sMax = "a{";
    sMax.append(255, 'k');
    sMax += "}b";
    const std::vector<unsigned short> srcMax = MagicWide(sMax);
    sLookups = 0;
    sLastName.clear();
    CHECK(ReplaceMagicStrings(&srcMax[0], ausDest, 64, FakeFind) == 0);
    CHECK(sLookups == 1);
    CHECK(sLastName == std::string(255, 'k'));
    CHECK(HoldsText(ausDest, "a b"));
}

TEST_CASE("magic strings stay inside the destination" * doctest::test_suite("regression"))
{
    const std::vector<unsigned short> src = MagicWide(std::string(40, 'p'));
    unsigned short ausBlock[64];
    for (unsigned int i = 0; i < 64; ++i)
    {
        ausBlock[i] = kMagicCanary;
    }
    CHECK(ReplaceMagicStrings(&src[0], ausBlock, 16, FakeFind) == 0);
    CHECK(ausBlock[15] == 0);
    CHECK(HoldsText(ausBlock, std::string(15, 'p').c_str()));
    for (unsigned int i = 16; i < 64; ++i)
    {
        CHECK(ausBlock[i] == kMagicCanary);
    }

    const std::vector<unsigned short> magic = MagicWide("abcdefghijklmn{btn}{btn}");
    for (unsigned int i = 0; i < 64; ++i)
    {
        ausBlock[i] = kMagicCanary;
    }
    CHECK(ReplaceMagicStrings(&magic[0], ausBlock, 16, FakeFind) == 1);
    CHECK(HoldsText(ausBlock, "abcdefghijklmnZ"));
    for (unsigned int i = 16; i < 64; ++i)
    {
        CHECK(ausBlock[i] == kMagicCanary);
    }

    for (unsigned int i = 0; i < 64; ++i)
    {
        ausBlock[i] = kMagicCanary;
    }
    CHECK(ReplaceMagicStrings(&src[0], ausBlock, 0, FakeFind) == 0);
    for (unsigned int i = 0; i < 64; ++i)
    {
        CHECK(ausBlock[i] == kMagicCanary);
    }
}

TEST_CASE("magic strings call site passes its capacity" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("src\\RedEngineFL\\Interface\\RedTextElement.cpp", sText));
    CHECK(sText.find("FindReplaceMagicStrings_(pcText, acBuffer, sizeof(acBuffer) / sizeof(acBuffer[0]));") != std::string::npos);
    CHECK(sText.find("return ReplaceMagicStrings(pcSource, pcDest, uiDestCount, FindMagicCharByName_);") != std::string::npos);
    CHECK(sText.find("return FindMagicChar_(PblHash(pcName));") != std::string::npos);
}
