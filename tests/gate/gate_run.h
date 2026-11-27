#pragma once
#include <map>
#include <string>
#include <vector>

#include "gate/gate_evidence.h"
#include "gate/gate_ini.h"
#include "gate/gate_keys.h"
#include "gate/gate_machine.h"

namespace Gate
{

struct PreflightSelection
{
    std::string sPlatform;
    std::vector<const Step*> kSteps;
};

struct GateHooks
{
    ProcessResult (*pRun)(const ProcessSpec& kSpec);
    bool (*pTree)(const std::string& sRoot, std::map<std::string, std::string>& kTree, std::string& sError);
    bool (*pRevision)(const std::string& sRoot, std::string& sTree, std::string& sCommit, std::string& sError);
    std::string (*pStamp)();
    HostFaultCountFn pFaults;
    ToolchainInfo (*pToolchain)(const std::string& sPlatform);
    std::string (*pSteam)();
    std::string (*pOverlay)();
    std::string (*pGameData)();
    std::string (*pMSBuild)(const std::string& sPlatform);
    std::vector<std::string> (*pMissing)(const std::string& sPlatform, const std::vector<std::string>& kDlls);
    std::vector<std::string> (*pWer)();
    bool (*pWerKey)();
    std::vector<PreflightResult> (*pPreflight)(const std::string& sRoot, const std::vector<PreflightSelection>& kSel);
};

GateHooks MachineHooks();

struct GateOptions
{
    std::string sRoot;
    std::string sEvidence;
    std::string sStepsText;
    std::string sTier;
    std::vector<std::string> kPlatforms;
    std::vector<std::string> kSteps;
    bool bResume;
    bool bListOrder;
    bool bEcho;
};

GateOptions NewGateOptions();
int RunGate(const GateOptions& kOpt, const GateHooks& kHooks, std::string& sLog);

bool NormalisePlatforms(const std::vector<std::string>& kIn, std::vector<std::string>& kOut, std::string& sError);
bool NormaliseTier(const std::string& sIn, std::string& sOut);
bool EvidenceInsideRepo(const std::string& sEvidence, const std::string& sRoot);
bool IsDirectStep(const Step& kStep);
std::vector<const Step*> OrderSteps(const std::vector<const Step*>& kSteps, const RecordMap& kRecords,
                                    const std::string& sPlatform);
std::vector<const Step*> SelectSteps(const StepTable& kTable, const std::string& sTier, const std::string& sPlatform,
                                     const std::vector<std::string>& kNames);

extern const int kKeepRuns;

}
