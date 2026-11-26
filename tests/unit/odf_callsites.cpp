#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>
#include <string>

#include "testutil.h"

namespace
{

std::string ReadSource(const std::string& sPath)
{
    std::string sOut;
    const bool bRead = ReadRepoText(sPath.c_str(), sOut);
    REQUIRE(bRead);
    return sOut;
}

int CountOf(const std::string& sText, const std::string& sNeedle)
{
    int iCount = 0;
    size_t uiPos = 0;
    while ((uiPos = sText.find(sNeedle, uiPos)) != std::string::npos)
    {
        ++iCount;
        uiPos += sNeedle.size();
    }
    return iCount;
}

bool IsIdentStart(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool IsIdentChar(char c)
{
    return IsIdentStart(c) || (c >= '0' && c <= '9');
}

int GuardedSetPropertyCount(const std::string& sText, const std::string& sGuard)
{
    static const std::string sTail = "->SetProperty(uProperty, pcValue);";
    int iCount = 0;
    size_t uiPos = 0;
    while ((uiPos = sText.find(sGuard, uiPos)) != std::string::npos)
    {
        uiPos += sGuard.size();
        size_t i = uiPos;
        while (i < sText.size() && (sText[i] == ' ' || sText[i] == '\t' || sText[i] == '\n'))
        {
            ++i;
        }
        if (i >= sText.size() || !IsIdentStart(sText[i]))
        {
            continue;
        }
        while (i < sText.size() && IsIdentChar(sText[i]))
        {
            ++i;
        }
        if (sText.compare(i, sTail.size(), sTail) == 0)
        {
            ++iCount;
        }
    }
    return iCount;
}

}

TEST_CASE_NAMED(FOdfCallsitesTest1, "Unit::OdfCallsites::odf value guard covers every property call site", "[SmokeFilter]")
{
    const std::string sRoot = "src\\Battlefront2\\Source\\";
    const std::string sGuard = "OdfValue::Sanitize(uProperty, pcValue);";
    const std::string sInclude = "#include \"OdfValue.h\"";

    struct Site
    {
        const char* pcFile;
        int iExpected;
    };
    const Site kSites[] = {
        { "Entity.cpp", 1 },
        { "Explosion.cpp", 1 },
        { "Ordnance.cpp", 1 },
        { "Weapon.cpp", 1 },
        { "LoadUtil.cpp", 3 },
    };
    for (size_t i = 0; i < sizeof(kSites) / sizeof(kSites[0]); ++i)
    {
        const std::string sPath = sRoot + kSites[i].pcFile;
        INFO(sPath);
        const std::string sText = ReadSource(sPath);
        CHECK(CountOf(sText, sGuard) == kSites[i].iExpected);
        CHECK(GuardedSetPropertyCount(sText, sGuard) == kSites[i].iExpected);
        CHECK(CountOf(sText, "SetProperty(uProperty, child.ReadString(") == 0);
        CHECK(CountOf(sText, sInclude) == 1);
    }

    const std::string sPath = sRoot + "LuaCallbacks_Mission.cpp";
    INFO(sPath);
    const std::string sText = ReadSource(sPath);
    CHECK(CountOf(sText, "OdfValue::SanitizeCopy(PblHash(pcProperty), pcValue, acScratch, "
                         "sizeof(acScratch))") == 2);
    CHECK(CountOf(sText, "SetProperty(PblHash(pcProperty), pcSafe);") == 3);
    CHECK(CountOf(sText, "SetProperty(PblHash(pcProperty), pcValue);") == 0);
    CHECK(CountOf(sText, sInclude) == 1);
}
