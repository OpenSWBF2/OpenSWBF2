#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>

#include "Battlefront2/Source/CommandLine.h"
#include "Battlefront2/Source/GameLoop.h"
#include "Battlefront2/Source/NetGame.h"

#include "testutil.h"

namespace
{

void Parse(const char* pcLine)
{
    GameLoop::mMissionScript[0] = 0;
    gExitOnGameOver = false;
    ParseCommandLine(0, pcLine);
}

std::string Mission()
{
    return GameLoop::mMissionScript;
}

}

TEST_CASE("command line keeps -perturbheap and -repeatmission out of the mission name" * doctest::test_suite("regression"))
{
    Parse("-perturbheap 1:pad,fill,pool geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("geo1c_con -perturbheap 7 -exitongameover");
    CHECK(Mission() == "geo1c_con");
    CHECK(gExitOnGameOver);

    Parse("-perturbheap -exitongameover geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(gExitOnGameOver);

    Parse("-perturbheap 2:pool, fill geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("-repeatmission geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("-repeatmission 9 geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("-repeatmission 0 geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("-shellfirst geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("-repeatmission 2 geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("geo1c_con -repeatmission 3 -exitongameover");
    CHECK(Mission() == "geo1c_con");
    CHECK(gExitOnGameOver);

    Parse("-exitonmovieclose nab2g_c -exitongameover");
    CHECK(Mission() == "nab2g_c");
    CHECK(gExitOnGameOver);
}

TEST_CASE("command line reports test-only switches it ignores" * doctest::test_suite("regression"))
{
    Parse("-perturbheap 1:pad -repeatmission 2 -shellfirst -exitonmovieclose geo1c_con");
    std::string ignored;
    const char* pcName = 0;
    int iCount = 0;
    for (int i = 0; TestOnlyArgIgnored(i, &pcName); ++i)
    {
        ++iCount;
        if (pcName)
        {
            ignored += pcName;
            ignored += " ";
        }
    }
    CHECK(iCount == 4);
#ifdef OPENSWBF2_TEST
    CHECK(ignored == "");
#else
    CHECK(ignored == "perturbheap repeatmission shellfirst exitonmovieclose ");
#endif

    Parse("geo1c_con");
    ignored.clear();
    for (int i = 0; TestOnlyArgIgnored(i, &pcName); ++i)
    {
        if (pcName)
        {
            ignored += pcName;
        }
    }
    CHECK(ignored == "");
    CHECK_FALSE(TestOnlyArgIgnored(-1, &pcName));
}

TEST_CASE("command line sets -novictory and keeps parsing after it" * doctest::test_suite("regression"))
{
    gNoVictory = true;
    Parse("geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK_FALSE(gNoVictory);

    Parse("geo1c_con -novictory");
    CHECK(Mission() == "geo1c_con");
    CHECK(gNoVictory);

    Parse("-NoVictory geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(gNoVictory);
    gNoVictory = false;
}

TEST_CASE("-novictory has no effect in a network game and says so once" * doctest::test_suite("regression"))
{
    const bool bInShell = netInShell;
    const bool bEnabled = netEnabled;
    const bool bEnabledNext = netEnabledNext;
    netInShell = false;
    netEnabled = false;
    netEnabledNext = false;
    Parse("geo1c_con -novictory");
    {
        LogCapture capture;
        CHECK(NoVictoryActive());
        netEnabledNext = true;
        CHECK_FALSE(NoVictoryActive());
        CHECK_FALSE(NoVictoryActive());
        netInShell = true;
        netEnabled = true;
        netEnabledNext = false;
        CHECK_FALSE(NoVictoryActive());
        CHECK(capture.Count("-novictory is ignored in a network game") == 1);
    }
    netInShell = false;
    netEnabled = false;
    Parse("geo1c_con");
    CHECK_FALSE(NoVictoryActive());
    netInShell = bInShell;
    netEnabled = bEnabled;
    netEnabledNext = bEnabledNext;
}

TEST_CASE("command line keeps -mipurge values out of the mission name" * doctest::test_suite("regression"))
{
    const char* apcLines[] = { "geo1c_con -mipurge 7", "geo1c_con -mipurge 0",
        "geo1c_con -mipurge 250 -exitongameover", "-mipurge 9 geo1c_con", "geo1c_con -mipurge",
        "geo1c_con /mipurge 7", "+mipurge 7 geo1c_con", "geo1c_con /mipurge 0 +mipurge 1" };
    for (int n = 0; n < 8; ++n)
    {
        Parse(apcLines[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gExitOnGameOver == (strstr(apcLines[n], "-exitongameover") != 0));
    }
}

TEST_CASE("command line keeps the values of removed switches out of the mission name" * doctest::test_suite("regression"))
{
    const char* apcLines[] = { "geo1c_con -simorder serial", "-simorder address geo1c_con", "geo1c_con -allocator mimalloc",
        "-allocator arena -exitongameover geo1c_con", "geo1c_con /simorder serial", "-simorder -exitongameover geo1c_con" };
    for (int n = 0; n < 6; ++n)
    {
        CAPTURE(n);
        Parse(apcLines[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gExitOnGameOver == (strstr(apcLines[n], "-exitongameover") != 0));
    }
}

TEST_CASE("command line keeps -heapcap values out of the mission name" * doctest::test_suite("regression"))
{
    const char* apcLines[] = { "geo1c_con -heapcap Runtime=96M", "-heapcap Runtime=96M geo1c_con",
        "geo1c_con -heapcap Runtime=1M -heapcap Runtime=96M -exitongameover", "geo1c_con /heapcap Bogus=1M", "geo1c_con -heapcap",
        "-heapcap -heapcap Runtime=96M geo1c_con" };
    for (int n = 0; n < 6; ++n)
    {
        CAPTURE(n);
        Parse(apcLines[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gExitOnGameOver == (strstr(apcLines[n], "-exitongameover") != 0));
    }
}

TEST_CASE("command line search returns each repeat of a switch in order" * doctest::test_suite("regression"))
{
    const char* pcLine = "game -heapcap A=1 -other x /heapcap B=2 -heapcap";
    int iPos = 0;
    const char* pcVal = 0;
    int iLen = 0;
    REQUIRE(FindNextCommandLineArg(pcLine, "-heapcap", &iPos, &pcVal, &iLen));
    CHECK(std::string(pcVal, iLen) == "A=1");
    REQUIRE(FindNextCommandLineArg(pcLine, "-heapcap", &iPos, &pcVal, &iLen));
    CHECK(std::string(pcVal, iLen) == "B=2");
    REQUIRE(FindNextCommandLineArg(pcLine, "-heapcap", &iPos, &pcVal, &iLen));
    CHECK(iLen == 0);
    CHECK_FALSE(FindNextCommandLineArg(pcLine, "-heapcap", &iPos, &pcVal, &iLen));
}

TEST_CASE("command line heapcap scan keeps a switch as the next token" * doctest::test_suite("regression"))
{
    const char* pcLine = "game -heapcap -heapcap A=1 -heapcap \"-other\" -heapcap \"-heapcap\" B=2 -heapcap";
    int iPos = 0;
    const char* pcVal = 0;
    int iLen = 0;
    const char* const apcValues[] = { "-heapcap", "A=1", "-other", "-heapcap", "B=2", "" };
    for (int n = 0; n < 6; ++n)
    {
        CAPTURE(n);
        REQUIRE(FindNextCommandLineArg(pcLine, "-heapcap", &iPos, &pcVal, &iLen, true));
        CHECK(std::string(pcVal, iLen) == apcValues[n]);
    }
    CHECK_FALSE(FindNextCommandLineArg(pcLine, "-heapcap", &iPos, &pcVal, &iLen, true));
}

TEST_CASE("command line first-match search still returns the first value" * doctest::test_suite("regression"))
{
    const char* pcVal = 0;
    int iLen = 0;
    REQUIRE(FindCommandLineArg("game -heapcap A=1 -heapcap B=2", "-heapcap", &pcVal, &iLen));
    CHECK(std::string(pcVal, iLen) == "A=1");
    CHECK_FALSE(FindCommandLineArg("game -other x", "-heapcap", &pcVal, &iLen));
}

TEST_CASE("command line search accepts the same switch forms as the parser" * doctest::test_suite("regression"))
{
    struct FormCase
    {
        const char* pcLine;
        const char* pcValue;
    };
    const FormCase akForms[] = {
        { "x.exe geo1c_con -mipurge 250", "250" },
        { "x.exe geo1c_con /mipurge 250", "250" },
        { "x.exe geo1c_con +mipurge 250", "250" },
        { "x.exe,geo1c_con,-mipurge,250", "250" },
        { "x.exe \"-mipurge\" \"250\" geo1c_con", "250" },
        { "x.exe -MIPURGE 7", "7" },
        { "\"c:\\game dir\\x.exe\" /mipurge 250", "250" },
        { "x.exe -mipurge", "" },
    };
    for (int n = 0; n < 8; ++n)
    {
        CAPTURE(n);
        const char* pcVal = 0;
        int iLen = -1;
        REQUIRE(FindCommandLineArg(akForms[n].pcLine, "-mipurge", &pcVal, &iLen));
        CHECK(std::string(pcVal, iLen) == akForms[n].pcValue);
    }
    const char* apcMiss[] = { "x.exe geo1c_con", "x.exe \"a -mipurge b\" c", "x.exe -mipurges 250", "x.exe mipurge 250" };
    for (int n = 0; n < 4; ++n)
    {
        CAPTURE(n);
        const char* pcVal = (const char*)1;
        int iLen = -1;
        CHECK_FALSE(FindCommandLineArg(apcMiss[n], "-mipurge", &pcVal, &iLen));
        CHECK(pcVal == nullptr);
        CHECK(iLen == 0);
    }
}

TEST_CASE("command line parser ignores tokens past its argument table" * doctest::test_suite("regression"))
{
    std::string sLine;
    for (int n = 0; n < 300; ++n)
    {
        sLine += "a ";
    }
    Parse((sLine + "geo1c_con").c_str());
    CHECK(Mission() == "a");
}

TEST_CASE("the startup argument search goes through the command line tokenizer (source pin)" * doctest::test_suite("regression"))
{
    std::string sCrash;
    if (!ReadRepoText("src\\Battlefront2\\Source\\pcCrash.cpp", sCrash))
    {
        SkipCase("pcCrash.cpp not found");
    }
    CHECK(sCrash.find("FindCommandLineArg(pcCmd, pcName, &pcVal, piLen);") != std::string::npos);
}

TEST_CASE("command line repeat count takes any digits and clamps them" * doctest::test_suite("regression"))
{
    CHECK(CommandLineRepeatCount("3", 1) == 3);
    CHECK(CommandLineRepeatCount("0", 1) == 1);
    CHECK(CommandLineRepeatCount("9", 1) == 9);
    CHECK(CommandLineRepeatCount("16", 2) == 16);
    CHECK(CommandLineRepeatCount("17", 2) == 16);
    CHECK(CommandLineRepeatCount("00", 2) == 1);
    CHECK(CommandLineRepeatCount("99999999999", 11) == 16);
    CHECK(CommandLineRepeatCount("abc", 3) == -1);
    CHECK(CommandLineRepeatCount("12x", 3) == -1);
    CHECK(CommandLineRepeatCount("", 0) == -1);
}

TEST_CASE("the startup repeat switch goes through the shared count (source pin)" * doctest::test_suite("regression"))
{
    std::string sCrash;
    if (!ReadRepoText("src\\Battlefront2\\Source\\pcCrash.cpp", sCrash))
    {
        SkipCase("pcCrash.cpp not found");
    }
    CHECK(sCrash.find("const int iCount = CommandLineRepeatCount(pcVal, iLen);") != std::string::npos);
    CHECK(sCrash.find("siRepeat = iCount > 0 ? iCount : 1;") != std::string::npos);
}
