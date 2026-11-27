#include <windows.h>
#include <stdio.h>

#include <algorithm>
#include <tuple>

#include "gate/gate_readlist.h"
#include "gate/gate_run.h"

namespace Gate
{

const int kKeepRuns = 3;

namespace
{

struct Ctx
{
    const GateOptions* pOpt;
    const GateHooks* pHooks;
    std::string* pLog;
    std::string sRoot;
    std::string sGateRoot;
    std::string sEvidence;
    std::string sRecords;
    std::string sEvidenceLedger;
    std::string sRunDir;
    std::string sStamp;
    std::string sLedger;
    std::string sJobs;
    std::string sTreeId;
    std::string sCommit;
    std::string sGameData;
    RecordMap kRecords;
    std::map<std::string, std::string> kTree;
    int iJudgeCount;
    int iWritten;
};

struct Expanded
{
    const Step* pStep;
    std::string sPlatform;
    std::vector<std::string> kHashes;
    std::string sDllDir;
    std::string sTlog;
    std::vector<std::string> kOutputs;
    std::string sDefinition;
    bool bDirect;
};

struct Decision
{
    std::string sAction;
    std::string sWhy;
    RecordRow kRecord;
};

std::string& HashRoot()
{
    static std::string ssRoot;
    return ssRoot;
}

std::string RootFileHash(const std::string& sRel)
{
    std::string sHash;
    const std::string sFull = JoinPath(HashRoot(), sRel);
    if (FileExists(sFull) && FileSha(sFull, sHash))
    {
        return sHash;
    }
    return "absent";
}

void Say(Ctx& c, const std::string& sLine)
{
    c.pLog->append(sLine + "\n");
    if (c.pOpt->bEcho)
    {
        fputs((sLine + "\n").c_str(), stdout);
        fflush(stdout);
    }
}

void Emit(Ctx& c, std::string& sOutput, const std::string& sLine)
{
    sOutput += sLine + "\r\n";
    Say(c, sLine);
}

std::string Number(long long iValue)
{
    char acText[32];
    _snprintf(acText, sizeof(acText), "%lld", iValue);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

std::string LocalIso()
{
    SYSTEMTIME k;
    GetLocalTime(&k);
    char acText[32];
    _snprintf(acText, sizeof(acText), "%04u-%02u-%02uT%02u:%02u:%02u", k.wYear, k.wMonth, k.wDay, k.wHour, k.wMinute,
              k.wSecond);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

std::string LocalStamp()
{
    SYSTEMTIME k;
    GetLocalTime(&k);
    char acText[32];
    _snprintf(acText, sizeof(acText), "%04u%02u%02u-%02u%02u%02u", k.wYear, k.wMonth, k.wDay, k.wHour, k.wMinute,
              k.wSecond);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

unsigned long long UtcNow()
{
    FILETIME k;
    GetSystemTimeAsFileTime(&k);
    return ((unsigned long long)k.dwHighDateTime << 32) | k.dwLowDateTime;
}

std::string TrimSlash(const std::string& sPath)
{
    std::string s = sPath;
    while (s.size() > 3 && s[s.size() - 1] == '\\')
    {
        s.erase(s.size() - 1);
    }
    return s;
}

bool IsRooted(const std::string& sPath)
{
    return (sPath.size() > 1 && sPath[1] == ':') || (sPath.size() > 1 && sPath[0] == '\\' && sPath[1] == '\\');
}

std::string Rooted(const Ctx& c, const std::string& sPath)
{
    return TrimSlash(FullPath(IsRooted(sPath) ? sPath : JoinPath(c.sRoot, sPath)));
}

std::vector<std::string> SubDirs(const std::string& sDir)
{
    std::vector<std::string> kOut;
    WIN32_FIND_DATAW kData;
    HANDLE hFind = FindFirstFileExW(Utf8ToWide(JoinPath(sDir, "*")).c_str(), FindExInfoBasic, &kData,
                                    FindExSearchNameMatch, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return kOut;
    }
    do
    {
        if ((kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(kData.cFileName, L".") != 0 &&
            wcscmp(kData.cFileName, L"..") != 0)
        {
            kOut.push_back(WideToUtf8(kData.cFileName));
        }
    } while (FindNextFileW(hFind, &kData));
    FindClose(hFind);
    std::sort(kOut.begin(), kOut.end());
    return kOut;
}

std::vector<std::string> DllFiles(const std::string& sDir)
{
    std::vector<std::string> kOut;
    WIN32_FIND_DATAW kData;
    HANDLE hFind = FindFirstFileExW(Utf8ToWide(JoinPath(sDir, "*.dll")).c_str(), FindExInfoBasic, &kData,
                                    FindExSearchNameMatch, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return kOut;
    }
    do
    {
        if (!(kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            kOut.push_back(WideToUtf8(kData.cFileName));
        }
    } while (FindNextFileW(hFind, &kData));
    FindClose(hFind);
    return kOut;
}

std::string NewStampedDir(const std::string& sParent, const std::string& sBase)
{
    MakeDirs(sParent);
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Local\\OpenSWBF2-stamped-dir");
    if (hMutex)
    {
        WaitForSingleObject(hMutex, INFINITE);
    }
    std::string sName = sBase;
    for (int i = 2; DirExists(JoinPath(sParent, sName)); ++i)
    {
        sName = sBase + "-" + Number(i);
    }
    MakeDirs(JoinPath(sParent, sName));
    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return sName;
}

bool Has(const std::vector<std::string>& kList, const std::string& sValue)
{
    return std::find(kList.begin(), kList.end(), sValue) != kList.end();
}

std::string RecordKey(const std::string& sStep, const std::string& sPlatform, const std::string& sConfig)
{
    return sStep + "\t" + sPlatform + "\t" + sConfig;
}

const RecordRow* FindRecord(const RecordMap& kRecords, const std::string& sKey)
{
    RecordMap::const_iterator it = kRecords.find(sKey);
    return it == kRecords.end() ? 0 : &it->second;
}

std::string Field(const RecordRow& kRow, const char* pcName)
{
    RecordRow::const_iterator it = kRow.find(pcName);
    return it == kRow.end() ? std::string() : it->second;
}

ExpandVars StepVars(const Ctx& c, const std::string& sPlatform, const std::string& sOut, const std::string& sRun)
{
    return GateVars(c.sRoot, sPlatform, c.sStamp, c.sJobs, sOut, sRun);
}

Expanded Expand(const Ctx& c, const Step& kStep, const std::string& sPlatform)
{
    Expanded x;
    x.pStep = &kStep;
    x.sPlatform = sPlatform;
    const ExpandVars kVars = StepVars(c, sPlatform, std::string(), std::string());
    for (size_t i = 0; i < kStep.kHashes.size(); ++i)
    {
        x.kHashes.push_back(ExpandText(kStep.kHashes[i], kVars));
    }
    x.sDllDir = kStep.sDllDir.empty() ? std::string() : Rooted(c, ExpandText(kStep.sDllDir, kVars));
    x.sTlog = kStep.sTlog.empty() ? std::string() : Rooted(c, ExpandText(kStep.sTlog, kVars));
    for (size_t i = 0; i < kStep.kOutputs.size(); ++i)
    {
        x.kOutputs.push_back(Rooted(c, ExpandText(kStep.kOutputs[i], kVars)));
    }
    x.sDefinition = TextSha(StepDefinitionText(kStep, sPlatform));
    x.bDirect = IsDirectStep(kStep);
    return x;
}

std::string RunExeHash(const std::string& sPath)
{
    if (!FileExists(sPath))
    {
        return "none";
    }
    std::string sHash;
    std::string sError;
    return PeNormalisedSha(sPath, sHash, sError) ? sHash : std::string("-");
}

StepKey ComputeKey(Ctx& c, const Expanded& x, const ToolchainInfo& kTc, std::set<std::string>& kClosure)
{
    const Step& s = *x.pStep;
    ReadTlog kRead;
    std::map<std::string, std::string> kCommands;
    bool bRead = false;
    bool bCommands = false;
    if (!s.kSources.empty())
    {
        bRead = LoadReadTlog(JoinPath(x.sTlog, "CL.read.1.tlog"), c.sRoot, kRead);
        bCommands = LoadCommandTlog(JoinPath(x.sTlog, "CL.command.1.tlog"), c.sRoot, kCommands);
    }
    HashRoot() = c.sRoot;
    KeyFilesInput kIn;
    kIn.pTree = &c.kTree;
    kIn.kReads = s.kReads;
    kIn.kSources = s.kSources;
    kIn.pRead = bRead ? &kRead : 0;
    kIn.pCommands = bCommands ? &kCommands : 0;
    kIn.sDefinition = x.sDefinition;
    kIn.pHash = RootFileHash;
    KeyInput kKey;
    std::string sTlogState;
    KeyFiles(kIn, kKey.kFiles, sTlogState);
    kClosure.clear();
    for (std::map<std::string, std::string>::const_iterator it = kKey.kFiles.begin(); it != kKey.kFiles.end(); ++it)
    {
        kClosure.insert(it->first);
    }
    std::string sExe = "none";
    if (!x.kHashes.empty())
    {
        KeyPairs kExe;
        for (size_t i = 0; i < x.kHashes.size(); ++i)
        {
            const std::string sFull = Rooted(c, x.kHashes[i]);
            std::string sHash;
            std::string sError;
            if (!FileExists(sFull) || !PeNormalisedSha(sFull, sHash, sError))
            {
                sHash = "-";
            }
            kExe.push_back(std::make_pair(x.kHashes[i], sHash));
        }
        sExe = KeyMap(kExe);
    }
    std::string sDlls = "none";
    std::string sVendor = "none";
    if (!x.sDllDir.empty())
    {
        sDlls = "-";
        sVendor = "-";
        if (DirExists(x.sDllDir))
        {
            KeyPairs kDlls;
            const std::vector<std::string> kNames = DllFiles(x.sDllDir);
            for (size_t i = 0; i < kNames.size(); ++i)
            {
                std::string sHash;
                if (!FileSha(JoinPath(x.sDllDir, kNames[i]), sHash))
                {
                    sHash = "-";
                }
                kDlls.push_back(std::make_pair(kNames[i], sHash));
            }
            sDlls = kDlls.empty() ? std::string("none") : KeyMap(kDlls);
            sVendor = KeyMap(VendorDllHashes(x.sDllDir, SystemDllDir(x.sPlatform), x.sPlatform));
        }
    }
    std::string sAsan = "none";
    if (s.sConfig == "Asan")
    {
        const std::string sRuntime =
            JoinPath(c.sRoot, "bin\\Tests\\" + x.sPlatform + "\\Asan\\" + AsanRuntimeName(x.sPlatform));
        if (!FileExists(sRuntime) || !FileSha(sRuntime, sAsan))
        {
            sAsan = "-";
        }
    }
    const GateHooks& h = *c.pHooks;
    kKey.kHeader.push_back(std::make_pair(std::string("step"), s.sName));
    kKey.kHeader.push_back(std::make_pair(std::string("platform"), x.sPlatform));
    kKey.kHeader.push_back(std::make_pair(std::string("config"), s.sConfig));
    kKey.kHeader.push_back(std::make_pair(std::string("toolset"), kTc.sToolsVersion));
    kKey.kHeader.push_back(std::make_pair(std::string("cl"), kTc.sCl));
    kKey.kHeader.push_back(std::make_pair(std::string("ucrt"), kTc.sUcrt));
    kKey.kHeader.push_back(std::make_pair(std::string("steam"), h.pSteam()));
    kKey.kHeader.push_back(std::make_pair(std::string("overlay"), h.pOverlay()));
    kKey.kHeader.push_back(std::make_pair(std::string("gamedata"), c.sGameData));
    kKey.kHeader.push_back(std::make_pair(std::string("exe"), sExe));
    kKey.kHeader.push_back(std::make_pair(std::string("dlls"), sDlls));
    kKey.kHeader.push_back(std::make_pair(std::string("vendor"), sVendor));
    kKey.kHeader.push_back(std::make_pair(std::string("asanrt"), sAsan));
    kKey.kHeader.push_back(std::make_pair(std::string("tlog"), sTlogState));
    kKey.kJudgeKinds = s.kJudgeKinds;
    return MakeStepKey(kKey);
}

JudgeSpec SpecOf(const Expanded& x)
{
    JudgeSpec k;
    k.sKind = x.pStep->sKind;
    k.bDirect = x.bDirect;
    k.kMustRun = x.pStep->kMustRun;
    return k;
}

long RunJudge(Ctx& c, const Expanded& x, const std::string& sOut, const std::string& sRun, std::string& sOutput)
{
    const ExpandVars kVars = StepVars(c, x.sPlatform, sOut, sRun);
    std::vector<std::string> kArgs;
    for (size_t i = 0; i < x.pStep->kJudgeArgs.size(); ++i)
    {
        kArgs.push_back(ExpandText(x.pStep->kJudgeArgs[i], kVars));
    }
    ProcessSpec kSpec = NewProcessSpec(ExpandText(x.pStep->sJudgeExe, kVars), kArgs);
    kSpec.sWorkDir = c.sRoot;
    kSpec.bEcho = c.pOpt->bEcho;
    const ProcessResult k = c.pHooks->pRun(kSpec);
    sOutput += k.sOutput;
    if (!k.bStarted)
    {
        Emit(c, sOutput, "gate: judge " + k.sError);
        return -1;
    }
    return (long)k.uCode;
}

std::string JudgeEvidence(Ctx& c, const Expanded& x, const std::string& sHash)
{
    ++c.iJudgeCount;
    const std::string sCopy =
        JoinPath(c.sRunDir, "judge\\" + x.sPlatform + "-" + x.pStep->sName + "-" + Number(c.iJudgeCount));
    MakeDirs(JoinPath(c.sRunDir, "judge"));
    const std::string sWhy = JudgeArchived(c.sEvidence, sHash, sCopy, SpecOf(x));
    if (!sWhy.empty() || x.pStep->sJudgeExe.empty())
    {
        return sWhy;
    }
    const std::string sOut = JoinPath(sCopy, "out\\0");
    if (!x.kOutputs.empty() && !DirExists(sOut))
    {
        return "no archived capture output in out\\0";
    }
    std::string sIgnored;
    const long iCode = RunJudge(c, x, sOut, JoinPath(sCopy, "runs\\0"), sIgnored);
    return iCode == 0 ? std::string() : "judge command exited " + Number(iCode);
}

Decision Decide(Ctx& c, const Expanded& x, const StepKey& kKey)
{
    Decision d;
    d.sAction = "RUN";
    if (!kKey.bValid)
    {
        d.sWhy = "key invalid: " + JoinText(InvalidKeyFields(kKey), ", ");
        return d;
    }
    const RecordRow* pRec = FindRecord(c.kRecords, RecordKey(x.pStep->sName, x.sPlatform, x.pStep->sConfig));
    if (!RecordMatches(pRec, kKey))
    {
        d.sWhy = "no record with this RunKey";
        return d;
    }
    d.kRecord = *pRec;
    if (Field(*pRec, "Result") != "PASS")
    {
        d.sWhy = "record result " + Field(*pRec, "Result");
        return d;
    }
    const std::string sIntact = EvidenceIntact(c.sEvidence, Field(*pRec, "Evidence"));
    if (!sIntact.empty())
    {
        d.sWhy = sIntact;
        return d;
    }
    if (Field(*pRec, "JudgeKey") == kKey.sJudgeKey)
    {
        const std::string sWhy = JudgeEvidence(c, x, Field(*pRec, "Evidence"));
        if (!sWhy.empty())
        {
            d.sWhy = "evidence judged FAIL: " + sWhy;
            return d;
        }
        d.sAction = "CARRY";
        return d;
    }
    if (JudgeOnlyFiles(x.pStep->kJudgeKinds).empty())
    {
        d.sWhy = "JudgeKey changed for a step with no judge-only files";
        return d;
    }
    d.sAction = "REJUDGE";
    return d;
}

long long AddLedger(Ctx& c, const Expanded& x, const StepKey& kKey, const std::string& sDecision,
                    const std::string& sResult, const std::string& sEvidence, unsigned long long uT0,
                    const std::string& sStart)
{
    const long long iWall = (long long)((GetTickCount64() - uT0 + 500) / 1000);
    std::vector<std::string> kParts;
    kParts.push_back(x.pStep->sName);
    kParts.push_back(x.sPlatform);
    kParts.push_back(x.pStep->sConfig);
    kParts.push_back(c.pOpt->sTier);
    kParts.push_back(kKey.sRunKey);
    kParts.push_back(kKey.sJudgeKey);
    kParts.push_back(sDecision);
    kParts.push_back(sResult);
    kParts.push_back(sEvidence);
    kParts.push_back(Number(iWall));
    kParts.push_back(sStart);
    kParts.push_back(LocalIso());
    const std::string sLine = JoinText(kParts, "\t");
    AppendLedgerLine(c.sLedger, sLine);
    AppendLedgerLine(c.sEvidenceLedger, sLine);
    return iWall;
}

void AddRecord(Ctx& c, const Expanded& x, const StepKey& kKey, const std::string& sResult,
               const std::string& sEvidence, const std::string& sWall)
{
    if (x.pStep->sKind == "build")
    {
        return;
    }
    RecordRow kRow;
    kRow["Step"] = x.pStep->sName;
    kRow["RunKey"] = kKey.sRunKey;
    kRow["JudgeKey"] = kKey.sJudgeKey;
    kRow["Platform"] = x.sPlatform;
    kRow["Config"] = x.pStep->sConfig;
    kRow["Evidence"] = sEvidence;
    kRow["Result"] = sResult;
    kRow["Wall_s"] = sWall;
    kRow["Tree"] = c.sTreeId;
    kRow["Commit"] = c.sCommit;
    kRow["Date"] = LocalIso();
    MergeStepRecordsFile(c.sRecords, std::vector<std::string>(1, FormatRecord(kRow, ColumnList(kStepColumns, kStepColumnCount))),
                         std::vector<std::string>());
    c.kRecords = ReadStepRecordsFile(c.sRecords);
    ++c.iWritten;
}

void Retention(Ctx& c)
{
    if (c.sRunDir.empty())
    {
        return;
    }
    std::string sError;
    const std::vector<std::string> kDirs = SubDirs(c.sGateRoot);
    const size_t uDrop = kDirs.size() > (size_t)kKeepRuns ? kDirs.size() - kKeepRuns : 0;
    size_t uDropped = 0;
    for (size_t i = 0; i < uDrop; ++i)
    {
        const std::string sDir = JoinPath(c.sGateRoot, kDirs[i]);
        if (EqualNoCase(sDir, c.sRunDir))
        {
            continue;
        }
        if (!RemoveTree(sDir, sError))
        {
            Say(c, "gate: RETENTION FAILED: " + sError);
            return;
        }
        ++uDropped;
    }
    std::set<std::string> kNamed;
    const RecordMap kAll = ReadStepRecordsFile(c.sRecords);
    for (RecordMap::const_iterator it = kAll.begin(); it != kAll.end(); ++it)
    {
        kNamed.insert(Field(it->second, "Evidence"));
    }
    size_t uGone = 0;
    const std::vector<std::string> kEvidence = SubDirs(c.sEvidence);
    for (size_t i = 0; i < kEvidence.size(); ++i)
    {
        if (!IsEvidenceName(kEvidence[i]) || kNamed.count(kEvidence[i]))
        {
            continue;
        }
        if (!RemoveTree(JoinPath(c.sEvidence, kEvidence[i]), sError))
        {
            Say(c, "gate: RETENTION FAILED: " + sError);
            return;
        }
        ++uGone;
    }
    Say(c, "gate: retention removed " + Number((long long)uDropped) + " run folder(s) under " + c.sGateRoot + " and " +
               Number((long long)uGone) + " unnamed evidence folder(s)");
}

int Stop(Ctx& c, int iCode)
{
    if (c.iWritten)
    {
        Say(c, "gate: " + Number(c.iWritten) + " record(s) written to " + c.sRecords);
    }
    Retention(c);
    return iCode;
}

std::string ArgValue(const std::vector<std::string>& kArgs, const std::string& sPrefix)
{
    for (size_t i = 0; i < kArgs.size(); ++i)
    {
        if (kArgs[i].compare(0, sPrefix.size(), sPrefix) == 0)
        {
            return kArgs[i].substr(sPrefix.size());
        }
    }
    return std::string();
}

long RunDirect(Ctx& c, const Expanded& x, const std::string& sWork, const KeyPairs& kEnv,
               const ToolchainInfo& kTc, std::string& sOutput, std::vector<std::string>& kArgsOut,
               KeyPairs& kRuns)
{
    const Step& s = *x.pStep;
    const GateHooks& h = *c.pHooks;
    const std::string sRun = JoinPath(sWork, "run");
    MakeDirs(sRun);
    kRuns.push_back(std::make_pair(std::string("0"), sRun));
    const ExpandVars kVars = StepVars(c, x.sPlatform, x.kOutputs.empty() ? std::string() : x.kOutputs[0], sRun);
    const std::string sExe = ExpandText(s.sExe, kVars);
    for (size_t i = 0; i < s.kArgs.size(); ++i)
    {
        kArgsOut.push_back(ExpandText(s.kArgs[i], kVars));
    }
    const bool bGame = !s.sGroup.empty();
    Emit(c, sOutput, "toolchain: " + kTc.sBanner + " (MSVC " + kTc.sToolsVersion + ")");
    Emit(c, sOutput, "ucrtbase: " + kTc.sUcrt);
    Emit(c, sOutput, "windows: " + kTc.sWindows);
    const std::vector<std::string> kDlls =
        PlatformRuntimes(bGame ? GameRuntimes(x.sPlatform, std::vector<std::string>(1, s.sGroup)) : T1Runtimes(),
                         x.sPlatform);
    const std::vector<std::string> kMissing = h.pMissing(x.sPlatform, kDlls);
    if (!kMissing.empty())
    {
        const std::string sSys = SystemDllDir(x.sPlatform);
        const std::string sArch = x.sPlatform == "x64" ? "x64" : "x86";
        Emit(c, sOutput, "Preflight failed: missing from " + sSys.substr(sSys.find_last_of('\\') + 1) + ": " +
                             JoinText(kMissing, ", ") + ". Install the DirectX June 2010 redist, the VC++ 2015-2022 " +
                             sArch + " redistributable and the VS2013 " + sArch + " runtime.");
        return 2;
    }
    RunParamsInput kParams;
    kParams.kArgs = kArgsOut;
    kParams.sPlatform = x.sPlatform;
    kParams.sConfiguration = s.sConfig;
    kParams.sSmokeConfiguration = (s.sConfig == "Test" || s.sConfig == "Asan") ? s.sConfig : std::string("Release");
    kParams.bWerLocalDumps = h.pWerKey();
    kParams.sSteam = h.pSteam();
    kParams.sTestsExe = sExe;
    kParams.kExes.push_back(std::make_pair(std::string("OpenSWBF2Tests.exe"), RunExeHash(sExe)));
    const std::string sGameDir = ArgValue(kArgsOut, "--game-dir=");
    if (bGame && !sGameDir.empty())
    {
        kParams.kExes.push_back(
            std::make_pair(std::string("BattlefrontII.exe"), RunExeHash(JoinPath(sGameDir, "BattlefrontII.exe"))));
        std::vector<std::string> kFiles = SmokeWorkerFiles(x.sPlatform);
        kFiles.push_back(AsanRuntimeName(x.sPlatform));
        for (size_t i = 0; i < kFiles.size(); ++i)
        {
            std::string sHash;
            const std::string sPath = JoinPath(sGameDir, kFiles[i]);
            if (LikeMatch(kFiles[i], "*.dll") && FileExists(sPath) && FileSha(sPath, sHash))
            {
                kParams.kDlls.push_back(std::make_pair(kFiles[i], sHash));
            }
        }
    }
    const unsigned long long uStart = UtcNow();
    std::vector<std::string> kParamLines = RunParamsLines(kParams);
    HostFaultLines("start", h.pFaults, uStart, kParamLines);
    const std::string sParams = JoinPath(sRun, "run-params.txt");
    WriteTextFile(sParams, LinesText(kParamLines));
    if (bGame)
    {
        const std::vector<std::string> kWer = h.pWer();
        for (size_t i = 0; i < kWer.size(); ++i)
        {
            Emit(c, sOutput, "WARNING: " + kWer[i]);
        }
    }
    ProcessSpec kSpec = NewProcessSpec(sExe, kArgsOut);
    kSpec.sWorkDir = s.sWorkDir.empty() ? c.sRoot : Rooted(c, ExpandText(s.sWorkDir, kVars));
    kSpec.kEnv = kEnv;
    kSpec.bEcho = c.pOpt->bEcho;
    const ProcessResult kRes = h.pRun(kSpec);
    sOutput += kRes.sOutput;
    long iCode = kRes.bStarted ? (long)kRes.uCode : -1;
    if (!kRes.bStarted)
    {
        Emit(c, sOutput, "gate: " + kRes.sError);
    }
    const std::string sIndex = JoinPath(sRun, "index.json");
    std::string sIndexText;
    IndexJson kIndex;
    std::string sIndexError;
    const bool bIndex = ReadTextFile(sIndex, sIndexText) && ReadIndexJson(sIndexText, kIndex, sIndexError);
    std::vector<std::string> kVerdicts = VerdictLines(kRes.sOutput);
    bool bBad = false;
    bool bFatal = false;
    if (bGame)
    {
        const std::string sTags = "*[" + s.sGroup + "]*";
        if (!kRes.bStarted || iCode >= 2)
        {
            kVerdicts.push_back("[FAIL] game runs " + sTags + " - the test runner exited " + Number(iCode));
            Say(c, kVerdicts.back());
        }
        else if (!bIndex)
        {
            kVerdicts.push_back("[FAIL] game runs " + sTags + " - no readable " + sIndex);
            Say(c, kVerdicts.back());
        }
        bBad = iCode != 0 || !bIndex;
    }
    else if (!kRes.bStarted || iCode >= 2)
    {
        Emit(c, sOutput, "The test runner exited " + Number(iCode) + ".");
        bFatal = true;
    }
    else if (!bIndex)
    {
        Emit(c, sOutput, "The test runner left no readable " + sIndex + ".");
        bFatal = true;
    }
    else
    {
        bBad = iCode != 0;
    }
    std::vector<std::string> kEnd;
    HostFaultLines("end", h.pFaults, uStart, kEnd);
    AppendTextFile(sParams, LinesText(kEnd));
    std::string sParamText;
    ReadTextFile(sParams, sParamText);
    const std::string sNote = HostFaultNote(ReadKeyValueText(sParamText), bBad || bFatal ? 1 : 0);
    WriteTextFile(JoinPath(sRun, "results.txt"), ResultsText(kVerdicts, s.sConfig == "Asan", sNote));
    Emit(c, sOutput, "Artifacts: " + sRun);
    return bFatal ? 2 : (bBad ? 1 : 0);
}

long RunScript(Ctx& c, const Expanded& x, const KeyPairs& kEnv, std::string& sOutput,
               std::vector<std::string>& kArgsOut)
{
    const Step& s = *x.pStep;
    const ExpandVars kVars = StepVars(c, x.sPlatform, x.kOutputs.empty() ? std::string() : x.kOutputs[0], std::string());
    for (size_t i = 0; i < s.kArgs.size(); ++i)
    {
        kArgsOut.push_back(ExpandText(s.kArgs[i], kVars));
    }
    ProcessSpec kSpec = NewProcessSpec(ExpandText(s.sExe, kVars), kArgsOut);
    kSpec.sWorkDir = s.sWorkDir.empty() ? c.sRoot : Rooted(c, ExpandText(s.sWorkDir, kVars));
    kSpec.kEnv = kEnv;
    kSpec.bEcho = c.pOpt->bEcho;
    const ProcessResult kRes = c.pHooks->pRun(kSpec);
    sOutput += kRes.sOutput;
    if (!kRes.bStarted)
    {
        Emit(c, sOutput, "gate: " + kRes.sError);
    }
    return kRes.bStarted ? (long)kRes.uCode : -1;
}

long RunBuild(Ctx& c, const Expanded& x, const std::string& sWork)
{
    const Step& s = *x.pStep;
    const ExpandVars kVars = StepVars(c, x.sPlatform, std::string(), std::string());
    const std::string sMSBuild = c.pHooks->pMSBuild(x.sPlatform);
    std::string sOutput;
    long iCode = 0;
    if (sMSBuild.empty())
    {
        Emit(c, sOutput, "gate: MSBuild not found for " + x.sPlatform);
        iCode = -1;
    }
    for (size_t p = 0; iCode == 0 && p < s.kProjects.size(); ++p)
    {
        std::vector<std::string> kArgs;
        kArgs.push_back(Rooted(c, ExpandText(s.kProjects[p], kVars)));
        kArgs.push_back("/p:Configuration=" + s.sConfig);
        kArgs.push_back("/p:Platform=" + (s.sProjectPlatform.empty() ? x.sPlatform : s.sProjectPlatform));
        for (size_t i = 0; i < s.kProperties.size(); ++i)
        {
            kArgs.push_back("/p:" + ExpandText(s.kProperties[i], kVars));
        }
        kArgs.push_back("/m");
        kArgs.push_back("/v:minimal");
        kArgs.push_back("/nologo");
        ProcessSpec kSpec = NewProcessSpec(sMSBuild, kArgs);
        kSpec.sWorkDir = c.sRoot;
        kSpec.bEcho = c.pOpt->bEcho;
        const ProcessResult kRes = c.pHooks->pRun(kSpec);
        sOutput += kRes.sOutput;
        if (!kRes.bStarted)
        {
            Emit(c, sOutput, "gate: " + kRes.sError);
            iCode = -1;
        }
        else
        {
            iCode = (long)kRes.uCode;
        }
    }
    MakeDirs(sWork);
    WriteTextFile(JoinPath(sWork, "stdout.txt"), sOutput);
    return iCode;
}

bool RunOneStep(Ctx& c, const Step& kStep, const std::string& sPlatform, const ToolchainInfo& kTc,
                const RecordMap& kResumed, int& iExit)
{
    const Expanded x = Expand(c, kStep, sPlatform);
    const std::string sWork = JoinPath(c.sRunDir, sPlatform + "\\" + kStep.sName);
    const unsigned long long uT0 = GetTickCount64();
    const std::string sStart = LocalIso();
    std::set<std::string> kClosure;
    const StepKey kKey = ComputeKey(c, x, kTc, kClosure);
    const RecordRow* pLedger = FindRecord(kResumed, kStep.sName + "\t" + sPlatform);
    if (pLedger && Field(*pLedger, "Result") == "PASS" && kKey.bValid && Field(*pLedger, "RunKey") == kKey.sRunKey &&
        Field(*pLedger, "JudgeKey") == kKey.sJudgeKey)
    {
        Say(c, "[RESUMED] gate:" + kStep.sName + " " + sPlatform + " ledger " + Field(*pLedger, "End"));
        if (Field(*pLedger, "Decision") != "CARRY")
        {
            AddRecord(c, x, kKey, "PASS", Field(*pLedger, "Evidence"), Field(*pLedger, "Wall_s"));
        }
        return true;
    }
    std::string sError;
    if (DirExists(sWork) && !RemoveTree(sWork, sError))
    {
        Say(c, "gate: ERROR: " + sError);
        iExit = 1;
        return false;
    }
    if (kStep.sKind == "build")
    {
        Say(c, "gate: " + kStep.sName + " " + sPlatform + " build");
        const long iCode = RunBuild(c, x, sWork);
        std::set<std::string> kPostClosure;
        const StepKey kPost = ComputeKey(c, x, kTc, kPostClosure);
        const std::string sResult = iCode == 0 ? "PASS" : "FAIL";
        AddLedger(c, x, kPost, "RUN", sResult, "-", uT0, sStart);
        Say(c, "[" + sResult + "] gate:" + kStep.sName + " " + sPlatform + " exit " + Number(iCode));
        iExit = 1;
        return iCode == 0;
    }
    const Decision d = Decide(c, x, kKey);
    if (d.sAction == "CARRY")
    {
        Say(c, "[CARRY] gate:" + kStep.sName + " " + sPlatform + " run=" + kKey.sRunKey + " judge=" + kKey.sJudgeKey +
                   " evidence=" + Field(d.kRecord, "Evidence") + " recorded " + Field(d.kRecord, "Date") + " at " +
                   Field(d.kRecord, "Commit"));
        AddLedger(c, x, kKey, "CARRY", "PASS", Field(d.kRecord, "Evidence"), uT0, sStart);
        return true;
    }
    if (d.sAction == "REJUDGE")
    {
        Say(c, "gate: " + kStep.sName + " " + sPlatform + " re-judging evidence " + Field(d.kRecord, "Evidence") +
                   " (JudgeKey " + Field(d.kRecord, "JudgeKey") + " -> " + kKey.sJudgeKey + ")");
        const std::string sWhy = JudgeEvidence(c, x, Field(d.kRecord, "Evidence"));
        const std::string sResult = sWhy.empty() ? "PASS" : "FAIL";
        AddLedger(c, x, kKey, "REJUDGE", sResult, Field(d.kRecord, "Evidence"), uT0, sStart);
        AddRecord(c, x, kKey, sResult, Field(d.kRecord, "Evidence"), Field(d.kRecord, "Wall_s"));
        if (!sWhy.empty())
        {
            Say(c, "[FAIL] gate:" + kStep.sName + " " + sPlatform + " re-judge: " + sWhy);
            iExit = 1;
            return false;
        }
        Say(c, "[REJUDGE] gate:" + kStep.sName + " " + sPlatform + " PASS run=" + kKey.sRunKey + " judge=" +
                   kKey.sJudgeKey + " evidence=" + Field(d.kRecord, "Evidence"));
        return true;
    }
    Say(c, "gate: " + kStep.sName + " " + sPlatform + " run (" + d.sWhy + ")");
    MakeDirs(JoinPath(sWork, "evidence"));
    const std::string sReads = JoinPath(sWork, "reads.txt");
    WriteTextFile(sReads, std::string());
    KeyPairs kEnv;
    kEnv.push_back(std::make_pair(std::string("OPENSWBF2_READS"), sReads));
    std::string sOutput;
    std::vector<std::string> kArgs;
    KeyPairs kRuns;
    long iCode = x.bDirect ? RunDirect(c, x, sWork, kEnv, kTc, sOutput, kArgs, kRuns)
                           : RunScript(c, x, kEnv, sOutput, kArgs);
    std::string sReadText;
    ReadTextFile(sReads, sReadText);
    ReadCheckInput kCheck;
    kCheck.sStep = kStep.sName;
    kCheck.sRoot = c.sRoot;
    kCheck.kReads = kStep.kReads;
    kCheck.kClosure = kClosure;
    kCheck.pTree = &c.kTree;
    kCheck.kArgs = kArgs;
    kCheck.sReadsText = sReadText;
    const std::vector<std::string> kFails = ReadCheck(kCheck);
    for (size_t i = 0; i < kFails.size(); ++i)
    {
        Emit(c, sOutput, kFails[i]);
    }
    if (!kFails.empty() && iCode == 0)
    {
        iCode = 1;
    }
    if (iCode == 0 && !kStep.sJudgeExe.empty())
    {
        const long iJudge = RunJudge(c, x, x.kOutputs.empty() ? std::string() : x.kOutputs[0],
                                     kRuns.empty() ? std::string() : kRuns[0].second, sOutput);
        if (iJudge != 0)
        {
            Emit(c, sOutput, "gate: judge command exited " + Number(iJudge));
            iCode = iJudge;
        }
    }
    WriteTextFile(JoinPath(sWork, "evidence\\stdout.txt"), sOutput);
    std::string sHash;
    if (!SaveEvidence(c.sEvidence, sWork, (unsigned long)iCode, kStep.sKind == "run", kRuns, x.kOutputs, sHash,
                      sError))
    {
        Say(c, "gate: ERROR: " + sError);
        iExit = 1;
        return false;
    }
    const std::string sResult = iCode == 0 ? "PASS" : "FAIL";
    const long long iWall = AddLedger(c, x, kKey, "RUN", sResult, sHash, uT0, sStart);
    AddRecord(c, x, kKey, sResult, sHash, Number(iWall));
    Say(c, "[" + sResult + "] gate:" + kStep.sName + " " + sPlatform + " exit " + Number(iCode) + " evidence=" + sHash);
    iExit = 1;
    return iCode == 0;
}

std::string OrderNames(const std::vector<const Step*>& kSteps)
{
    std::vector<std::string> kNames;
    for (size_t i = 0; i < kSteps.size(); ++i)
    {
        kNames.push_back(kSteps[i]->sName);
    }
    return JoinText(kNames, ", ");
}

ProcessResult MachineRun(const ProcessSpec& kSpec)
{
    return RunProcess(kSpec);
}

bool MachineTree(const std::string& sRoot, std::map<std::string, std::string>& kTree, std::string& sError)
{
    return TreeHashes(sRoot, GitTreeList, kTree, sError);
}

bool GitLine(const std::string& sRoot, const std::string& sRev, std::string& sOut, std::string& sError)
{
    std::vector<std::string> kArgs;
    kArgs.push_back("-C");
    kArgs.push_back(sRoot);
    kArgs.push_back("rev-parse");
    kArgs.push_back(sRev);
    const ProcessResult k = RunProcess(NewProcessSpec("git", kArgs));
    sOut = TrimText(k.sOutput);
    if (!k.bStarted || k.uCode != 0)
    {
        sError = "git rev-parse " + sRev + " failed: " + (k.bStarted ? sOut : k.sError);
        return false;
    }
    return true;
}

bool MachineRevision(const std::string& sRoot, std::string& sTree, std::string& sCommit, std::string& sError)
{
    return GitLine(sRoot, "HEAD^{tree}", sTree, sError) && GitLine(sRoot, "HEAD", sCommit, sError);
}

std::string MachineGameData()
{
    return GameDataManifestHash(GameDataRoot());
}

std::string MachineMSBuild(const std::string& sPlatform)
{
    const VsTools k = FindVsTools(sPlatform);
    return !k.sMSBuild.empty() && FileExists(k.sMSBuild) ? k.sMSBuild : std::string();
}

std::vector<std::string> MachineMissing(const std::string& sPlatform, const std::vector<std::string>& kDlls)
{
    return MissingRuntimes(SystemDllDir(sPlatform), kDlls);
}

std::vector<PreflightResult> MachinePreflight(const std::string& sRoot, const std::vector<PreflightSelection>& kSel)
{
    PreflightFacts f;
    f.sSteam = SteamRunning();
    f.sOverlay = SteamOverlayState();
    for (size_t p = 0; p < kSel.size(); ++p)
    {
        const std::string& sPlat = kSel[p].sPlatform;
        f.kPlatforms.push_back(sPlat);
        if (kSel[p].kSteps.empty())
        {
            continue;
        }
        std::vector<std::string> kGroups;
        for (size_t i = 0; i < kSel[p].kSteps.size(); ++i)
        {
            if (kSel[p].kSteps[i]->bStartsGame)
            {
                kGroups.push_back(kSel[p].kSteps[i]->sGroup.empty() ? std::string("Standard")
                                                                     : kSel[p].kSteps[i]->sGroup);
            }
        }
        if (!kGroups.empty())
        {
            f.kGamePlatforms.push_back(sPlat);
            const std::vector<std::string> kPaths = PreflightRulePaths(sRoot, kSel[p].kSteps, sPlat, ProcessorCount());
            for (size_t i = 0; i < kPaths.size(); ++i)
            {
                if (!Has(f.kRulePaths, kPaths[i]))
                {
                    f.kRulePaths.push_back(kPaths[i]);
                }
            }
        }
        const VsTools t = FindVsTools(sPlat);
        ToolchainFacts tf;
        tf.sPlatform = sPlat;
        tf.bMSBuild = !t.sMSBuild.empty() && FileExists(t.sMSBuild);
        tf.bDumpbin = !t.sBin.empty() && FileExists(JoinPath(t.sBin, "dumpbin.exe"));
        tf.sRuntimeDir = SystemDllDir(sPlat);
        tf.kMissingRuntimes = MissingRuntimes(tf.sRuntimeDir, PlatformRuntimes(GameRuntimes(sPlat, kGroups), sPlat));
        f.kToolchain.push_back(tf);
    }
    std::string sError;
    InboundAllowPrograms(f.kAllowRules, sError);
    f.kWer = WerWarnings();
    f.iFreeRamKB = FreeRamKB();
    DriveFree(sRoot, f.sRepoDrive, f.uRepoFree);
    f.sGameData = GameDataRoot();
    return PreflightVerdicts(f);
}

}

GateHooks MachineHooks()
{
    GateHooks h;
    h.pRun = MachineRun;
    h.pTree = MachineTree;
    h.pRevision = MachineRevision;
    h.pStamp = LocalStamp;
    h.pFaults = HostFaultCount;
    h.pToolchain = GetToolchainInfo;
    h.pSteam = SteamRunning;
    h.pOverlay = SteamOverlayState;
    h.pGameData = MachineGameData;
    h.pMSBuild = MachineMSBuild;
    h.pMissing = MachineMissing;
    h.pWer = WerWarnings;
    h.pWerKey = WerKeyPresent;
    h.pPreflight = MachinePreflight;
    return h;
}

GateOptions NewGateOptions()
{
    GateOptions k;
    k.bResume = false;
    k.bListOrder = false;
    k.bEcho = false;
    return k;
}

bool NormalisePlatforms(const std::vector<std::string>& kIn, std::vector<std::string>& kOut, std::string& sError)
{
    std::vector<std::string> kRaw;
    for (size_t i = 0; i < kIn.size(); ++i)
    {
        const std::vector<std::string> kParts = SplitText(kIn[i], ',');
        for (size_t j = 0; j < kParts.size(); ++j)
        {
            if (!kParts[j].empty())
            {
                kRaw.push_back(kParts[j]);
            }
        }
    }
    kOut.clear();
    for (size_t i = 0; i < kRaw.size(); ++i)
    {
        if (EqualNoCase(kRaw[i], "Win32"))
        {
            kOut.push_back("Win32");
        }
        else if (EqualNoCase(kRaw[i], "x64"))
        {
            kOut.push_back("x64");
        }
    }
    if (kOut.empty() || kOut.size() != kRaw.size())
    {
        sError = "gate: -Platform takes Win32 and x64, not '" + JoinText(kRaw, ",") + "'";
        return false;
    }
    return true;
}

bool NormaliseTier(const std::string& sIn, std::string& sOut)
{
    for (size_t i = 0; i < kGateTierCount; ++i)
    {
        if (EqualNoCase(sIn, kGateTiers[i]))
        {
            sOut = kGateTiers[i];
            return true;
        }
    }
    return false;
}

bool EvidenceInsideRepo(const std::string& sEvidence, const std::string& sRoot)
{
    const std::string sEv = TrimSlash(FullPath(sEvidence));
    const std::string sRepo = TrimSlash(FullPath(sRoot));
    return EqualNoCase(sEv, sRepo) || StartsWithNoCase(sEv, sRepo + "\\");
}

bool IsDirectStep(const Step& kStep)
{
    if (kStep.sKind != "run")
    {
        return false;
    }
    for (size_t i = 0; i < kStep.kArgs.size(); ++i)
    {
        if (kStep.kArgs[i].find("$Run") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

std::vector<const Step*> OrderSteps(const std::vector<const Step*>& kSteps, const RecordMap& kRecords,
                                    const std::string& sPlatform)
{
    std::vector<std::tuple<int, long long, size_t> > kTagged;
    for (size_t i = 0; i < kSteps.size(); ++i)
    {
        const RecordRow* pRec = FindRecord(kRecords, RecordKey(kSteps[i]->sName, sPlatform, kSteps[i]->sConfig));
        long long iWall = -1;
        if (pRec)
        {
            const std::string sWall = Field(*pRec, "Wall_s");
            if (!sWall.empty() && sWall.find_first_not_of("0123456789") == std::string::npos)
            {
                iWall = _atoi64(sWall.c_str());
            }
        }
        const int iGroup = kSteps[i]->sKind == "build" ? 0 : (iWall < 0 ? 1 : 2);
        kTagged.push_back(std::make_tuple(iGroup, iWall < 0 ? 0 : iWall, i));
    }
    std::sort(kTagged.begin(), kTagged.end());
    std::vector<const Step*> kOut;
    for (size_t i = 0; i < kTagged.size(); ++i)
    {
        kOut.push_back(kSteps[std::get<2>(kTagged[i])]);
    }
    return kOut;
}

std::vector<const Step*> SelectSteps(const StepTable& kTable, const std::string& sTier, const std::string& sPlatform,
                                     const std::vector<std::string>& kNames)
{
    std::vector<std::string> kWant;
    for (size_t i = 0; i < kNames.size(); ++i)
    {
        kWant.push_back(kNames[i]);
        const Step* pStep = FindStep(kTable, kNames[i]);
        if (pStep)
        {
            kWant.insert(kWant.end(), pStep->kRequires.begin(), pStep->kRequires.end());
        }
    }
    std::vector<const Step*> kOut;
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        const Step& s = kTable.kSteps[i];
        if (!StepOnPlatform(s, sPlatform))
        {
            continue;
        }
        if (kNames.empty() ? StepInTier(s, sTier, sPlatform) : Has(kWant, s.sName))
        {
            kOut.push_back(&s);
        }
    }
    return kOut;
}

int RunGate(const GateOptions& kOpt, const GateHooks& kHooks, std::string& sLog)
{
    Ctx c;
    c.pOpt = &kOpt;
    c.pHooks = &kHooks;
    c.pLog = &sLog;
    c.sRoot = TrimSlash(FullPath(kOpt.sRoot));
    c.sGateRoot = JoinPath(c.sRoot, "artifacts\\gate");
    c.sEvidence = TrimSlash(FullPath(kOpt.sEvidence));
    c.sRecords = JoinPath(c.sEvidence, "steps.tsv");
    c.sEvidenceLedger = JoinPath(c.sEvidence, "ledger.tsv");
    c.sJobs = Number(DefaultJobs(ProcessorCount()));
    c.iJudgeCount = 0;
    c.iWritten = 0;

    StepTable kTable;
    std::vector<std::string> kProblems;
    ParseSteps(kOpt.sStepsText, kTable, kProblems);
    ValidateSteps(kTable, kProblems);
    if (!kProblems.empty())
    {
        for (size_t i = 0; i < kProblems.size(); ++i)
        {
            Say(c, "gate: " + kProblems[i]);
        }
        return 2;
    }
    std::vector<std::string> kNames;
    if (!kOpt.kSteps.empty())
    {
        std::vector<std::string> kUnknown;
        std::vector<std::string> kNowhere;
        for (size_t i = 0; i < kOpt.kSteps.size(); ++i)
        {
            const Step* pFound = 0;
            for (size_t s = 0; s < kTable.kSteps.size() && !pFound; ++s)
            {
                if (EqualNoCase(kTable.kSteps[s].sName, kOpt.kSteps[i]))
                {
                    pFound = &kTable.kSteps[s];
                }
            }
            if (!pFound)
            {
                kUnknown.push_back(kOpt.kSteps[i]);
                continue;
            }
            kNames.push_back(pFound->sName);
            bool bSomewhere = false;
            for (size_t p = 0; p < kOpt.kPlatforms.size(); ++p)
            {
                bSomewhere = bSomewhere || StepOnPlatform(*pFound, kOpt.kPlatforms[p]);
            }
            if (!bSomewhere)
            {
                kNowhere.push_back(pFound->sName);
            }
        }
        if (!kUnknown.empty())
        {
            Say(c, "gate: no step " + JoinText(kUnknown, ", "));
            return 2;
        }
        if (!kNowhere.empty())
        {
            Say(c, "gate: step " + JoinText(kNowhere, ", ") + " is not defined on " + JoinText(kOpt.kPlatforms, ", "));
            return 2;
        }
    }
    else
    {
        bool bAny = false;
        for (size_t p = 0; p < kOpt.kPlatforms.size(); ++p)
        {
            bAny = bAny || !SelectSteps(kTable, kOpt.sTier, kOpt.kPlatforms[p], kNames).empty();
        }
        if (!bAny)
        {
            Say(c, "gate: tier " + kOpt.sTier + " has no steps on " + JoinText(kOpt.kPlatforms, ", "));
            return 2;
        }
    }
    c.kRecords = ReadStepRecordsFile(c.sRecords);
    if (kOpt.bListOrder)
    {
        for (size_t p = 0; p < kOpt.kPlatforms.size(); ++p)
        {
            const std::string& sPlat = kOpt.kPlatforms[p];
            Say(c, "gate: " + kOpt.sTier + " " + sPlat + " order " +
                       OrderNames(OrderSteps(SelectSteps(kTable, kOpt.sTier, sPlat, kNames), c.kRecords, sPlat)));
        }
        return 0;
    }
    std::vector<PreflightSelection> kSel;
    for (size_t p = 0; p < kOpt.kPlatforms.size(); ++p)
    {
        PreflightSelection k;
        k.sPlatform = kOpt.kPlatforms[p];
        k.kSteps = SelectSteps(kTable, kOpt.sTier, k.sPlatform, kNames);
        kSel.push_back(k);
    }
    const std::vector<PreflightResult> kChecks = kHooks.pPreflight(c.sRoot, kSel);
    std::vector<std::string> kFailed;
    for (size_t i = 0; i < kChecks.size(); ++i)
    {
        Say(c, PreflightLine(kChecks[i]));
        if (!kChecks[i].bPass)
        {
            kFailed.push_back(kChecks[i].sCheck);
        }
    }
    if (!kFailed.empty())
    {
        Say(c, "gate: preflight failed: " + JoinText(kFailed, ", ") + "; no step ran");
        return 2;
    }
    if (kOpt.bResume)
    {
        const std::vector<std::string> kDirs = SubDirs(c.sGateRoot);
        if (kDirs.empty() || !FileExists(JoinPath(JoinPath(c.sGateRoot, kDirs.back()), "ledger.tsv")))
        {
            Say(c, "gate: -Resume found no ledger under " + c.sGateRoot);
            return 2;
        }
        c.sStamp = kDirs.back();
    }
    else
    {
        c.sStamp = NewStampedDir(c.sGateRoot, kHooks.pStamp());
    }
    c.sRunDir = JoinPath(c.sGateRoot, c.sStamp);
    MakeDirs(c.sRunDir);
    MakeDirs(c.sEvidence);
    c.sLedger = JoinPath(c.sRunDir, "ledger.tsv");
    RecordMap kResumed;
    if (kOpt.bResume)
    {
        std::string sText;
        ReadTextFile(c.sLedger, sText);
        std::vector<std::string> kKeyCols;
        kKeyCols.push_back("Step");
        kKeyCols.push_back("Platform");
        kResumed = ReadKeyedRecords(sText, kKeyCols);
    }
    std::string sError;
    if (!kHooks.pRevision(c.sRoot, c.sTreeId, c.sCommit, sError) || !kHooks.pTree(c.sRoot, c.kTree, sError))
    {
        Say(c, "gate: " + sError);
        return 2;
    }
    c.sGameData = kHooks.pGameData();
    Say(c, "gate: tier " + kOpt.sTier + " platform " + JoinText(kOpt.kPlatforms, ",") + " run " + c.sRunDir);
    Say(c, "gate: evidence " + c.sEvidence);
    for (size_t p = 0; p < kOpt.kPlatforms.size(); ++p)
    {
        const std::string& sPlat = kOpt.kPlatforms[p];
        const ToolchainInfo kTc = kHooks.pToolchain(sPlat);
        Say(c, "gate: " + sPlat + " toolset " + KeyScalar(kTc.sToolsVersion) + " cl " + KeyScalar(kTc.sCl) + " ucrt " +
                   KeyScalar(kTc.sUcrt));
        const std::vector<const Step*> kOrder =
            OrderSteps(SelectSteps(kTable, kOpt.sTier, sPlat, kNames), c.kRecords, sPlat);
        Say(c, "gate: " + sPlat + " order " + OrderNames(kOrder));
        for (size_t i = 0; i < kOrder.size(); ++i)
        {
            int iExit = 0;
            if (!RunOneStep(c, *kOrder[i], sPlat, kTc, kResumed, iExit))
            {
                return Stop(c, iExit);
            }
        }
    }
    Say(c, kNames.empty() ? "gate: tier " + kOpt.sTier + " passed" : "gate: tier " + kOpt.sTier + " partial");
    return Stop(c, 0);
}

}
