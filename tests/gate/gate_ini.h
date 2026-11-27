#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Gate
{

struct Step
{
    std::string sName;
    std::vector<std::string> kTiers;
    std::vector<std::string> kPlatforms;
    std::string sKind;
    std::string sConfig;
    std::vector<std::string> kRequires;
    bool bStartsGame;
    std::string sGroup;
    std::string sWorkDir;
    std::string sExe;
    std::vector<std::string> kArgs;
    std::vector<std::string> kProjects;
    std::string sProjectPlatform;
    std::vector<std::string> kProperties;
    std::vector<std::string> kHashes;
    std::string sDllDir;
    std::vector<std::string> kSources;
    std::string sTlog;
    std::vector<std::string> kReads;
    std::vector<std::string> kOutputs;
    std::vector<std::string> kJudgeKinds;
    std::vector<std::string> kMustRun;
    std::string sJudgeExe;
    std::vector<std::string> kJudgeArgs;
};

struct StepTable
{
    std::map<std::string, std::vector<std::string> > kInputs;
    std::vector<Step> kSteps;
};

extern const char* const kGateTiers[];
extern const size_t kGateTierCount;
extern const char* const kGateKinds[];
extern const size_t kGateKindCount;

bool ParseSteps(const std::string& sText, StepTable& kTable, std::vector<std::string>& kProblems);
void ValidateSteps(const StepTable& kTable, std::vector<std::string>& kProblems);
const Step* FindStep(const StepTable& kTable, const std::string& sName);
bool StepOnPlatform(const Step& kStep, const std::string& sPlatform);
bool StepInTier(const Step& kStep, const std::string& sTier, const std::string& sPlatform);

typedef std::vector<std::pair<std::string, std::string> > ExpandVars;
std::string ExpandText(const std::string& sText, const ExpandVars& kVars);
ExpandVars GateVars(const std::string& sRoot, const std::string& sPlatform, const std::string& sStamp,
                    const std::string& sJobs, const std::string& sOut, const std::string& sRun);
std::string StepDefinitionText(const Step& kStep, const std::string& sPlatform);
bool PlatformPropsHasX64(const std::string& sPropsText);

}
