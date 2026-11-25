#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "harness/logparse.h"
#include "RedEngineFL/Util/RedLog.h"

TEST_CASE_NAMED(FHarnessLogparseTest1, "Unit::HarnessLogparse::both line forms parse and a line without a category does not",
                "[SmokeFilter]")
{
    LogLine k;
    REQUIRE(ParseLogLine("[2026.11.25-00.00.00:000][  7]LogSim: Warning: pool full", k));
    CHECK(k.sCategory == "LogSim");
    CHECK(k.iSeverity == RedLog::Warning);
    CHECK(k.sMessage == "pool full");
    REQUIRE(ParseLogLine("LogMemory: Error: alloc failure heap Runtime (3) size 268435456", k));
    CHECK(k.sCategory == "LogMemory");
    CHECK(k.iSeverity == RedLog::Error);
    CHECK(k.sMessage == "alloc failure heap Runtime (3) size 268435456");
    CHECK(!ParseLogLine("[2026.11.25-00.00.00:000][  7]: Error: no category", k));
    CHECK(!ParseLogLine("Error: no category", k));
    CHECK(!ParseLogLine("LogSim: Loud: unknown severity", k));
    CHECK(!ParseLogLine("[t=1.000] [f=7] LogSim Warning x.cpp:1: old layout", k));
    CHECK(!ParseLogLine("[2026.11.25-00.00.00:0x0][  7]LogSim: Warning: bad timestamp", k));
    CHECK(!ParseLogLine("FATAL: exception 0xC0000005 at 0x00401000 (BattlefrontII.exe+0x1000)", k));
    CHECK(k.iSeverity == -1);
}

TEST_CASE_NAMED(FHarnessLogparseTest2, "Unit::HarnessLogparse::a line is cut at 1024 bytes", "[SmokeFilter]")
{
    LogLine k;
    const std::string sLong = "LogCore: Error: " + std::string(3000, 'x');
    REQUIRE(ParseLogLine(sLong, k));
    CHECK(k.sText.size() == kLogLineCap);
    CHECK(k.sMessage.size() == kLogLineCap - 16);
    ParsedLog kLog;
    ParseLogText(sLong + "\r\nLogCore: Inform: next\r\n", kLog);
    REQUIRE(kLog.kLines.size() == 2);
    CHECK(kLog.kLines[0].sText.size() == kLogLineCap);
    CHECK(kLog.kLines[1].sMessage == "next");
}

TEST_CASE_NAMED(FHarnessLogparseTest3, "Unit::HarnessLogparse::the end marker needs a numeric code", "[SmokeFilter]")
{
    int iCode = 7;
    CHECK(ParseEndMarker("**** TEST COMPLETE. EXIT CODE: 0 ****", iCode));
    CHECK(iCode == 0);
    CHECK(ParseEndMarker("**** TEST COMPLETE. EXIT CODE: -1 ****", iCode));
    CHECK(iCode == -1);
    iCode = 7;
    CHECK(!ParseEndMarker("**** TEST COMPLETE. EXIT CODE: x ****", iCode));
    CHECK(!ParseEndMarker("**** TEST COMPLETE. EXIT CODE: - ****", iCode));
    CHECK(!ParseEndMarker("**** TEST COMPLETE. EXIT CODE:  ****", iCode));
    CHECK(!ParseEndMarker("**** TEST COMPLETE. EXIT CODE: 1x ****", iCode));
    CHECK(!ParseEndMarker("**** TEST COMPLETE. EXIT CODE: 99999999999 ****", iCode));
    CHECK(!ParseEndMarker("**** TEST COMPLETE. EXIT CODE: 0 ***", iCode));
    CHECK(iCode == 7);
    ParsedLog kLog;
    ParseLogText("[2026.11.25-00.00.00:000][  0]LogCore: Inform: **** TEST COMPLETE. EXIT CODE: zero ****\n", kLog);
    CHECK(!kLog.bHasEndMarker);
    ParseLogText("LogCore: Inform: **** TEST COMPLETE. EXIT CODE: -1 ****\n", kLog);
    CHECK(kLog.bHasEndMarker);
    CHECK(kLog.iMarkerExitCode == -1);
}

TEST_CASE_NAMED(FHarnessLogparseTest4, "Unit::HarnessLogparse::ensures and the fatal line and legacy severity lines are found",
                "[SmokeFilter]")
{
    ParsedLog kLog;
    ParseLogText("[2026.11.25-00.00.00:000][  1]LogCore: Error: ensure failed: false\r\n"
                 "LogCore: Error: ensure failed: pPool\r\n"
                 "LogCore: Inform: an ensure failed: inside the text\r\n"
                 "FATAL: initialization failed, exiting with code 1\r\n"
                 "FATAL: second\r\n"
                 "Message Severity: 3\r\n"
                 "Message Severity: 9\r\n",
                 kLog);
    REQUIRE(kLog.kEnsures.size() == 2);
    CHECK(kLog.kEnsures[0] == "[2026.11.25-00.00.00:000][  1]LogCore: Error: ensure failed: false");
    CHECK(kLog.sFatalLine == "FATAL: initialization failed, exiting with code 1");
    REQUIRE(kLog.kLines.size() == 7);
    CHECK(kLog.kLines[5].iSeverity == RedLog::Error);
    CHECK(!kLog.kLines[5].bParsed);
    CHECK(kLog.kLines[6].iSeverity == -1);
    CHECK(!kLog.bHasEndMarker);
}
