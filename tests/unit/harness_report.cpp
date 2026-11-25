#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>

#include "harness/report.h"

TEST_CASE_NAMED(FHarnessReportTest1, "Unit::HarnessReport::strings are escaped and bounded", "[SmokeFilter]")
{
    CHECK(JsonString("a\"b\\c\x01\n\t") == "\"a\\\"b\\\\c\\u0001\\u000A\\u0009\"");
    CHECK(JsonString("\xE9") == "\"\\u00E9\"");
    CHECK(JsonString(std::string(3000, 'z')) == "\"" + std::string(kReportLineCap, 'z') + "\"");
}

TEST_CASE_NAMED(FHarnessReportTest2, "Unit::HarnessReport::the report holds unreal's fields with every child string escaped",
                "[SmokeFilter]")
{
    Report kReport;
    kReport.sCreatedOn = "2026.11.25-01.02.03";
    kReport.fTotalDuration = 1.5;
    ReportTest kPass;
    kPass.sDisplayName = "one";
    kPass.sFullPath = "Unit::A::one";
    kPass.kTags.push_back("[SmokeFilter]");
    kPass.sState = "Success";
    kPass.fDuration = 0.25;
    kPass.sDateTime = "2026.11.25-01.02.03";
    ReportTest kFail = kPass;
    kFail.sDisplayName = "two";
    kFail.sFullPath = "Unit::A::two";
    kFail.sState = "Fail";
    ReportEntry kEntry = {"Error", "x(1): ERROR: CHECK( s == \"q\\\" ) \x01"};
    kFail.kEntries.push_back(kEntry);
    kFail.kArtifacts.push_back("C:\\out\\two\\output.txt");
    ReportTest kSkip = kPass;
    kSkip.sState = "NotRun";
    kReport.kTests.push_back(kPass);
    kReport.kTests.push_back(kFail);
    kReport.kTests.push_back(kSkip);

    char acTemp[MAX_PATH];
    REQUIRE(GetTempPathA(sizeof(acTemp), acTemp) > 0);
    char acPath[MAX_PATH];
    _snprintf(acPath, sizeof(acPath), "%sowb2_report_%lu.json", acTemp, GetCurrentProcessId());
    acPath[sizeof(acPath) - 1] = 0;
    REQUIRE(WriteReport(acPath, kReport));
    std::string sText;
    FILE* pFile = fopen(acPath, "rb");
    REQUIRE(pFile);
    char acBuf[4096];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    DeleteFileA(acPath);
    const char* const kExpected =
        "{\n"
        "  \"reportCreatedOn\": \"2026.11.25-01.02.03\",\n"
        "  \"succeeded\": 1,\n"
        "  \"succeededWithWarnings\": 0,\n"
        "  \"failed\": 1,\n"
        "  \"notRun\": 1,\n"
        "  \"totalDuration\": 1.500,\n"
        "  \"tests\": [\n"
        "    {\n"
        "      \"testDisplayName\": \"one\",\n"
        "      \"fullTestPath\": \"Unit::A::one\",\n"
        "      \"tags\": [\"[SmokeFilter]\"],\n"
        "      \"state\": \"Success\",\n"
        "      \"duration\": 0.250,\n"
        "      \"dateTime\": \"2026.11.25-01.02.03\",\n"
        "      \"entries\": [],\n"
        "      \"warnings\": 0,\n"
        "      \"errors\": 0,\n"
        "      \"artifacts\": []\n"
        "    },\n"
        "    {\n"
        "      \"testDisplayName\": \"two\",\n"
        "      \"fullTestPath\": \"Unit::A::two\",\n"
        "      \"tags\": [\"[SmokeFilter]\"],\n"
        "      \"state\": \"Fail\",\n"
        "      \"duration\": 0.250,\n"
        "      \"dateTime\": \"2026.11.25-01.02.03\",\n"
        "      \"entries\": [\n"
        "        { \"event\": { \"type\": \"Error\", \"message\": \"x(1): ERROR: CHECK( s == \\\"q\\\\\\\" ) \\u0001\" }, "
        "\"filename\": \"\", \"lineNumber\": 0, \"timestamp\": \"2026.11.25-01.02.03\" }\n"
        "      ],\n"
        "      \"warnings\": 0,\n"
        "      \"errors\": 1,\n"
        "      \"artifacts\": [\"C:\\\\out\\\\two\\\\output.txt\"]\n"
        "    },\n"
        "    {\n"
        "      \"testDisplayName\": \"one\",\n"
        "      \"fullTestPath\": \"Unit::A::one\",\n"
        "      \"tags\": [\"[SmokeFilter]\"],\n"
        "      \"state\": \"NotRun\",\n"
        "      \"duration\": 0.250,\n"
        "      \"dateTime\": \"2026.11.25-01.02.03\",\n"
        "      \"entries\": [],\n"
        "      \"warnings\": 0,\n"
        "      \"errors\": 0,\n"
        "      \"artifacts\": []\n"
        "    }\n"
        "  ]\n"
        "}\n";
    CHECK(sText == kExpected);
}
