#pragma once
#include <string>
#include <vector>

struct RunnerCase
{
    std::string sName;
    std::string sTags;
    std::string sFile;
    std::string sTestName;
    std::string sParam;
};

typedef bool (*ComplexTestsFn)(std::vector<std::string>& kParams, std::string& sError);

struct ComplexTest
{
    const char* pcName;
    ComplexTestsFn pfnGetTests;
};

struct ComplexTestRegistrar
{
    ComplexTestRegistrar(const char* pcName, ComplexTestsFn pfnGetTests);
};

const std::vector<ComplexTest>& ComplexTests();
bool ExpandCases(const std::vector<RunnerCase>& kRegistered, const std::vector<ComplexTest>& kComplex,
                 std::vector<RunnerCase>& kOut, std::string& sError);
bool RegisteredCases(std::vector<RunnerCase>& kAll, std::string& sError);
bool ComplexNeeded(const RunnerCase& kParent, const std::string& sTagFilter, bool bGameDir, bool bData,
                   const std::string& sExcludeText);

struct ChildResult
{
    unsigned long uCode;
    unsigned long uPid;
    bool bStalled;
    bool bKilled;
    std::string sSummary;
    bool bAsanReport;
    std::string sAsanReport;
    bool bHasProgress;
    unsigned int uTurns;
    unsigned int uSteps;
};

struct ChildOptions
{
    std::string sWorkDir;
    bool bBelowNormal;
    void (*pfnTick)();
};

struct CanaryExpect
{
    std::string sKind;
    std::string sFile;
};

enum CaseVerdict
{
    kVerdictPass,
    kVerdictFail,
    kVerdictSkip,
    kVerdictStall,
    kVerdictCrash,
    kVerdictHarness
};

struct Judgement
{
    CaseVerdict eVerdict;
    std::string sDetail;
};

bool ParseTags(const std::string& sTags, std::vector<std::string>& kTags);
bool CheckCases(const std::vector<RunnerCase>& kCases, std::string& sError);
bool MatchesFilter(const std::string& sText, const std::string& sFilters);
bool IsGameRunCase(const RunnerCase& kCase);
bool IsSelected(const RunnerCase& kCase, const std::string& sTagFilter, const std::string& sNameFilter,
                bool bGameDir = false);
bool IsAfterAllCase(const RunnerCase& kCase);
bool AfterAllSelected(const RunnerCase& kCase, const std::string& sTagFilter, const std::string& sNameFilter);
void SelectRunCases(const std::vector<RunnerCase>& kAll, const std::string& sTagFilter, const std::string& sNameFilter,
                    bool bGameDir, bool bData, std::vector<RunnerCase>& kOrdinary, std::vector<RunnerCase>& kAfterAll);

bool RunnerArg(const char* pcArg);
const std::string& RunnerPlatform();
const std::string& RunnerConfig();
const std::string& RunnerOut();
const std::string& RunnerData();
const std::string& RunnerParam();
bool AsanReportMatchesCanary(const std::string& sReport, const CanaryExpect& kCanary, std::string& sWhy);
Judgement JudgeCase(const ChildResult& kChild, bool bAsanRun, const CanaryExpect* pCanary);
std::string QuoteArg(const std::string& sArg);
bool RunChild(const std::string& sCmdLine, const std::string& sOutFile, const std::string& sAsanOptions,
              ChildResult& kResult, std::string& sError, const ChildOptions* pOptions = 0);

int RunnerMain(int argc, char** argv);
