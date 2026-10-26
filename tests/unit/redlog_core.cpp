#include "third_party/doctest/doctest.h"

#include <string.h>

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

TEST_CASE("redlog line format" * doctest::test_suite("regression"))
{
    Scope scope;
    LogCapture capture;
    RedLog::Write(RedLog::LogNet, RedLog::Warning, "src\\a\\b.cpp", 7, "x %d", 3);
    CHECK(strncmp(sLast, "[t=", 3) == 0);
    CHECK(strstr(sLast, "] [f=0] LogNet Warning a\\b.cpp:7: x 3\n") != nullptr);
    CHECK(!strstr(sLast, "src\\"));
    CHECK(capture.Contains("LogNet Warning a\\b.cpp:7: x 3"));
}

TEST_CASE("redlog strips line breaks and terminates once" * doctest::test_suite("regression"))
{
    Scope scope;
    LogCapture capture;
    ExpectLog expect("a  b cd e");
    RedLog::Write(RedLog::LogCore, RedLog::Error, "x.cpp", 1, "a\r\nb\nc%s", "d\re");
    size_t uLen = strlen(sLast);
    REQUIRE(uLen > 0);
    CHECK(sLast[uLen - 1] == '\n');
    CHECK(!strchr(sLast, '\r'));
    CHECK(strchr(sLast, '\n') == sLast + uLen - 1);
    CHECK(strstr(sLast, "a  b cd e") != nullptr);
}

TEST_CASE("redlog caps line length" * doctest::test_suite("regression"))
{
    Scope scope;
    LogCapture capture;
    ExpectLog expect("zzzz");
    char acBig[3000];
    memset(acBig, 'z', sizeof(acBig) - 1);
    acBig[sizeof(acBig) - 1] = 0;
    RedLog::Write(RedLog::LogCore, RedLog::Error, "x.cpp", 1, "%s", acBig);
    CHECK(strlen(sLast) <= 1023);
    CHECK(sLast[strlen(sLast) - 1] == '\n');
}

TEST_CASE("redlog verbosity filters by category" * doctest::test_suite("regression"))
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

TEST_CASE("redlog defaults to Inform and ignores bad commands" * doctest::test_suite("regression"))
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

TEST_CASE("redlog trims source paths" * doctest::test_suite("regression"))
{
    CHECK(strcmp(RedLog::TrimPath("unit\\x.cpp"), "unit\\x.cpp") == 0);
    CHECK(strcmp(RedLog::TrimPath("..\\src\\A\\b.cpp"), "A\\b.cpp") == 0);
    CHECK(strcmp(RedLog::TrimPath("C:\\r\\src\\A\\b.cpp"), "A\\b.cpp") == 0);
}

TEST_CASE("redlog ensure counts once per site" * doctest::test_suite("regression"))
{
    Scope scope;
    LogCapture capture;
    ExpectEnsure expect("bValue");
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

TEST_CASE("redlog ensure evaluates its expression once" * doctest::test_suite("regression"))
{
    ExpectLog expect("ensure failed: ++i == 5");
    Scope scope;
    LogCapture capture;
    int i = 0;
    CHECK(ensure(++i == 1));
    CHECK(i == 1);
    CHECK(!ensure(++i == 5));
    CHECK(i == 2);
}

TEST_CASE("redlog ensure table overflow logs once" * doctest::test_suite("regression"))
{
    Scope scope;
    LogCapture capture;
    ExpectEnsure expect("e");
    ExpectLog expectFull("ensure table full");
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

TEST_CASE("redlog treats arguments as data" * doctest::test_suite("regression"))
{
    Scope scope;
    LogCapture capture;
    ExpectLog expect("name %n%s");
    LOG(LogCore, Error, "name %s", "%n%s");
    CHECK(strstr(sLast, "name %n%s\n") != nullptr);
    CHECK(capture.Contains("name %n%s"));
}

TEST_CASE("strictness probe emits an undeclared error" * doctest::skip())
{
    LOG(LogCore, Error, "probe undeclared");
}

TEST_CASE("strictness probe unmatched expectation" * doctest::skip())
{
    ExpectLog expect("never logged");
}
