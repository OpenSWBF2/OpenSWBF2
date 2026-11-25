#pragma once
#include <string>
#include <vector>

struct ReportEntry
{
    std::string sType;
    std::string sMessage;
};

struct ReportTest
{
    std::string sDisplayName;
    std::string sFullPath;
    std::vector<std::string> kTags;
    std::string sState;
    double fDuration;
    std::string sDateTime;
    std::vector<ReportEntry> kEntries;
    std::vector<std::string> kArtifacts;
};

struct Report
{
    std::string sCreatedOn;
    double fTotalDuration;
    std::vector<ReportTest> kTests;
};

const unsigned int kReportLineCap = 1024;

std::string JsonString(const std::string& sText);
bool WriteReport(const std::string& sPath, const Report& kReport);
