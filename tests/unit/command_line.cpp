#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>

#include "Battlefront2/Source/CommandLine.h"
#include "Battlefront2/Source/GameLoop.h"
#include "Battlefront2/Source/NetGame.h"
#include "PebbleFL/Common/PblSimOrder.h"

#include "testutil.h"

namespace
{

class SimOrderRestore
{
public:
    SimOrderRestore() : mePrev(PblGetSimOrder()) {}
    ~SimOrderRestore() { PblSetSimOrder(mePrev); }

private:
    PblSimOrderMode mePrev;
};

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

TEST_CASE("command line keeps -simorder values out of the mission name" * doctest::test_suite("regression"))
{
    SimOrderRestore restore;
    PblSetSimOrder(PBL_SIMORDER_SERIAL);
    Parse("geo1c_con -simorder address");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    CHECK_FALSE(SimOrderArgRejected());

    Parse("geo1c_con -simorder serial");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_SERIAL);
    CHECK_FALSE(SimOrderArgRejected());
}

TEST_CASE("command line leaves the order unchanged without a valid -simorder value" * doctest::test_suite("regression"))
{
    SimOrderRestore restore;
    const PblSimOrderMode aeSeeds[] = { PBL_SIMORDER_ADDRESS, PBL_SIMORDER_SERIAL };
    const char* apcLines[] = {
        "-simorder bogus geo1c_con",
        "-simorder -exitongameover geo1c_con",
        "geo1c_con -simorder",
        "geo1c_con",
    };
    for (size_t s = 0; s < sizeof(aeSeeds) / sizeof(aeSeeds[0]); ++s)
    {
        for (size_t n = 0; n < sizeof(apcLines) / sizeof(apcLines[0]); ++n)
        {
            CAPTURE(apcLines[n]);
            const int iSeed = (int)aeSeeds[s];
            CAPTURE(iSeed);
            PblSetSimOrder(aeSeeds[s]);
            Parse(apcLines[n]);
            CHECK(Mission() == "geo1c_con");
            CHECK(PblGetSimOrder() == aeSeeds[s]);
            CHECK(SimOrderArgRejected() == (strstr(apcLines[n], "-simorder") != 0));
            CHECK(gExitOnGameOver == (strstr(apcLines[n], "-exitongameover") != 0));
        }
    }
}

TEST_CASE("command line lets the last -simorder value win" * doctest::test_suite("regression"))
{
    SimOrderRestore restore;
    PblSetSimOrder(PBL_SIMORDER_SERIAL);
    Parse("-simorder bogus -simorder address geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    CHECK_FALSE(SimOrderArgRejected());

    PblSetSimOrder(PBL_SIMORDER_SERIAL);
    Parse("-simorder address -simorder bogus geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    CHECK(SimOrderArgRejected());
}

TEST_CASE("command line keeps -perturbheap and -repeatmission out of the mission name" * doctest::test_suite("regression"))
{
    SimOrderRestore restore;
    Parse("-perturbheap 1:pad,top,fill,pool geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("geo1c_con -perturbheap 7 -exitongameover");
    CHECK(Mission() == "geo1c_con");
    CHECK(gExitOnGameOver);

    Parse("-perturbheap -exitongameover geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(gExitOnGameOver);

    PblSetSimOrder(PBL_SIMORDER_SERIAL);
    Parse("-perturbheap 2:pool, top geo1c_con -simorder address");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);

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
    SimOrderRestore restore;
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
    SimOrderRestore restore;
    gNoVictory = true;
    Parse("geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK_FALSE(gNoVictory);

    PblSetSimOrder(PBL_SIMORDER_SERIAL);
    Parse("geo1c_con -novictory -simorder address");
    CHECK(Mission() == "geo1c_con");
    CHECK(gNoVictory);
    CHECK(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    CHECK_FALSE(SimOrderArgRejected());

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

TEST_CASE("mission victory and defeat return early under -novictory" * doctest::test_suite("regression"))
{
    std::string sMission;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp", sMission));
    const char* apcFns[] = { "int MissionVictory(lua_State* L)\n{\n", "int MissionDefeat(lua_State* L)\n{\n" };
    for (size_t n = 0; n < sizeof(apcFns) / sizeof(apcFns[0]); ++n)
    {
        CAPTURE(apcFns[n]);
        std::string::size_type uiBegin = sMission.find(apcFns[n]);
        REQUIRE(uiBegin != std::string::npos);
        uiBegin += strlen(apcFns[n]);
        const char* pcGuard = "    if (NoVictoryActive())\n    {\n        return 0;\n    }\n    unsigned int u";
        CHECK(sMission.compare(uiBegin, strlen(pcGuard), pcGuard) == 0);
    }
}

TEST_CASE("command line keeps -allocator and -mipurge values out of the mission name" * doctest::test_suite("regression"))
{
    const char* apcLines[] = { "geo1c_con -allocator arena", "geo1c_con -allocator mimalloc -mipurge 0",
        "geo1c_con -mipurge 250 -exitongameover", "-allocator bogus geo1c_con", "geo1c_con -mipurge",
        "geo1c_con /allocator arena", "+allocator arena geo1c_con", "geo1c_con /mipurge 0 +mipurge 1" };
    for (int n = 0; n < 8; ++n)
    {
        Parse(apcLines[n]);
        CHECK(Mission() == "geo1c_con");
        CHECK(gExitOnGameOver == (strstr(apcLines[n], "-exitongameover") != 0));
    }
}

TEST_CASE("command line search accepts the same switch forms as the parser" * doctest::test_suite("regression"))
{
    struct FormCase
    {
        const char* pcLine;
        const char* pcValue;
    };
    const FormCase akForms[] = {
        { "x.exe geo1c_con -allocator arena", "arena" },
        { "x.exe geo1c_con /allocator arena", "arena" },
        { "x.exe geo1c_con +allocator arena", "arena" },
        { "x.exe,geo1c_con,-allocator,arena", "arena" },
        { "x.exe \"-allocator\" \"arena\" geo1c_con", "arena" },
        { "x.exe -ALLOCATOR Arena", "Arena" },
        { "\"c:\\game dir\\x.exe\" /allocator arena", "arena" },
        { "x.exe -allocator", "" },
    };
    for (int n = 0; n < 8; ++n)
    {
        CAPTURE(n);
        const char* pcVal = 0;
        int iLen = -1;
        REQUIRE(FindCommandLineArg(akForms[n].pcLine, "-allocator", &pcVal, &iLen));
        CHECK(std::string(pcVal, iLen) == akForms[n].pcValue);
    }
    const char* apcMiss[] = { "x.exe geo1c_con", "x.exe \"a -allocator b\" c", "x.exe -allocators arena", "x.exe allocator arena" };
    for (int n = 0; n < 4; ++n)
    {
        CAPTURE(n);
        const char* pcVal = (const char*)1;
        int iLen = -1;
        CHECK_FALSE(FindCommandLineArg(apcMiss[n], "-allocator", &pcVal, &iLen));
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
    CHECK(CommandLineRepeatCount("9", 1) == 8);
    CHECK(CommandLineRepeatCount("00", 2) == 1);
    CHECK(CommandLineRepeatCount("99999999999", 11) == 8);
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
