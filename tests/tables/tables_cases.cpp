#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "gate/gate_evidence.h"
#include "gate/gate_keys.h"
#include "gate/gate_machine.h"
#include "harness/runner.h"
#include "tables/tables_build.h"
#include "tables/tables_compare.h"
#include "tables/tables_readers.h"
#include "tables/tables_verify.h"
#include "testutil.h"

namespace
{

using Tables::FileText;

FileText ReadDataFile(const std::string& sPath)
{
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return Tables::Absent();
    }
    std::string sText;
    char acBuf[65536];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    return Tables::Found(sText);
}

std::vector<std::string> ListDataFolder(const std::string& sDir)
{
    std::vector<std::string> kNames;
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + "\\*").c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return kNames;
    }
    do
    {
        if ((kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp(kFind.cFileName, ".") != 0 &&
            strcmp(kFind.cFileName, "..") != 0)
        {
            kNames.push_back(kFind.cFileName);
        }
    } while (FindNextFileA(hFind, &kFind));
    FindClose(hFind);
    std::sort(kNames.begin(), kNames.end());
    return kNames;
}

bool WriteOutFile(const std::string& sPath, const std::string& sText)
{
    Gate::MakeDirs(sPath.substr(0, sPath.find_last_of('\\')));
    FILE* pFile = fopen(sPath.c_str(), "wb");
    if (!pFile)
    {
        return false;
    }
    const bool bOk = fwrite(sText.data(), 1, sText.size(), pFile) == sText.size();
    return fclose(pFile) == 0 && bOk;
}

FileText ReadBuildFile(const std::string& sPath)
{
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return Tables::Absent();
    }
    std::string sText;
    char acBuf[65536];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    return Tables::Found(sText);
}

FileText ReadAnyFile(const std::string& sPath)
{
    const std::string sRoot = RepoRoot() + "\\";
    if (Gate::StartsWithNoCase(sPath, sRoot))
    {
        std::string sText;
        return ReadRepoBytes(sPath.substr(sRoot.size()), sText) ? Tables::Found(sText) : Tables::Absent();
    }
    return ReadDataFile(sPath);
}

std::vector<Tables::CaseLog> SmokeCases(const std::string& sDir)
{
    std::vector<Tables::CaseLog> kCases;
    const std::vector<std::string> kNames = ListDataFolder(sDir);
    for (size_t i = 0; i < kNames.size(); ++i)
    {
        Tables::CaseLog k;
        k.sName = kNames[i];
        k.kRunLog = ReadDataFile(sDir + "\\" + kNames[i] + "\\run.log");
        k.kFinalScore = ReadDataFile(sDir + "\\" + kNames[i] + "\\finalscore.log");
        kCases.push_back(k);
    }
    return kCases;
}

std::string EnvText(const char* pcName)
{
    const char* pc = getenv(pcName);
    return pc ? std::string(pc) : std::string();
}

Tables::Scrub ScrubFor()
{
    Tables::Scrub k;
    k.sRoot = RepoRoot();
    k.sProfile = EnvText("USERPROFILE");
    k.sUser = EnvText("USERNAME");
    return k;
}

bool GitOutput(const std::vector<std::string>& kArgs, std::string& sOut)
{
    std::vector<std::string> kAll;
    kAll.push_back("-C");
    kAll.push_back(RepoRoot());
    kAll.insert(kAll.end(), kArgs.begin(), kArgs.end());
    Gate::ProcessSpec kSpec = Gate::NewProcessSpec("git", kAll);
    kSpec.bMergeErr = false;
    const Gate::ProcessResult k = Gate::RunProcess(kSpec);
    sOut = k.sOutput;
    return k.bStarted && k.uCode == 0;
}

std::string HeadCommit()
{
    std::vector<std::string> kArgs;
    kArgs.push_back("rev-parse");
    kArgs.push_back("HEAD");
    std::string sOut;
    const bool bOk = GitOutput(kArgs, sOut);
    REQUIRE_MESSAGE(bOk, "git rev-parse HEAD failed: " << sOut);
    return Gate::TrimText(sOut);
}

std::string MissionsText()
{
    std::string sText;
    const bool bRead = ReadRepoText("tests\\smoke\\missions-all.txt", sText);
    REQUIRE_MESSAGE(bRead, "cannot read tests\\smoke\\missions-all.txt");
    return sText;
}

std::map<std::string, std::string> Demangle(const std::vector<std::string>& kNames)
{
    std::map<std::string, std::string> kDem;
    const std::string sUndname = Gate::JoinPath(Gate::FindVsTools(RunnerPlatform()).sBin, "undname.exe");
    const bool bTool = Gate::FileExists(sUndname);
    REQUIRE_MESSAGE(bTool, "undname.exe not found (install Visual Studio 2022 C++ build tools).");
    const std::vector<std::string> kLines = Tables::UndnameArgLines(kNames);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::vector<std::string> kArgs;
        const std::string& sLine = kLines[i];
        size_t uStart = 0;
        while (uStart < sLine.size())
        {
            const size_t uOpen = sLine.find('"', uStart);
            const size_t uClose = uOpen == std::string::npos ? std::string::npos : sLine.find('"', uOpen + 1);
            if (uClose == std::string::npos)
            {
                break;
            }
            kArgs.push_back(sLine.substr(uOpen + 1, uClose - uOpen - 1));
            uStart = uClose + 1;
        }
        Gate::ProcessSpec kSpec = Gate::NewProcessSpec(sUndname, kArgs);
        kSpec.sWorkDir = RepoRoot();
        kSpec.bMergeErr = false;
        const Gate::ProcessResult k = Gate::RunProcess(kSpec);
        Tables::ParseUndname(k.sOutput, kDem);
    }
    return kDem;
}

std::string Label(const Tables::Reader& kReader)
{
    const std::string s = kReader.pcLabel ? kReader.pcLabel : "";
    const size_t uColon = s.find(':');
    return uColon == std::string::npos ? s : s.substr(uColon + 1);
}

Tables::CompareInput CompareFor(const std::string& sTables, const std::map<std::string, std::string>& kBuilt)
{
    Tables::CompareInput k;
    k.sPlatform = RunnerPlatform();
    k.sBaseX86 = RepoRoot() + "\\tests\\baseline\\v143";
    k.sBaseX64 = RepoRoot() + "\\tests\\baseline\\v143-x64";
    k.sTables = sTables;
    const std::map<std::string, std::string> kCopy = kBuilt;
    k.fnRead = [sTables, kCopy](const std::string& sPath) {
        const std::string sPrefix = sTables + "\\";
        if (Gate::StartsWithNoCase(sPath, sPrefix))
        {
            std::map<std::string, std::string>::const_iterator it = kCopy.find(sPath.substr(sPrefix.size()));
            if (it != kCopy.end())
            {
                return Tables::Found(it->second);
            }
        }
        return ReadAnyFile(sPath);
    };
    return k;
}

void CheckVerdict(const Tables::TableVerdict& kVerdict)
{
    const std::string sText = Tables::VerdictText(kVerdict);
    printf("%s", sText.c_str());
    fflush(stdout);
    CHECK_MESSAGE(kVerdict.bOk, sText);
}

void RequireBuilt(const char* pcStep, const Tables::BuildResult& kBuild)
{
    for (size_t i = 0; i < kBuild.kInfo.size(); ++i)
    {
        printf("%s\n", kBuild.kInfo[i].c_str());
    }
    for (size_t i = 0; i < kBuild.kVerdicts.size(); ++i)
    {
        printf("%s\n", kBuild.kVerdicts[i].c_str());
    }
    fflush(stdout);
    std::string sText;
    for (size_t i = 0; i < kBuild.kVerdicts.size(); ++i)
    {
        sText += kBuild.kVerdicts[i] + "\n";
    }
    if (!kBuild.sWhy.empty())
    {
        sText += std::string("[FAIL] capture:") + pcStep + " - " + kBuild.sWhy;
    }
    REQUIRE_MESSAGE(kBuild.bOk, sText);
}

std::map<std::string, std::string> BuiltTexts(const Tables::BuildResult& kBuild)
{
    std::map<std::string, std::string> kOut;
    for (size_t i = 0; i < kBuild.kTables.size(); ++i)
    {
        std::string sText;
        std::string sWhy;
        const bool bText = Tables::BaselineText(kBuild.kTables[i].kLines, ScrubFor(), kBuild.kTables[i].sName, sText, sWhy);
        REQUIRE_MESSAGE(bText, sWhy);
        kOut[kBuild.kTables[i].sName] = sText;
    }
    return kOut;
}

void WriteIncoming(const std::string& sDir, const std::string& sName, const std::map<std::string, std::string>& kBuilt)
{
    std::map<std::string, std::string>::const_iterator it = kBuilt.find(sName);
    REQUIRE_MESSAGE(it != kBuilt.end(), sName << " was not built");
    const std::string sPath = sDir + "\\" + sName;
    const bool bWritten = WriteOutFile(sPath, it->second);
    REQUIRE_MESSAGE(bWritten, "cannot write " << sPath);
}

std::vector<Tables::ReaderKind> AllKinds()
{
    std::vector<Tables::ReaderKind> k;
    if (RunnerConfig() == "Release")
    {
        k.push_back(Tables::kReadTables);
        k.push_back(Tables::kReadMemPeaksX64);
    }
    else if (RunnerConfig() == "Test")
    {
        k.push_back(Tables::kReadLuaBins);
        k.push_back(Tables::kReadLuaHist);
        k.push_back(Tables::kReadOffThread);
    }
    return k;
}

bool AllLabels(std::vector<std::string>& kParams, std::string& sError)
{
    const std::vector<Tables::ReaderKind> kKinds = AllKinds();
    for (size_t k = 0; k < kKinds.size(); ++k)
    {
        const std::vector<Tables::Reader> kEntries = Tables::SelectReaders(kKinds[k], RunnerPlatform());
        for (size_t i = 0; i < kEntries.size(); ++i)
        {
            kParams.push_back(Label(kEntries[i]));
            if (kEntries[i].pcPair && *kEntries[i].pcPair)
            {
                kParams.push_back("heap-caps-x64");
            }
        }
    }
    if (kParams.empty())
    {
        kParams.push_back("compare");
    }
    return true;
}

bool KindLabels(Tables::ReaderKind eKind, std::vector<std::string>& kParams, std::string& sError)
{
    const std::vector<Tables::Reader> kEntries = Tables::SelectReaders(eKind, RunnerPlatform());
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        kParams.push_back(Label(kEntries[i]));
    }
    if (kParams.empty())
    {
        sError = std::string("no reader map entry for -") + Tables::KindName(eKind) + " on " + RunnerPlatform();
        return false;
    }
    return true;
}

bool CollectLabels(std::vector<std::string>& kParams, std::string& sError)
{
    return KindLabels(Tables::kReadCollect, kParams, sError);
}

bool MapLabels(std::vector<std::string>& kParams, std::string& sError)
{
    return KindLabels(Tables::kReadMap, kParams, sError);
}

bool FindEntry(Tables::ReaderKind eKind, const std::string& sLabel, Tables::Reader& kOut)
{
    const std::vector<Tables::Reader> kEntries = Tables::SelectReaders(eKind, RunnerPlatform());
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        if (Label(kEntries[i]) == sLabel || (sLabel == "heap-caps-x64" && kEntries[i].pcPair && *kEntries[i].pcPair))
        {
            kOut = kEntries[i];
            return true;
        }
    }
    return false;
}

std::string MapPath()
{
    return RepoRoot() + "\\obj\\" + RunnerPlatform() + "\\" + RunnerConfig() + "\\BattlefrontII.map";
}

void CompareFromData(Tables::ReaderKind eKind, const std::string& sTables)
{
    Tables::Reader kEntry;
    const bool bFound = FindEntry(eKind, RunnerParam(), kEntry);
    REQUIRE_MESSAGE(bFound, "no reader map entry " << RunnerParam());
    CheckVerdict(Tables::CompareEntry(kEntry, CompareFor(sTables, std::map<std::string, std::string>())));
}

void CollectFiles(const std::string& sDir, const std::string& sSub, std::vector<Tables::BaselineFile>& kFiles)
{
    std::vector<RepoEntry> kEntries;
    const bool bListed = ListRepoFolder(sSub.empty() ? sDir : sDir + "\\" + sSub, "*", kEntries);
    REQUIRE_MESSAGE(bListed, "cannot list " << sDir);
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        const std::string sRel = sSub.empty() ? kEntries[i].sName : sSub + "\\" + kEntries[i].sName;
        if (kEntries[i].bDir)
        {
            CollectFiles(sDir, sRel, kFiles);
            continue;
        }
        Tables::BaselineFile k;
        k.sRel = sRel;
        const bool bRead = ReadRepoBytes(sDir + "\\" + sRel, k.sText);
        REQUIRE_MESSAGE(bRead, "cannot read " << sDir << "\\" << sRel);
        kFiles.push_back(k);
    }
}

}

COMPLEX_TEST_CASE_NAMED(FTablesAllTest, "Tables::All", "[EngineFilter][All][AfterAll]", AllLabels)
{
    const std::string sLabel = RunnerParam();
    if (sLabel == "compare")
    {
        SkipCase(("smoke configuration is " + RunnerConfig()).c_str());
    }
    const std::vector<Tables::ReaderKind> kKinds = AllKinds();
    Tables::Reader kEntry;
    bool bFound = false;
    for (size_t i = 0; i < kKinds.size() && !bFound; ++i)
    {
        bFound = FindEntry(kKinds[i], sLabel, kEntry);
    }
    REQUIRE_MESSAGE(bFound, "no reader map entry " << sLabel << " on " << RunnerPlatform() << " " << RunnerConfig());
    const std::string sSmoke = RunnerData() + "\\smoke";
    const std::vector<Tables::CaseLog> kCases = SmokeCases(sSmoke);
    std::map<std::string, std::string> kBuilt;
    std::string sTables;
    if (RunnerConfig() == "Release")
    {
        sTables = RunnerOut() + "\\tables";
        const Tables::BuildResult kBuild = Tables::BuildSmokeAllTables(kCases, MissionsText(), HeadCommit());
        RequireBuilt("tables", kBuild);
        kBuilt = BuiltTexts(kBuild);
    }
    else if (kEntry.eKind == Tables::kReadOffThread)
    {
        sTables = RunnerOut() + "\\offthread";
        const std::string sMap = MapPath();
        const FileText kMap = ReadBuildFile(sMap);
        REQUIRE_MESSAGE(kMap.bFound, "[FAIL] capture:tables - map not found: " << sMap);
        Tables::MapSymbols kSym;
        std::string sWhy;
        const bool bSym = Tables::ReadMapSymbols(kMap.sText, kSym, sWhy);
        REQUIRE_MESSAGE(bSym, "[FAIL] capture:tables - " << sWhy);
        std::vector<Tables::OffRow> kRows = Tables::OffThreadHeader(HeadCommit());
        const bool bRows = Tables::OffThreadRows(kCases, sSmoke, kSym, kRows, sWhy);
        REQUIRE_MESSAGE(bRows, "[FAIL] capture:tables - " << sWhy);
        const std::vector<std::string> kLines =
            Tables::FormatOffThreadRows(kRows, Demangle(Tables::OffThreadNames(kRows)));
        Tables::BuildResult kBuild;
        kBuild.bOk = true;
        Tables::BuiltTable kTable;
        kTable.sName = "offthread.txt";
        kTable.kLines = kLines;
        kBuild.kTables.push_back(kTable);
        kBuilt = BuiltTexts(kBuild);
    }
    else
    {
        sTables = RunnerOut() + "\\lua-bins";
        const Tables::BuildResult kBuild = Tables::BuildLuaTables(kCases, MissionsText(), HeadCommit());
        RequireBuilt("tables", kBuild);
        kBuilt = BuiltTexts(kBuild);
    }
    const bool bShared = sLabel == "mem-peaks-x64" || sLabel == "heap-caps-x64";
    if (!bShared)
    {
        WriteIncoming(sTables, kEntry.pcFile, kBuilt);
    }
    const Tables::CompareInput kIn = CompareFor(sTables, kBuilt);
    CheckVerdict(sLabel == "heap-caps-x64" ? Tables::CompareHeapCaps(kEntry, kIn) : Tables::CompareEntry(kEntry, kIn));
}

COMPLEX_TEST_CASE_NAMED(FTablesCollectTest, "Tables::Collect", "[EngineFilter][Capture][AfterAll]", CollectLabels)
{
    Tables::Reader kEntry;
    const bool bFound = FindEntry(Tables::kReadCollect, RunnerParam(), kEntry);
    REQUIRE_MESSAGE(bFound, "no reader map entry " << RunnerParam());
    const Tables::BuildResult kBuild = Tables::BuildCollect(SmokeCases(RunnerData() + "\\smoke"), 0);
    RequireBuilt("collect", kBuild);
    const std::map<std::string, std::string> kBuilt = BuiltTexts(kBuild);
    const std::string sTables = RunnerOut() + "\\collect";
    WriteIncoming(sTables, kEntry.pcFile, kBuilt);
    CheckVerdict(Tables::CompareEntry(kEntry, CompareFor(sTables, kBuilt)));
}

TEST_CASE_NAMED(FTablesCaptureMapTest, "Tables::Capture::map", "[EngineFilter][Map][OnRequest]")
{
    const FileText kMap = ReadBuildFile(MapPath());
    REQUIRE_MESSAGE(kMap.bFound, "[FAIL] capture:map - map not found: obj\\" << RunnerPlatform() << "\\" << RunnerConfig()
                                                                            << "\\BattlefrontII.map");
    Tables::MapTables kTables;
    std::string sWhy;
    const bool bMap = Tables::MapInitializers(kMap.sText, kTables, sWhy);
    REQUIRE_MESSAGE(bMap, "[FAIL] capture:map - " << sWhy);
    const std::map<std::string, std::string> kDem = Demangle(Tables::MapNames(kTables));
    Tables::BuildResult kBuild;
    kBuild.bOk = true;
    const char* const apcNames[] = {"initializers.txt", "initializers-discarded.txt", "initializers-di.txt"};
    const std::vector<Tables::MapRow>* const apRows[] = {&kTables.kInRange, &kTables.kDiscarded, &kTables.kDi};
    for (size_t i = 0; i < 3; ++i)
    {
        Tables::BuiltTable kTable;
        kTable.sName = apcNames[i];
        kTable.kLines = Tables::FormatMapRows(*apRows[i], kDem);
        kBuild.kTables.push_back(kTable);
    }
    const std::map<std::string, std::string> kBuilt = BuiltTexts(kBuild);
    for (size_t i = 0; i < 3; ++i)
    {
        WriteIncoming(RunnerOut() + "\\map", apcNames[i], kBuilt);
    }
    printf("%s\n", Tables::MapInfoLine(kTables, kDem.size()).c_str());
    fflush(stdout);
}

COMPLEX_TEST_CASE_NAMED(FTablesMapTest, "Tables::Map", "[EngineFilter][Map][AfterAll]", MapLabels)
{
    CompareFromData(Tables::kReadMap, RunnerData() + "\\map");
}

TEST_CASE_NAMED(FTablesDebugDigestsTest, "Tables::DebugDigests", "[EngineFilter][Twice][AfterAll]")
{
    if (RunnerConfig() != "Debug")
    {
        SkipCase(("the debug digests come from a Debug Twice run; this run is " + RunnerConfig()).c_str());
    }
    std::string sSmoke = RunnerData();
    if (Gate::DirExists(sSmoke + "\\smoke"))
    {
        sSmoke += "\\smoke";
    }
    const Tables::BuildResult kBuild = Tables::BuildDebugDigests(SmokeCases(sSmoke));
    RequireBuilt("debugdigests", kBuild);
    const std::map<std::string, std::string> kBuilt = BuiltTexts(kBuild);
    const std::string sTables = RunnerOut() + "\\debug-digests";
    WriteIncoming(sTables, "digests-debug.txt", kBuilt);
    const std::vector<Tables::Reader> kEntries = Tables::SelectReaders(Tables::kReadDebugDigests, RunnerPlatform());
    REQUIRE(kEntries.size() == 1);
    CheckVerdict(Tables::CompareEntry(kEntries[0], CompareFor(sTables, kBuilt)));
}

namespace
{

void VerifyBaseline(const std::string& sFolder)
{
    const std::string sDir = "tests\\baseline\\" + sFolder;
    std::string sManifest;
    const FileText kManifest =
        ReadRepoBytes(sDir + "\\MANIFEST.md", sManifest) ? Tables::Found(sManifest) : Tables::Absent();
    std::vector<Tables::BaselineFile> kFiles;
    CollectFiles(sDir, std::string(), kFiles);
    std::vector<std::string> kArgs;
    kArgs.push_back("ls-files");
    kArgs.push_back("--");
    kArgs.push_back("bin");
    kArgs.push_back("obj");
    std::string sTracked;
    const bool bGit = GitOutput(kArgs, sTracked);
    REQUIRE_MESSAGE(bGit, "git ls-files -- bin obj failed: " << sTracked);
    const Tables::VerifyResult k = Tables::ManifestVerify(sFolder, kManifest, kFiles, EnvText("USERNAME"), sTracked);
    std::string sText;
    for (size_t i = 0; i < k.kLines.size(); ++i)
    {
        printf("%s\n", k.kLines[i].c_str());
        sText += k.kLines[i] + "\n";
    }
    fflush(stdout);
    CHECK_MESSAGE(k.bOk, sText);
}

}

TEST_CASE_NAMED(FBaselineVerifyTest1, "Baseline::Verify::v143", "[EngineFilter][Verify][OnRequest]")
{
    VerifyBaseline("v143");
}

TEST_CASE_NAMED(FBaselineVerifyTest2, "Baseline::Verify::v143-x64", "[EngineFilter][Verify][OnRequest]")
{
    VerifyBaseline("v143-x64");
}
