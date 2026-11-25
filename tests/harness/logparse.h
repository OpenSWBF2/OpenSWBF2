#pragma once
#include <string>
#include <vector>

const size_t kLogLineCap = 1024;

struct LogLine
{
    std::string sText;
    bool bParsed = false;
    std::string sCategory;
    int iSeverity = -1;
    std::string sMessage;
};

struct ParsedLog
{
    std::vector<LogLine> kLines;
    std::vector<std::string> kEnsures;
    std::string sFatalLine;
    bool bHasEndMarker = false;
    int iMarkerExitCode = 0;
};

bool ParseLogLine(const std::string& sLine, LogLine& kOut);
bool ParseEndMarker(const std::string& sMessage, int& iCode);
void ParseLogText(const std::string& sText, ParsedLog& kLog);
