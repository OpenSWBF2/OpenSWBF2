#include <windows.h>
#include <stdio.h>

#include <algorithm>
#include <string>
#include <vector>

#include "gate/gate_run.h"

using namespace Gate;

namespace
{

bool IsSwitch(const std::string& sArg)
{
    return sArg.size() > 1 && sArg[0] == '-';
}

std::string SwitchName(const std::string& sArg)
{
    const size_t uStart = sArg.find_first_not_of('-');
    const size_t uEq = sArg.find_first_of("=:");
    return uStart == std::string::npos ? std::string()
                                       : sArg.substr(uStart, uEq == std::string::npos ? std::string::npos : uEq - uStart);
}

bool Param(const std::vector<std::string>& kArgs, const char* pcName)
{
    for (size_t i = 0; i < kArgs.size(); ++i)
    {
        if (IsSwitch(kArgs[i]) && EqualNoCase(SwitchName(kArgs[i]), pcName))
        {
            return true;
        }
    }
    return false;
}

bool Value(const std::vector<std::string>& kArgs, const char* pcName, std::vector<std::string>& kValues)
{
    for (size_t i = 0; i < kArgs.size(); ++i)
    {
        if (!IsSwitch(kArgs[i]) || !EqualNoCase(SwitchName(kArgs[i]), pcName))
        {
            continue;
        }
        const size_t uEq = kArgs[i].find_first_of("=:");
        if (uEq != std::string::npos)
        {
            kValues.push_back(kArgs[i].substr(uEq + 1));
        }
        for (size_t j = i + 1; j < kArgs.size() && !IsSwitch(kArgs[j]); ++j)
        {
            kValues.push_back(kArgs[j]);
        }
        return !kValues.empty();
    }
    return false;
}

std::string Single(const std::vector<std::string>& kArgs, const char* pcName)
{
    std::vector<std::string> kValues;
    return Value(kArgs, pcName, kValues) ? kValues[0] : std::string();
}

std::string EnvValue(const wchar_t* pcName)
{
    wchar_t acValue[MAX_PATH * 4];
    const DWORD uLen = GetEnvironmentVariableW(pcName, acValue, MAX_PATH * 4);
    return uLen == 0 || uLen >= MAX_PATH * 4 ? std::string() : WideToUtf8(std::wstring(acValue, uLen));
}

std::string RepoFromExe()
{
    wchar_t acPath[MAX_PATH * 4];
    const DWORD uLen = GetModuleFileNameW(NULL, acPath, MAX_PATH * 4);
    std::string sPath = WideToUtf8(std::wstring(acPath, uLen));
    for (int i = 0; i < 3; ++i)
    {
        sPath = sPath.substr(0, sPath.find_last_of('\\'));
    }
    return sPath;
}

int Fail(const std::string& sLine)
{
    printf("%s\n", sLine.c_str());
    return 2;
}

bool UnknownArgument(const std::vector<std::string>& kArgs, const char* const* ppcKnown, size_t uKnown,
                     std::string& sBad)
{
    for (size_t i = 0; i < kArgs.size(); ++i)
    {
        bool bKnown = !IsSwitch(kArgs[i]) && i > 0;
        for (size_t k = 0; k < uKnown && !bKnown; ++k)
        {
            bKnown = IsSwitch(kArgs[i]) && EqualNoCase(SwitchName(kArgs[i]), ppcKnown[k]);
        }
        if (!bKnown)
        {
            sBad = kArgs[i];
            return true;
        }
    }
    return false;
}

std::vector<std::string> CommaValues(const std::vector<std::string>& kArgs, const char* pcName)
{
    std::vector<std::string> kRaw;
    std::vector<std::string> kOut;
    Value(kArgs, pcName, kRaw);
    for (size_t i = 0; i < kRaw.size(); ++i)
    {
        const std::vector<std::string> kParts = SplitText(kRaw[i], ',');
        for (size_t j = 0; j < kParts.size(); ++j)
        {
            if (!TrimText(kParts[j]).empty())
            {
                kOut.push_back(TrimText(kParts[j]));
            }
        }
    }
    return kOut;
}

bool PathExists(const std::string& sPath)
{
    return GetFileAttributesW(Utf8ToWide(sPath).c_str()) != INVALID_FILE_ATTRIBUTES;
}

void PrintLines(const std::vector<std::string>& kLines)
{
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        printf("%s\n", kLines[i].c_str());
    }
    fflush(stdout);
}

int RunClean(const std::vector<std::string>& kArgs, const std::string& sRoot)
{
    const char* const kKnown[] = {"Platform", "Config"};
    std::string sBad;
    if (UnknownArgument(kArgs, kKnown, 2, sBad))
    {
        return Fail("gate: unknown argument '" + sBad + "'");
    }
    std::vector<std::string> kRaw = CommaValues(kArgs, "Platform");
    if (kRaw.empty())
    {
        kRaw.push_back("Win32");
    }
    std::vector<std::string> kPlatforms;
    std::string sError;
    if (!NormalisePlatforms(kRaw, kPlatforms, sError))
    {
        return Fail(sError);
    }
    const std::vector<std::string> kWant = CommaValues(kArgs, "Config");
    std::vector<std::string> kConfigs;
    for (size_t i = 0; i < kWant.size(); ++i)
    {
        bool bFound = false;
        for (size_t c = 0; c < 4 && !bFound; ++c)
        {
            if (EqualNoCase(kWant[i], kWorkerConfigurations[c]))
            {
                kConfigs.push_back(kWorkerConfigurations[c]);
                bFound = true;
            }
        }
        if (!bFound)
        {
            return Fail("gate: -Config takes Debug, Release, Test and Asan, not '" + kWant[i] + "'");
        }
    }
    std::vector<std::string> kLines;
    const bool bOk = CleanStaged(sRoot, kPlatforms, kConfigs, kLines, sError);
    PrintLines(kLines);
    if (!bOk)
    {
        printf("gate: clean: %s\n", sError.c_str());
        return 1;
    }
    return 0;
}

int RunFirewall(const std::vector<std::string>& kArgs, const std::string& sRoot)
{
    const char* const kKnown[] = {"List", "Root"};
    std::string sBad;
    if (UnknownArgument(kArgs, kKnown, 2, sBad))
    {
        return Fail("gate: unknown argument '" + sBad + "'");
    }
    const bool bList = Param(kArgs, "List");
    std::vector<std::string> kRoots;
    std::vector<std::string> kWant = CommaValues(kArgs, "Root");
    if (kWant.empty())
    {
        kWant.push_back(sRoot);
    }
    for (size_t i = 0; i < kWant.size(); ++i)
    {
        std::string sFull = FullPath(kWant[i]);
        while (sFull.size() > 3 && sFull[sFull.size() - 1] == '\\')
        {
            sFull.erase(sFull.size() - 1);
        }
        bool bSeen = false;
        for (size_t r = 0; r < kRoots.size(); ++r)
        {
            bSeen = bSeen || EqualNoCase(kRoots[r], sFull);
        }
        if (!bSeen)
        {
            kRoots.push_back(sFull);
        }
    }
    std::sort(kRoots.begin(), kRoots.end());
    std::vector<FirewallRule> kRules;
    std::string sError;
    if (!FirewallRules(kRules, sError))
    {
        printf("gate: firewall: %s\n", sError.c_str());
        return 1;
    }
    const FirewallPlan kPlan = PlanFirewall(kRoots, ProcessorCount(), kRules, PathExists);
    if (!bList && !IsElevated())
    {
        return Fail("gate: firewall must run from an elevated prompt (or use --list)");
    }
    if (!bList && !ApplyFirewall(kPlan, sError))
    {
        printf("gate: firewall: %s\n", sError.c_str());
        return 1;
    }
    PrintLines(FirewallPlanLines(kPlan, bList));
    return 0;
}

}

int wmain(int iArgc, wchar_t** ppcArgv)
{
    std::vector<std::string> kArgs;
    for (int i = 1; i < iArgc; ++i)
    {
        kArgs.push_back(WideToUtf8(ppcArgv[i]));
    }
    if (!kArgs.empty() && (EqualNoCase(kArgs[0], "clean") || EqualNoCase(kArgs[0], "firewall")))
    {
        const std::vector<std::string> kRest(kArgs.begin() + 1, kArgs.end());
        return EqualNoCase(kArgs[0], "clean") ? RunClean(kRest, RepoFromExe()) : RunFirewall(kRest, RepoFromExe());
    }
    const char* const kKnown[] = {"Tier", "Platform", "Step", "Resume", "StepsFile", "EvidenceDir", "ListOrder"};
    std::string sBad;
    if (UnknownArgument(kArgs, kKnown, sizeof(kKnown) / sizeof(kKnown[0]), sBad))
    {
        return Fail("gate: unknown argument '" + sBad + "'");
    }
    GateOptions kOpt = NewGateOptions();
    kOpt.bEcho = true;
    kOpt.bResume = Param(kArgs, "Resume");
    kOpt.bListOrder = Param(kArgs, "ListOrder");
    if (!NormaliseTier(Single(kArgs, "Tier"), kOpt.sTier))
    {
        return Fail("gate: -Tier takes plan, phase or milestone");
    }
    std::vector<std::string> kPlatforms;
    if (!Value(kArgs, "Platform", kPlatforms))
    {
        kPlatforms.push_back("Win32");
    }
    std::string sError;
    if (!NormalisePlatforms(kPlatforms, kOpt.kPlatforms, sError))
    {
        return Fail(sError);
    }
    std::vector<std::string> kSteps;
    Value(kArgs, "Step", kSteps);
    for (size_t i = 0; i < kSteps.size(); ++i)
    {
        const std::vector<std::string> kParts = SplitText(kSteps[i], ',');
        for (size_t j = 0; j < kParts.size(); ++j)
        {
            if (!kParts[j].empty())
            {
                kOpt.kSteps.push_back(kParts[j]);
            }
        }
    }
    kOpt.sRoot = RepoFromExe();
    std::string sStepsFile = Single(kArgs, "StepsFile");
    sStepsFile = FullPath(sStepsFile.empty() ? JoinPath(kOpt.sRoot, "tests\\gate\\steps.ini") : sStepsFile);
    if (!FileExists(sStepsFile) || !ReadTextFile(sStepsFile, kOpt.sStepsText))
    {
        return Fail("gate: steps file not found: " + sStepsFile);
    }
    bool bX64 = false;
    for (size_t i = 0; i < kOpt.kPlatforms.size(); ++i)
    {
        bX64 = bX64 || kOpt.kPlatforms[i] == "x64";
    }
    const std::string sProps = JoinPath(kOpt.sRoot, "OpenSWBF2.Platform.props");
    std::string sPropsText;
    if (bX64 && (!FileExists(sProps) || !ReadTextFile(sProps, sPropsText) || !PlatformPropsHasX64(sPropsText)))
    {
        return Fail("gate: x64 steps need an x64 configuration in " + sProps);
    }
    kOpt.sEvidence = Single(kArgs, "EvidenceDir");
    if (kOpt.sEvidence.empty())
    {
        kOpt.sEvidence = EnvValue(L"OPENSWBF2_EVIDENCE_DIR");
    }
    if (kOpt.sEvidence.empty())
    {
        kOpt.sEvidence = JoinPath(EnvValue(L"LOCALAPPDATA"), "OpenSWBF2\\evidence");
    }
    kOpt.sEvidence = FullPath(kOpt.sEvidence);
    while (kOpt.sEvidence.size() > 3 && kOpt.sEvidence[kOpt.sEvidence.size() - 1] == '\\')
    {
        kOpt.sEvidence.erase(kOpt.sEvidence.size() - 1);
    }
    if (EvidenceInsideRepo(kOpt.sEvidence, kOpt.sRoot))
    {
        return Fail("gate: evidence root " + kOpt.sEvidence + " is inside the repo");
    }
    std::string sLog;
    return RunGate(kOpt, MachineHooks(), sLog);
}
