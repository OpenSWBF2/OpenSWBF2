#include "harness/expect.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "testutil.h"

std::string LogMessageText(const char* pcLine)
{
    const char* pc = pcLine;
    for (int i = 0; i < 2 && *pc == '['; ++i)
    {
        const char* pcClose = strchr(pc, ']');
        if (!pcClose)
        {
            break;
        }
        pc = pcClose + 1;
    }
    const char* pcSeverity = strstr(pc, ": ");
    const char* pcBody = 0;
    for (int i = RedLog::Verbose; pcSeverity && !pcBody && i <= RedLog::Fatal; ++i)
    {
        const char* pcName = RedLog::SeverityName(i);
        const size_t uLen = strlen(pcName);
        if (strncmp(pcSeverity + 2, pcName, uLen) == 0 && strncmp(pcSeverity + 2 + uLen, ": ", 2) == 0)
        {
            pcBody = pcSeverity + 2 + uLen + 2;
        }
    }
    std::string sText = pcBody ? std::string(pcBody) : std::string(pcLine);
    while (!sText.empty() && (sText[sText.size() - 1] == '\n' || sText[sText.size() - 1] == '\r'))
    {
        sText.erase(sText.size() - 1);
    }
    return sText;
}

static bool ContainsNoCase(const std::string& sText, const std::string& sPattern)
{
    if (sPattern.size() > sText.size())
    {
        return false;
    }
    for (size_t i = 0; i + sPattern.size() <= sText.size(); ++i)
    {
        size_t j = 0;
        while (j < sPattern.size() && tolower((unsigned char)sText[i + j]) == tolower((unsigned char)sPattern[j]))
        {
            ++j;
        }
        if (j == sPattern.size())
        {
            return true;
        }
    }
    return false;
}

void ExpectedMessages::Add(const char* pcPattern, int iVerbosity, EAutomationExpectedMessageFlags::MatchType eCompareType,
                           int iOccurrences, bool bIsRegex)
{
    Entry kEntry;
    kEntry.sPattern = pcPattern ? pcPattern : "";
    kEntry.bRegex = bIsRegex;
    kEntry.eCompareType = eCompareType;
    kEntry.iVerbosity = iVerbosity;
    kEntry.iExpected = iOccurrences;
    kEntry.iActual = 0;
    if (bIsRegex)
    {
        const std::string sRegex =
            eCompareType == EAutomationExpectedMessageFlags::Exact ? "^" + kEntry.sPattern + "$" : kEntry.sPattern;
        try
        {
            kEntry.kRegex = std::regex(sRegex, std::regex::ECMAScript | std::regex::icase);
        }
        catch (const std::regex_error&)
        {
            HarnessError(("expected message pattern is not a valid regex: " + kEntry.sPattern).c_str());
        }
    }
    mEntries.push_back(kEntry);
}

bool ExpectedMessages::Matches(const Entry& kEntry, int iSeverity, const std::string& sMessage) const
{
    if (kEntry.iVerbosity >= RedLog::Warning && iSeverity < RedLog::Warning)
    {
        return false;
    }
    if (kEntry.bRegex)
    {
        return std::regex_search(sMessage, kEntry.kRegex);
    }
    return ContainsNoCase(sMessage, kEntry.sPattern) &&
           (kEntry.eCompareType != EAutomationExpectedMessageFlags::Exact || sMessage.size() == kEntry.sPattern.size());
}

bool ExpectedMessages::Observe(int iSeverity, const char* pcLine)
{
    if (!mEntries.empty())
    {
        const std::string sMessage = LogMessageText(pcLine);
        for (size_t i = 0; i < mEntries.size(); ++i)
        {
            if (Matches(mEntries[i], iSeverity, sMessage))
            {
                ++mEntries[i].iActual;
                return true;
            }
        }
    }
    if (iSeverity >= RedLog::Error)
    {
        mUndeclared.push_back(pcLine);
    }
    return false;
}

int ExpectedMessages::Report(const char* pcCase, std::string& sOut) const
{
    int iCount = 0;
    char acLine[2048];
    for (size_t i = 0; i < mUndeclared.size(); ++i)
    {
        sOut += std::string("[UNEXPECTED-LOG] ") + pcCase + ": " + mUndeclared[i];
        if (sOut.empty() || sOut[sOut.size() - 1] != '\n')
        {
            sOut += "\n";
        }
        ++iCount;
    }
    for (size_t i = 0; i < mEntries.size(); ++i)
    {
        const Entry& kEntry = mEntries[i];
        const char* pcVerbosity = kEntry.iVerbosity == kAnyVerbosity ? "Any" : RedLog::SeverityName(kEntry.iVerbosity);
        const char* pcType = kEntry.eCompareType == EAutomationExpectedMessageFlags::Exact ? "Exact" : "Contains";
        acLine[0] = 0;
        if (kEntry.iExpected > 0 && kEntry.iExpected != kEntry.iActual)
        {
            _snprintf(acLine, sizeof(acLine),
                      "[UNEXPECTED-LOG] %s: Expected ('%s') level log message or higher matching '%s' to occur %d times "
                      "with %s match type, but it was found %d time(s).\n",
                      pcCase, pcVerbosity, kEntry.sPattern.c_str(), kEntry.iExpected, pcType, kEntry.iActual);
        }
        else if (kEntry.iExpected == 0 && kEntry.iActual == 0)
        {
            _snprintf(acLine, sizeof(acLine),
                      "[UNEXPECTED-LOG] %s: Expected suppressed ('%s') level log message or higher matching '%s' did not "
                      "occur.\n",
                      pcCase, pcVerbosity, kEntry.sPattern.c_str());
        }
        acLine[sizeof(acLine) - 1] = 0;
        if (acLine[0])
        {
            sOut += acLine;
            ++iCount;
        }
    }
    return iCount;
}

void ExpectedMessages::Clear()
{
    mEntries.clear();
    mUndeclared.clear();
}

static ExpectedMessages sExpected;

void AddExpectedError(const char* pcPattern, EAutomationExpectedMessageFlags::MatchType eCompareType, int iOccurrences,
                      bool bIsRegex)
{
    sExpected.Add(pcPattern, RedLog::Warning, eCompareType, iOccurrences, bIsRegex);
}

void AddExpectedErrorPlain(const char* pcPattern, EAutomationExpectedMessageFlags::MatchType eCompareType,
                           int iOccurrences)
{
    sExpected.Add(pcPattern, RedLog::Warning, eCompareType, iOccurrences, false);
}

void AddExpectedMessage(const char* pcPattern, RedLog::Severity eVerbosity,
                        EAutomationExpectedMessageFlags::MatchType eCompareType, int iOccurrences, bool bIsRegex)
{
    sExpected.Add(pcPattern, eVerbosity, eCompareType, iOccurrences, bIsRegex);
}

void AddExpectedMessage(const char* pcPattern, EAutomationExpectedMessageFlags::MatchType eCompareType, int iOccurrences,
                        bool bIsRegex)
{
    sExpected.Add(pcPattern, ExpectedMessages::kAnyVerbosity, eCompareType, iOccurrences, bIsRegex);
}

void AddExpectedMessagePlain(const char* pcPattern, RedLog::Severity eVerbosity,
                             EAutomationExpectedMessageFlags::MatchType eCompareType, int iOccurrences)
{
    sExpected.Add(pcPattern, eVerbosity, eCompareType, iOccurrences, false);
}

void AddExpectedMessagePlain(const char* pcPattern, EAutomationExpectedMessageFlags::MatchType eCompareType,
                             int iOccurrences)
{
    sExpected.Add(pcPattern, ExpectedMessages::kAnyVerbosity, eCompareType, iOccurrences, false);
}

void __cdecl TestLogObserver(int iSeverity, int, char const* pcLine)
{
    sExpected.Observe(iSeverity, pcLine);
}

int TestStrictnessReport(const char* pcCase)
{
    std::string sText;
    const int iCount = sExpected.Report(pcCase, sText);
    fputs(sText.c_str(), stdout);
    fflush(stdout);
    return iCount;
}
