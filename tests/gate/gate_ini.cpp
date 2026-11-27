#include <stdio.h>

#include <algorithm>
#include <set>

#include "gate/gate_ini.h"
#include "gate/gate_keys.h"

namespace Gate
{

const char* const kGateTiers[] = {"plan", "phase", "milestone"};
const size_t kGateTierCount = sizeof(kGateTiers) / sizeof(kGateTiers[0]);
const char* const kGateKinds[] = {"build", "run", "capture", "script"};
const size_t kGateKindCount = sizeof(kGateKinds) / sizeof(kGateKinds[0]);

namespace
{

const char* const kListKeys[] = {"Tiers", "Platforms", "Requires", "Hashes", "Sources", "Reads", "Outputs", "JudgeKinds"};
const char* const kRepeatKeys[] = {"Arg", "Project", "Property", "MustRun", "JudgeArg"};
const char* const kScalarKeys[] = {"Kind",    "Config",          "StartsGame", "Group", "WorkDir",
                                   "Exe",     "ProjectPlatform", "DllDir",     "Tlog",  "JudgeExe"};

bool InList(const char* const* ppcList, size_t uCount, const std::string& sValue)
{
    for (size_t i = 0; i < uCount; ++i)
    {
        if (sValue == ppcList[i])
        {
            return true;
        }
    }
    return false;
}

std::string LineNo(size_t uLine)
{
    char acText[32];
    _snprintf(acText, sizeof(acText), "line %u: ", (unsigned int)uLine);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

std::vector<std::string> Words(const std::string& sText)
{
    std::vector<std::string> kOut;
    std::string sCur;
    for (size_t i = 0; i <= sText.size(); ++i)
    {
        if (i == sText.size() || sText[i] == ' ' || sText[i] == '\t')
        {
            if (!sCur.empty())
            {
                kOut.push_back(sCur);
            }
            sCur.clear();
        }
        else
        {
            sCur += sText[i];
        }
    }
    return kOut;
}

std::vector<std::string>* ListField(Step& kStep, const std::string& sKey)
{
    if (sKey == "Tiers")
    {
        return &kStep.kTiers;
    }
    if (sKey == "Platforms")
    {
        return &kStep.kPlatforms;
    }
    if (sKey == "Requires")
    {
        return &kStep.kRequires;
    }
    if (sKey == "Hashes")
    {
        return &kStep.kHashes;
    }
    if (sKey == "Sources")
    {
        return &kStep.kSources;
    }
    if (sKey == "Reads")
    {
        return &kStep.kReads;
    }
    if (sKey == "Outputs")
    {
        return &kStep.kOutputs;
    }
    if (sKey == "JudgeKinds")
    {
        return &kStep.kJudgeKinds;
    }
    if (sKey == "Arg")
    {
        return &kStep.kArgs;
    }
    if (sKey == "Project")
    {
        return &kStep.kProjects;
    }
    if (sKey == "Property")
    {
        return &kStep.kProperties;
    }
    if (sKey == "MustRun")
    {
        return &kStep.kMustRun;
    }
    if (sKey == "JudgeArg")
    {
        return &kStep.kJudgeArgs;
    }
    return 0;
}

std::string* ScalarField(Step& kStep, const std::string& sKey)
{
    if (sKey == "Kind")
    {
        return &kStep.sKind;
    }
    if (sKey == "Config")
    {
        return &kStep.sConfig;
    }
    if (sKey == "Group")
    {
        return &kStep.sGroup;
    }
    if (sKey == "WorkDir")
    {
        return &kStep.sWorkDir;
    }
    if (sKey == "Exe")
    {
        return &kStep.sExe;
    }
    if (sKey == "ProjectPlatform")
    {
        return &kStep.sProjectPlatform;
    }
    if (sKey == "DllDir")
    {
        return &kStep.sDllDir;
    }
    if (sKey == "Tlog")
    {
        return &kStep.sTlog;
    }
    if (sKey == "JudgeExe")
    {
        return &kStep.sJudgeExe;
    }
    return 0;
}

Step NewStep(const std::string& sName)
{
    Step k;
    k.sName = sName;
    k.bStartsGame = false;
    return k;
}

std::string JoinUnit(const std::vector<std::string>& kParts, const std::string& sPlatform)
{
    std::vector<std::string> kOut;
    ExpandVars kVars;
    kVars.push_back(std::make_pair(std::string("$Platform"), sPlatform));
    for (size_t i = 0; i < kParts.size(); ++i)
    {
        kOut.push_back(ExpandText(kParts[i], kVars));
    }
    return JoinText(kOut, std::string(1, '\x1f'));
}

}

bool ParseSteps(const std::string& sText, StepTable& kTable, std::vector<std::string>& kProblems)
{
    kTable.kInputs.clear();
    kTable.kSteps.clear();
    const size_t uBefore = kProblems.size();
    std::string sInputs;
    Step* pStep = 0;
    std::set<std::string> kScalarsSeen;
    const std::vector<std::string> kLines = SplitLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string sLine = TrimText(kLines[i]);
        if (sLine.empty())
        {
            continue;
        }
        if (sLine[0] == '[')
        {
            const std::vector<std::string> kWords = Words(sLine.size() > 1 ? sLine.substr(1, sLine.size() - 2) : "");
            if (sLine[sLine.size() - 1] != ']' || kWords.size() != 2 || (kWords[0] != "Inputs" && kWords[0] != "Step"))
            {
                kProblems.push_back(LineNo(i + 1) + "malformed section '" + sLine + "'");
                pStep = 0;
                sInputs.clear();
                continue;
            }
            pStep = 0;
            sInputs.clear();
            kScalarsSeen.clear();
            if (kWords[0] == "Inputs")
            {
                if (kTable.kInputs.count(kWords[1]))
                {
                    kProblems.push_back(LineNo(i + 1) + "duplicate inputs " + kWords[1]);
                }
                sInputs = kWords[1];
                kTable.kInputs[sInputs];
            }
            else
            {
                kTable.kSteps.push_back(NewStep(kWords[1]));
                pStep = &kTable.kSteps.back();
            }
            continue;
        }
        const size_t uEq = sLine.find('=');
        if (uEq == std::string::npos || uEq == 0)
        {
            kProblems.push_back(LineNo(i + 1) + "malformed line '" + sLine + "'");
            continue;
        }
        const std::string sKey = TrimText(sLine.substr(0, uEq));
        const std::string sValue = sLine.substr(uEq + 1);
        if (!sInputs.empty())
        {
            if (sKey != "Path")
            {
                kProblems.push_back(LineNo(i + 1) + "unknown key '" + sKey + "' in [Inputs " + sInputs + "]");
            }
            else if (!TrimText(sValue).empty())
            {
                kTable.kInputs[sInputs].push_back(TrimText(sValue));
            }
            continue;
        }
        if (!pStep)
        {
            kProblems.push_back(LineNo(i + 1) + "'" + sLine + "' is outside a section");
            continue;
        }
        if (InList(kListKeys, sizeof(kListKeys) / sizeof(kListKeys[0]), sKey))
        {
            const std::vector<std::string> kWords = Words(sValue);
            std::vector<std::string>* pList = ListField(*pStep, sKey);
            pList->insert(pList->end(), kWords.begin(), kWords.end());
        }
        else if (InList(kRepeatKeys, sizeof(kRepeatKeys) / sizeof(kRepeatKeys[0]), sKey))
        {
            if (sKey == "Property")
            {
                const size_t uPropEq = sValue.find('=');
                if (uPropEq == std::string::npos || uPropEq == 0)
                {
                    kProblems.push_back(LineNo(i + 1) + "malformed line '" + sLine + "'");
                    continue;
                }
            }
            ListField(*pStep, sKey)->push_back(sValue);
        }
        else if (InList(kScalarKeys, sizeof(kScalarKeys) / sizeof(kScalarKeys[0]), sKey))
        {
            if (!kScalarsSeen.insert(sKey).second)
            {
                kProblems.push_back(LineNo(i + 1) + "duplicate key '" + sKey + "' in [Step " + pStep->sName + "]");
                continue;
            }
            const std::string sTrim = TrimText(sValue);
            if (sKey == "StartsGame")
            {
                if (sTrim != "true" && sTrim != "false")
                {
                    kProblems.push_back(LineNo(i + 1) + "StartsGame '" + sTrim + "' is not true or false");
                }
                pStep->bStartsGame = sTrim == "true";
            }
            else
            {
                *ScalarField(*pStep, sKey) = sTrim;
            }
        }
        else
        {
            kProblems.push_back(LineNo(i + 1) + "unknown key '" + sKey + "' in [Step " + pStep->sName + "]");
        }
    }
    for (size_t s = 0; s < kTable.kSteps.size(); ++s)
    {
        std::vector<std::string> kExpanded;
        const std::vector<std::string>& kReads = kTable.kSteps[s].kReads;
        for (size_t r = 0; r < kReads.size(); ++r)
        {
            std::map<std::string, std::vector<std::string> >::const_iterator it = kTable.kInputs.find(kReads[r]);
            if (it != kTable.kInputs.end())
            {
                kExpanded.insert(kExpanded.end(), it->second.begin(), it->second.end());
            }
            else
            {
                kExpanded.push_back(kReads[r]);
            }
        }
        kTable.kSteps[s].kReads = kExpanded;
    }
    return kProblems.size() == uBefore;
}

const Step* FindStep(const StepTable& kTable, const std::string& sName)
{
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        if (kTable.kSteps[i].sName == sName)
        {
            return &kTable.kSteps[i];
        }
    }
    return 0;
}

void ValidateSteps(const StepTable& kTable, std::vector<std::string>& kProblems)
{
    std::set<std::string> kNames;
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        const Step& s = kTable.kSteps[i];
        const std::string& n = s.sName;
        if (!kNames.insert(n).second)
        {
            kProblems.push_back("duplicate step " + n);
        }
        if (!InList(kGateKinds, kGateKindCount, s.sKind))
        {
            kProblems.push_back(n + ": Kind '" + s.sKind + "'");
        }
        if (s.sConfig.empty())
        {
            kProblems.push_back(n + ": no Config");
        }
        const bool bBuild = s.sKind == "build";
        if (bBuild)
        {
            if (s.kProjects.empty())
            {
                kProblems.push_back(n + ": no Project");
            }
            if (!s.sExe.empty())
            {
                kProblems.push_back(n + ": Kind build takes no Exe");
            }
        }
        else
        {
            if (s.sExe.empty())
            {
                kProblems.push_back(n + ": no Exe");
            }
            if (!s.kProjects.empty())
            {
                kProblems.push_back(n + ": Kind " + s.sKind + " takes no Project");
            }
        }
        if (s.kTiers.empty())
        {
            kProblems.push_back(n + ": no Tiers");
        }
        for (size_t t = 0; t < s.kTiers.size(); ++t)
        {
            const std::vector<std::string> kParts = SplitText(s.kTiers[t], ':');
            const bool bPlatOk = kParts.size() != 2 || s.kPlatforms.empty() ||
                                 std::find(s.kPlatforms.begin(), s.kPlatforms.end(), kParts[1]) != s.kPlatforms.end();
            if (!InList(kGateTiers, kGateTierCount, kParts[0]) || kParts.size() > 2 || !bPlatOk)
            {
                kProblems.push_back(n + ": tier '" + s.kTiers[t] + "'");
            }
        }
        if (!bBuild && s.kReads.empty())
        {
            kProblems.push_back(n + ": no Reads");
        }
        if (!s.kSources.empty() && s.sTlog.empty())
        {
            kProblems.push_back(n + ": Sources needs Tlog");
        }
        if (s.kSources.empty() && !s.sTlog.empty())
        {
            kProblems.push_back(n + ": Tlog needs Sources");
        }
        if (s.sKind == "capture" && s.kOutputs.empty())
        {
            kProblems.push_back(n + ": Kind " + s.sKind + " needs Outputs");
        }
        bool bRunArg = false;
        for (size_t a = 0; a < s.kArgs.size(); ++a)
        {
            bRunArg = bRunArg || s.kArgs[a].find("$Run") != std::string::npos;
        }
        if (s.sKind == "run" && !bRunArg)
        {
            kProblems.push_back(n + ": Kind run needs an Arg with $Run");
        }
        if ((s.sKind == "script" || bBuild) && !s.kOutputs.empty())
        {
            kProblems.push_back(n + ": Kind " + s.sKind + " takes no Outputs");
        }
        const bool bCompare = std::find(s.kJudgeKinds.begin(), s.kJudgeKinds.end(), "compare") != s.kJudgeKinds.end();
        if (bCompare && s.kOutputs.empty())
        {
            kProblems.push_back(n + ": JudgeKinds compare needs Outputs");
        }
        if (bCompare && s.sJudgeExe.empty())
        {
            kProblems.push_back(n + ": JudgeKinds compare needs JudgeExe");
        }
        if (s.sJudgeExe.empty() && !s.kJudgeArgs.empty())
        {
            kProblems.push_back(n + ": JudgeArg needs JudgeExe");
        }
    }
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        const Step& s = kTable.kSteps[i];
        for (size_t r = 0; r < s.kRequires.size(); ++r)
        {
            const Step* pNeed = FindStep(kTable, s.kRequires[r]);
            if (!pNeed || pNeed->sKind != "build")
            {
                kProblems.push_back(s.sName + ": Requires '" + s.kRequires[r] + "' is not a build step");
            }
        }
    }
}

bool StepOnPlatform(const Step& kStep, const std::string& sPlatform)
{
    if (kStep.kPlatforms.empty())
    {
        return true;
    }
    for (size_t i = 0; i < kStep.kPlatforms.size(); ++i)
    {
        if (EqualNoCase(kStep.kPlatforms[i], sPlatform))
        {
            return true;
        }
    }
    return false;
}

bool StepInTier(const Step& kStep, const std::string& sTier, const std::string& sPlatform)
{
    for (size_t i = 0; i < kStep.kTiers.size(); ++i)
    {
        if (kStep.kTiers[i] == sTier || kStep.kTiers[i] == sTier + ":" + sPlatform)
        {
            return true;
        }
    }
    return false;
}

std::string ExpandText(const std::string& sText, const ExpandVars& kVars)
{
    std::string sOut = sText;
    for (size_t v = 0; v < kVars.size(); ++v)
    {
        const std::string& sName = kVars[v].first;
        size_t uAt = 0;
        while ((uAt = sOut.find(sName, uAt)) != std::string::npos)
        {
            sOut.replace(uAt, sName.size(), kVars[v].second);
            uAt += kVars[v].second.size();
        }
    }
    return sOut;
}

ExpandVars GateVars(const std::string& sRoot, const std::string& sPlatform, const std::string& sStamp,
                    const std::string& sJobs, const std::string& sOut, const std::string& sRun)
{
    ExpandVars k;
    k.push_back(std::make_pair(std::string("$Root"), sRoot));
    k.push_back(std::make_pair(std::string("$Platform"), sPlatform));
    k.push_back(std::make_pair(std::string("$Stamp"), sStamp));
    k.push_back(std::make_pair(std::string("$Jobs"), sJobs));
    k.push_back(std::make_pair(std::string("$Out"), sOut));
    k.push_back(std::make_pair(std::string("$Run"), sRun));
    return k;
}

std::string StepDefinitionText(const Step& kStep, const std::string& sPlatform)
{
    std::vector<std::string> kLines;
    kLines.push_back("kind\t" + kStep.sKind);
    kLines.push_back("config\t" + kStep.sConfig);
    kLines.push_back("workdir\t" + JoinUnit(std::vector<std::string>(1, kStep.sWorkDir), sPlatform));
    kLines.push_back("exe\t" + JoinUnit(std::vector<std::string>(1, kStep.sExe), sPlatform));
    kLines.push_back("args\t" + JoinUnit(kStep.kArgs, sPlatform));
    kLines.push_back("projects\t" + JoinUnit(kStep.kProjects, sPlatform));
    kLines.push_back("projectplatform\t" + kStep.sProjectPlatform);
    kLines.push_back("properties\t" + JoinUnit(kStep.kProperties, sPlatform));
    kLines.push_back("outputs\t" + JoinUnit(kStep.kOutputs, sPlatform));
    kLines.push_back("hashes\t" + JoinUnit(kStep.kHashes, sPlatform));
    kLines.push_back("dlldir\t" + JoinUnit(std::vector<std::string>(1, kStep.sDllDir), sPlatform));
    kLines.push_back("judgekinds\t" + JoinText(kStep.kJudgeKinds, ","));
    std::vector<std::string> kJudge(1, kStep.sJudgeExe);
    kJudge.insert(kJudge.end(), kStep.kJudgeArgs.begin(), kStep.kJudgeArgs.end());
    kLines.push_back("judge\t" + (kStep.sJudgeExe.empty() ? std::string() : JoinUnit(kJudge, sPlatform)));
    kLines.push_back("mustrun\t" + JoinUnit(kStep.kMustRun, sPlatform));
    kLines.push_back("tlog\t" + JoinUnit(std::vector<std::string>(1, kStep.sTlog), sPlatform));
    return JoinText(kLines, "\n");
}

bool PlatformPropsHasX64(const std::string& sPropsText)
{
    const std::string sNeedle = "<projectconfiguration include=\"";
    const std::vector<std::string> kLines = SplitLines(sPropsText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string sLine = LowerText(kLines[i]);
        size_t uAt = sLine.find(sNeedle);
        while (uAt != std::string::npos)
        {
            const size_t uFrom = uAt + sNeedle.size();
            const size_t uQuote = sLine.find('"', uFrom);
            if (uQuote != std::string::npos && uQuote - uFrom >= 4 && sLine.compare(uQuote - 4, 4, "|x64") == 0)
            {
                return true;
            }
            uAt = sLine.find(sNeedle, uAt + 1);
        }
    }
    return false;
}

}
