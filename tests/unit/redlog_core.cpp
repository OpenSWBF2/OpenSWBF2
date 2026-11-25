#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string.h>

#include <regex>
#include <string>

#include "testutil.h"
#include "RedEngineFL/Util/RedLog.h"

namespace
{

char sLast[1100];
int sSeen;

void __cdecl Record(int iSeverity, int iCategory, char const* pcLine)
{
    ++sSeen;
    strncpy(sLast, pcLine, sizeof(sLast) - 1);
    sLast[sizeof(sLast) - 1] = 0;
    TestLogObserver(iSeverity, iCategory, pcLine);
}

struct Scope
{
    RedLog::Observer mOld;
    Scope()
    {
        RedLog::ResetForTest();
        sSeen = 0;
        sLast[0] = 0;
        mOld = RedLog::gObserver;
        RedLog::gObserver = Record;
    }
    ~Scope()
    {
        RedLog::gObserver = mOld;
        RedLog::ResetForTest();
    }
};

bool EnsureOnce(bool bValue)
{
    return ensure(bValue);
}

}

TEST_CASE_NAMED(FRedlogCoreTest1, "Unit::RedlogCore::redlog line format", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    RedLog::Write(RedLog::LogNet, RedLog::Warning, "src" "\\a\\b.cpp", 7, "x %d", 3);
    const std::regex kTimed("^\\[\\d{4}\\.\\d\\d\\.\\d\\d-\\d\\d\\.\\d\\d\\.\\d\\d:\\d{3}\\]\\[  0\\]LogNet: Warning: x 3\n$");
    CHECK(std::regex_match(std::string(sLast), kTimed));
    CHECK(!strstr(sLast, "b.cpp"));
    CHECK(capture.Contains("LogNet: Warning: x 3"));

    RedLog::gFrame = 7;
    RedLog::Write(RedLog::LogNet, RedLog::Warning, "x.cpp", 1, "y");
    CHECK(strstr(sLast, "][  7]LogNet: Warning: y\n") != nullptr);
    RedLog::gFrame = 12345;
    RedLog::Write(RedLog::LogNet, RedLog::Warning, "x.cpp", 1, "y");
    CHECK(strstr(sLast, "][345]LogNet: Warning: y\n") != nullptr);
    RedLog::gFrame = 1000;
    RedLog::Write(RedLog::LogNet, RedLog::Warning, "x.cpp", 1, "y");
    CHECK(strstr(sLast, "][  0]LogNet: Warning: y\n") != nullptr);
}

TEST_CASE_NAMED(FRedlogCoreTest11, "Unit::RedlogCore::redlog -NOLOGTIMES drops the time and frame fields", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    RedLog::gFrame = 42;
    RedLog::gNoLogTimes = true;
    RedLog::Write(RedLog::LogSim, RedLog::Inform, "x.cpp", 1, "z %d", 9);
    CHECK(strcmp(sLast, "LogSim: Inform: z 9\n") == 0);
    RedLog::gNoLogTimes = false;
    RedLog::Write(RedLog::LogSim, RedLog::Inform, "x.cpp", 1, "z");
    CHECK(sLast[0] == '[');
}

TEST_CASE_NAMED(FRedlogCoreTest2, "Unit::RedlogCore::redlog strips line breaks and terminates once", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    AddExpectedError("a  b cd e");
    RedLog::Write(RedLog::LogCore, RedLog::Error, "x.cpp", 1, "a\r\nb\nc%s", "d\re");
    size_t uLen = strlen(sLast);
    REQUIRE(uLen > 0);
    CHECK(sLast[uLen - 1] == '\n');
    CHECK(!strchr(sLast, '\r'));
    CHECK(strchr(sLast, '\n') == sLast + uLen - 1);
    CHECK(strstr(sLast, "a  b cd e") != nullptr);
}

TEST_CASE_NAMED(FRedlogCoreTest3, "Unit::RedlogCore::redlog caps line length", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    AddExpectedError("zzzz");
    char acBig[3000];
    memset(acBig, 'z', sizeof(acBig) - 1);
    acBig[sizeof(acBig) - 1] = 0;
    RedLog::Write(RedLog::LogCore, RedLog::Error, "x.cpp", 1, "%s", acBig);
    CHECK(strlen(sLast) <= 1023);
    CHECK(sLast[strlen(sLast) - 1] == '\n');
}

TEST_CASE_NAMED(FRedlogCoreTest4, "Unit::RedlogCore::redlog verbosity filters by category", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    RedLog::ParseCmds("all Warning, LogNet Verbose");
    RedLog::Write(RedLog::LogNet, RedLog::Verbose, "x.cpp", 1, "netv");
    CHECK(sSeen == 1);
    RedLog::Write(RedLog::LogSim, RedLog::Inform, "x.cpp", 1, "simi");
    CHECK(sSeen == 1);
    RedLog::Write(RedLog::LogSim, RedLog::Warning, "x.cpp", 1, "simw");
    CHECK(sSeen == 2);
    CHECK(RedLog::MinVerbosity() == RedLog::Verbose);
}

TEST_CASE_NAMED(FRedlogCoreTest5, "Unit::RedlogCore::redlog defaults to Inform and ignores bad commands", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    RedLog::ParseCmds("bogus Loud, Net");
    RedLog::Write(RedLog::LogNet, RedLog::Verbose, "x.cpp", 1, "v");
    CHECK(sSeen == 0);
    RedLog::Write(RedLog::LogNet, RedLog::Inform, "x.cpp", 1, "i");
    CHECK(sSeen == 1);
    CHECK(RedLog::MinVerbosity() == RedLog::Inform);
    RedWarning::SetLogFileName("NUL");
    RedLog::Apply("test");
    CHECK(capture.Contains("-logcmds ignored: bogus Loud, Net"));
    CHECK(capture.Contains("log start"));
}

TEST_CASE_NAMED(FRedlogCoreTest7, "Unit::RedlogCore::redlog ensure counts once per site", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    AddExpectedError("ensure failed: bValue");
    CHECK(EnsureOnce(true));
    CHECK(RedLog::EnsureFailureCount() == 0);
    CHECK(!EnsureOnce(false));
    CHECK(!EnsureOnce(false));
    CHECK(!EnsureOnce(false));
    CHECK(RedLog::EnsureFailureCount() == 3);
    CHECK(RedLog::EnsureSiteCount() == 1);
    CHECK(sSeen == 1);
    CHECK(capture.Contains("ensure failed: bValue"));
    RedLog::LogEnsureSummary();
    CHECK(capture.Contains("ensure summary: 3 failures at 1 sites"));
}

TEST_CASE_NAMED(FRedlogCoreTest8, "Unit::RedlogCore::redlog ensure evaluates its expression once", "[EngineFilter]")
{
    AddExpectedErrorPlain("ensure failed: ++i == 5");
    Scope scope;
    LogCapture capture;
    int i = 0;
    CHECK(ensure(++i == 1));
    CHECK(i == 1);
    CHECK(!ensure(++i == 5));
    CHECK(i == 2);
}

TEST_CASE_NAMED(FRedlogCoreTest9, "Unit::RedlogCore::redlog ensure table overflow logs once", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    AddExpectedError("ensure failed: e", EAutomationExpectedErrorFlags::Exact, 64);
    AddExpectedError("ensure table full");
    for (int i = 0; i < 65; ++i)
    {
        RedLog::EnsureFailed("e", "t.cpp", 100 + i);
    }
    CHECK(RedLog::EnsureSiteCount() == 64);
    CHECK(RedLog::EnsureFailureCount() == 65);
    CHECK(capture.Contains("ensure table full"));
    int iBefore = sSeen;
    RedLog::EnsureFailed("e", "t.cpp", 999);
    CHECK(sSeen == iBefore);
    CHECK(RedLog::EnsureFailureCount() == 66);
}

TEST_CASE_NAMED(FRedlogCoreTest10, "Unit::RedlogCore::redlog treats arguments as data", "[EngineFilter]")
{
    Scope scope;
    LogCapture capture;
    AddExpectedErrorPlain("name %n%s");
    LOG(LogCore, Error, "name %s", "%n%s");
    CHECK(strstr(sLast, "name %n%s\n") != nullptr);
    CHECK(capture.Contains("name %n%s"));
}
