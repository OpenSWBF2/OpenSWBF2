#include "harness/report.h"

#include <stdio.h>

std::string JsonString(const std::string& sText)
{
    const size_t uLen = sText.size() < kReportLineCap ? sText.size() : kReportLineCap;
    std::string sOut = "\"";
    for (size_t i = 0; i < uLen; ++i)
    {
        const unsigned char c = (unsigned char)sText[i];
        if (c == '"')
        {
            sOut += "\\\"";
        }
        else if (c == '\\')
        {
            sOut += "\\\\";
        }
        else if (c < 0x20 || c >= 0x7F)
        {
            char acEsc[8];
            _snprintf(acEsc, sizeof(acEsc), "\\u%04X", (unsigned int)c);
            acEsc[sizeof(acEsc) - 1] = 0;
            sOut += acEsc;
        }
        else
        {
            sOut += (char)c;
        }
    }
    sOut += "\"";
    return sOut;
}

static int CountEntries(const ReportTest& kTest, const char* pcType)
{
    int iCount = 0;
    for (size_t i = 0; i < kTest.kEntries.size(); ++i)
    {
        if (kTest.kEntries[i].sType == pcType)
        {
            ++iCount;
        }
    }
    return iCount;
}

bool WriteReport(const std::string& sPath, const Report& kReport)
{
    int iSucceeded = 0;
    int iWithWarnings = 0;
    int iFailed = 0;
    int iNotRun = 0;
    for (size_t i = 0; i < kReport.kTests.size(); ++i)
    {
        const ReportTest& kTest = kReport.kTests[i];
        if (kTest.sState == "Success")
        {
            if (CountEntries(kTest, "Warning") > 0)
            {
                ++iWithWarnings;
            }
            else
            {
                ++iSucceeded;
            }
        }
        else if (kTest.sState == "Fail")
        {
            ++iFailed;
        }
        else
        {
            ++iNotRun;
        }
    }
    FILE* pFile = fopen(sPath.c_str(), "wb");
    if (!pFile)
    {
        return false;
    }
    fprintf(pFile, "{\n  \"reportCreatedOn\": %s,\n", JsonString(kReport.sCreatedOn).c_str());
    fprintf(pFile, "  \"succeeded\": %d,\n  \"succeededWithWarnings\": %d,\n  \"failed\": %d,\n  \"notRun\": %d,\n",
            iSucceeded, iWithWarnings, iFailed, iNotRun);
    fprintf(pFile, "  \"totalDuration\": %.3f,\n  \"tests\": [", kReport.fTotalDuration);
    for (size_t i = 0; i < kReport.kTests.size(); ++i)
    {
        const ReportTest& kTest = kReport.kTests[i];
        fprintf(pFile, "%s\n    {\n", i ? "," : "");
        fprintf(pFile, "      \"testDisplayName\": %s,\n", JsonString(kTest.sDisplayName).c_str());
        fprintf(pFile, "      \"fullTestPath\": %s,\n", JsonString(kTest.sFullPath).c_str());
        fprintf(pFile, "      \"tags\": [");
        for (size_t j = 0; j < kTest.kTags.size(); ++j)
        {
            fprintf(pFile, "%s%s", j ? ", " : "", JsonString(kTest.kTags[j]).c_str());
        }
        fprintf(pFile, "],\n      \"state\": %s,\n", JsonString(kTest.sState).c_str());
        fprintf(pFile, "      \"duration\": %.3f,\n", kTest.fDuration);
        fprintf(pFile, "      \"dateTime\": %s,\n", JsonString(kTest.sDateTime).c_str());
        fprintf(pFile, "      \"entries\": [");
        for (size_t j = 0; j < kTest.kEntries.size(); ++j)
        {
            const ReportEntry& kEntry = kTest.kEntries[j];
            fprintf(pFile,
                    "%s\n        { \"event\": { \"type\": %s, \"message\": %s }, \"filename\": \"\", \"lineNumber\": 0, "
                    "\"timestamp\": %s }",
                    j ? "," : "", JsonString(kEntry.sType).c_str(), JsonString(kEntry.sMessage).c_str(),
                    JsonString(kTest.sDateTime).c_str());
        }
        fprintf(pFile, "%s],\n", kTest.kEntries.empty() ? "" : "\n      ");
        fprintf(pFile, "      \"warnings\": %d,\n      \"errors\": %d,\n", CountEntries(kTest, "Warning"),
                CountEntries(kTest, "Error"));
        fprintf(pFile, "      \"artifacts\": [");
        for (size_t j = 0; j < kTest.kArtifacts.size(); ++j)
        {
            fprintf(pFile, "%s%s", j ? ", " : "", JsonString(kTest.kArtifacts[j]).c_str());
        }
        fprintf(pFile, "]\n    }");
    }
    fprintf(pFile, "%s]\n}\n", kReport.kTests.empty() ? "" : "\n  ");
    const bool bOk = ferror(pFile) == 0;
    return fclose(pFile) == 0 && bOk;
}
