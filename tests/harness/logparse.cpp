#include "harness/logparse.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "RedEngineFL/Util/RedLog.h"

static bool ShapeMatches(const char* pc, const char* pcShape)
{
    for (; *pcShape; ++pc, ++pcShape)
    {
        if (*pcShape == 'd' ? !isdigit((unsigned char)*pc) : *pcShape == 'f' ? !(isdigit((unsigned char)*pc) || *pc == ' ')
                                                                              : *pc != *pcShape)
        {
            return false;
        }
    }
    return true;
}

static const char* const kTimestampShape = "[dddd.dd.dd-dd.dd.dd:ddd]";
static const char* const kFrameShape = "[fff]";

bool ParseLogLine(const std::string& sLine, LogLine& kOut)
{
    kOut.sText = sLine.size() > kLogLineCap ? sLine.substr(0, kLogLineCap) : sLine;
    kOut.bParsed = false;
    kOut.sCategory.clear();
    kOut.iSeverity = -1;
    kOut.sMessage.clear();
    const char* pc = kOut.sText.c_str();
    if (ShapeMatches(pc, kTimestampShape))
    {
        pc += strlen(kTimestampShape);
    }
    if (ShapeMatches(pc, kFrameShape))
    {
        pc += strlen(kFrameShape);
    }
    const char* pcCategory = pc;
    while (isalnum((unsigned char)*pc) || *pc == '_')
    {
        ++pc;
    }
    if (pc != pcCategory && strncmp(pc, ": ", 2) == 0)
    {
        const char* pcSeverity = pc + 2;
        for (int i = RedLog::Verbose; i <= RedLog::Fatal; ++i)
        {
            const char* pcName = RedLog::SeverityName(i);
            const size_t uLen = strlen(pcName);
            if (strncmp(pcSeverity, pcName, uLen) == 0 && strncmp(pcSeverity + uLen, ": ", 2) == 0)
            {
                kOut.bParsed = true;
                kOut.sCategory.assign(pcCategory, pc - pcCategory);
                kOut.iSeverity = i;
                kOut.sMessage = pcSeverity + uLen + 2;
                return true;
            }
        }
    }
    const char* pcLegacy = "Message Severity: ";
    const size_t uLegacy = strlen(pcLegacy);
    if (kOut.sText.size() == uLegacy + 1 && kOut.sText.compare(0, uLegacy, pcLegacy) == 0 &&
        kOut.sText[uLegacy] >= '0' + RedLog::Verbose && kOut.sText[uLegacy] <= '0' + RedLog::Fatal)
    {
        kOut.iSeverity = kOut.sText[uLegacy] - '0';
    }
    return false;
}

bool ParseEndMarker(const std::string& sMessage, int& iCode)
{
    const char* pcHead = "**** TEST COMPLETE. EXIT CODE: ";
    const char* pcTail = " ****";
    const size_t uHead = strlen(pcHead);
    const size_t uTail = strlen(pcTail);
    if (sMessage.size() <= uHead + uTail || sMessage.compare(0, uHead, pcHead) != 0 ||
        sMessage.compare(sMessage.size() - uTail, uTail, pcTail) != 0)
    {
        return false;
    }
    const std::string sNumber = sMessage.substr(uHead, sMessage.size() - uHead - uTail);
    size_t i = sNumber[0] == '-' ? 1 : 0;
    if (i == sNumber.size() || sNumber.size() - i > 10)
    {
        return false;
    }
    for (; i < sNumber.size(); ++i)
    {
        if (!isdigit((unsigned char)sNumber[i]))
        {
            return false;
        }
    }
    const long long llCode = _strtoi64(sNumber.c_str(), 0, 10);
    if (llCode < -2147483647LL - 1 || llCode > 2147483647LL)
    {
        return false;
    }
    iCode = (int)llCode;
    return true;
}

void ParseLogText(const std::string& sText, ParsedLog& kLog)
{
    size_t uStart = 0;
    while (uStart < sText.size())
    {
        size_t uEnd = sText.find('\n', uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uStart, uEnd - uStart);
        uStart = uEnd + 1;
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        if (sLine.empty())
        {
            continue;
        }
        LogLine kLine;
        ParseLogLine(sLine, kLine);
        int iCode = 0;
        if (kLine.bParsed && ParseEndMarker(kLine.sMessage, iCode))
        {
            kLog.bHasEndMarker = true;
            kLog.iMarkerExitCode = iCode;
        }
        if (kLine.bParsed && kLine.sMessage.compare(0, 15, "ensure failed: ") == 0)
        {
            kLog.kEnsures.push_back(kLine.sText);
        }
        if (kLog.sFatalLine.empty() && kLine.sText.compare(0, 7, "FATAL: ") == 0)
        {
            kLog.sFatalLine = kLine.sText;
        }
        kLog.kLines.push_back(kLine);
    }
}
