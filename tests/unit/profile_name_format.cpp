#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"
#include "Battlefront2/Source/ProfileNameFormat.h"

namespace
{

const unsigned short kNameCanary = 0xCDCD;

std::vector<unsigned short> WideOf(const char* pcText)
{
    std::vector<unsigned short> v;
    for (const char* p = pcText; *p; ++p)
    {
        v.push_back((unsigned short)(unsigned char)*p);
    }
    v.push_back(0);
    return v;
}

bool SameUpToTerminator(const unsigned short* pusA, const unsigned short* pusB, unsigned int uiCount)
{
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        if (pusA[i] != pusB[i])
        {
            return false;
        }
        if (!pusA[i])
        {
            return true;
        }
    }
    return true;
}

}

TEST_CASE("profile name template formats like retail" * doctest::test_suite("regression"))
{
    const char* const kTemplates[] = { "Player %d", "Jugador %d", "Giocatore %d", "Joueur %d", "Spieler %d", "%%d %d",
                                       "Plain" };
    const int kNumbers[] = { 1, 2, 10, 9999, 7 };
    const unsigned int kCounts[] = { 64, 256, 8, 3 };
    for (size_t t = 0; t < sizeof(kTemplates) / sizeof(kTemplates[0]); ++t)
    {
        const std::vector<unsigned short> tmpl = WideOf(kTemplates[t]);
        for (size_t n = 0; n < sizeof(kNumbers) / sizeof(kNumbers[0]); ++n)
        {
            for (size_t c = 0; c < sizeof(kCounts) / sizeof(kCounts[0]); ++c)
            {
                CAPTURE(kTemplates[t]);
                CAPTURE(kNumbers[n]);
                CAPTURE(kCounts[c]);
                std::vector<unsigned short> got(kCounts[c] + 8, kNameCanary);
                std::vector<unsigned short> want(kCounts[c] + 8, kNameCanary);
                const int iGot = FormatProfileName(&got[0], kCounts[c], &tmpl[0], kNumbers[n]);
                const int iWant = PblUnicode::usnprintf(&want[0], kCounts[c], &tmpl[0], kNumbers[n]);
                CHECK(iGot == iWant);
                CHECK(SameUpToTerminator(&got[0], &want[0], kCounts[c]));
            }
        }
    }

    unsigned short ausName[64];
    const std::vector<unsigned short> player = WideOf("Player %d");
    CHECK(FormatProfileName(ausName, 64, &player[0], 1) == 8);
    const std::vector<unsigned short> player1 = WideOf("Player 1");
    CHECK(memcmp(ausName, &player1[0], 9 * sizeof(unsigned short)) == 0);

    const std::vector<unsigned short> percent = WideOf("%%d %d");
    CHECK(FormatProfileName(ausName, 64, &percent[0], 7) == 4);
    const std::vector<unsigned short> percent7 = WideOf("%d 7");
    CHECK(memcmp(ausName, &percent7[0], 5 * sizeof(unsigned short)) == 0);

    const std::vector<unsigned short> plain = WideOf("Plain");
    CHECK(FormatProfileName(ausName, 64, &plain[0], 7) == 5);
    CHECK(memcmp(ausName, &plain[0], 6 * sizeof(unsigned short)) == 0);
}

TEST_CASE("profile name template is never used as a raw format" * doctest::test_suite("regression"))
{
    const char* const kHostile[] = { "%s%s%n", "%n%d", "%d%d", "%x", "100%", "%s", "a%d%s", "%c" };
    for (size_t i = 0; i < sizeof(kHostile) / sizeof(kHostile[0]); ++i)
    {
        CAPTURE(kHostile[i]);
        const std::vector<unsigned short> tmpl = WideOf(kHostile[i]);
        unsigned short ausName[64];
        for (unsigned int j = 0; j < 64; ++j)
        {
            ausName[j] = kNameCanary;
        }
        const int iRet = FormatProfileName(ausName, 64, &tmpl[0], 7);
        CHECK(iRet == (int)(tmpl.size() - 1));
        CHECK(memcmp(ausName, &tmpl[0], tmpl.size() * sizeof(unsigned short)) == 0);
    }

    std::string sLong = "%s";
    sLong.append(38, 'x');
    const std::vector<unsigned short> longTmpl = WideOf(sLong.c_str());
    unsigned short ausBlock[16 + 8];
    for (unsigned int j = 0; j < 16 + 8; ++j)
    {
        ausBlock[j] = kNameCanary;
    }
    CHECK(FormatProfileName(ausBlock, 16, &longTmpl[0], 7) == 15);
    CHECK(ausBlock[15] == 0);
    CHECK(memcmp(ausBlock, &longTmpl[0], 15 * sizeof(unsigned short)) == 0);
    for (unsigned int j = 16; j < 16 + 8; ++j)
    {
        CHECK(ausBlock[j] == kNameCanary);
    }

    for (unsigned int j = 0; j < 16 + 8; ++j)
    {
        ausBlock[j] = kNameCanary;
    }
    CHECK(FormatProfileName(ausBlock, 0, &longTmpl[0], 7) == 0);
    CHECK(FormatProfileName(ausBlock + 8, 0, 0, 7) == 0);
    for (unsigned int j = 0; j < 16 + 8; ++j)
    {
        CHECK(ausBlock[j] == kNameCanary);
    }
    CHECK(FormatProfileName(ausBlock, 16, 0, 7) == 0);
    CHECK(ausBlock[0] == 0);
    CHECK(ausBlock[1] == kNameCanary);
}

TEST_CASE("profile name sites use the template seam" * doctest::test_suite("regression"))
{
    std::string sLoadSave;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp", sLoadSave));
    CHECK(sLoadSave.find("FormatProfileName(ausName, 64, RedLocalizeDB::Find(0x3802F15B, false), 1);") != std::string::npos);
    CHECK(sLoadSave.find("FormatProfileName(Lua_Callbacks::sTempReturnString16, 256, pusBase, (int)uiTry);") != std::string::npos);
    CHECK(sLoadSave.find("usnprintf(ausName, 64, RedLocalizeDB::Find(0x3802F15B") == std::string::npos);
    CHECK(sLoadSave.find("usnprintf(Lua_Callbacks::sTempReturnString16") == std::string::npos);

    std::string sSaveGame;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\SaveGameManager.cpp", sSaveGame));
    CHECK(sSaveGame.find("FormatProfileName(ausName, 256, pusFilename, iStart);") != std::string::npos);
    CHECK(sSaveGame.find("usnprintf(ausName, 256, pusFilename") == std::string::npos);
}
