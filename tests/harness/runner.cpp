#include <windows.h>
#include <winternl.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <atomic>
#include <mutex>
#include <set>
#include <thread>

#include "third_party/doctest/doctest.h"
#include "harness/excludelist.h"
#include "harness/report.h"
#include "harness/runner.h"
#include "harness/stallclock.h"
#include "Battlefront2/Platform/Pc/pcProgress.h"

namespace doctest
{
namespace detail
{
std::set<TestCase>& getRegisteredTests();
int wildcmp(const char* str, const char* wild, bool caseSensitive);
}
}

static const char* const sapcFilterTags[] = {"[SmokeFilter]",  "[EngineFilter]", "[ProductFilter]",
                                             "[PerfFilter]",   "[StressFilter]", "[NegativeFilter]"};

static std::string Lower(const std::string& s)
{
    std::string sOut = s;
    for (size_t i = 0; i < sOut.size(); ++i)
    {
        sOut[i] = (char)tolower((unsigned char)sOut[i]);
    }
    return sOut;
}

static bool HasTag(const std::vector<std::string>& kTags, const char* pcTag)
{
    for (size_t i = 0; i < kTags.size(); ++i)
    {
        if (kTags[i] == pcTag)
        {
            return true;
        }
    }
    return false;
}

bool ParseTags(const std::string& sTags, std::vector<std::string>& kTags)
{
    kTags.clear();
    size_t i = 0;
    while (i < sTags.size())
    {
        if (sTags[i] != '[')
        {
            return false;
        }
        const size_t uEnd = sTags.find(']', i);
        if (uEnd == std::string::npos || uEnd == i + 1 || sTags.find('[', i + 1) < uEnd)
        {
            return false;
        }
        kTags.push_back(sTags.substr(i, uEnd - i + 1));
        i = uEnd + 1;
    }
    return !kTags.empty();
}

bool CheckCases(const std::vector<RunnerCase>& kCases, std::string& sError)
{
    std::set<std::string> kSeen;
    for (size_t i = 0; i < kCases.size(); ++i)
    {
        const RunnerCase& kCase = kCases[i];
        if (kCase.sName.empty() || kCase.sName.find_first_of(",*?\"") != std::string::npos)
        {
            sError = "case name '" + kCase.sName + "' is empty or holds a comma, '*', '?' or a quote";
            return false;
        }
        if (!kSeen.insert(Lower(kCase.sName)).second)
        {
            sError = "two cases share the name '" + kCase.sName + "'";
            return false;
        }
        std::vector<std::string> kTags;
        if (!ParseTags(kCase.sTags, kTags))
        {
            sError = "case '" + kCase.sName + "' has malformed tags '" + kCase.sTags + "'";
            return false;
        }
        int iFilters = 0;
        for (size_t j = 0; j < sizeof(sapcFilterTags) / sizeof(sapcFilterTags[0]); ++j)
        {
            iFilters += HasTag(kTags, sapcFilterTags[j]) ? 1 : 0;
        }
        if (iFilters != 1)
        {
            sError = "case '" + kCase.sName + "' needs exactly one filter tag, has '" + kCase.sTags + "'";
            return false;
        }
    }
    return true;
}

static std::vector<ComplexTest>& ComplexRegistry()
{
    static std::vector<ComplexTest> sRegistry;
    return sRegistry;
}

ComplexTestRegistrar::ComplexTestRegistrar(const char* pcName, ComplexTestsFn pfnGetTests)
{
    ComplexTest k = {pcName, pfnGetTests};
    ComplexRegistry().push_back(k);
}

const std::vector<ComplexTest>& ComplexTests()
{
    return ComplexRegistry();
}

static bool IsChildParam(const std::string& sParam)
{
    if (sParam.empty())
    {
        return false;
    }
    for (size_t i = 0; i < sParam.size(); ++i)
    {
        if (!isalnum((unsigned char)sParam[i]) && sParam[i] != '_')
        {
            return false;
        }
    }
    return true;
}

bool ExpandCases(const std::vector<RunnerCase>& kRegistered, const std::vector<ComplexTest>& kComplex,
                 std::vector<RunnerCase>& kOut, std::string& sError)
{
    kOut.clear();
    for (size_t i = 0; i < kComplex.size(); ++i)
    {
        bool bFound = false;
        for (size_t j = 0; j < kRegistered.size() && !bFound; ++j)
        {
            bFound = kRegistered[j].sName == kComplex[i].pcName;
        }
        if (!bFound)
        {
            sError = std::string("complex test '") + kComplex[i].pcName + "' has no test case";
            return false;
        }
    }
    for (size_t i = 0; i < kRegistered.size(); ++i)
    {
        const RunnerCase& kCase = kRegistered[i];
        const ComplexTest* pComplex = 0;
        for (size_t j = 0; j < kComplex.size() && !pComplex; ++j)
        {
            if (kCase.sName == kComplex[j].pcName)
            {
                pComplex = &kComplex[j];
            }
        }
        if (!pComplex)
        {
            RunnerCase kPlain = kCase;
            kPlain.sTestName = kCase.sName;
            kPlain.sParam.clear();
            kOut.push_back(kPlain);
            continue;
        }
        std::vector<std::string> kParams;
        std::string sWhy;
        if (!pComplex->pfnGetTests(kParams, sWhy))
        {
            sError = kCase.sName + ": " + sWhy;
            return false;
        }
        if (kParams.empty())
        {
            sError = kCase.sName + ": the complex test lists no child";
            return false;
        }
        for (size_t j = 0; j < kParams.size(); ++j)
        {
            if (!IsChildParam(kParams[j]))
            {
                sError = kCase.sName + ": child '" + kParams[j] + "' is not a name of letters, digits and '_'";
                return false;
            }
            RunnerCase kChild = kCase;
            kChild.sName = kCase.sName + "::" + kParams[j];
            kChild.sTestName = kCase.sName;
            kChild.sParam = kParams[j];
            kOut.push_back(kChild);
        }
    }
    return true;
}

bool MatchesFilter(const std::string& sText, const std::string& sFilters)
{
    if (sFilters.empty())
    {
        return true;
    }
    size_t uStart = 0;
    for (;;)
    {
        const size_t uComma = sFilters.find(',', uStart);
        const std::string sOne = sFilters.substr(uStart, uComma == std::string::npos ? std::string::npos : uComma - uStart);
        if (!sOne.empty() && doctest::detail::wildcmp(sText.c_str(), sOne.c_str(), false))
        {
            return true;
        }
        if (uComma == std::string::npos)
        {
            return false;
        }
        uStart = uComma + 1;
    }
}

static const char* const sapcGameGroupTags[] = {"[Standard]", "[Boot]",      "[Vendor]",  "[Crash]",
                                                 "[Dedicated]", "[All]",     "[Twice]",   "[Perturb]",
                                                 "[Soak]",     "[SaveCross]", "[Capture]", "[Laa]"};

bool IsGameRunCase(const RunnerCase& kCase)
{
    std::vector<std::string> kTags;
    ParseTags(kCase.sTags, kTags);
    for (size_t i = 0; i < sizeof(sapcGameGroupTags) / sizeof(sapcGameGroupTags[0]); ++i)
    {
        if (HasTag(kTags, sapcGameGroupTags[i]))
        {
            return true;
        }
    }
    return false;
}

bool IsSelected(const RunnerCase& kCase, const std::string& sTagFilter, const std::string& sNameFilter, bool bGameDir)
{
    std::vector<std::string> kTags;
    ParseTags(kCase.sTags, kTags);
    if (HasTag(kTags, "[OnRequest]") && Lower(sTagFilter).find("[onrequest]") == std::string::npos)
    {
        return false;
    }
    if (IsGameRunCase(kCase) != bGameDir)
    {
        return false;
    }
    return MatchesFilter(kCase.sTags, sTagFilter) && MatchesFilter(kCase.sName, sNameFilter);
}

static std::string CanaryKind(const std::vector<std::string>& kTags)
{
    const std::string sPrefix = "[AsanError=";
    for (size_t i = 0; i < kTags.size(); ++i)
    {
        if (kTags[i].compare(0, sPrefix.size(), sPrefix) == 0)
        {
            return kTags[i].substr(sPrefix.size(), kTags[i].size() - sPrefix.size() - 1);
        }
    }
    return "";
}

static std::string BaseName(const std::string& sPath)
{
    const size_t uSlash = sPath.find_last_of("\\/");
    return uSlash == std::string::npos ? sPath : sPath.substr(uSlash + 1);
}

bool AsanReportMatchesCanary(const std::string& sReport, const CanaryExpect& kCanary, std::string& sWhy)
{
    const std::string sError = "ERROR: AddressSanitizer: ";
    const size_t uError = sReport.find(sError);
    if (uError == std::string::npos)
    {
        sWhy = "the Asan report names no error";
        return false;
    }
    const size_t uKind = uError + sError.size();
    const size_t uKindEnd = sReport.find_first_of(" \r\n", uKind);
    const std::string sKind = sReport.substr(uKind, uKindEnd == std::string::npos ? std::string::npos : uKindEnd - uKind);
    if (kCanary.sKind.empty() || sKind != kCanary.sKind)
    {
        sWhy = "the Asan report is " + sKind + ", the canary is written to trigger " +
               (kCanary.sKind.empty() ? std::string("no declared kind") : kCanary.sKind);
        return false;
    }
    size_t uFrame = sReport.find("#0 ", uKind);
    while (uFrame != std::string::npos && uFrame > 0 && sReport[uFrame - 1] != ' ' && sReport[uFrame - 1] != '\n')
    {
        uFrame = sReport.find("#0 ", uFrame + 1);
    }
    if (uFrame == std::string::npos)
    {
        sWhy = "the Asan report has no frame #0";
        return false;
    }
    const size_t uFrameEnd = sReport.find_first_of("\r\n", uFrame);
    const std::string sFrame = Lower(sReport.substr(uFrame, uFrameEnd == std::string::npos ? std::string::npos : uFrameEnd - uFrame));
    const std::string sFile = Lower(BaseName(kCanary.sFile));
    if (sFile.empty() || sFrame.find(sFile) == std::string::npos)
    {
        sWhy = "frame #0 of the Asan report is not in " + BaseName(kCanary.sFile) + ": " + sFrame;
        return false;
    }
    return true;
}

static bool RanOnePassed(const std::string& sSummary)
{
    const size_t uAt = sSummary.rfind("test cases:");
    int iCases = 0;
    int iPassed = 0;
    return uAt != std::string::npos && sscanf(sSummary.c_str() + uAt + 11, " %d | %d passed", &iCases, &iPassed) == 2 &&
           iCases == 1 && iPassed == 1;
}

Judgement JudgeCase(const ChildResult& kChild, bool bAsanRun, const CanaryExpect* pCanary)
{
    Judgement k;
    char acCode[32];
    _snprintf(acCode, sizeof(acCode), "0x%08lX", kChild.uCode);
    acCode[sizeof(acCode) - 1] = 0;
    if (pCanary)
    {
        std::string sWhy;
        if (bAsanRun && kChild.bAsanReport && AsanReportMatchesCanary(kChild.sAsanReport, *pCanary, sWhy))
        {
            k.eVerdict = kVerdictPass;
            k.sDetail = "";
        }
        else if (bAsanRun && kChild.bAsanReport)
        {
            k.eVerdict = kVerdictFail;
            k.sDetail = "Asan canary: " + sWhy;
        }
        else
        {
            k.eVerdict = kVerdictFail;
            k.sDetail = bAsanRun ? std::string("Asan canary left no Asan report (exit ") + acCode + ")"
                                 : "Asan canary run outside the Asan configuration";
        }
        return k;
    }
    if (kChild.bAsanReport)
    {
        k.eVerdict = kVerdictFail;
        k.sDetail = "asan";
        return k;
    }
    if (kChild.bKilled || kChild.uCode == 124)
    {
        k.eVerdict = kVerdictStall;
        k.sDetail = kChild.bKilled ? "no answer to the stall flag" : "stalled";
        return k;
    }
    if (bAsanRun && kChild.uCode == 66)
    {
        k.eVerdict = kVerdictFail;
        k.sDetail = "Asan exit 66 without an asan.* report: the Asan runtime failed before main";
        return k;
    }
    switch (kChild.uCode)
    {
    case 0:
        k.eVerdict = RanOnePassed(kChild.sSummary) ? kVerdictPass : kVerdictFail;
        k.sDetail = k.eVerdict == kVerdictPass ? "" : "case did not run exactly once";
        return k;
    case 1:
        k.eVerdict = kVerdictFail;
        k.sDetail = "";
        return k;
    case 77:
        k.eVerdict = kVerdictSkip;
        k.sDetail = "";
        return k;
    case 3:
        k.eVerdict = kVerdictHarness;
        k.sDetail = "harness error";
        return k;
    default:
        break;
    }
    k.eVerdict = kChild.uCode >= 0x80000000ul ? kVerdictCrash : kVerdictFail;
    k.sDetail = acCode;
    return k;
}

std::string QuoteArg(const std::string& sArg)
{
    std::string sOut = "\"";
    size_t uSlashes = 0;
    for (size_t i = 0; i < sArg.size(); ++i)
    {
        if (sArg[i] == '\\')
        {
            ++uSlashes;
            continue;
        }
        if (sArg[i] == '"')
        {
            sOut.append(uSlashes * 2 + 1, '\\');
        }
        else
        {
            sOut.append(uSlashes, '\\');
        }
        uSlashes = 0;
        sOut += sArg[i];
    }
    sOut.append(uSlashes * 2, '\\');
    sOut += "\"";
    return sOut;
}

namespace
{

typedef NTSTATUS(NTAPI* QuerySystemFn)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

struct ProgressView
{
    HANDLE hMap;
    volatile LONG* plBlock;
};

void OpenProgress(DWORD uPid, ProgressView& kView)
{
    char acName[64];
    pcProgress::FormatName(acName, sizeof(acName), uPid);
    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, acName);
    if (!hMap)
    {
        return;
    }
    void* pView = MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 12);
    if (!pView)
    {
        CloseHandle(hMap);
        return;
    }
    kView.hMap = hMap;
    kView.plBlock = (volatile LONG*)pView;
}

void CloseProgress(ProgressView& kView)
{
    if (kView.plBlock)
    {
        UnmapViewOfFile((void*)kView.plBlock);
    }
    if (kView.hMap)
    {
        CloseHandle(kView.hMap);
    }
    kView.hMap = 0;
    kView.plBlock = 0;
}

double FileTimeMs(const LARGE_INTEGER& k)
{
    return (double)k.QuadPart / 10000.0;
}

void SampleThreads(DWORD uPid, DWORD& uMainTid, HANDLE hProcess, std::vector<unsigned char>& kBuf, StallSample& k)
{
    static QuerySystemFn sfnQuery =
        (QuerySystemFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");
    if (!sfnQuery)
    {
        return;
    }
    for (;;)
    {
        ULONG uNeed = 0;
        const NTSTATUS iStatus = sfnQuery(SystemProcessInformation, &kBuf[0], (ULONG)kBuf.size(), &uNeed);
        if (iStatus == (NTSTATUS)0xC0000004L)
        {
            kBuf.resize((uNeed > kBuf.size() ? uNeed : kBuf.size()) * 2);
            continue;
        }
        if (iStatus < 0)
        {
            return;
        }
        break;
    }
    const unsigned char* p = &kBuf[0];
    for (;;)
    {
        const SYSTEM_PROCESS_INFORMATION* pProc = (const SYSTEM_PROCESS_INFORMATION*)p;
        if ((DWORD)(ULONG_PTR)pProc->UniqueProcessId == uPid)
        {
            const SYSTEM_THREAD_INFORMATION* pThreads = (const SYSTEM_THREAD_INFORMATION*)(pProc + 1);
            const SYSTEM_THREAD_INFORMATION* pMain = 0;
            for (ULONG i = 0; i < pProc->NumberOfThreads; ++i)
            {
                if ((DWORD)(ULONG_PTR)pThreads[i].ClientId.UniqueThread == uMainTid)
                {
                    pMain = &pThreads[i];
                }
            }
            if (!pMain)
            {
                for (ULONG i = 0; i < pProc->NumberOfThreads; ++i)
                {
                    if (!pMain || pThreads[i].Reserved1[2].QuadPart < pMain->Reserved1[2].QuadPart)
                    {
                        pMain = &pThreads[i];
                    }
                }
            }
            if (!pMain)
            {
                return;
            }
            uMainTid = (DWORD)(ULONG_PTR)pMain->ClientId.UniqueThread;
            k.bWaiting = pMain->ThreadState == 5;
            k.uWaitReason = pMain->WaitReason;
            k.bHasCpu = true;
            k.fCpuMs = FileTimeMs(pMain->Reserved1[0]) + FileTimeMs(pMain->Reserved1[1]);
            FILETIME kCreate, kExit, kKernel, kUser;
            if (GetProcessTimes(hProcess, &kCreate, &kExit, &kKernel, &kUser))
            {
                LARGE_INTEGER kK, kU;
                kK.LowPart = kKernel.dwLowDateTime;
                kK.HighPart = (LONG)kKernel.dwHighDateTime;
                kU.LowPart = kUser.dwLowDateTime;
                kU.HighPart = (LONG)kUser.dwHighDateTime;
                k.bHasProcCpu = true;
                k.fProcCpuMs = FileTimeMs(kK) + FileTimeMs(kU);
            }
            for (ULONG i = 0; i < pProc->NumberOfThreads; ++i)
            {
                if (&pThreads[i] != pMain && (pThreads[i].ThreadState == 1 || pThreads[i].ThreadState == 3))
                {
                    k.bOtherReady = true;
                    break;
                }
            }
            return;
        }
        if (pProc->NextEntryOffset == 0)
        {
            return;
        }
        p += pProc->NextEntryOffset;
    }
}

double NowMs()
{
    static LARGE_INTEGER sFreq;
    if (sFreq.QuadPart == 0)
    {
        QueryPerformanceFrequency(&sFreq);
    }
    LARGE_INTEGER kNow;
    QueryPerformanceCounter(&kNow);
    return (double)kNow.QuadPart * 1000.0 / (double)sFreq.QuadPart;
}

std::vector<char> ChildEnvironment(const std::string& sAsanOptions)
{
    std::vector<char> kEnv;
    char* pcBlock = GetEnvironmentStringsA();
    for (const char* pc = pcBlock; pc && *pc; pc += strlen(pc) + 1)
    {
        if (_strnicmp(pc, "ASAN_OPTIONS=", 13) != 0)
        {
            kEnv.insert(kEnv.end(), pc, pc + strlen(pc) + 1);
        }
    }
    if (pcBlock)
    {
        FreeEnvironmentStringsA(pcBlock);
    }
    const std::string sVar = "ASAN_OPTIONS=" + sAsanOptions;
    kEnv.insert(kEnv.end(), sVar.begin(), sVar.end());
    kEnv.push_back(0);
    kEnv.push_back(0);
    return kEnv;
}

const char* const sapcDiagnostics[] = {"ERROR:", "values:", "logged:", "[UNEXPECTED-LOG]", "[STALL]", "[CRASH]",
                                       "[HARNESS]", "[SKIP]"};

void ReadOutput(const std::string& sPath, std::string& sSummary, std::vector<std::string>& kDiag)
{
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return;
    }
    char acLine[kReportLineCap];
    while (fgets(acLine, sizeof(acLine), pFile))
    {
        size_t uLen = strlen(acLine);
        while (uLen > 0 && (acLine[uLen - 1] == '\n' || acLine[uLen - 1] == '\r'))
        {
            acLine[--uLen] = 0;
        }
        if (strstr(acLine, "test cases:"))
        {
            sSummary = acLine;
        }
        for (size_t i = 0; i < sizeof(sapcDiagnostics) / sizeof(sapcDiagnostics[0]) && kDiag.size() < 64; ++i)
        {
            if (strstr(acLine, sapcDiagnostics[i]))
            {
                kDiag.push_back(acLine);
                break;
            }
        }
    }
    fclose(pFile);
}

bool ReadAsanReport(const std::string& sDir, std::vector<std::string>& kLines)
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + "\\asan.*").c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    do
    {
        if (kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }
        FILE* pFile = fopen((sDir + "\\" + kFind.cFileName).c_str(), "rb");
        char acLine[kReportLineCap];
        for (int i = 0; pFile && i < 60 && fgets(acLine, sizeof(acLine), pFile); ++i)
        {
            size_t uLen = strlen(acLine);
            while (uLen > 0 && (acLine[uLen - 1] == '\n' || acLine[uLen - 1] == '\r'))
            {
                acLine[--uLen] = 0;
            }
            kLines.push_back(acLine);
        }
        if (pFile)
        {
            fclose(pFile);
        }
    } while (FindNextFileA(hFind, &kFind));
    FindClose(hFind);
    return true;
}

std::string UtcStamp()
{
    SYSTEMTIME k;
    GetSystemTime(&k);
    char ac[32];
    _snprintf(ac, sizeof(ac), "%04u.%02u.%02u-%02u.%02u.%02u", k.wYear, k.wMonth, k.wDay, k.wHour, k.wMinute,
              k.wSecond);
    ac[sizeof(ac) - 1] = 0;
    return ac;
}

std::string SafeName(const std::string& sName, size_t uIndex)
{
    char acPrefix[16];
    _snprintf(acPrefix, sizeof(acPrefix), "%04u-", (unsigned int)uIndex);
    acPrefix[sizeof(acPrefix) - 1] = 0;
    std::string sOut = acPrefix;
    for (size_t i = 0; i < sName.size() && i < 100; ++i)
    {
        const char c = sName[i];
        sOut += (isalnum((unsigned char)c) || c == '_' || c == '.' || c == '-') ? c : '_';
    }
    return sOut;
}

bool ReadTextFile(const std::string& sPath, std::string& sText)
{
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return false;
    }
    char acBuf[4096];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    return true;
}

struct RunnerOptions
{
    std::string sOut;
    std::string sPlatform;
    std::string sConfig;
    std::string sTags;
    std::string sName;
    std::string sExclude;
    std::string sGameDir;
    std::string sPassThrough;
    std::string sSoak;
    std::string sParam;
    int iJobs;
};

bool ParseOptions(int argc, char** argv, RunnerOptions& k, std::string& sError)
{
    k.iJobs = 1;
    k.sSoak = "Short";
    for (int i = 1; i < argc; ++i)
    {
        const std::string sArg = argv[i];
        const size_t uEq = sArg.find('=');
        const std::string sKey = sArg.substr(0, uEq);
        const std::string sValue = uEq == std::string::npos ? "" : sArg.substr(uEq + 1);
        if (sArg == "--runner")
        {
            continue;
        }
        else if (sKey == "--out")
        {
            k.sOut = sValue;
        }
        else if (sKey == "--platform")
        {
            k.sPlatform = sValue;
        }
        else if (sKey == "--config")
        {
            k.sConfig = sValue;
        }
        else if (sKey == "--tags")
        {
            k.sTags = sValue;
        }
        else if (sKey == "--name")
        {
            k.sName = sValue;
        }
        else if (sKey == "--exclude")
        {
            k.sExclude = sValue;
        }
        else if (sKey == "--game-dir")
        {
            k.sGameDir = sValue;
        }
        else if (sKey == "--jobs")
        {
            k.iJobs = atoi(sValue.c_str());
        }
        else if (sKey == "--soak")
        {
            k.sSoak = sValue;
        }
        else if (sKey == "--param")
        {
            k.sParam = sValue;
        }
        else if (sKey == "--fuzz-seed" || sKey == "--fuzz-iter" || sKey == "--fuzz-count" || sKey == "--fuzz-out" ||
                 sArg == "--fuzz-dump")
        {
            k.sPassThrough += " " + QuoteArg(sArg);
        }
        else
        {
            sError = "unknown runner argument " + sArg;
            return false;
        }
    }
    if (k.sOut.empty())
    {
        sError = "--out is required";
        return false;
    }
    if (k.sPlatform != "Win32" && k.sPlatform != "x64")
    {
        sError = "--platform must be Win32 or x64";
        return false;
    }
    if (k.sConfig != "Debug" && k.sConfig != "Release" && k.sConfig != "Test" && k.sConfig != "Asan")
    {
        sError = "--config must be Debug, Release, Test or Asan";
        return false;
    }
    if (k.iJobs < 1 || k.iJobs > 64)
    {
        sError = "--jobs must be 1 to 64";
        return false;
    }
    if (k.sSoak != "Short" && k.sSoak != "Long")
    {
        sError = "--soak must be Short or Long";
        return false;
    }
    if (!k.sParam.empty() && !IsChildParam(k.sParam))
    {
        sError = "--param must be letters, digits and '_'";
        return false;
    }
    if (!k.sGameDir.empty())
    {
        const DWORD uAttr = GetFileAttributesA(k.sGameDir.c_str());
        if (uAttr == INVALID_FILE_ATTRIBUTES || !(uAttr & FILE_ATTRIBUTE_DIRECTORY))
        {
            sError = "--game-dir " + k.sGameDir + " is not a folder";
            return false;
        }
    }
    return true;
}

struct CaseRun
{
    RunnerCase kCase;
    std::vector<std::string> kTags;
    std::string sNotRun;
    ReportTest kReport;
    std::string sLine;
    bool bFailed;
};

int Harness(const std::string& sWhy)
{
    printf("[HARNESS] runner: %s\n", sWhy.c_str());
    fflush(stdout);
    return 3;
}

}

bool RunChild(const std::string& sCmdLine, const std::string& sOutFile, const std::string& sAsanOptions,
              ChildResult& kResult, std::string& sError, const ChildOptions* pOptions)
{
    kResult.uCode = 0xFFFFFFFFul;
    kResult.uPid = 0;
    kResult.bStalled = false;
    kResult.bKilled = false;
    kResult.bAsanReport = false;
    kResult.bHasProgress = false;
    kResult.uTurns = 0;
    kResult.uSteps = 0;
    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hOut = CreateFileA(sOutFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, &sa,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        sError = "cannot create " + sOutFile;
        return false;
    }
    HANDLE hJob = CreateJobObjectA(0, 0);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION kLimit;
    memset(&kLimit, 0, sizeof(kLimit));
    kLimit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!hJob || !SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &kLimit, sizeof(kLimit)))
    {
        CloseHandle(hOut);
        if (hJob)
        {
            CloseHandle(hJob);
        }
        sError = "cannot create a job object";
        return false;
    }
    std::vector<char> kCmd(sCmdLine.begin(), sCmdLine.end());
    kCmd.push_back(0);
    std::vector<char> kEnv;
    if (!sAsanOptions.empty())
    {
        kEnv = ChildEnvironment(sAsanOptions);
    }
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hOut;
    si.hStdError = hOut;
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    const DWORD uFlags = CREATE_NO_WINDOW | CREATE_SUSPENDED |
                         (pOptions && pOptions->bBelowNormal ? BELOW_NORMAL_PRIORITY_CLASS : 0);
    const char* pcWorkDir = pOptions && !pOptions->sWorkDir.empty() ? pOptions->sWorkDir.c_str() : 0;
    const BOOL bStarted =
        CreateProcessA(0, &kCmd[0], 0, 0, TRUE, uFlags, kEnv.empty() ? 0 : &kEnv[0], pcWorkDir, &si, &pi);
    CloseHandle(hOut);
    if (!bStarted)
    {
        CloseHandle(hJob);
        char acErr[48];
        _snprintf(acErr, sizeof(acErr), "cannot start (%lu)", GetLastError());
        acErr[sizeof(acErr) - 1] = 0;
        sError = acErr;
        return false;
    }
    if (!AssignProcessToJobObject(hJob, pi.hProcess))
    {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(hJob);
        sError = "cannot assign the job object";
        return false;
    }
    kResult.uPid = pi.dwProcessId;
    ResumeThread(pi.hThread);
    DWORD uMainTid = pi.dwThreadId;
    std::vector<unsigned char> kBuf(1 << 20);
    ProgressView kView = {0, 0};
    StallState kState = NewStallState();
    bool bFlagged = false;
    const double fStart = NowMs();
    double fLast = 0.0;
    while (WaitForSingleObject(pi.hProcess, kView.plBlock ? 1000 : 10) == WAIT_TIMEOUT)
    {
        if (!kView.plBlock)
        {
            OpenProgress(pi.dwProcessId, kView);
            if (NowMs() - fStart - fLast < 1000.0)
            {
                continue;
            }
        }
        StallSample kSample = NewStallSample();
        if (kView.plBlock)
        {
            kSample.bHasProgress = true;
            kSample.uTurns = (unsigned int)kView.plBlock[0];
            kSample.uSteps = (unsigned int)kView.plBlock[1];
        }
        SampleThreads(pi.dwProcessId, uMainTid, pi.hProcess, kBuf, kSample);
        const double fNow = NowMs() - fStart;
        kSample.fIntervalMs = fNow - fLast;
        fLast = fNow;
        kState = StepStallClock(kState, kSample, kStallSilenceMs);
        if (pOptions && pOptions->pfnTick)
        {
            pOptions->pfnTick();
        }
        if (kState.bKill)
        {
            TerminateJobObject(hJob, 1);
            kResult.bKilled = true;
            break;
        }
        if (kState.bStalled && !bFlagged && kView.plBlock)
        {
            InterlockedExchange(&kView.plBlock[2], 1);
            bFlagged = true;
        }
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD uCode = 0xFFFFFFFF;
    GetExitCodeProcess(pi.hProcess, &uCode);
    kResult.uCode = uCode;
    kResult.bStalled = kState.bStalled;
    if (kView.plBlock)
    {
        kResult.bHasProgress = true;
        kResult.uTurns = (unsigned int)kView.plBlock[0];
        kResult.uSteps = (unsigned int)kView.plBlock[1];
    }
    CloseProgress(kView);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hJob);
    return true;
}

int RunnerMain(int argc, char** argv)
{
    RunnerOptions kOpt;
    std::string sError;
    if (!ParseOptions(argc, argv, kOpt, sError))
    {
        return Harness(sError);
    }
    std::vector<RunnerCase> kDeclared;
    const std::set<doctest::detail::TestCase>& kRegistered = doctest::detail::getRegisteredTests();
    for (std::set<doctest::detail::TestCase>::const_iterator it = kRegistered.begin(); it != kRegistered.end(); ++it)
    {
        RunnerCase kCase;
        kCase.sName = it->m_name ? it->m_name : "";
        kCase.sTags = it->m_test_suite ? it->m_test_suite : "";
        kCase.sFile = it->m_file ? it->m_file : "";
        kDeclared.push_back(kCase);
    }
    std::vector<RunnerCase> kAll;
    if (!ExpandCases(kDeclared, ComplexTests(), kAll, sError))
    {
        return Harness(sError);
    }
    std::vector<std::string> kNames;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        kNames.push_back(kAll[i].sName);
    }
    if (!CheckCases(kAll, sError))
    {
        return Harness(sError);
    }
    std::vector<ExcludeRow> kExcluded;
    if (!kOpt.sExclude.empty())
    {
        std::string sText;
        if (!ReadTextFile(kOpt.sExclude, sText))
        {
            return Harness("cannot read the exclude list " + kOpt.sExclude);
        }
        if (!ParseExcludeList(sText, kNames, kOpt.sPlatform, kOpt.sConfig, kExcluded, sError))
        {
            return Harness(kOpt.sExclude + ": " + sError);
        }
    }
    std::vector<CaseRun> kRuns;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (!IsSelected(kAll[i], kOpt.sTags, kOpt.sName, !kOpt.sGameDir.empty()))
        {
            continue;
        }
        CaseRun kRun;
        kRun.kCase = kAll[i];
        ParseTags(kAll[i].sTags, kRun.kTags);
        kRun.bFailed = false;
        const ExcludeRow* pRow = FindExclusion(kExcluded, kAll[i].sName, kOpt.sPlatform, kOpt.sConfig);
        if (pRow)
        {
            kRun.sNotRun = pRow->sReason;
        }
        else if (HasTag(kRun.kTags, "[AsanCanary]") && kOpt.sConfig != "Asan")
        {
            kRun.sNotRun = "Asan canary: runs in the Asan configuration only";
        }
        kRuns.push_back(kRun);
    }
    if (kRuns.empty())
    {
        printf("[HARNESS] runner: no case matches --tags=%s --name=%s\n", kOpt.sTags.c_str(), kOpt.sName.c_str());
        fflush(stdout);
        return 2;
    }
    const std::string sCaseRoot = kOpt.sOut + "\\t1";
    const std::string sGameRoot = kOpt.sOut + "\\smoke";
    CreateDirectoryA(kOpt.sOut.c_str(), 0);
    CreateDirectoryA(sCaseRoot.c_str(), 0);
    char acSlots[64];
    _snprintf(acSlots, sizeof(acSlots), "Local\\OpenSWBF2GameSlots.%lu", GetCurrentProcessId());
    acSlots[sizeof(acSlots) - 1] = 0;
    HANDLE hSlots = 0;
    if (!kOpt.sGameDir.empty())
    {
        hSlots = CreateSemaphoreA(0, kOpt.iJobs, kOpt.iJobs, acSlots);
        if (!hSlots || GetLastError() == ERROR_ALREADY_EXISTS)
        {
            return Harness(std::string("cannot create the game slot semaphore ") + acSlots);
        }
    }
    char acSelf[MAX_PATH];
    const DWORD uSelf = GetModuleFileNameA(0, acSelf, sizeof(acSelf));
    if (uSelf == 0 || uSelf >= sizeof(acSelf))
    {
        return Harness("cannot resolve the tests exe path");
    }
    const std::string sSelf(acSelf, uSelf);
    const bool bAsan = kOpt.sConfig == "Asan";
    std::mutex kPrintLock;
    std::atomic<size_t> uNext(0);
    std::atomic<bool> bHarness(false);
    const double fRunStart = NowMs();
    Report kReport;
    kReport.sCreatedOn = UtcStamp();

    auto fnWork = [&]() {
        std::vector<std::string> kDiag;
        for (;;)
        {
            const size_t uIndex = uNext++;
            if (uIndex >= kRuns.size())
            {
                return;
            }
            CaseRun& kRun = kRuns[uIndex];
            ReportTest& kTest = kRun.kReport;
            const size_t uSep = kRun.kCase.sName.rfind("::");
            kTest.sDisplayName = uSep == std::string::npos ? kRun.kCase.sName : kRun.kCase.sName.substr(uSep + 2);
            kTest.sFullPath = kRun.kCase.sName;
            kTest.kTags = kRun.kTags;
            kTest.sDateTime = UtcStamp();
            kTest.fDuration = 0.0;
            if (!kRun.sNotRun.empty())
            {
                kTest.sState = "NotRun";
                ReportEntry kEntry = {"Info", kRun.sNotRun};
                kTest.kEntries.push_back(kEntry);
                kRun.sLine = "[N/A] " + kRun.kCase.sName + " - " + kRun.sNotRun;
                std::lock_guard<std::mutex> kLock(kPrintLock);
                printf("%s\n", kRun.sLine.c_str());
                fflush(stdout);
                continue;
            }
            const std::string sDir = sCaseRoot + "\\" + SafeName(kRun.kCase.sName, uIndex);
            CreateDirectoryA(sDir.c_str(), 0);
            const std::string sOutFile = sDir + "\\output.txt";
            std::string sCmd = QuoteArg(sSelf) + " " + QuoteArg("-tc=" + kRun.kCase.sTestName) + kOpt.sPassThrough;
            if (IsGameRunCase(kRun.kCase))
            {
                const std::string sParam = kRun.kCase.sParam.empty() ? kOpt.sParam : kRun.kCase.sParam;
                sCmd += " " + QuoteArg("--game-platform=" + kOpt.sPlatform) + " " + QuoteArg("--game-config=" + kOpt.sConfig) +
                        " " + QuoteArg("--game-dir=" + kOpt.sGameDir) + " " + QuoteArg("--case-dir=" + sGameRoot) + " " +
                        QuoteArg("--game-soak=" + kOpt.sSoak) + " " + QuoteArg(std::string("--game-slots=") + acSlots);
                if (!sParam.empty())
                {
                    sCmd += " " + QuoteArg("--game-param=" + sParam);
                }
            }
            std::string sAsanOptions;
            if (bAsan)
            {
                sAsanOptions = "exitcode=66:log_path='" + sDir + "\\asan'";
            }
            ChildResult kChild;
            std::string sWhy;
            const double fStart = NowMs();
            if (!RunChild(sCmd, sOutFile, sAsanOptions, kChild, sWhy))
            {
                bHarness = true;
                kRun.bFailed = true;
                kTest.sState = "Fail";
                ReportEntry kEntry = {"Error", "runner: " + sWhy};
                kTest.kEntries.push_back(kEntry);
                kRun.sLine = "[FAIL] " + kRun.kCase.sName + " - runner: " + sWhy;
                std::lock_guard<std::mutex> kLock(kPrintLock);
                printf("%s\n", kRun.sLine.c_str());
                fflush(stdout);
                continue;
            }
            const double fMs = NowMs() - fStart;
            kTest.fDuration = fMs / 1000.0;
            kDiag.clear();
            ReadOutput(sOutFile, kChild.sSummary, kDiag);
            std::vector<std::string> kAsanLines;
            kChild.bAsanReport = bAsan && ReadAsanReport(sDir, kAsanLines);
            for (size_t i = 0; i < kAsanLines.size(); ++i)
            {
                kChild.sAsanReport += kAsanLines[i] + "\n";
            }
            CanaryExpect kCanary;
            kCanary.sKind = CanaryKind(kRun.kTags);
            kCanary.sFile = kRun.kCase.sFile;
            const Judgement kJudge = JudgeCase(kChild, bAsan, HasTag(kRun.kTags, "[AsanCanary]") ? &kCanary : 0);
            kTest.kArtifacts.push_back(sOutFile);
            const char* pcToken = "PASS";
            switch (kJudge.eVerdict)
            {
            case kVerdictPass:
                kTest.sState = "Success";
                break;
            case kVerdictSkip:
                kTest.sState = "Skipped";
                pcToken = "SKIP";
                break;
            case kVerdictStall:
                kTest.sState = "Fail";
                pcToken = "STALL";
                break;
            case kVerdictCrash:
                kTest.sState = "Fail";
                pcToken = "CRASH";
                break;
            default:
                kTest.sState = "Fail";
                pcToken = "FAIL";
                break;
            }
            kRun.bFailed = kTest.sState == "Fail";
            if (!kJudge.sDetail.empty())
            {
                ReportEntry kEntry = {kRun.bFailed ? "Error" : "Info", kJudge.sDetail};
                kTest.kEntries.push_back(kEntry);
            }
            if (kRun.bFailed || kJudge.eVerdict == kVerdictSkip)
            {
                for (size_t i = 0; i < kDiag.size(); ++i)
                {
                    ReportEntry kEntry = {kRun.bFailed ? "Error" : "Info", kDiag[i]};
                    kTest.kEntries.push_back(kEntry);
                }
            }
            if (kChild.bAsanReport && kRun.bFailed)
            {
                for (size_t i = 0; i < kAsanLines.size(); ++i)
                {
                    ReportEntry kEntry = {"Error", kAsanLines[i]};
                    kTest.kEntries.push_back(kEntry);
                }
            }
            char acMs[32];
            _snprintf(acMs, sizeof(acMs), " (%.0f ms)", fMs);
            acMs[sizeof(acMs) - 1] = 0;
            kRun.sLine = std::string("[") + pcToken + "] " + kRun.kCase.sName + acMs;
            if (!kJudge.sDetail.empty())
            {
                kRun.sLine += " - " + kJudge.sDetail;
            }
            std::lock_guard<std::mutex> kLock(kPrintLock);
            printf("%s\n", kRun.sLine.c_str());
            if (kRun.bFailed)
            {
                printf("    details: %s\n", sOutFile.c_str());
                for (size_t i = 0; i < kAsanLines.size(); ++i)
                {
                    printf("    %s\n", kAsanLines[i].c_str());
                }
            }
            fflush(stdout);
        }
    };

    std::vector<std::thread> kThreads;
    const size_t uJobs = (size_t)kOpt.iJobs < kRuns.size() ? (size_t)kOpt.iJobs : kRuns.size();
    for (size_t i = 1; i < uJobs; ++i)
    {
        kThreads.push_back(std::thread(fnWork));
    }
    fnWork();
    for (size_t i = 0; i < kThreads.size(); ++i)
    {
        kThreads[i].join();
    }
    if (hSlots)
    {
        CloseHandle(hSlots);
    }

    bool bFailed = false;
    for (size_t i = 0; i < kRuns.size(); ++i)
    {
        kReport.kTests.push_back(kRuns[i].kReport);
        bFailed = bFailed || kRuns[i].bFailed;
    }
    kReport.fTotalDuration = (NowMs() - fRunStart) / 1000.0;
    const std::string sIndex = kOpt.sOut + "\\index.json";
    if (!WriteReport(sIndex, kReport))
    {
        return Harness("cannot write " + sIndex);
    }
    printf("Report: %s\n", sIndex.c_str());
    fflush(stdout);
    if (bHarness)
    {
        return 3;
    }
    return bFailed ? 1 : 0;
}
