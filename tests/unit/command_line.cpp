#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string.h>
#include <string>

#include "Battlefront2/Source/AutomationTest.h"
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

TEST_CASE_NAMED(FCommandLineTest1, "Unit::CommandLine::command line keeps -perturbheap and -repeatmission out of the mission name", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest2, "Unit::CommandLine::command line reports test-only switches it ignores", "[EngineFilter]")
{
    Parse("-perturbheap 1:pad -repeatmission 2 -shellfirst -exitonmovieclose -digestdetail 4 -savetest write -handsoff "
          "-movietest shell.mvs:0xB4A21ED9:10 -movieloops 2 -movielocal -movieskip 300 -moviefault hold "
          "-autotest Harness.AutoTest.Pass geo1c_con");
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
    CHECK(iCount == 13);
    CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
    CHECK(ignored == "");
    CHECK(gAutoTestRequested);
    CHECK(strcmp(gAutoTestFilter, "Harness.AutoTest.Pass") == 0);
#else
    CHECK(ignored == "perturbheap repeatmission shellfirst exitonmovieclose digestdetail savetest handsoff movietest "
                     "movieloops movielocal movieskip moviefault autotest ");
    CHECK(!gAutoTestRequested);
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

TEST_CASE_NAMED(FCommandLineTest3, "Unit::CommandLine::command line -digestdetail takes a turn in Test builds and keeps it out of the mission name", "[EngineFilter]")
{
    {
        LogCapture capture;
        Parse("-digestdetail 4 geo1c_con");
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(gDigestDetail == 4);
#else
        CHECK(gDigestDetail == 0);
#endif
        Parse("geo1c_con -digestdetail 999999");
#ifdef OPENSWBF2_TEST
        CHECK(gDigestDetail == 100000);
#else
        CHECK(gDigestDetail == 0);
#endif
        CHECK(capture.Count("-digestdetail needs a number, ignored") == 0);
    }
    const char* apcBad[] = { "-digestdetail abc geo1c_con", "-digestdetail -5 geo1c_con", "geo1c_con -digestdetail" };
    for (int n = 0; n < 3; ++n)
    {
        CAPTURE(n);
        LogCapture capture;
        Parse(apcBad[n]);
        CHECK(gDigestDetail == 0);
        CHECK(capture.Count("-digestdetail needs a number, ignored") == 1);
    }
    Parse("geo1c_con -digestdetail 7");
    CHECK(Mission() == "geo1c_con");
    Parse("geo1c_con");
    CHECK(gDigestDetail == 0);
}

TEST_CASE_NAMED(FCommandLineTest4, "Unit::CommandLine::command line switches movietest and movieloops take checked values in Test builds and keep them out of the mission name", "[EngineFilter]")
{
    {
        LogCapture capture;
        Parse("-movietest shell.mvs:0xB4A21ED9:1349 -movieloops 2 geo1c_con");
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(std::string(gMovieTestFile) == "shell.mvs");
        CHECK(gMovieTestSegment == 0xB4A21ED9u);
        CHECK(gMovieTestFrames == 1349);
        CHECK(gMovieLoops == 2);
#else
        CHECK(gMovieTestFile[0] == 0);
        CHECK(gMovieTestFrames == 0);
        CHECK(gMovieLoops == 0);
#endif
        CHECK(capture.Count("ignored") == 0);
    }
    {
        LogCapture capture;
        Parse("geo1c_con -movietest In-Game_2.MVS:shell:100000");
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(std::string(gMovieTestFile) == "In-Game_2.MVS");
        CHECK(gMovieTestSegment == 0x11E1FC01u);
        CHECK(gMovieTestFrames == 100000);
#endif
        CHECK(capture.Count("-movietest needs") == 0);
    }
    const char* apcBadTest[] = { "-movietest shell.mvs:0xB4A21ED9 geo1c_con", "-movietest shell.bik:0x1:5 geo1c_con",
        "-movietest ..\shell.mvs:0x1:5 geo1c_con", "-movietest a/b.mvs:0x1:5 geo1c_con", "-movietest .mvs:0x1:5 geo1c_con",
        "-movietest shell.mvs:0x:5 geo1c_con", "-movietest shell.mvs:0x123456789:5 geo1c_con",
        "-movietest shell.mvs:0x1G:5 geo1c_con", "-movietest shell.mvs:a.b:5 geo1c_con", "-movietest shell.mvs::5 geo1c_con",
        "-movietest shell.mvs:0x1:0 geo1c_con", "-movietest shell.mvs:0x1:100001 geo1c_con",
        "-movietest shell.mvs:0x1:5x geo1c_con", "-movietest shell.mvs:0x1:+5 geo1c_con", "-movietest shell.mvs:0x1:5:6 geo1c_con",
        "-movietest abcdefghijklmnopqrstuvwxyz12.mvs:0x1:5 geo1c_con", "-movietest -movieloops 2 geo1c_con" };
    for (int n = 0; n < (int)(sizeof(apcBadTest) / sizeof(apcBadTest[0])); ++n)
    {
        CAPTURE(n);
        LogCapture capture;
        Parse(apcBadTest[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gMovieTestFile[0] == 0);
        CHECK(gMovieTestFrames == 0);
        CHECK(capture.Count("-movietest needs <name>.mvs:<segment>:<frames from 1 to 100000>, ignored") == 1);
    }
    const char* apcBadLoops[] = { "-movieloops 0 geo1c_con", "-movieloops 1001 geo1c_con", "-movieloops 2x geo1c_con",
        "-movieloops +2 geo1c_con", "geo1c_con -movieloops" };
    for (int n = 0; n < (int)(sizeof(apcBadLoops) / sizeof(apcBadLoops[0])); ++n)
    {
        CAPTURE(n);
        LogCapture capture;
        Parse(apcBadLoops[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gMovieLoops == 0);
        CHECK(capture.Count("-movieloops needs a count from 1 to 1000, ignored") == 1);
    }
    Parse("geo1c_con");
    CHECK(gMovieTestFile[0] == 0);
    CHECK(gMovieTestSegment == 0u);
    CHECK(gMovieTestFrames == 0);
    CHECK(gMovieLoops == 0);
}

TEST_CASE_NAMED(FCommandLineTest5, "Unit::CommandLine::command line switches movielocal and movieskip take checked values in Test builds and keep them out of the mission name", "[EngineFilter]")
{
    {
        LogCapture capture;
        Parse("-movielocal -movieskip 300 geo1c_con");
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(gMovieLocal);
        CHECK(gMovieSkip == 300);
#else
        CHECK_FALSE(gMovieLocal);
        CHECK(gMovieSkip == 0);
#endif
        CHECK(capture.Count("ignored") == 0);
    }
    {
        LogCapture capture;
        Parse("geo1c_con -movieskip 100000 -movielocal");
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(gMovieLocal);
        CHECK(gMovieSkip == 100000);
#endif
        CHECK(capture.Count("-movieskip needs") == 0);
    }
    const char* apcBadSkip[] = { "-movieskip 0 geo1c_con", "-movieskip 100001 geo1c_con", "-movieskip 30x geo1c_con",
        "-movieskip +30 geo1c_con", "geo1c_con -movieskip", "-movieskip -movielocal geo1c_con" };
    for (int n = 0; n < (int)(sizeof(apcBadSkip) / sizeof(apcBadSkip[0])); ++n)
    {
        CAPTURE(n);
        LogCapture capture;
        Parse(apcBadSkip[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gMovieSkip == 0);
        CHECK(capture.Count("-movieskip needs a frame from 1 to 100000, ignored") == 1);
    }
    Parse("geo1c_con");
    CHECK_FALSE(gMovieLocal);
    CHECK(gMovieSkip == 0);
}

TEST_CASE_NAMED(FCommandLineTest6, "Unit::CommandLine::command line switch moviefault takes a named fault in Test builds and keeps it out of the mission name", "[EngineFilter]")
{
    const char* apcGood[] = { "-moviefault texture geo1c_con", "geo1c_con -moviefault SURFACE", "-moviefault hold geo1c_con" };
    const int aiWant[] = { kMovieFaultTexture, kMovieFaultSurface, kMovieFaultHold };
    CHECK(kMovieFaultTexture == 1);
    CHECK(kMovieFaultSurface == 2);
    CHECK(kMovieFaultHold == 3);
    for (int n = 0; n < 3; ++n)
    {
        CAPTURE(n);
        LogCapture capture;
        Parse(apcGood[n]);
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(gMovieFault == aiWant[n]);
#else
        (void)aiWant;
        CHECK(gMovieFault == 0);
#endif
        CHECK(capture.Count("-moviefault needs") == 0);
    }
    const char* apcBad[] = { "-moviefault textures geo1c_con", "-moviefault 1 geo1c_con", "geo1c_con -moviefault",
        "-moviefault -movielocal geo1c_con", "-moviefault hol geo1c_con" };
    for (int n = 0; n < (int)(sizeof(apcBad) / sizeof(apcBad[0])); ++n)
    {
        CAPTURE(n);
        LogCapture capture;
        Parse(apcBad[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gMovieFault == 0);
        CHECK(capture.Count("-moviefault needs texture, surface or hold, ignored") == 1);
    }
    Parse("-moviefault hold geo1c_con");
    Parse("geo1c_con");
    CHECK(gMovieFault == 0);
}

TEST_CASE_NAMED(FCommandLineTest7, "Unit::CommandLine::command line -savetest takes write or load with a checked type and name and keeps them out of the mission name", "[EngineFilter]")
{
    {
        LogCapture capture;
        Parse("-savetest write geo1c_con");
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(gSaveTestMode == 1);
#else
        CHECK(gSaveTestMode == 0);
#endif
        Parse("-savetest load 1 SaveTest geo1c_con");
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(gSaveTestMode == 2);
        CHECK(gSaveTestType == 1);
        CHECK(std::string(gSaveTestName) == "SaveTest");
#else
        CHECK(gSaveTestMode == 0);
#endif
        Parse("geo1c_con -savetest load 3 Save-Test_15chr");
        CHECK(Mission() == "geo1c_con");
#ifdef OPENSWBF2_TEST
        CHECK(gSaveTestMode == 2);
        CHECK(gSaveTestType == 3);
        CHECK(std::string(gSaveTestName) == "Save-Test_15chr");
#endif
        CHECK(capture.Count("-savetest") == 0);
    }
    const char* apcBad[] = { "-savetest load 1 ..\\x geo1c_con", "-savetest load 1 a/b geo1c_con",
        "-savetest load 1 C:x geo1c_con", "-savetest load 1 x.profile geo1c_con", "-savetest load 1 Sixteen_chars_xx geo1c_con",
        "-savetest load 2 SaveTest geo1c_con", "-savetest load 0 SaveTest geo1c_con", "-savetest load 13 SaveTest geo1c_con" };
    for (int n = 0; n < (int)(sizeof(apcBad) / sizeof(apcBad[0])); ++n)
    {
        CAPTURE(n);
        LogCapture capture;
        Parse(apcBad[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gSaveTestMode == 0);
        CHECK(gSaveTestName[0] == 0);
        CHECK(capture.Count("-savetest load needs a type 1 or 3 and a name of 1 to 15 letters, digits, _ or -, ignored") == 1);
    }
    {
        LogCapture capture;
        Parse("-savetest load 1 -exitongameover geo1c_con");
        CHECK(Mission() == "geo1c_con");
        CHECK(gExitOnGameOver);
        CHECK(gSaveTestMode == 0);
        CHECK(capture.Count("-savetest load needs a type 1 or 3") == 1);
    }
    {
        LogCapture capture;
        Parse("-savetest geo1c_con");
        CHECK(Mission() == "geo1c_con");
        CHECK(gSaveTestMode == 0);
        CHECK(capture.Count("-savetest needs write or load, ignored") == 1);
    }
    CHECK(SaveTestNameOk("A"));
    CHECK(SaveTestNameOk("Save-Test_15chr"));
    CHECK_FALSE(SaveTestNameOk(""));
    CHECK_FALSE(SaveTestNameOk("Sixteen_chars_xx"));
    CHECK_FALSE(SaveTestNameOk("a b"));
    CHECK_FALSE(SaveTestNameOk("a\\b"));
    CHECK_FALSE(SaveTestNameOk("a:b"));
    CHECK_FALSE(SaveTestNameOk("a.b"));
    Parse("geo1c_con");
    CHECK(gSaveTestMode == 0);
}

TEST_CASE_NAMED(FCommandLineTest8, "Unit::CommandLine::command line sets -novictory and keeps parsing after it", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest9, "Unit::CommandLine::command line -soakturns takes a positive turn count and rejects the rest", "[EngineFilter]")
{
    {
        LogCapture capture;
        Parse("-soakturns 9300 geo1c_con");
        CHECK(gSoakTurns == 9300);
        CHECK(Mission() == "geo1c_con");

        Parse("-soakturns 1500 geo1c_con");
        CHECK(gSoakTurns == 1500);
        CHECK(Mission() == "geo1c_con");

        Parse("-soakturns 1 geo1c_con");
        CHECK(gSoakTurns == 1);
        CHECK(Mission() == "geo1c_con");

        Parse("-soakturns 2147483647 geo1c_con");
        CHECK(gSoakTurns == 2147483647);
        CHECK(capture.Count("-soakturns needs a positive turn count, ignored") == 0);

        Parse("-soakturns 0 geo1c_con");
        CHECK(gSoakTurns == 0);
        CHECK(Mission() == "geo1c_con");
        CHECK(capture.Count("-soakturns needs a positive turn count, ignored") == 1);
    }

    Parse("-soakturns -5 geo1c_con");
    CHECK(gSoakTurns == 0);

    Parse("-soakturns abc geo1c_con");
    CHECK(gSoakTurns == 0);

    Parse("-soakturns 99999999999 geo1c_con");
    CHECK(gSoakTurns == 0);
    CHECK(Mission() == "geo1c_con");

    Parse("-soakturns 2147483648 geo1c_con");
    CHECK(gSoakTurns == 0);

    Parse("-soakturns");
    CHECK(gSoakTurns == 0);

    Parse("geo1c_con");
    CHECK(gSoakTurns == 0);
}

extern int gStubHangTimeout;

TEST_CASE_NAMED(FCommandLineTest10, "Unit::CommandLine::command line -hangtimeout passes only a whole number", "[EngineFilter]")
{
    const char* pcWarn = "-hangtimeout needs 0 or a whole number of seconds from 1 to 3600, ignored";
    const char* apcGood[] = { "-hangtimeout 0 geo1c_con", "-hangtimeout 5 geo1c_con", "-hangtimeout -5 geo1c_con" };
    const int aiGood[] = { 0, 5, -5 };
    const int aiWarn[] = { 0, 0, 1 };
    for (int n = 0; n < 3; ++n)
    {
        LogCapture capture;
        gStubHangTimeout = 99;
        Parse(apcGood[n]);
        CHECK(gStubHangTimeout == aiGood[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(capture.Count(pcWarn) == aiWarn[n]);
    }
    const char* apcBad[] = { "-hangtimeout abc geo1c_con", "-hangtimeout 0x geo1c_con", "-hangtimeout 5s geo1c_con",
        "geo1c_con -hangtimeout" };
    for (int n = 0; n < 4; ++n)
    {
        LogCapture capture;
        gStubHangTimeout = 99;
        Parse(apcBad[n]);
        CHECK(gStubHangTimeout == 99);
        CHECK(Mission() == "geo1c_con");
        CHECK(capture.Count(pcWarn) == 1);
    }
}

TEST_CASE_NAMED(FCommandLineTest11, "Unit::CommandLine::-novictory has no effect in a network game and says so once", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest12, "Unit::CommandLine::command line keeps -mipurge values out of the mission name", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest13, "Unit::CommandLine::command line keeps the values of removed switches out of the mission name", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest14, "Unit::CommandLine::command line keeps -heapcap values out of the mission name", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest15, "Unit::CommandLine::command line search returns each repeat of a switch in order", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest16, "Unit::CommandLine::command line heapcap scan keeps a switch as the next token", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest17, "Unit::CommandLine::command line first-match search still returns the first value", "[EngineFilter]")
{
    const char* pcVal = 0;
    int iLen = 0;
    REQUIRE(FindCommandLineArg("game -heapcap A=1 -heapcap B=2", "-heapcap", &pcVal, &iLen));
    CHECK(std::string(pcVal, iLen) == "A=1");
    CHECK_FALSE(FindCommandLineArg("game -other x", "-heapcap", &pcVal, &iLen));
}

TEST_CASE_NAMED(FCommandLineTest18, "Unit::CommandLine::command line search accepts the same switch forms as the parser", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest19, "Unit::CommandLine::command line parser ignores tokens past its argument table", "[EngineFilter]")
{
    std::string sLine;
    for (int n = 0; n < 300; ++n)
    {
        sLine += "a ";
    }
    Parse((sLine + "geo1c_con").c_str());
    CHECK(Mission() == "a");
}

TEST_CASE_NAMED(FCommandLineTest20, "Unit::CommandLine::the startup argument search goes through the command line tokenizer (source pin)", "[EngineFilter]")
{
    std::string sCrash;
    if (!ReadRepoText("src\\Battlefront2\\Source\\pcCrash.cpp", sCrash))
    {
        SkipCase("pcCrash.cpp not found");
    }
    CHECK(sCrash.find("FindCommandLineArg(pcCmd, pcName, &pcVal, piLen);") != std::string::npos);
}

TEST_CASE_NAMED(FCommandLineTest21, "Unit::CommandLine::command line repeat count takes any digits and clamps them", "[EngineFilter]")
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

TEST_CASE_NAMED(FCommandLineTest22, "Unit::CommandLine::the startup repeat switch goes through the shared count (source pin)", "[EngineFilter]")
{
    std::string sCrash;
    if (!ReadRepoText("src\\Battlefront2\\Source\\pcCrash.cpp", sCrash))
    {
        SkipCase("pcCrash.cpp not found");
    }
    CHECK(sCrash.find("const int iCount = CommandLineRepeatCount(pcVal, iLen);") != std::string::npos);
    CHECK(sCrash.find("siRepeat = iCount > 0 ? iCount : 1;") != std::string::npos);
}

#ifdef OPENSWBF2_TEST
TEST_CASE_NAMED(FCommandLineTestAutoTestFilter, "Unit::CommandLine::-autotest selects names and wildcards joined by plus with parameters", "[EngineFilter]")
{
    char acParameters[16];
    CHECK(AutomationTest::Select("Harness.AutoTest.Pass", "harness.autotest.pass", acParameters, sizeof(acParameters)));
    CHECK(strcmp(acParameters, "") == 0);
    CHECK(AutomationTest::Select("Other+Harness.*=a b", "Harness.AutoTest.Fail", acParameters, sizeof(acParameters)));
    CHECK(strcmp(acParameters, "a b") == 0);
    CHECK(AutomationTest::Select("*.Pass=1+*=2", "Harness.AutoTest.Pass", acParameters, sizeof(acParameters)));
    CHECK(strcmp(acParameters, "1") == 0);
    CHECK_FALSE(AutomationTest::Select("Harness.AutoTest", "Harness.AutoTest.Pass", acParameters, sizeof(acParameters)));
    CHECK_FALSE(AutomationTest::Select("Harness.AutoTest.Pass+", "Harness.AutoTest.Fail", acParameters, sizeof(acParameters)));
    CHECK_FALSE(AutomationTest::Select("", "Harness.AutoTest.Pass", acParameters, sizeof(acParameters)));
    CHECK_FALSE(AutomationTest::Select("=x", "Harness.AutoTest.Pass", acParameters, sizeof(acParameters)));
    CHECK_FALSE(AutomationTest::Select("*=0123456789abcdef", "Harness.AutoTest.Pass", acParameters, sizeof(acParameters)));
}
#endif
