#pragma once
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "gate/gate_ini.h"
#include "gate/gate_keys.h"

namespace Gate
{

std::string QuoteArg(const std::string& sArg);
std::string CommandLine(const std::string& sExe, const std::vector<std::string>& kArgs);

struct ProcessSpec
{
    std::string sExe;
    std::vector<std::string> kArgs;
    std::string sWorkDir;
    KeyPairs kEnv;
    std::string sOutFile;
    bool bEcho;
    bool bMergeErr;
};

struct ProcessResult
{
    bool bStarted;
    unsigned long uCode;
    std::string sOutput;
    std::string sError;
};

ProcessSpec NewProcessSpec(const std::string& sExe, const std::vector<std::string>& kArgs);
ProcessResult RunProcess(const ProcessSpec& kSpec);
bool GitTreeList(const std::string& sRoot, std::vector<std::string>& kPaths, std::string& sError);

extern const char* const kWorkerPlatforms[];
extern const char* const kWorkerConfigurations[];
int ProcessorCount();
int DefaultJobs(int iProcessors);
bool WorkerPath(const std::string& sRoot, const std::string& sPlatform, const std::string& sConfig,
                const std::string& sKind, int iIndex, std::string& sPath, std::string& sError);
std::vector<std::string> SmokeWorkerFiles(const std::string& sPlatform);
std::string AsanRuntimeName(const std::string& sPlatform);
std::vector<std::string> FirewallRulePaths(const std::string& sRoot, const std::vector<std::string>& kPlatforms,
                                           const std::vector<std::string>& kConfigs, int iWorkers);
std::vector<std::string> StepConfigs(const std::vector<const Step*>& kSteps);
std::vector<std::string> CrossRulePaths(const std::string& sRoot, const std::vector<const Step*>& kSteps,
                                        const std::string& sPlatform, int iWorkers);
std::vector<std::string> PreflightRulePaths(const std::string& sRoot, const std::vector<const Step*>& kSteps,
                                            const std::string& sPlatform, int iWorkers);

struct VsTools
{
    bool bFound;
    std::string sInstall;
    std::string sToolsVersion;
    std::string sBin;
    std::string sMSBuild;
};

struct ToolchainInfo
{
    bool bOk;
    std::string sBanner;
    std::string sCl;
    std::string sToolsVersion;
    std::string sUcrt;
    std::string sWindows;
};

VsTools FindVsTools(const std::string& sPlatform);
ToolchainInfo GetToolchainInfo(const std::string& sPlatform);
std::string FileVersionText(const std::string& sPath);
std::string SystemDllDir(const std::string& sPlatform);
std::vector<std::string> T1Runtimes();
std::vector<std::string> GameRuntimes(const std::string& sPlatform, const std::vector<std::string>& kGroups);
std::vector<std::string> PlatformRuntimes(const std::vector<std::string>& kDlls, const std::string& sPlatform);
std::vector<std::string> MissingRuntimes(const std::string& sDir, const std::vector<std::string>& kDlls);

unsigned long SteamActiveUser();
std::string SteamRunning();
std::string OverlayFromVdf(const std::string& sText, const std::string& sAppId);
std::string SteamOverlayState();
bool WerKeyPresent();
std::vector<std::string> WerWarnings();
bool InboundAllowPrograms(std::set<std::string>& kPrograms, std::string& sError);

typedef bool (*HostFaultCountFn)(bool bSince, unsigned long long uSince, long long& iCount);
bool HostFaultCount(bool bSince, unsigned long long uSince, long long& iCount);

long long FreeRamKB();
void DriveFree(const std::string& sPath, std::string& sDrive, unsigned long long& uFree);
std::string GameDataRoot();
std::string GameDataManifestHash(const std::string& sRoot);
KeyPairs VendorDllHashes(const std::string& sWorkerDir, const std::string& sSystemDir, const std::string& sPlatform);

struct ToolchainFacts
{
    std::string sPlatform;
    bool bMSBuild;
    bool bDumpbin;
    std::string sRuntimeDir;
    std::vector<std::string> kMissingRuntimes;
};

struct PreflightFacts
{
    std::string sSteam;
    std::string sOverlay;
    std::vector<std::string> kRulePaths;
    std::vector<std::string> kGamePlatforms;
    std::vector<std::string> kPlatforms;
    std::set<std::string> kAllowRules;
    std::vector<std::string> kWer;
    long long iFreeRamKB;
    std::string sRepoDrive;
    unsigned long long uRepoFree;
    std::string sGameData;
    std::vector<ToolchainFacts> kToolchain;
};

struct PreflightResult
{
    std::string sCheck;
    bool bPass;
    std::string sText;
};

extern const long long kRamFloorKB;
extern const long long kDiskFloorBytes;
std::vector<PreflightResult> PreflightVerdicts(const PreflightFacts& kFacts);
std::string PreflightLine(const PreflightResult& kResult);

bool FindReparsePoints(const std::string& sDir, std::vector<std::string>& kFound, std::string& sError);
bool CopyTree(const std::string& sFrom, const std::string& sTo, std::string& sError);
bool RemoveTree(const std::string& sDir, std::string& sError);
bool RemoveJunctions(const std::string& sDir, std::string& sError);

std::vector<std::string> CleanFolders(const std::string& sRoot, const std::vector<std::string>& kPlatforms,
                                      const std::vector<std::string>& kConfigs);
bool CleanStaged(const std::string& sRoot, const std::vector<std::string>& kPlatforms,
                 const std::vector<std::string>& kConfigs, std::vector<std::string>& kLines, std::string& sError);

struct FirewallRule
{
    std::string sName;
    std::string sProgram;
};

struct FirewallRootPlan
{
    std::string sRoot;
    size_t uWanted;
    std::vector<std::string> kAdd;
    std::vector<FirewallRule> kRemove;
};

struct FirewallPlan
{
    std::vector<FirewallRootPlan> kRoots;
    std::vector<FirewallRule> kStrays;
};

typedef bool (*PathExistsFn)(const std::string& sPath);
FirewallPlan PlanFirewall(const std::vector<std::string>& kRoots, int iWorkers, const std::vector<FirewallRule>& kRules,
                          PathExistsFn pExists);
std::vector<std::string> FirewallPlanLines(const FirewallPlan& kPlan, bool bList);
bool FirewallRules(std::vector<FirewallRule>& kRules, std::string& sError);
bool ApplyFirewall(const FirewallPlan& kPlan, std::string& sError);
bool IsElevated();

}
