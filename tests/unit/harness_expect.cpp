#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "harness/expect.h"
#include "harness/runner.h"
#include "RedEngineFL/Util/RedLog.h"

namespace
{

std::string Line(int iSeverity, const char* pcMessage)
{
    return std::string("[2026.11.25-01.02.03:004][  0]LogCore: ") + RedLog::SeverityName(iSeverity) + ": " + pcMessage + "\n";
}

void Observe(ExpectedMessages& k, int iSeverity, const char* pcMessage)
{
    k.Observe(iSeverity, Line(iSeverity, pcMessage).c_str());
}

int Failures(const ExpectedMessages& k, std::string& sText)
{
    sText.clear();
    return k.Report("case", sText);
}

int Failures(const ExpectedMessages& k)
{
    std::string sText;
    return k.Report("case", sText);
}

bool RunProbe(const char* pcKind, unsigned long& uCode, std::string& sText)
{
    char acSelf[MAX_PATH];
    const DWORD uSelf = GetModuleFileNameA(0, acSelf, sizeof(acSelf));
    char acTemp[MAX_PATH];
    if (uSelf == 0 || uSelf >= sizeof(acSelf) || GetTempPathA(sizeof(acTemp), acTemp) == 0)
    {
        return false;
    }
    char acOut[MAX_PATH];
    _snprintf(acOut, sizeof(acOut), "%sowb2_probe_%s_%lu.txt", acTemp, pcKind, GetCurrentProcessId());
    acOut[sizeof(acOut) - 1] = 0;
    ChildResult kChild;
    std::string sError;
    const std::string sCmd = QuoteArg(std::string(acSelf, uSelf)) + " " + QuoteArg(std::string("--expect-probe=") + pcKind);
    if (!RunChild(sCmd, acOut, "", kChild, sError))
    {
        return false;
    }
    uCode = kChild.uCode;
    FILE* pFile = fopen(acOut, "rb");
    if (pFile)
    {
        char acBuf[4096];
        size_t uRead;
        while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
        {
            sText.append(acBuf, uRead);
        }
        fclose(pFile);
    }
    DeleteFileA(acOut);
    return true;
}

}

int RunExpectProbe(const char* pcKind)
{
    if (strcmp(pcKind, "undeclared") == 0)
    {
        LOG(LogCore, Error, "probe undeclared");
    }
    else if (strcmp(pcKind, "unmet") == 0)
    {
        AddExpectedError("never logged");
    }
    else if (strcmp(pcKind, "met") == 0)
    {
        AddExpectedError("probe met");
        LOG(LogCore, Error, "probe met");
    }
    return 0;
}

TEST_CASE_NAMED(FHarnessExpectTest1, "Unit::HarnessExpect::an expectation met once too few fails", "[SmokeFilter]")
{
    ExpectedMessages k;
    k.Add("pool full", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, 2, true);
    Observe(k, RedLog::Error, "pool full");
    std::string sText;
    CHECK(Failures(k, sText) == 1);
    CHECK(sText ==
          "[UNEXPECTED-LOG] case: Expected ('Warning') level log message or higher matching 'pool full' to occur 2 times "
          "with Contains match type, but it was found 1 time(s).\n");
    Observe(k, RedLog::Error, "pool full");
    CHECK(Failures(k) == 0);
}

TEST_CASE_NAMED(FHarnessExpectTest2, "Unit::HarnessExpect::an expectation met once too many fails", "[SmokeFilter]")
{
    ExpectedMessages k;
    k.Add("pool full", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, 1, true);
    Observe(k, RedLog::Error, "pool full");
    CHECK(Failures(k) == 0);
    Observe(k, RedLog::Warning, "Pool Full again");
    std::string sText;
    CHECK(Failures(k, sText) == 1);
    CHECK(sText.find("but it was found 2 time(s).") != std::string::npos);
}

TEST_CASE_NAMED(FHarnessExpectTest3, "Unit::HarnessExpect::occurrences 0 needs at least one match", "[SmokeFilter]")
{
    ExpectedMessages k;
    k.Add("retry", ExpectedMessages::kAnyVerbosity, EAutomationExpectedMessageFlags::Contains, 0, true);
    std::string sText;
    CHECK(Failures(k, sText) == 1);
    CHECK(sText ==
          "[UNEXPECTED-LOG] case: Expected suppressed ('Any') level log message or higher matching 'retry' did not occur.\n");
    Observe(k, RedLog::Inform, "retry 1");
    Observe(k, RedLog::Inform, "retry 2");
    CHECK(Failures(k) == 0);
}

TEST_CASE_NAMED(FHarnessExpectTest4, "Unit::HarnessExpect::a negative count suppresses and never fails", "[SmokeFilter]")
{
    ExpectedMessages k;
    k.Add("noise", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, -1, true);
    CHECK(Failures(k) == 0);
    Observe(k, RedLog::Error, "noise");
    Observe(k, RedLog::Fatal, "noise");
    CHECK(Failures(k) == 0);
}

TEST_CASE_NAMED(FHarnessExpectTest5, "Unit::HarnessExpect::exact refuses a longer line", "[SmokeFilter]")
{
    ExpectedMessages kRegex;
    kRegex.Add("ensure failed: e", RedLog::Warning, EAutomationExpectedMessageFlags::Exact, 1, true);
    Observe(kRegex, RedLog::Error, "ensure failed: ex");
    CHECK(Failures(kRegex) == 2);
    ExpectedMessages kPlain;
    kPlain.Add("ensure failed: e", RedLog::Warning, EAutomationExpectedMessageFlags::Exact, 1, false);
    Observe(kPlain, RedLog::Error, "ensure failed: ex");
    CHECK(Failures(kPlain) == 2);
    ExpectedMessages kExact;
    kExact.Add("ensure failed: e", RedLog::Warning, EAutomationExpectedMessageFlags::Exact, 1, true);
    Observe(kExact, RedLog::Error, "Ensure Failed: E");
    CHECK(Failures(kExact) == 0);
}

TEST_CASE_NAMED(FHarnessExpectTest6, "Unit::HarnessExpect::a plain pattern is not read as a regex", "[SmokeFilter]")
{
    ExpectedMessages kPlain;
    kPlain.Add("++i == 5", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
    Observe(kPlain, RedLog::Error, "i == 5");
    CHECK(Failures(kPlain) == 2);
    ExpectedMessages kMatch;
    kMatch.Add("a.c", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
    Observe(kMatch, RedLog::Error, "abc");
    CHECK(Failures(kMatch) == 2);
    kMatch.Clear();
    kMatch.Add("a.c", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
    Observe(kMatch, RedLog::Error, "x a.c y");
    CHECK(Failures(kMatch) == 0);
}

TEST_CASE_NAMED(FHarnessExpectTest7, "Unit::HarnessExpect::an uncovered error line fails", "[SmokeFilter]")
{
    ExpectedMessages k;
    Observe(k, RedLog::Warning, "only a warning");
    Observe(k, RedLog::Inform, "only a note");
    CHECK(Failures(k) == 0);
    Observe(k, RedLog::Bug, "a bug");
    std::string sText;
    CHECK(Failures(k, sText) == 1);
    CHECK(sText == "[UNEXPECTED-LOG] case: " + Line(RedLog::Bug, "a bug"));
}

TEST_CASE_NAMED(FHarnessExpectTest8, "Unit::HarnessExpect::an expected error does not match a line below warning",
                "[SmokeFilter]")
{
    ExpectedMessages k;
    k.Add("pool", RedLog::Warning, EAutomationExpectedMessageFlags::Contains, 1, true);
    Observe(k, RedLog::Inform, "pool");
    CHECK(Failures(k) == 1);
    CHECK(LogMessageText(Line(RedLog::Error, "a: b").c_str()) == "a: b");
}

TEST_CASE_NAMED(FHarnessExpectTest10, "Unit::HarnessExpect::the message is the text after the category and severity",
                "[SmokeFilter]")
{
    CHECK(LogMessageText("[2026.11.25-01.02.03:004][ 17]LogSim: Warning: x: y\r\n") == "x: y");
    CHECK(LogMessageText("LogSim: Fatal: z\n") == "z");
    CHECK(LogMessageText("LogSim: Fatal: ") == "");
    CHECK(LogMessageText("LogSim: Loud: z") == "LogSim: Loud: z");
    CHECK(LogMessageText("[t=1.000] [f=1] LogSim Error x.cpp:1: z") == "[t=1.000] [f=1] LogSim Error x.cpp:1: z");
}

TEST_CASE_NAMED(FHarnessExpectTest9, "Unit::HarnessExpect::the tests exe fails a case on an unmet expectation or an uncovered error",
                "[SmokeFilter]")
{
    unsigned long uCode = 0;
    std::string sText;
    REQUIRE(RunProbe("undeclared", uCode, sText));
    CHECK(uCode == 1);
    CHECK(sText.find("[UNEXPECTED-LOG] undeclared: ") != std::string::npos);
    CHECK(sText.find("probe undeclared") != std::string::npos);
    sText.clear();
    REQUIRE(RunProbe("unmet", uCode, sText));
    CHECK(uCode == 1);
    CHECK(sText.find("matching 'never logged' to occur 1 times with Contains match type, but it was found 0 time(s).") !=
          std::string::npos);
    sText.clear();
    REQUIRE(RunProbe("met", uCode, sText));
    CHECK(uCode == 0);
    CHECK(sText.find("[UNEXPECTED-LOG]") == std::string::npos);
}
