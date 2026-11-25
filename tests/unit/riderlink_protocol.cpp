#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#ifdef OPENSWBF2_RIDERLINK

#include <random>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include <windows.h>

#include "testutil.h"
#include "Battlefront2/Source/RiderLinkProtocol.h"
#include "LuaFL/include/lua.h"
#include "LuaFL/include/lauxlib.h"
#include "LuaFL/include/lualib.h"
#include "RedEngineFL/Util/RedLog.h"
#include "RedEngineFL/Util/RedLogTap.h"

using namespace RiderLinkProtocol;

namespace
{

std::string FeedAll(LineAssembler& kAsm, const std::string& sIn, const std::vector<size_t>& kCuts)
{
    std::string sOut;
    size_t uFrom = 0;
    for (size_t c = 0; c <= kCuts.size(); ++c)
    {
        const size_t uTo = c < kCuts.size() ? kCuts[c] : sIn.size();
        const unsigned int uLen = (unsigned int)(uTo - uFrom);
        const char* pcChunk = sIn.data() + uFrom;
        unsigned int uPos = 0;
        for (;;)
        {
            const TakeResult eResult = Take(kAsm, pcChunk, uLen, uPos);
            if (eResult == TakeNeedMore)
            {
                break;
            }
            if (eResult == TakeOversized)
            {
                sOut += "O|";
            }
            else
            {
                CHECK(kAsm.uLen < kInCap - 1);
                sOut += "L:" + std::string(kAsm.acLine, kAsm.uLen) + "|";
            }
        }
        uFrom = uTo;
    }
    return sOut;
}

std::string FeedWhole(const std::string& sIn)
{
    LineAssembler kAsm;
    ResetAssembler(kAsm);
    return FeedAll(kAsm, sIn, std::vector<size_t>());
}

Message ParseText(const std::string& sLine, std::string& sToken, unsigned int& uVersion)
{
    char acToken[kTokenLen + 1];
    char acMission[kMissionCap];
    const Message eMsg = Parse(sLine.data(), (unsigned int)sLine.size(), acToken, uVersion, acMission);
    sToken = acToken;
    return eMsg;
}

Message ParseMission(const std::string& sLine, std::string& sMission)
{
    char acToken[kTokenLen + 1];
    char acMission[kMissionCap];
    unsigned int uVersion = 0;
    const Message eMsg = Parse(sLine.data(), (unsigned int)sLine.size(), acToken, uVersion, acMission);
    sMission = acMission;
    return eMsg;
}

bool MissionOk(const std::string& sName)
{
    return IsMissionName(sName.data(), (unsigned int)sName.size());
}

struct TestRing
{
    explicit TestRing(unsigned int uSize) : kData(uSize)
    {
        RingInit(kRing, &kData[0], uSize);
    }

    std::string Read(const char* pcState = "Play", unsigned int uCap = 1 << 20)
    {
        std::vector<char> kOut(uCap);
        const unsigned int uLen = RingRead(kRing, &kOut[0], uCap, pcState);
        return std::string(&kOut[0], uLen);
    }

    std::vector<unsigned char> kData;
    Ring kRing;
};

const char* const kToken = "0123456789abcdef0123456789abcdef";

TapBuilder sTestTap;
Ring sTestRing;
unsigned char sTestRingData[65536];
std::string sObserved;
RedLog::Observer sPrevObserver;

void __cdecl TestTap(char const* pcText)
{
    RiderLinkProtocol::Tap(sTestTap, sTestRing, pcText, (unsigned int)GetCurrentThreadId());
}

void __cdecl TestObserver(int iSeverity, int iCategory, char const* pcLine)
{
    sObserved = pcLine;
    if (sPrevObserver)
    {
        sPrevObserver(iSeverity, iCategory, pcLine);
    }
}

std::string ReadTestRing()
{
    std::vector<char> kOut(65536);
    const unsigned int uLen = RingRead(sTestRing, &kOut[0], (unsigned int)kOut.size(), "Idle");
    return std::string(&kOut[0], uLen);
}

}

TEST_CASE_NAMED(FRiderLinkTest1, "Unit::RiderLink::line assembler gives the same lines for any split", "[SmokeFilter]")
{
    const std::string sIn = "HELLO 1 abc\r\nPLAY x\n\nA\rB\nTAIL";
    const std::string sWant = "L:HELLO 1 abc|L:PLAY x|L:|L:A\rB|";
    CHECK(FeedWhole(sIn) == sWant);
    for (size_t a = 0; a <= sIn.size(); ++a)
    {
        for (size_t b = a; b <= sIn.size(); ++b)
        {
            LineAssembler kAsm;
            ResetAssembler(kAsm);
            std::vector<size_t> kCuts;
            kCuts.push_back(a);
            kCuts.push_back(b);
            INFO("cuts " << a << " " << b);
            CHECK(FeedAll(kAsm, sIn, kCuts) == sWant);
            CHECK(std::string(kAsm.acLine, kAsm.uLen) == "TAIL");
        }
    }
    LineAssembler kAsm;
    ResetAssembler(kAsm);
    std::vector<size_t> kEvery;
    for (size_t i = 1; i < sIn.size(); ++i)
    {
        kEvery.push_back(i);
    }
    CHECK(FeedAll(kAsm, sIn, kEvery) == sWant);
}

TEST_CASE_NAMED(FRiderLinkTest2, "Unit::RiderLink::line assembler discards a line of 256 bytes once and keeps binary inside the buffer", "[SmokeFilter]")
{
    const std::string s254(254, 'a');
    const std::string s255(255, 'b');
    CHECK(FeedWhole(s254 + "\nnext\n") == "L:" + s254 + "|L:next|");
    CHECK(FeedWhole(s255 + "\nnext\n") == "O|L:next|");
    CHECK(FeedWhole(std::string(253, 'c') + "\r\n") == "L:" + std::string(253, 'c') + "|");
    CHECK(FeedWhole(std::string(254, 'c') + "\r\n") == "O|");
    CHECK(FeedWhole(std::string(5000, 'x') + "\nok\n") == "O|L:ok|");
    std::string sBinary("\0\xff\x80z", 4);
    CHECK(FeedWhole(sBinary + "\n") == "L:" + sBinary + "|");
    LineAssembler kAsm;
    ResetAssembler(kAsm);
    const std::string sFlood(100000, '\xff');
    CHECK(FeedAll(kAsm, sFlood, std::vector<size_t>()) == "O|");
    CHECK(kAsm.bDiscard);
    CHECK(FeedAll(kAsm, "\nafter\n", std::vector<size_t>()) == "L:after|");
}

TEST_CASE_NAMED(FRiderLinkTest3, "Unit::RiderLink::hello parser accepts only version 1 with a 32 lowercase hex token", "[SmokeFilter]")
{
    std::string sToken;
    unsigned int uVersion = 99;
    CHECK(ParseText(std::string("HELLO 1 ") + kToken, sToken, uVersion) == MsgHello);
    CHECK(sToken == kToken);
    CHECK(uVersion == 1);
    CHECK(ParseText(std::string("HELLO 2 ") + kToken, sToken, uVersion) == MsgHelloVersion);
    CHECK(uVersion == 2);
    CHECK(sToken.empty());
    CHECK(ParseText("HELLO 1", sToken, uVersion) == MsgBadHello);
    CHECK(ParseText("HELLO 1 ", sToken, uVersion) == MsgBadHello);
    CHECK(ParseText("HELLO 1 short", sToken, uVersion) == MsgBadHello);
    CHECK(ParseText(std::string("HELLO 1 ") + kToken + "0", sToken, uVersion) == MsgBadHello);
    CHECK(ParseText(std::string("HELLO  1 ") + kToken, sToken, uVersion) == MsgBadHello);
    CHECK(ParseText("HELLO 1 0123456789ABCDEF0123456789abcdef", sToken, uVersion) == MsgBadHello);
    CHECK(ParseText(std::string("HELLO 1 ") + std::string("0123456789abcdef0123456789abcde\0", 32), sToken, uVersion) ==
          MsgBadHello);
    CHECK(ParseText("HELLO", sToken, uVersion) == MsgBadHello);
    CHECK(ParseText("HELLO x", sToken, uVersion) == MsgBadHello);
    CHECK(ParseText("HELLO 1234567890 x", sToken, uVersion) == MsgBadHello);
    CHECK(ParseText(std::string("HELLOX 1 ") + kToken, sToken, uVersion) == MsgUnknown);
    CHECK(ParseText(std::string("\xff\0", 2), sToken, uVersion) == MsgUnknown);
    CHECK(ParseText("", sToken, uVersion) == MsgEmpty);
    CHECK(std::string(PlatformWord()) == (sizeof(void*) == 8 ? "x64" : "Win32"));
    const std::string sConfig = ConfigWord();
    CHECK((sConfig == "Debug" || sConfig == "Test" || sConfig == "Asan"));
}

TEST_CASE_NAMED(FRiderLinkTest4, "Unit::RiderLink::token compare reads all 32 characters", "[SmokeFilter]")
{
    char acA[kTokenLen];
    char acB[kTokenLen];
    memcpy(acA, kToken, kTokenLen);
    memcpy(acB, kToken, kTokenLen);
    CHECK(TokenEqual(acA, acB));
    for (unsigned int i = 0; i < kTokenLen; ++i)
    {
        memcpy(acB, kToken, kTokenLen);
        acB[i] = acB[i] == 'f' ? 'e' : 'f';
        INFO("position " << i);
        CHECK(!TokenEqual(acA, acB));
    }
    memset(acA, 0, sizeof(acA));
    memset(acB, 0, sizeof(acB));
    acA[kTokenLen - 1] = 'a';
    acB[kTokenLen - 1] = 'b';
    CHECK(!TokenEqual(acA, acB));
    acB[kTokenLen - 1] = 'a';
    CHECK(TokenEqual(acA, acB));
}

TEST_CASE_NAMED(FRiderLinkTest5, "Unit::RiderLink::tap builder joins fragments of one thread and keeps other threads apart", "[SmokeFilter]")
{
    TestRing kOut(65536);
    TapBuilder kTap;
    ResetTap(kTap);
    Tap(kTap, kOut.kRing, "a", 7);
    Tap(kTap, kOut.kRing, "    ", 7);
    Tap(kTap, kOut.kRing, "b", 7);
    CHECK(kOut.Read().empty());
    Tap(kTap, kOut.kRing, "\n", 7);
    CHECK(kOut.Read() == "LOG a    b\n");

    Tap(kTap, kOut.kRing, "first", 1);
    Tap(kTap, kOut.kRing, "LogSim: Inform: whole\n", 2);
    CHECK(kOut.Read() == "LOG LogSim: Inform: whole\n");
    Tap(kTap, kOut.kRing, " half\n", 1);
    CHECK(kOut.Read() == "LOG first half\n");

    Tap(kTap, kOut.kRing, "one\ntwo\n\nthree", 3);
    CHECK(kOut.Read() == "LOG one\nLOG two\nLOG \n");
    Tap(kTap, kOut.kRing, "\n", 3);
    CHECK(kOut.Read() == "LOG three\n");

    Tap(kTap, kOut.kRing, (std::string(2000, 'x') + "\n").c_str(), 4);
    const std::string sLong = kOut.Read();
    CHECK(sLong.size() == kOutCap);
    CHECK(sLong == "LOG " + std::string(kOutCap - 5, 'x') + "\n");

    Tap(kTap, kOut.kRing, std::string(700, 'y').c_str(), 5);
    Tap(kTap, kOut.kRing, (std::string(700, 'z') + "\n").c_str(), 5);
    const std::string sJoined = kOut.Read();
    CHECK(sJoined.size() == kOutCap);
    CHECK(sJoined.back() == '\n');
}

TEST_CASE_NAMED(FRiderLinkTest6, "Unit::RiderLink::queue keeps order and drops whole oldest lines and reports them with the state", "[SmokeFilter]")
{
    TestRing kOut(4096);
    RingPush(kOut.kRing, "LOG ", "one", 3);
    RingPush(kOut.kRing, "", "STATE Play", 10);
    RingPush(kOut.kRing, "LOG ", "three", 5);
    CHECK(kOut.Read() == "LOG one\nSTATE Play\nLOG three\n");
    CHECK(kOut.Read().empty());
    CHECK(RingUsed(kOut.kRing) == 0);

    std::vector<std::string> kLines;
    for (int i = 0; i < 50; ++i)
    {
        char acBody[128];
        snprintf(acBody, sizeof(acBody), "%04d%s", i, std::string(96, (char)('a' + i % 26)).c_str());
        kLines.push_back(acBody);
        RingPush(kOut.kRing, "LOG ", acBody, (unsigned int)strlen(acBody));
    }
    const unsigned int uKept = 4096 / 105;
    const unsigned int uDropped = 50 - uKept;
    std::string sWant = "DROPPED " + std::to_string(uDropped) + "\nSTATE Pause\n";
    for (int i = (int)uDropped; i < 50; ++i)
    {
        sWant += "LOG " + kLines[i] + "\n";
    }
    CHECK(kOut.Read("Pause") == sWant);
    RingPush(kOut.kRing, "LOG ", "after", 5);
    CHECK(kOut.Read("Pause") == "LOG after\n");

    RingPush(kOut.kRing, "LOG ", "aaaa", 4);
    RingPush(kOut.kRing, "LOG ", "bbbb", 4);
    CHECK(kOut.Read("Play", 12) == "LOG aaaa\n");
    CHECK(kOut.Read("Play", 8) == "");
    CHECK(kOut.Read("Play", 9) == "LOG bbbb\n");

    RingPush(kOut.kRing, "LOG ", "a\nb", 3);
    CHECK(kOut.Read() == "LOG a b\n");
}

TEST_CASE_NAMED(FRiderLinkTest7, "Unit::RiderLink::start rule starts unless the run is norender or handsoff without riderlink", "[SmokeFilter]")
{
    CHECK(ShouldStart(false, false, false));
    CHECK(!ShouldStart(true, false, false));
    CHECK(!ShouldStart(false, true, false));
    CHECK(!ShouldStart(true, true, false));
    CHECK(ShouldStart(false, false, true));
    CHECK(ShouldStart(true, false, true));
    CHECK(ShouldStart(false, true, true));
    CHECK(ShouldStart(true, true, true));
}

TEST_CASE_NAMED(FRiderLinkTest8, "Unit::RiderLink::warning budget logs eight bad inputs and counts the rest", "[SmokeFilter]")
{
    WarnBudget kBudget;
    ResetBudget(kBudget);
    for (unsigned int i = 0; i < kWarnBudget; ++i)
    {
        CHECK(SpendWarning(kBudget));
    }
    for (int i = 0; i < 5; ++i)
    {
        CHECK(!SpendWarning(kBudget));
    }
    CHECK(kBudget.uSuppressed == 5);
    ResetBudget(kBudget);
    CHECK(SpendWarning(kBudget));
}

TEST_CASE_NAMED(FRiderLinkTest10, "Unit::RiderLink::log tap gets lua print as one line and a log line byte for byte", "[SmokeFilter]")
{
    ResetTap(sTestTap);
    RingInit(sTestRing, sTestRingData, sizeof(sTestRingData));
    lua_State* L = lua_open();
    REQUIRE(L);
    luaopen_base(L);
    lua_settop(L, 0);
    const char* pcChunk = "print(\"a\",\"b\")";
    REQUIRE(luaL_loadbuffer(L, pcChunk, strlen(pcChunk), "=tap") == 0);
    RedWarning::g_fnLineTap = TestTap;
    const int iCall = lua_pcall(L, 0, 0, 0);
    RedWarning::g_fnLineTap = 0;
    lua_close(L);
    CHECK(iCall == 0);
    CHECK(ReadTestRing() == "LOG a    b\n");

    sPrevObserver = RedLog::gObserver;
    RedLog::gObserver = TestObserver;
    RedWarning::g_fnLineTap = TestTap;
    LOG(LogSim, Inform, "riderlink tap marker %d", 7);
    RedWarning::g_fnLineTap = 0;
    RedLog::gObserver = sPrevObserver;
    REQUIRE(!sObserved.empty());
    CHECK(sObserved.find("LogSim: Inform: riderlink tap marker 7\n") != std::string::npos);
    CHECK(ReadTestRing() == "LOG " + sObserved);
}

TEST_CASE_NAMED(FRiderLinkTest11, "Unit::RiderLink::commands are accepted only as the whole line and play carries the mission", "[SmokeFilter]")
{
    std::string sMission;
    CHECK(ParseMission("PAUSE", sMission) == MsgPause);
    CHECK(ParseMission("RESUME", sMission) == MsgResume);
    CHECK(ParseMission("STEP", sMission) == MsgStep);
    CHECK(ParseMission("STOP", sMission) == MsgStop);
    CHECK(sMission.empty());
    CHECK(ParseMission("PLAY geo1c_con", sMission) == MsgPlay);
    CHECK(sMission == "geo1c_con");
    CHECK(ParseMission("PAUSE x", sMission) == MsgBadCommand);
    CHECK(ParseMission("PAUSE ", sMission) == MsgBadCommand);
    CHECK(ParseMission("STEP 2", sMission) == MsgBadCommand);
    CHECK(ParseMission("RESUME now", sMission) == MsgBadCommand);
    CHECK(ParseMission("STOP STOP", sMission) == MsgBadCommand);
    CHECK(ParseMission("pause", sMission) == MsgUnknown);
    CHECK(ParseMission("STOPX", sMission) == MsgUnknown);
    CHECK(ParseMission("Play geo1c_con", sMission) == MsgUnknown);
    CHECK(ParseMission(" PAUSE", sMission) == MsgUnknown);
    CHECK(ParseMission("PLAY", sMission) == MsgBadCommand);
    CHECK(ParseMission("PLAY ", sMission) == MsgBadCommand);
    CHECK(ParseMission("PLAY a b", sMission) == MsgBadCommand);
    CHECK(ParseMission("PLAY  geo1c_con", sMission) == MsgBadCommand);
    CHECK(ParseMission("PLAY ..\\x", sMission) == MsgBadCommand);
    CHECK(sMission.empty());
    CHECK(ParseMission(std::string("PAUSE\0", 6), sMission) == MsgUnknown);
    CHECK(ParseMission(std::string("PLAY a\0", 7), sMission) == MsgBadCommand);
    CHECK(ParseMission("PLAY " + std::string(63, 'm'), sMission) == MsgPlay);
    CHECK(sMission == std::string(63, 'm'));
    CHECK(ParseMission("PLAY " + std::string(64, 'm'), sMission) == MsgBadCommand);
    CHECK(sMission.empty());
}

TEST_CASE_NAMED(FRiderLinkTest12, "Unit::RiderLink::mission name is 1 to 63 letters digits or underscores", "[SmokeFilter]")
{
    CHECK(MissionOk("geo1c_con"));
    CHECK(MissionOk("A"));
    CHECK(MissionOk("_"));
    CHECK(MissionOk("Zz09_az"));
    CHECK(MissionOk(std::string(63, 'x')));
    CHECK(!MissionOk(""));
    CHECK(!MissionOk(std::string(64, 'x')));
    CHECK(!MissionOk(std::string(1000, 'x')));
    static const char* const apcBad[] = { "a\\b", "a/b", "a.b", "c:", "a%s", "a b", " a", "a ", "a-b", "a;b", "a\tb" };
    for (size_t i = 0; i < sizeof(apcBad) / sizeof(apcBad[0]); ++i)
    {
        INFO(apcBad[i]);
        CHECK(!MissionOk(apcBad[i]));
    }
    for (int c = 0x80; c <= 0xff; ++c)
    {
        INFO("byte " << c);
        CHECK(!MissionOk(std::string("geo") + (char)c));
    }
    CHECK(!MissionOk(std::string("geo\0", 4)));
}

TEST_CASE_NAMED(FRiderLinkTest13, "Unit::RiderLink::state table allows each command only in its states and names every refusal", "[SmokeFilter]")
{
    const Refusal N = RefuseNone;
    const Refusal W = RefuseWrongState;
    const Refusal X = RefuseNetwork;
    const Refusal H = RefuseGameHolds;
    static const struct
    {
        Message eCommand;
        LinkState eState;
        Refusal aeWant[4];
    } akRows[] = {
        { MsgPause, StateIdle, { W, W, X, X } },
        { MsgPause, StatePlay, { N, N, X, X } },
        { MsgPause, StatePause, { W, W, X, X } },
        { MsgResume, StateIdle, { W, W, X, X } },
        { MsgResume, StatePlay, { W, W, X, X } },
        { MsgResume, StatePause, { N, H, X, X } },
        { MsgStep, StateIdle, { W, W, X, X } },
        { MsgStep, StatePlay, { W, W, X, X } },
        { MsgStep, StatePause, { N, H, X, X } },
        { MsgStop, StateIdle, { W, W, W, W } },
        { MsgStop, StatePlay, { N, N, N, N } },
        { MsgStop, StatePause, { N, N, N, N } },
        { MsgPlay, StateIdle, { N, N, N, N } },
        { MsgPlay, StatePlay, { W, W, W, W } },
        { MsgPlay, StatePause, { W, W, W, W } },
    };
    CHECK(sizeof(akRows) / sizeof(akRows[0]) == 15);
    for (size_t r = 0; r < sizeof(akRows) / sizeof(akRows[0]); ++r)
    {
        for (int f = 0; f < 4; ++f)
        {
            const bool bNet = (f & 2) != 0;
            const bool bHolds = (f & 1) != 0;
            const std::string sCase = std::string(CommandWord(akRows[r].eCommand)) + " " +
                                      StateWord(akRows[r].eState) + (bNet ? " net" : "") + (bHolds ? " holds" : "");
            INFO(sCase);
            CHECK(Check(akRows[r].eCommand, akRows[r].eState, bNet, bHolds) == akRows[r].aeWant[f]);
        }
    }
    CHECK(std::string(RefusalText(W)) != RefusalText(X));
    CHECK(std::string(RefusalText(W)) != RefusalText(H));
    CHECK(std::string(RefusalText(X)) != RefusalText(H));
    CHECK(std::string(CommandWord(MsgPlay)) == "PLAY");
    CHECK(std::string(CommandWord(MsgPause)) == "PAUSE");
    CHECK(std::string(CommandWord(MsgResume)) == "RESUME");
    CHECK(std::string(CommandWord(MsgStep)) == "STEP");
    CHECK(std::string(CommandWord(MsgStop)) == "STOP");
    CHECK(std::string(StateWord(StateIdle)) == "Idle");
    CHECK(std::string(StateWord(StatePlay)) == "Play");
    CHECK(std::string(StateWord(StatePause)) == "Pause");
}

TEST_CASE_NAMED(FRiderLinkTest9, "Unit::RiderLink::fuzz assembler and parser", "[StressFilter][OnRequest]")
{
    const unsigned int uCount = gFuzzCount ? gFuzzCount : 100000;
    std::mt19937 rng(gFuzzSeed);
    static const char* const apcSeeds[] = { "HELLO 1 ", "HELLO ", "PLAY ", "\n", "\r\n", "0123456789abcdef",
                                            "PAUSE\n", "RESUME\n", "STEP\n", "STOP\n", "PLAY geo1c_con\n" };
    LineAssembler kAsm;
    ResetAssembler(kAsm);
    for (unsigned int uIter = 0; uIter < uCount; ++uIter)
    {
        std::string sIn;
        const unsigned int uParts = 1 + rng() % 16;
        for (unsigned int p = 0; p < uParts; ++p)
        {
            if (rng() % 2)
            {
                sIn += apcSeeds[rng() % (sizeof(apcSeeds) / sizeof(apcSeeds[0]))];
            }
            else
            {
                const unsigned int uLen = rng() % 400;
                for (unsigned int i = 0; i < uLen; ++i)
                {
                    sIn += (char)rng();
                }
            }
        }
        unsigned int uPos = 0;
        while (uPos < sIn.size())
        {
            const unsigned int uChunk = 1 + rng() % ((unsigned int)sIn.size() - uPos);
            const char* pcChunk = sIn.data() + uPos;
            unsigned int uAt = 0;
            for (;;)
            {
                const TakeResult eResult = Take(kAsm, pcChunk, uChunk, uAt);
                if (eResult == TakeNeedMore)
                {
                    break;
                }
                REQUIRE(kAsm.uLen < kInCap - 1);
                if (eResult == TakeLine)
                {
                    char acToken[kTokenLen + 1];
                    char acMission[kMissionCap];
                    unsigned int uVersion = 0;
                    const Message eMsg = Parse(kAsm.acLine, kAsm.uLen, acToken, uVersion, acMission);
                    REQUIRE((eMsg != MsgHello || strlen(acToken) == kTokenLen));
                    REQUIRE((eMsg != MsgPlay || MissionOk(acMission)));
                    if (eMsg >= MsgPlay && eMsg <= MsgStop)
                    {
                        const LinkState eState = (LinkState)(rng() % 3);
                        const bool bNet = (rng() & 1) != 0;
                        const bool bHolds = (rng() & 1) != 0;
                        const Refusal eRefusal = Check(eMsg, eState, bNet, bHolds);
                        REQUIRE(eRefusal >= RefuseNone);
                        REQUIRE(eRefusal <= RefuseGameHolds);
                        REQUIRE((!bNet || eMsg == MsgStop || eMsg == MsgPlay || eRefusal == RefuseNetwork));
                        REQUIRE(RefusalText(eRefusal)[0] != 0);
                    }
                }
            }
            uPos += uChunk;
        }
    }
    printf("riderlink-fuzz seed=%u count=%u\n", gFuzzSeed, uCount);
}

#endif
