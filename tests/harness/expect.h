#pragma once
#include <regex>
#include <string>
#include <vector>

#include "RedEngineFL/Util/RedLog.h"

namespace EAutomationExpectedMessageFlags
{
enum MatchType
{
    Exact,
    Contains
};
}
namespace EAutomationExpectedErrorFlags = EAutomationExpectedMessageFlags;

class ExpectedMessages
{
public:
    enum
    {
        kAnyVerbosity = -1
    };

    void Add(const char* pcPattern, int iVerbosity, EAutomationExpectedMessageFlags::MatchType eCompareType,
             int iOccurrences, bool bIsRegex);
    bool Observe(int iSeverity, const char* pcLine);
    int Report(const char* pcCase, std::string& sOut) const;
    void Clear();
    const std::vector<std::string>& Undeclared() const { return mUndeclared; }

private:
    struct Entry
    {
        std::string sPattern;
        std::regex kRegex;
        bool bRegex;
        EAutomationExpectedMessageFlags::MatchType eCompareType;
        int iVerbosity;
        int iExpected;
        int iActual;
    };
    bool Matches(const Entry& kEntry, int iSeverity, const std::string& sMessage) const;

    std::vector<Entry> mEntries;
    std::vector<std::string> mUndeclared;
};

std::string LogMessageText(const char* pcLine);

void AddExpectedError(const char* pcPattern,
                      EAutomationExpectedMessageFlags::MatchType eCompareType = EAutomationExpectedMessageFlags::Contains,
                      int iOccurrences = 1, bool bIsRegex = true);
void AddExpectedErrorPlain(const char* pcPattern,
                           EAutomationExpectedMessageFlags::MatchType eCompareType =
                               EAutomationExpectedMessageFlags::Contains,
                           int iOccurrences = 1);
void AddExpectedMessage(const char* pcPattern, RedLog::Severity eVerbosity,
                        EAutomationExpectedMessageFlags::MatchType eCompareType =
                            EAutomationExpectedMessageFlags::Contains,
                        int iOccurrences = 1, bool bIsRegex = true);
void AddExpectedMessage(const char* pcPattern,
                        EAutomationExpectedMessageFlags::MatchType eCompareType =
                            EAutomationExpectedMessageFlags::Contains,
                        int iOccurrences = 1, bool bIsRegex = true);
void AddExpectedMessagePlain(const char* pcPattern, RedLog::Severity eVerbosity,
                             EAutomationExpectedMessageFlags::MatchType eCompareType =
                                 EAutomationExpectedMessageFlags::Contains,
                             int iOccurrences = 1);
void AddExpectedMessagePlain(const char* pcPattern,
                             EAutomationExpectedMessageFlags::MatchType eCompareType =
                                 EAutomationExpectedMessageFlags::Contains,
                             int iOccurrences = 1);

void __cdecl TestLogObserver(int iSeverity, int iCategory, char const* pcLine);
int TestStrictnessReport(const char* pcCase);
