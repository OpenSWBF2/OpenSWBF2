#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"
#include "PebbleFL/Common/PblUnicode.h"

unsigned int __cdecl ConvertUnicodeSkippingWide(char* pDest, const unsigned short* pSrc, unsigned int uiCount);

namespace
{

const unsigned short kCanary = 0xCDCD;

void FillCanary(unsigned short* pusBlock, unsigned int uiCount)
{
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        pusBlock[i] = kCanary;
    }
}

bool CanaryIntact(const unsigned short* pusBlock, unsigned int uiFrom, unsigned int uiTo)
{
    for (unsigned int i = uiFrom; i < uiTo; ++i)
    {
        if (pusBlock[i] != kCanary)
        {
            return false;
        }
    }
    return true;
}

struct CallSitePin
{
    const char* pcFile;
    const char* pcOld;
    const char* pcNew;
};

const CallSitePin kPins[] = {
    { "src\\Battlefront2\\Platform\\Pc\\pcSaveFile.cpp", "PblUnicode::ustrncpy(mFileName, pusFileName, sizeof(mFileName) / sizeof(mFileName[0]));", "PblUnicode::ustrlcpy(mFileName, pusFileName, sizeof(mFileName) / sizeof(mFileName[0]));" },
    { "src\\Battlefront2\\Platform\\Pc\\pcSaveFile.cpp", "PblUnicode::ustrncnv(pusDate, acDate, 128);", "PblUnicode::ustrncnv(pusDate, acDate, 64);" },
    { "src\\Battlefront2\\Source\\Character.cpp", "PblUnicode::ustrncpy(mRegularName, pName, 64);", "PblUnicode::ustrlcpy(mRegularName, pName, 64);" },
    { "src\\Battlefront2\\Source\\Character.cpp", "PblUnicode::ustrncpy(mName, pName, 64);", "PblUnicode::ustrlcpy(mName, pName, 64);" },
    { "src\\Battlefront2\\Source\\Character.cpp", "PblUnicode::ustrncpy(mRegularName, name, 64);", "PblUnicode::ustrlcpy(mRegularName, name, 64);" },
    { "src\\Battlefront2\\Source\\GameLoop.cpp", "ustrncnv(acTeam1, pTeam1, sizeof(acTeam1))", "ustrncnv(acTeam1, pTeam1, sizeof(acTeam1) / sizeof(acTeam1[0]))" },
    { "src\\Battlefront2\\Source\\GameLoop.cpp", "ustrncnv(acTeam2, pTeam2, sizeof(acTeam2))", "ustrncnv(acTeam2, pTeam2, sizeof(acTeam2) / sizeof(acTeam2[0]))" },
    { "src\\Battlefront2\\Source\\HeroMessageDisplay.cpp", "PblUnicode::ustrncpy(playerName, pHero->GetRegularName(), 64);", "PblUnicode::ustrlcpy(playerName, pHero->GetRegularName(), 64);" },
    { "src\\Battlefront2\\Source\\HUDElementMap.cpp", "usnprintf(buffer, sizeof(buffer) - 1, \"%d\"", "usnprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), \"%d\"" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "GetLocalizedString(usAction, sizeof(usAction),", "GetLocalizedString(usAction, sizeof(usAction) / sizeof(usAction[0])," },
    { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "GetLocalizedString(usButton, sizeof(usButton),", "GetLocalizedString(usButton, sizeof(usButton) / sizeof(usButton[0])," },
    { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "GetLocalizedString(usKey, sizeof(usKey),", "GetLocalizedString(usKey, sizeof(usKey) / sizeof(usKey[0])," },
    { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "ustrlcpy(usKey, usHash, 512)", "ustrlcpy(usKey, usHash, 256)" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "ustrlcat(usKey, usNum, 512)", "ustrlcat(usKey, usNum, 256)" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "ustrlcat(usKey, usButton, 512)", "ustrlcat(usKey, usButton, 256)" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "ustrlcpy(usKey, usButton, 512)", "ustrlcpy(usKey, usButton, 256)" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", "ustrncnv(usBuffer, acBuffer, 512)", "ustrncnv(usBuffer, acBuffer, 256)" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp", "ustrncnv(acName, pusName, sizeof(acName))", "ustrncnv(acName, pusName, sizeof(acName) / sizeof(acName[0]))" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp", "ustrlcpy(sMetagameFilename, pusName, sizeof(sMetagameFilename))", "ustrlcpy(sMetagameFilename, pusName, sizeof(sMetagameFilename) / sizeof(sMetagameFilename[0]))" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp", "ustrlcpy(sCampaignFilename, pusName, sizeof(sCampaignFilename))", "ustrlcpy(sCampaignFilename, pusName, sizeof(sCampaignFilename) / sizeof(sCampaignFilename[0]))" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp", "PblUnicode::ustrncpy(Lua_Callbacks::sTempReturnString16, pusBase, 256);", "PblUnicode::ustrlcpy(Lua_Callbacks::sTempReturnString16, pusBase, 256);" },
    { "src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp", "PblUnicode::ustrncpy(pusDest, pusSource, 128);", "PblUnicode::ustrlcpy(pusDest, pusSource, 128);" },
    { "src\\Battlefront2\\Source\\NetGame.cpp", "PblUnicode::ustrncpy(message->text, text, 128);", "PblUnicode::ustrlcpy(message->text, text, 128);" },
    { "src\\Battlefront2\\Source\\PlayerProfile.cpp", "PblUnicode::ustrncpy(mName, pusName, 16);", "PblUnicode::ustrlcpy(mName, pusName, 16);" },
    { "src\\Battlefront2\\Source\\PlayerProfile.cpp", "PblUnicode::ustrncpy(mName, RedLocalizeDB::Find(0x62a0afc0, false), 16);", "PblUnicode::ustrlcpy(mName, RedLocalizeDB::Find(0x62a0afc0, false), 16);" },
    { "src\\Battlefront2\\Source\\SaveFsm.cpp", "PblUnicode::uvsnprintf((unsigned short*)pDest, unsigned int(-1), pusFormat,", "PblUnicode::uvsnprintf((unsigned short*)pDest, uiSize, pusFormat," },
    { "src\\Battlefront2\\Source\\SaveFsm.cpp", "wcscpy((wchar_t*)pusDest, (const wchar_t*)pusText);", "PblUnicode::ustrlcpy(pusDest, pusText, uiSize);" },
    { "src\\Battlefront2\\Source\\Team.cpp", "PblUnicode::ustrncpy(pName, pusName, 64);", "PblUnicode::ustrlcpy(pName, pusName, 64);" },
    { "src\\Battlefront2\\Source\\Team.cpp", "ustrlcat(pName, mFirstNames[uRand % mFirstCount], 64)", "ustrlcat(pName, mFirstNames[uRand % mFirstCount], 60)" },
    { "src\\RedEngineFL\\Util\\RedLocalizeDB.cpp", "memset(pusBuffer, 0, uiSize);", "memset(pusBuffer, 0, uiSize * sizeof(pusBuffer[0]));" },
    { "src\\Battlefront2\\Source\\LuaCallbacks.cpp", "PblUnicode::ustrncpy(sTempReturnString16, LuaWideString::Check(", "PblUnicode::ustrlcpy(sTempReturnString16, LuaWideString::Check(LuaHelper::mState, 1), 256);" },
    { "src\\Battlefront2\\Platform\\Pc\\pcSaveFile.cpp", "PblUnicode::ustrncpy(pusName, pusFilename, uiLen - 7);", "    if (uiLen < 8)\n    {\n        if (pusName)\n        {\n            pusName[0] = 0;\n        }\n        if (pusDate)\n        {\n            pusDate[0] = 0;\n        }\n        return;\n    }\n" },
    { "src\\Battlefront2\\Platform\\Pc\\pcSaveFile.cpp", "pusName[uiLen - 8] = 0;", "const unsigned int uiNameLen = uiLen - 8 < 127 ? uiLen - 8 : 127;\n        PblUnicode::ustrlcpy(pusName, pusFilename, uiNameLen + 1);" },
};

const char* const kScanFiles[] = {
    "src\\Battlefront2\\Platform\\Pc\\pcFLJoystick.cpp",
    "src\\Battlefront2\\Platform\\Pc\\pcSaveFile.cpp",
    "src\\Battlefront2\\Platform\\Pc\\pcSpawnDisplay.cpp",
    "src\\Battlefront2\\Source\\BootDisplay.cpp",
    "src\\Battlefront2\\Source\\Character.cpp",
    "src\\Battlefront2\\Source\\FLInputManager.cpp",
    "src\\Battlefront2\\Source\\GameLoop.cpp",
    "src\\Battlefront2\\Source\\HeroMessageDisplay.cpp",
    "src\\Battlefront2\\Source\\HUDElementMap.cpp",
    "src\\Battlefront2\\Source\\HUDGameEvents.cpp",
    "src\\Battlefront2\\Source\\LuaCallbacks.cpp",
    "src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp",
    "src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp",
    "src\\Battlefront2\\Source\\LuaCallbacks_Net.cpp",
    "src\\Battlefront2\\Source\\LuaCallbacks_Stats.cpp",
    "src\\Battlefront2\\Source\\MedalsMgr.cpp",
    "src\\Battlefront2\\Source\\NetAdmin.cpp",
    "src\\Battlefront2\\Source\\NetGame.cpp",
    "src\\Battlefront2\\Source\\PlayerProfile.cpp",
    "src\\Battlefront2\\Source\\PopupText.cpp",
    "src\\Battlefront2\\Source\\SaveFsm.cpp",
    "src\\Battlefront2\\Source\\SaveGameManager.cpp",
    "src\\Battlefront2\\Source\\SpawnManager.cpp",
    "src\\Battlefront2\\Source\\Team.cpp",
    "src\\RedEngineFL\\Interface\\RedTextElement.cpp",
    "src\\RedEngineFL\\Util\\RedLocalizeDB.cpp",
};

const char* const kCallNames[] = { "ustrlcpy(", "ustrlcat(", "ustrncpy(", "ustrncnv(", "GetLocalizedString(" };

bool HasBareSizeofCount(const std::string& sLine)
{
    bool bCall = false;
    for (size_t i = 0; i < sizeof(kCallNames) / sizeof(kCallNames[0]); ++i)
    {
        if (sLine.find(kCallNames[i]) != std::string::npos)
        {
            bCall = true;
        }
    }
    if (!bCall)
    {
        return false;
    }
    for (size_t uiAt = sLine.find("sizeof("); uiAt != std::string::npos; uiAt = sLine.find("sizeof(", uiAt + 1))
    {
        if (uiAt >= 2 && sLine.compare(uiAt - 2, 2, "/ ") == 0)
        {
            continue;
        }
        const size_t uiClose = sLine.find(')', uiAt);
        if (uiClose == std::string::npos || uiClose + 1 >= sLine.size())
        {
            continue;
        }
        const char cNext = sLine[uiClose + 1];
        if (cNext == ')' || cNext == ',')
        {
            return true;
        }
    }
    return false;
}


const unsigned int kBoundSizes[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 256 };
const unsigned int kTail = 8;
const char kCharCanary = (char)0xCD;

std::vector<unsigned int> SourceLengths(unsigned int uiN)
{
    std::vector<unsigned int> v;
    v.push_back(0);
    if (uiN > 0)
    {
        v.push_back(uiN - 1);
    }
    v.push_back(uiN);
    v.push_back(uiN + 1);
    v.push_back(399);
    return v;
}

std::vector<unsigned short> WideText(unsigned int uiLen, unsigned short usBase)
{
    std::vector<unsigned short> v(uiLen + 1, 0);
    for (unsigned int i = 0; i < uiLen; ++i)
    {
        v[i] = (unsigned short)(usBase + i % 26);
    }
    return v;
}

std::string NarrowText(unsigned int uiLen)
{
    std::string s;
    for (unsigned int i = 0; i < uiLen; ++i)
    {
        s += (char)('a' + i % 26);
    }
    return s;
}

std::vector<unsigned short> Widen(const std::string& s)
{
    std::vector<unsigned short> v;
    for (size_t i = 0; i < s.size(); ++i)
    {
        v.push_back((unsigned short)(unsigned char)s[i]);
    }
    v.push_back(0);
    return v;
}

unsigned int MinU(unsigned int a, unsigned int b)
{
    return a < b ? a : b;
}

struct WideBlock
{
    std::vector<unsigned short> v;
    unsigned int n;

    explicit WideBlock(unsigned int uiN) : v(uiN + kTail + 1, kCanary), n(uiN)
    {
        v[uiN + kTail] = 0;
    }

    unsigned short* Data()
    {
        return &v[0];
    }

    bool TailIntact() const
    {
        for (unsigned int i = n; i < n + kTail; ++i)
        {
            if (v[i] != kCanary)
            {
                return false;
            }
        }
        return true;
    }

    bool Holds(const unsigned short* pusExpected, unsigned int uiLen) const
    {
        for (unsigned int i = 0; i < uiLen; ++i)
        {
            if (v[i] != pusExpected[i])
            {
                return false;
            }
        }
        return v[uiLen] == 0;
    }
};

struct CharBlock
{
    std::vector<char> v;
    unsigned int n;

    explicit CharBlock(unsigned int uiN) : v(uiN + kTail, kCharCanary), n(uiN)
    {
    }

    char* Data()
    {
        return &v[0];
    }

    bool TailIntact() const
    {
        for (unsigned int i = n; i < n + kTail; ++i)
        {
            if (v[i] != kCharCanary)
            {
                return false;
            }
        }
        return true;
    }

    bool Holds(const char* pcExpected, unsigned int uiLen) const
    {
        return memcmp(&v[0], pcExpected, uiLen) == 0 && v[uiLen] == 0;
    }
};

void CheckWideResult(const WideBlock& block, const unsigned short* pusFull, unsigned int uiFull)
{
    CHECK(block.TailIntact());
    if (block.n > 0)
    {
        CHECK(block.Holds(pusFull, MinU(uiFull, block.n - 1)));
    }
}

void CheckCharResult(const CharBlock& block, const char* pcFull, unsigned int uiFull)
{
    CHECK(block.TailIntact());
    if (block.n > 0)
    {
        CHECK(block.Holds(pcFull, MinU(uiFull, block.n - 1)));
    }
}

int CallCharFormat(unsigned short* pusDest, unsigned int uiCount, int iCase)
{
    switch (iCase)
    {
    case 0:
        return PblUnicode::usnprintf(pusDest, uiCount, "abcdefghij");
    case 1:
        return PblUnicode::usnprintf(pusDest, uiCount, "ab%dcd", 42);
    case 2:
        return PblUnicode::usnprintf(pusDest, uiCount, "%d", -12345);
    case 3:
        return PblUnicode::usnprintf(pusDest, uiCount, "x%sy", "str");
    case 4:
        return PblUnicode::usnprintf(pusDest, uiCount, "%c%f", 'Q', 0.0);
    default:
        return PblUnicode::usnprintf(pusDest, uiCount, "v=%d!", 42);
    }
}

const char* const kCharFormatFull[] = { "abcdefghij", "ab42cd", "-12345", "xstry", "Q0.00", "v=42!" };

int CallWideFormat(unsigned short* pusDest, unsigned int uiCount, int iCase, const unsigned short* pusLong)
{
    static const std::vector<unsigned short> kFormats[] = {
        Widen("abcdefghij"), Widen("ab%dcd"), Widen("%d"), Widen("x%sy"), Widen("%c%f"), Widen("a%%b"), Widen("%s"),
    };
    static const std::vector<unsigned short> kStr = Widen("str");
    const unsigned short* pusFormat = &kFormats[iCase][0];
    switch (iCase)
    {
    case 1:
        return PblUnicode::usnprintf(pusDest, uiCount, pusFormat, 42);
    case 2:
        return PblUnicode::usnprintf(pusDest, uiCount, pusFormat, -12345);
    case 3:
        return PblUnicode::usnprintf(pusDest, uiCount, pusFormat, &kStr[0]);
    case 4:
        return PblUnicode::usnprintf(pusDest, uiCount, pusFormat, 'Q', 0.0);
    case 6:
        return PblUnicode::usnprintf(pusDest, uiCount, pusFormat, pusLong);
    default:
        return PblUnicode::usnprintf(pusDest, uiCount, pusFormat);
    }
}

const char* const kWideFormatFull[] = { "abcdefghij", "ab42cd", "-12345", "xstry", "Q0.00", "a%b" };

}

TEST_CASE("pblunicode bound: ustrlcpy" * doctest::test_suite("regression"))
{
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        const std::vector<unsigned int> lengths = SourceLengths(uiN);
        for (size_t l = 0; l < lengths.size(); ++l)
        {
            CAPTURE(uiN);
            CAPTURE(lengths[l]);
            const std::vector<unsigned short> src = WideText(lengths[l], 'a');
            WideBlock block(uiN);
            CHECK(PblUnicode::ustrlcpy(block.Data(), &src[0], uiN) == MinU(lengths[l], uiN));
            CheckWideResult(block, &src[0], lengths[l]);
        }
    }
}

TEST_CASE("pblunicode bound: ustrlcat" * doctest::test_suite("regression"))
{
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        const std::vector<unsigned int> lengths = SourceLengths(uiN);
        for (int iPrefix = 0; iPrefix < 3; ++iPrefix)
        {
            if ((uiN == 0 && iPrefix != 0) || (iPrefix == 1 && uiN < 3))
            {
                continue;
            }
            for (size_t l = 0; l < lengths.size(); ++l)
            {
                CAPTURE(uiN);
                CAPTURE(iPrefix);
                CAPTURE(lengths[l]);
                WideBlock block(uiN);
                unsigned int uiPrefix = 0;
                if (iPrefix == 0 && uiN > 0)
                {
                    block.v[0] = 0;
                }
                else if (iPrefix == 1)
                {
                    block.v[0] = 'P';
                    block.v[1] = 'P';
                    block.v[2] = 0;
                    uiPrefix = 2;
                }
                else if (iPrefix == 2)
                {
                    for (unsigned int i = 0; i < uiN; ++i)
                    {
                        block.v[i] = 'P';
                    }
                    uiPrefix = uiN;
                }
                const std::vector<unsigned short> src = WideText(lengths[l], 'a');
                std::vector<unsigned short> full(uiPrefix, 'P');
                full.insert(full.end(), src.begin(), src.end());
                CHECK(PblUnicode::ustrlcat(block.Data(), &src[0], uiN) == MinU(uiPrefix + lengths[l], uiN));
                CheckWideResult(block, &full[0], uiPrefix + lengths[l]);
            }
        }
    }

    unsigned short ausDest[8];
    ausDest[0] = 'a';
    ausDest[1] = 'b';
    ausDest[2] = 0;
    const std::vector<unsigned short> cd = Widen("cd");
    CHECK(PblUnicode::ustrlcat(ausDest, &cd[0], 8) == 4);
    const std::vector<unsigned short> abcd = Widen("abcd");
    CHECK(memcmp(ausDest, &abcd[0], 5 * sizeof(unsigned short)) == 0);
}

TEST_CASE("pblunicode bound: ustrncpy" * doctest::test_suite("regression"))
{
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        const std::vector<unsigned int> lengths = SourceLengths(uiN);
        for (size_t l = 0; l < lengths.size(); ++l)
        {
            CAPTURE(uiN);
            CAPTURE(lengths[l]);
            const std::vector<unsigned short> src = WideText(lengths[l], 'a');
            WideBlock block(uiN);
            CHECK(PblUnicode::ustrncpy(block.Data(), &src[0], uiN) == block.Data());
            CheckWideResult(block, &src[0], lengths[l]);
        }
    }
}

TEST_CASE("pblunicode bound: ustrncnv wide to char" * doctest::test_suite("regression"))
{
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        const std::vector<unsigned int> lengths = SourceLengths(uiN);
        for (size_t l = 0; l < lengths.size(); ++l)
        {
            CAPTURE(uiN);
            CAPTURE(lengths[l]);
            const std::vector<unsigned short> src = WideText(lengths[l], 'a');
            const std::string full = NarrowText(lengths[l]);
            CharBlock block(uiN);
            CHECK(PblUnicode::ustrncnv(block.Data(), &src[0], uiN) == MinU(lengths[l], uiN));
            CheckCharResult(block, full.c_str(), lengths[l]);
        }
    }

    const unsigned short ausMixed[] = { 'a', 0x80, 'b', 0xFF, 'c', 0x100, 'd', 0 };
    char acDest[8];
    CHECK(PblUnicode::ustrncnv(acDest, ausMixed, 8) == 4);
    CHECK(strcmp(acDest, "abcd") == 0);
}

TEST_CASE("pblunicode bound: ConvertUnicodeSkippingWide" * doctest::test_suite("regression"))
{
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        const std::vector<unsigned int> lengths = SourceLengths(uiN);
        for (size_t l = 0; l < lengths.size(); ++l)
        {
            CAPTURE(uiN);
            CAPTURE(lengths[l]);
            const std::vector<unsigned short> src = WideText(lengths[l], 'a');
            const std::string full = NarrowText(lengths[l]);
            CharBlock block(uiN);
            CHECK(ConvertUnicodeSkippingWide(block.Data(), &src[0], uiN) == MinU(lengths[l], uiN));
            CheckCharResult(block, full.c_str(), lengths[l]);
        }
    }

    const unsigned short ausMixed[] = { 'a', 0x80, 'b', 0x100, 'c', 0 };
    char acDest[8];
    CHECK(ConvertUnicodeSkippingWide(acDest, ausMixed, 8) == 4);
    CHECK(strcmp(acDest, "a\x80" "bc") == 0);
}

TEST_CASE("pblunicode bound: ustrncnv char to wide" * doctest::test_suite("regression"))
{
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        const std::vector<unsigned int> lengths = SourceLengths(uiN);
        for (size_t l = 0; l < lengths.size(); ++l)
        {
            CAPTURE(uiN);
            CAPTURE(lengths[l]);
            const std::string src = NarrowText(lengths[l]);
            const std::vector<unsigned short> full = Widen(src);
            WideBlock block(uiN);
            CHECK(PblUnicode::ustrncnv(block.Data(), src.c_str(), uiN) == MinU(lengths[l], uiN));
            CheckWideResult(block, &full[0], lengths[l]);
        }
    }

    unsigned short ausDest[4];
    CHECK(PblUnicode::ustrncnv(ausDest, "\xE9z", 4) == 2);
    CHECK(ausDest[0] == 0xFFE9);
    CHECK(ausDest[1] == 'z');
    CHECK(ausDest[2] == 0);
}

TEST_CASE("pblunicode bound: ustrncnv bytes to wide" * doctest::test_suite("regression"))
{
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        const std::vector<unsigned int> lengths = SourceLengths(uiN);
        for (size_t l = 0; l < lengths.size(); ++l)
        {
            CAPTURE(uiN);
            CAPTURE(lengths[l]);
            const std::string src = NarrowText(lengths[l]);
            const std::vector<unsigned short> full = Widen(src);
            WideBlock block(uiN);
            CHECK(PblUnicode::ustrncnv(block.Data(), (const unsigned char*)src.c_str(), uiN) == MinU(lengths[l], uiN));
            CheckWideResult(block, &full[0], lengths[l]);
        }
    }

    unsigned short ausDest[4];
    CHECK(PblUnicode::ustrncnv(ausDest, (const unsigned char*)"\xE9z", 4) == 2);
    CHECK(ausDest[0] == 0x00E9);
    CHECK(ausDest[1] == 'z');
    CHECK(ausDest[2] == 0);
}

TEST_CASE("pblunicode bound: char uvsnprintf" * doctest::test_suite("regression"))
{
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        for (int iCase = 0; iCase < 6; ++iCase)
        {
            CAPTURE(uiN);
            CAPTURE(iCase);
            const std::vector<unsigned short> full = Widen(kCharFormatFull[iCase]);
            const unsigned int uiFull = (unsigned int)strlen(kCharFormatFull[iCase]);
            WideBlock block(uiN);
            CHECK(CallCharFormat(block.Data(), uiN, iCase) == (int)MinU(uiFull, uiN));
            CheckWideResult(block, &full[0], uiFull);
        }
    }

    unsigned short ausDest[8];
    CHECK(PblUnicode::usnprintf(ausDest, 8, "v=%d!", 42) == 5);
    const std::vector<unsigned short> expected = Widen("v=42!");
    CHECK(memcmp(ausDest, &expected[0], 6 * sizeof(unsigned short)) == 0);
    CHECK(PblUnicode::usnprintf(ausDest, 8, "\xE9") == 1);
    CHECK(ausDest[0] == 0xFFE9);
    CHECK(ausDest[1] == 0);
    CHECK(PblUnicode::usnprintf(ausDest, 8, "ab%q") == -1);
    CHECK(ausDest[0] == 'a');
    CHECK(ausDest[1] == 'b');
    CHECK(ausDest[2] == 0);
}

TEST_CASE("pblunicode bound: wide uvsnprintf" * doctest::test_suite("regression"))
{
    const std::vector<unsigned short> longArg = WideText(399, 'A');
    for (size_t s = 0; s < sizeof(kBoundSizes) / sizeof(kBoundSizes[0]); ++s)
    {
        const unsigned int uiN = kBoundSizes[s];
        for (int iCase = 0; iCase < 7; ++iCase)
        {
            CAPTURE(uiN);
            CAPTURE(iCase);
            std::vector<unsigned short> full;
            unsigned int uiFull = 0;
            int iExpected = 0;
            if (iCase == 6)
            {
                full = longArg;
                uiFull = 399;
                iExpected = uiN == 0 ? 0 : (int)MinU(uiFull, uiN - 1);
            }
            else
            {
                full = Widen(kWideFormatFull[iCase]);
                uiFull = (unsigned int)strlen(kWideFormatFull[iCase]);
                iExpected = (int)MinU(uiFull, uiN);
            }
            WideBlock block(uiN);
            CHECK(CallWideFormat(block.Data(), uiN, iCase, &longArg[0]) == iExpected);
            CheckWideResult(block, &full[0], uiFull);
        }
    }
}

TEST_CASE("wide string call sites pass element counts" * doctest::test_suite("regression"))
{
    for (size_t i = 0; i < sizeof(kPins) / sizeof(kPins[0]); ++i)
    {
        std::string sText;
        CAPTURE(kPins[i].pcFile);
        CAPTURE(kPins[i].pcNew);
        REQUIRE(ReadRepoText(kPins[i].pcFile, sText));
        CHECK(sText.find(kPins[i].pcNew) != std::string::npos);
        CHECK(sText.find(kPins[i].pcOld) == std::string::npos);
    }

    std::string sIFace;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp", sIFace));
    CHECK(sIFace.find("usKey, usHash, 512)") == std::string::npos);
    CHECK(sIFace.find("usKey, usNum, 512)") == std::string::npos);
    CHECK(sIFace.find("usKey, usButton, 512)") == std::string::npos);
    CHECK(sIFace.find("GetLocalizedString(usButton, sizeof(usButton),") == std::string::npos);
    CHECK(sIFace.find("GetLocalizedString(usAction, sizeof(usAction),") == std::string::npos);

    std::string sLocalize;
    REQUIRE(ReadRepoText("src\\RedEngineFL\\Util\\RedLocalizeDB.cpp", sLocalize));
    CHECK(sLocalize.find("memset(pusBuffer, 0, uiSize);") == std::string::npos);
    CHECK(sLocalize.find("PblUnicode::ustrncpy(pusBuffer, pusString, uiSize - 1);") != std::string::npos);

    for (size_t i = 0; i < sizeof(kScanFiles) / sizeof(kScanFiles[0]); ++i)
    {
        std::string sText;
        REQUIRE(ReadRepoText(kScanFiles[i], sText));
        size_t uiStart = 0;
        while (uiStart < sText.size())
        {
            size_t uiEnd = sText.find('\n', uiStart);
            if (uiEnd == std::string::npos)
            {
                uiEnd = sText.size();
            }
            const std::string sLine = sText.substr(uiStart, uiEnd - uiStart);
            if (HasBareSizeofCount(sLine))
            {
                FAIL_CHECK(std::string(kScanFiles[i]) + ": " + sLine);
            }
            uiStart = uiEnd + 1;
        }
    }
}
