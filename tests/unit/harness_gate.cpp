#include <windows.h>
#include <stdio.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "third_party/doctest/doctest.h"
#include "harness/tags.h"
#include "testutil.h"

#include "gate/gate_run.h"

using namespace Gate;

namespace
{

const char* const kStubTable = "[Step build]\r\n"
                               "Tiers=plan phase\r\n"
                               "Kind=build\r\n"
                               "Config=Debug\r\n"
                               "Project=build\r\n"
                               "\r\n"
                               "[Step fail-c]\r\n"
                               "Tiers=plan\r\n"
                               "Kind=run\r\n"
                               "Config=Debug\r\n"
                               "Requires=build\r\n"
                               "Exe=stub\r\n"
                               "Arg=fail-c\r\n"
                               "Arg=direct\r\n"
                               "Arg=--out=$Run\r\n"
                               "Reads=fail\\\r\n"
                               "\r\n"
                               "[Step run-a]\r\n"
                               "Tiers=plan\r\n"
                               "Kind=run\r\n"
                               "Config=Debug\r\n"
                               "Requires=build\r\n"
                               "Exe=stub\r\n"
                               "Arg=run-a\r\n"
                               "Arg=direct\r\n"
                               "Arg=--out=$Run\r\n"
                               "Reads=src\\a.cpp\r\n"
                               "\r\n"
                               "[Step run-b]\r\n"
                               "Tiers=phase\r\n"
                               "Kind=run\r\n"
                               "Config=Debug\r\n"
                               "Exe=stub\r\n"
                               "Arg=run-b\r\n"
                               "Arg=direct\r\n"
                               "Arg=--out=$Run\r\n"
                               "Reads=src\\\r\n"
                               "\r\n"
                               "[Step corpus]\r\n"
                               "Tiers=plan\r\n"
                               "Kind=run\r\n"
                               "Config=Debug\r\n"
                               "Requires=build\r\n"
                               "Exe=stub\r\n"
                               "Arg=corpus\r\n"
                               "Arg=direct\r\n"
                               "Arg=--out=$Run\r\n"
                               "Sources=src\\movie\\ tests\\unit\\movie_corpus.cpp\r\n"
                               "Tlog=artifacts\\tl\r\n"
                               "Reads=src\\movie\\\r\n";

struct Stub
{
    std::string sRoot;
    std::map<std::string, std::string> kTree;
    bool bRealTree;
    std::vector<std::string> kMarks;
    std::string sFail;
    std::string sBadRun;
    std::string sBadIndex;
    std::string sBadUnit;
    std::string sReadPath;
    long iJudgeCode;
    std::vector<std::string> kJudgeOuts;
    std::vector<ProcessSpec> kSpecs;
    int iStamp;
};

Stub& S()
{
    static Stub ss;
    return ss;
}

std::string Leaf(const std::string& sPath)
{
    const size_t uSlash = sPath.find_last_of('\\');
    return uSlash == std::string::npos ? sPath : sPath.substr(uSlash + 1);
}

std::string EnvOf(const ProcessSpec& kSpec, const char* pcName)
{
    for (size_t i = 0; i < kSpec.kEnv.size(); ++i)
    {
        if (kSpec.kEnv[i].first == pcName)
        {
            return kSpec.kEnv[i].second;
        }
    }
    return std::string();
}

std::string ArgAfter(const std::vector<std::string>& kArgs, const std::string& sPrefix)
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

ProcessResult StubRun(const ProcessSpec& kSpec)
{
    Stub& s = S();
    s.kSpecs.push_back(kSpec);
    ProcessResult r;
    r.bStarted = true;
    r.uCode = 0;
    if (kSpec.sExe == "msbuild")
    {
        const std::string sName = Leaf(kSpec.kArgs[0]);
        s.kMarks.push_back(sName);
        r.uCode = s.sFail == sName ? 1 : 0;
        r.sOutput = "msbuild " + sName + "\r\n";
        return r;
    }
    if (kSpec.sExe == "judge")
    {
        s.kJudgeOuts.push_back(kSpec.kArgs.empty() ? std::string() : kSpec.kArgs[0]);
        r.uCode = (unsigned long)s.iJudgeCode;
        r.sOutput = "judge\r\n";
        return r;
    }
    const std::string sName = kSpec.kArgs[0];
    const std::string sKind = kSpec.kArgs[1];
    s.kMarks.push_back(sName);
    if (sKind == "direct")
    {
        const std::string sOut = ArgAfter(kSpec.kArgs, "--out=");
        WriteTextFile(JoinPath(sOut, "index.json"),
                      std::string("{ \"failed\": ") + (s.sBadIndex == sName ? "1" : "0") +
                          ", \"tests\": [ { \"fullTestPath\": \"Unit::Stub::one\", \"state\": \"Success\" } ] }\r\n");
        if (s.sBadUnit == sName)
        {
            MakeDirs(JoinPath(sOut, "unit"));
            WriteTextFile(JoinPath(sOut, "unit\\index.json"), "{ \"failed\": 1, \"tests\": [] }\r\n");
        }
        r.sOutput = std::string(s.sBadRun == sName ? "[FAIL] " : "[PASS] ") + "Unit::Stub::one\r\n";
    }
    else if (sKind == "capture")
    {
        MakeDirs(kSpec.kArgs[2]);
        WriteTextFile(JoinPath(kSpec.kArgs[2], "t.txt"), sName + "\r\n");
    }
    else if (sKind == "read")
    {
        AppendTextFile(EnvOf(kSpec, "OPENSWBF2_READS"), JoinPath(s.sRoot, kSpec.kArgs[2]) + "\r\n");
    }
    if (s.sFail == sName)
    {
        r.uCode = 1;
        r.sOutput += "stub " + sName + " fails\r\n";
        return r;
    }
    r.sOutput += "stub " + sName + "\r\n";
    return r;
}

bool StubTree(const std::string& sRoot, std::map<std::string, std::string>& kTree, std::string& sError)
{
    if (S().bRealTree)
    {
        return TreeHashes(sRoot, GitTreeList, kTree, sError);
    }
    kTree = S().kTree;
    return true;
}

bool StubRevision(const std::string& sRoot, std::string& sTree, std::string& sCommit, std::string& sError)
{
    sTree = "stub-tree";
    sCommit = "stub-commit";
    return true;
}

std::string StubStamp()
{
    char acText[32];
    _snprintf(acText, sizeof(acText), "20000101-%06d", ++S().iStamp);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

bool StubFaults(bool bSince, unsigned long long uSince, long long& iCount)
{
    iCount = 0;
    return true;
}

ToolchainInfo StubToolchain(const std::string& sPlatform)
{
    ToolchainInfo k;
    k.bOk = true;
    k.sBanner = "stub compiler";
    k.sCl = "stub-cl";
    k.sToolsVersion = "stub-tools";
    k.sUcrt = "stub-ucrt";
    k.sWindows = "stub-windows";
    return k;
}

std::string StubYes()
{
    return "yes";
}

std::string StubOff()
{
    return "off";
}

std::string StubGameData()
{
    return "stub-gamedata";
}

std::string StubMSBuild(const std::string& sPlatform)
{
    return "msbuild";
}

std::vector<std::string> StubMissing(const std::string& sPlatform, const std::vector<std::string>& kDlls)
{
    return std::vector<std::string>();
}

std::vector<std::string> StubWer()
{
    return std::vector<std::string>();
}

bool StubWerKey()
{
    return true;
}

std::vector<PreflightResult> StubPreflight(const std::string& sRoot, const std::vector<PreflightSelection>& kSel)
{
    return std::vector<PreflightResult>();
}

GateHooks StubHooks()
{
    GateHooks h;
    h.pRun = StubRun;
    h.pTree = StubTree;
    h.pRevision = StubRevision;
    h.pStamp = StubStamp;
    h.pFaults = StubFaults;
    h.pToolchain = StubToolchain;
    h.pSteam = StubYes;
    h.pOverlay = StubOff;
    h.pGameData = StubGameData;
    h.pMSBuild = StubMSBuild;
    h.pMissing = StubMissing;
    h.pWer = StubWer;
    h.pWerKey = StubWerKey;
    h.pPreflight = StubPreflight;
    return h;
}

struct GateRun
{
    int iCode;
    std::string sLog;
    std::vector<std::string> kMarks;
};

void WriteUtf16(const std::string& sPath, const std::vector<std::string>& kLines)
{
    std::wstring sWide;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        sWide += Utf8ToWide(kLines[i]) + L"\r\n";
    }
    std::string sBytes("\xFF\xFE", 2);
    for (size_t i = 0; i < sWide.size(); ++i)
    {
        sBytes += (char)(sWide[i] & 0xFF);
        sBytes += (char)((sWide[i] >> 8) & 0xFF);
    }
    MakeDirs(sPath.substr(0, sPath.find_last_of('\\')));
    WriteTextFile(sPath, sBytes);
}

void SetFileAge(const std::string& sPath, long long iMinutes)
{
    HANDLE h = CreateFileW(Utf8ToWide(sPath).c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        return;
    }
    FILETIME kNow;
    GetSystemTimeAsFileTime(&kNow);
    unsigned long long uTime = ((unsigned long long)kNow.dwHighDateTime << 32) | kNow.dwLowDateTime;
    uTime = (unsigned long long)((long long)uTime + iMinutes * 60ll * 10000000ll);
    FILETIME kSet;
    kSet.dwLowDateTime = (DWORD)uTime;
    kSet.dwHighDateTime = (DWORD)(uTime >> 32);
    SetFileTime(h, NULL, NULL, &kSet);
    CloseHandle(h);
}

std::string RunGit(const std::string& sDir, const std::vector<std::string>& kArgs)
{
    std::vector<std::string> kAll;
    kAll.push_back("-C");
    kAll.push_back(sDir);
    kAll.push_back("-c");
    kAll.push_back("user.name=gatetests");
    kAll.push_back("-c");
    kAll.push_back("user.email=gatetests@invalid");
    kAll.push_back("-c");
    kAll.push_back("core.autocrlf=false");
    kAll.insert(kAll.end(), kArgs.begin(), kArgs.end());
    const ProcessResult k = RunProcess(NewProcessSpec("git", kAll));
    return k.bStarted && k.uCode == 0 ? std::string() : "git failed: " + k.sOutput + k.sError;
}

struct Fixture
{
    std::string sDir;
    std::string sRoot;
    std::string sEvidence;
    std::string sTable;

    explicit Fixture(bool bGit = false)
    {
        wchar_t acTemp[MAX_PATH];
        GetTempPathW(MAX_PATH, acTemp);
        char acTail[96];
        _snprintf(acTail, sizeof(acTail), "openswbf2-gaterun-%lu-%llu", GetCurrentProcessId(),
                  (unsigned long long)GetTickCount64());
        acTail[sizeof(acTail) - 1] = 0;
        sDir = FullPath(JoinPath(WideToUtf8(acTemp), acTail));
        sRoot = JoinPath(sDir, "repo");
        sEvidence = JoinPath(sDir, "evidence");
        sTable = kStubTable;
        MakeDirs(sRoot);
        Stub& s = S();
        s = Stub();
        s.sRoot = sRoot;
        s.bRealTree = bGit;
        s.iJudgeCode = 0;
        s.iStamp = 0;
        Put(".gitignore", "artifacts/\n");
        Put("src\\a.cpp", "int a;\r\n");
        Put("src\\movie\\dec.c", "int dec;\r\n");
        Put("src\\common\\shared.h", "int shared;\r\n");
        Put("src\\common\\unrelated.h", "int unrelated;\r\n");
        Put("tests\\unit\\movie_corpus.cpp", "int corpus;\r\n");
        Put("tests\\unit\\other.cpp", "int other;\r\n");
        Put("fail\\c.txt", "c\r\n");
        WriteTlogs();
        if (bGit)
        {
            const char* const kSteps[][3] = {{"init", "-q", 0}, {"add", "-A", 0}, {"commit", "-q", "-m"}};
            for (size_t i = 0; i < 3; ++i)
            {
                std::vector<std::string> kArgs;
                for (size_t j = 0; j < 3 && kSteps[i][j]; ++j)
                {
                    kArgs.push_back(kSteps[i][j]);
                }
                if (i == 2)
                {
                    kArgs.push_back("gatetests");
                }
                const std::string sGit = RunGit(sRoot, kArgs);
                REQUIRE_MESSAGE(sGit.empty(), sGit);
            }
        }
    }

    ~Fixture()
    {
        std::string sError;
        RemoveTree(sDir, sError);
    }

    void Put(const std::string& sRel, const std::string& sText)
    {
        const std::string sPath = JoinPath(sRoot, sRel);
        MakeDirs(sPath.substr(0, sPath.find_last_of('\\')));
        WriteTextFile(sPath, sText);
        if (sRel != ".gitignore")
        {
            S().kTree[LowerText(sRel)] = TextSha(sText);
        }
    }

    void Change(const std::string& sRel, const std::string& sHash)
    {
        S().kTree[LowerText(sRel)] = sHash;
    }

    std::string Tlog() const
    {
        return JoinPath(sRoot, "artifacts\\tl");
    }

    void WriteTlogs()
    {
        const std::string u = UpperText(sRoot);
        std::vector<std::string> kRead;
        kRead.push_back("^" + u + "\\TESTS\\UNIT\\MOVIE_CORPUS.CPP");
        kRead.push_back(u + "\\TESTS\\UNIT\\MOVIE_CORPUS.CPP");
        kRead.push_back(u + "\\SRC\\COMMON\\SHARED.H");
        kRead.push_back("C:\\OUTSIDE\\SDK.H");
        kRead.push_back("^" + u + "\\TESTS\\UNIT\\OTHER.CPP");
        kRead.push_back(u + "\\SRC\\COMMON\\UNRELATED.H");
        WriteUtf16(JoinPath(Tlog(), "CL.read.1.tlog"), kRead);
        std::vector<std::string> kCmd;
        kCmd.push_back("^" + u + "\\TESTS\\UNIT\\MOVIE_CORPUS.CPP");
        kCmd.push_back("/c /Zi /O2 " + u + "\\TESTS\\UNIT\\MOVIE_CORPUS.CPP");
        kCmd.push_back("^" + u + "\\TESTS\\UNIT\\OTHER.CPP");
        kCmd.push_back("/c /Zi /O2 " + u + "\\TESTS\\UNIT\\OTHER.CPP");
        WriteUtf16(JoinPath(Tlog(), "CL.command.1.tlog"), kCmd);
        SetFileAge(JoinPath(Tlog(), "CL.read.1.tlog"), 1);
    }

    GateRun Gate(const std::string& sTier, const std::string& sSteps = std::string(), bool bResume = false,
                 const std::vector<std::string>& kPlatforms = std::vector<std::string>(1, "Win32"),
                 bool bListOrder = false)
    {
        S().kMarks.clear();
        GateOptions kOpt = NewGateOptions();
        kOpt.sRoot = sRoot;
        kOpt.sEvidence = sEvidence;
        kOpt.sStepsText = sTable;
        kOpt.sTier = sTier;
        kOpt.kPlatforms = kPlatforms;
        const std::vector<std::string> kNames = SplitText(sSteps, ',');
        for (size_t i = 0; i < kNames.size(); ++i)
        {
            if (!kNames[i].empty())
            {
                kOpt.kSteps.push_back(kNames[i]);
            }
        }
        kOpt.bResume = bResume;
        kOpt.bListOrder = bListOrder;
        GateRun r;
        r.iCode = RunGate(kOpt, StubHooks(), r.sLog);
        r.kMarks = S().kMarks;
        std::sort(r.kMarks.begin(), r.kMarks.end());
        return r;
    }

    std::string Result(const std::string& sStep, const char* pcField = "Result") const
    {
        const RecordMap k = ReadStepRecordsFile(JoinPath(sEvidence, "steps.tsv"));
        RecordMap::const_iterator it = k.find(sStep + "\tWin32\tDebug");
        if (it == k.end())
        {
            return std::string();
        }
        RecordRow::const_iterator f = it->second.find(pcField);
        return f == it->second.end() ? std::string() : f->second;
    }
};

bool HasLine(const GateRun& r, const std::string& sStart)
{
    const std::vector<std::string> kLines = SplitLines(r.sLog);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (kLines[i].compare(0, sStart.size(), sStart) == 0)
        {
            return true;
        }
    }
    return false;
}

bool Marked(const GateRun& r, const std::string& sName)
{
    return std::find(r.kMarks.begin(), r.kMarks.end(), sName) != r.kMarks.end();
}

std::string Marks(const GateRun& r)
{
    return JoinText(r.kMarks, ",");
}

void CheckRan(const GateRun& r, const std::string& sName, int iCode = 0)
{
    CHECK_MESSAGE(r.iCode == iCode, r.sLog);
    CHECK_MESSAGE(Marked(r, sName), r.sLog);
    CHECK_MESSAGE(!HasLine(r, "[CARRY] gate:" + sName + " "), r.sLog);
}

void CheckCarried(const GateRun& r, const std::string& sName)
{
    CHECK_MESSAGE(r.iCode == 0, r.sLog);
    CHECK_MESSAGE(!Marked(r, sName), r.sLog);
    CHECK_MESSAGE(HasLine(r, "[CARRY] gate:" + sName + " "), r.sLog);
}

void FullRun(Fixture& f)
{
    const GateRun r = f.Gate("plan");
    REQUIRE_MESSAGE(r.iCode == 0, r.sLog);
    REQUIRE_MESSAGE(HasLine(r, "gate: tier plan passed"), r.sLog);
}

std::vector<std::string> Names(const std::vector<const Step*>& kSteps)
{
    std::vector<std::string> k;
    for (size_t i = 0; i < kSteps.size(); ++i)
    {
        k.push_back(kSteps[i]->sName);
    }
    return k;
}

}

TEST_CASE_NAMED(FGateRunTest1, "Unit::Gate::a step with no Reads stops the run with exit 2", "[EngineFilter]")
{
    Fixture f;
    f.sTable = "[Step build]\r\nTiers=plan\r\nKind=build\r\nConfig=Debug\r\nProject=build\r\n\r\n"
               "[Step bare]\r\nTiers=plan\r\nKind=run\r\nConfig=Debug\r\nExe=stub\r\nArg=bare\r\nArg=run\r\n";
    const GateRun r = f.Gate("plan");
    CHECK(r.iCode == 2);
    CHECK_MESSAGE(r.kMarks.empty(), r.sLog);
    CHECK_MESSAGE(HasLine(r, "gate: bare: no Reads"), r.sLog);
}

TEST_CASE_NAMED(FGateRunTest2, "Unit::Gate::the first failure stops the run and the step re-runs alone", "[EngineFilter]")
{
    Fixture f;
    S().sFail = "fail-c";
    const GateRun r = f.Gate("plan");
    CHECK_MESSAGE(r.iCode == 1, r.sLog);
    CHECK_MESSAGE(Marks(r) == "build,fail-c", r.sLog);
    CHECK_MESSAGE(HasLine(r, "[FAIL] gate:fail-c "), r.sLog);
    CHECK(f.Result("fail-c") == "FAIL");
    S().sFail.clear();
    const GateRun a = f.Gate("plan", "fail-c");
    CHECK_MESSAGE(a.iCode == 0, a.sLog);
    CHECK_MESSAGE(Marks(a) == "build,fail-c", a.sLog);
    CHECK_MESSAGE(HasLine(a, "gate: tier plan partial"), a.sLog);
    CHECK(f.Result("fail-c") == "PASS");
}

TEST_CASE_NAMED(FGateRunTest3, "Unit::Gate::an unchanged tree carries every step", "[EngineFilter]")
{
    Fixture f(true);
    FullRun(f);
    CHECK(!f.Result("run-a").empty());
    CHECK(!f.Result("corpus").empty());
    const GateRun r = f.Gate("plan");
    CheckCarried(r, "run-a");
    CheckCarried(r, "corpus");
    CheckCarried(r, "fail-c");
    CHECK(Marks(r) == "build");
}

TEST_CASE_NAMED(FGateRunTest4, "Unit::Gate::a changed input runs only its step", "[EngineFilter]")
{
    Fixture f;
    FullRun(f);
    f.Change("src\\a.cpp", "changed");
    const GateRun r = f.Gate("plan");
    CheckRan(r, "run-a");
    CheckCarried(r, "corpus");
    CheckCarried(r, "fail-c");
}

TEST_CASE_NAMED(FGateRunTest5, "Unit::Gate::a carried step whose evidence records a failure runs again", "[EngineFilter]")
{
    Fixture f;
    FullRun(f);
    std::string* pBad = 0;
    SUBCASE("results.txt")
    {
        pBad = &S().sBadRun;
    }
    SUBCASE("index.json")
    {
        pBad = &S().sBadIndex;
    }
    SUBCASE("unit\\index.json")
    {
        pBad = &S().sBadUnit;
    }
    REQUIRE(pBad);
    f.Change("src\\a.cpp", "bad");
    *pBad = "run-a";
    const GateRun b = f.Gate("plan");
    CheckRan(b, "run-a");
    pBad->clear();
    const GateRun n = f.Gate("plan");
    CheckRan(n, "run-a");
    CHECK_MESSAGE(HasLine(n, "gate: run-a Win32 run (evidence judged FAIL: run "), n.sLog);
}

TEST_CASE_NAMED(FGateRunTest6, "Unit::Gate::a change no step reads carries", "[EngineFilter]")
{
    Fixture f;
    FullRun(f);
    REQUIRE(f.Gate("phase", "run-b").iCode == 0);
    f.Change(".planning\\phase\\note.md", "note");
    f.Change("notes.md", "notes");
    const GateRun r = f.Gate("plan");
    CheckCarried(r, "run-a");
    CheckCarried(r, "corpus");
    CheckCarried(r, "fail-c");
    CheckCarried(f.Gate("phase", "run-b"), "run-b");
    f.Change("src\\other.cpp", "other");
    const GateRun o = f.Gate("plan");
    CheckCarried(o, "run-a");
    CheckCarried(o, "corpus");
    CheckCarried(o, "fail-c");
}

TEST_CASE_NAMED(FGateRunTest7, "Unit::Gate::a missing record runs the step", "[EngineFilter]")
{
    Fixture f;
    FullRun(f);
    DeleteFileW(Utf8ToWide(JoinPath(f.sEvidence, "steps.tsv")).c_str());
    const GateRun r = f.Gate("plan");
    CheckRan(r, "run-a");
    CheckRan(r, "corpus");
    CHECK_MESSAGE(HasLine(r, "gate: run-a Win32 run (no record with this RunKey)"), r.sLog);
}

TEST_CASE_NAMED(FGateRunTest8, "Unit::Gate::missing evidence runs the step", "[EngineFilter]")
{
    Fixture f;
    FullRun(f);
    const std::string sGone = f.Result("run-a", "Evidence");
    std::string sError;
    REQUIRE(RemoveTree(JoinPath(f.sEvidence, sGone), sError));
    const GateRun r = f.Gate("plan");
    CheckRan(r, "run-a");
    CHECK_MESSAGE(r.sLog.find("evidence " + sGone + " is missing") != std::string::npos, r.sLog);
    CheckCarried(r, "corpus");
}

TEST_CASE_NAMED(FGateRunTest9, "Unit::Gate::Step runs a step of another tier and its record carries", "[EngineFilter]")
{
    Fixture f;
    const GateRun p = f.Gate("phase", "run-b");
    CHECK_MESSAGE(p.iCode == 0, p.sLog);
    CHECK_MESSAGE(Marks(p) == "run-b", p.sLog);
    CHECK_MESSAGE(HasLine(p, "gate: tier phase partial"), p.sLog);
    const GateRun w = f.Gate("plan", "run-b");
    CheckCarried(w, "run-b");
    CHECK(w.kMarks.empty());
    f.Gate("phase", "run-a");
    f.Change("src\\a.cpp", "c");
    const GateRun s = f.Gate("phase", "run-a");
    CheckRan(s, "run-a");
    CheckCarried(f.Gate("plan"), "run-a");
}

TEST_CASE_NAMED(FGateRunTest10, "Unit::Gate::Step takes a build step and refuses an unknown name", "[EngineFilter]")
{
    Fixture f;
    const GateRun b = f.Gate("phase", "build");
    CHECK_MESSAGE(b.iCode == 0, b.sLog);
    CHECK_MESSAGE(Marks(b) == "build", b.sLog);
    CHECK_MESSAGE(HasLine(b, "gate: tier phase partial"), b.sLog);
    const GateRun u = f.Gate("plan", "nope");
    CHECK(u.iCode == 2);
    CHECK(u.kMarks.empty());
    CHECK_MESSAGE(HasLine(u, "gate: no step nope"), u.sLog);
}

TEST_CASE_NAMED(FGateRunTest11, "Unit::Gate::Platform takes Win32 and x64 only", "[EngineFilter]")
{
    Fixture f;
    std::vector<std::string> kOut;
    std::string sError;
    REQUIRE(NormalisePlatforms(std::vector<std::string>(1, "Win32,x64"), kOut, sError));
    CHECK(JoinText(kOut, ",") == "Win32,x64");
    const GateRun r = f.Gate("plan", std::string(), false, kOut, true);
    CHECK_MESSAGE(r.iCode == 0, r.sLog);
    CHECK_MESSAGE(HasLine(r, "gate: plan Win32 order build"), r.sLog);
    CHECK_MESSAGE(HasLine(r, "gate: plan x64 order build"), r.sLog);
    CHECK(r.kMarks.empty());
    CHECK(NormalisePlatforms(std::vector<std::string>(1, "win32"), kOut, sError));
    CHECK(JoinText(kOut, ",") == "Win32");
    CHECK_FALSE(NormalisePlatforms(std::vector<std::string>(1, "Win32,arm"), kOut, sError));
    CHECK(sError == "gate: -Platform takes Win32 and x64, not 'Win32,arm'");
    CHECK_FALSE(NormalisePlatforms(std::vector<std::string>(1, ","), kOut, sError));
}

TEST_CASE_NAMED(FGateRunTest12, "Unit::Gate::source key", "[EngineFilter]")
{
    Fixture f;
    FullRun(f);
    SUBCASE("carried")
    {
        CheckCarried(f.Gate("plan"), "corpus");
    }
    SUBCASE("a TU outside the sources")
    {
        f.Change("src\\common\\unrelated.h", "u2");
        f.Change("tests\\unit\\other.cpp", "o2");
        CheckCarried(f.Gate("plan"), "corpus");
    }
    SUBCASE("a listed source")
    {
        f.Change("src\\movie\\dec.c", "d2");
        CheckRan(f.Gate("plan"), "corpus");
    }
    SUBCASE("a header in the closure")
    {
        f.Change("src\\common\\shared.h", "s2");
        CheckRan(f.Gate("plan"), "corpus");
    }
    SUBCASE("a stale tlog")
    {
        SetFileAge(JoinPath(f.Tlog(), "CL.read.1.tlog"), -60);
        const GateRun r = f.Gate("plan");
        CheckRan(r, "corpus");
        CHECK_MESSAGE(HasLine(r, "gate: corpus Win32 run (key invalid: tlog)"), r.sLog);
    }
    SUBCASE("a missing record")
    {
        REQUIRE(!f.Result("corpus").empty());
        REQUIRE(MergeStepRecordsFile(JoinPath(f.sEvidence, "steps.tsv"), std::vector<std::string>(),
                                     std::vector<std::string>(1, "corpus\tWin32\tDebug")));
        CheckRan(f.Gate("plan"), "corpus");
    }
    SUBCASE("a missing tlog")
    {
        DeleteFileW(Utf8ToWide(JoinPath(f.Tlog(), "CL.read.1.tlog")).c_str());
        const GateRun r = f.Gate("plan");
        CheckRan(r, "corpus");
        CHECK_MESSAGE(HasLine(r, "gate: corpus Win32 run (key invalid: tlog)"), r.sLog);
    }
}

TEST_CASE_NAMED(FGateRunTest13, "Unit::Gate::retention and Resume", "[EngineFilter]")
{
    Fixture f;
    const std::string sOrphan = JoinPath(f.sEvidence, std::string(64, 'a'));
    MakeDirs(sOrphan);
    WriteTextFile(JoinPath(sOrphan, "exit.txt"), "0\r\n");
    GateRun r;
    for (int i = 0; i < 4; ++i)
    {
        r = f.Gate("plan");
    }
    const std::string sGateRoot = JoinPath(f.sRoot, "artifacts\\gate");
    std::vector<std::string> kRuns;
    WIN32_FIND_DATAW kData;
    HANDLE hFind = FindFirstFileW(Utf8ToWide(JoinPath(sGateRoot, "*")).c_str(), &kData);
    REQUIRE(hFind != INVALID_HANDLE_VALUE);
    do
    {
        if ((kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && kData.cFileName[0] != L'.')
        {
            kRuns.push_back(WideToUtf8(kData.cFileName));
        }
    } while (FindNextFileW(hFind, &kData));
    FindClose(hFind);
    std::sort(kRuns.begin(), kRuns.end());
    CHECK_MESSAGE(r.iCode == 0, r.sLog);
    CHECK(kRuns.size() == 3);
    CHECK_FALSE(DirExists(sOrphan));
    const RecordMap kRecords = ReadStepRecordsFile(JoinPath(f.sEvidence, "steps.tsv"));
    for (RecordMap::const_iterator it = kRecords.begin(); it != kRecords.end(); ++it)
    {
        CHECK(DirExists(JoinPath(f.sEvidence, it->second.find("Evidence")->second)));
    }
    S().sFail = "run-a";
    f.Change("src\\a.cpp", "d");
    const GateRun fr = f.Gate("plan");
    S().sFail.clear();
    const std::string sFailRun = "20000101-000005";
    CHECK_MESSAGE(fr.iCode == 1, fr.sLog);
    CHECK(DirExists(JoinPath(sGateRoot, sFailRun)));
    CHECK(sFailRun != kRuns.back());
    const GateRun rr = f.Gate("plan", std::string(), true);
    CHECK_MESSAGE(rr.iCode == 0, rr.sLog);
    CHECK_MESSAGE(HasLine(rr, "gate: tier plan platform Win32 run " + JoinPath(sGateRoot, sFailRun)), rr.sLog);
    CHECK_MESSAGE(HasLine(rr, "[RESUMED] gate:build "), rr.sLog);
    CHECK(DirExists(JoinPath(sGateRoot, sFailRun)));
    CHECK_FALSE(DirExists(JoinPath(sGateRoot, "20000101-000002")));
    CHECK(DirExists(JoinPath(sGateRoot, "20000101-000003")));
    CHECK(DirExists(JoinPath(sGateRoot, "20000101-000004")));
}

TEST_CASE_NAMED(FGateRunTest14, "Unit::Gate::a read outside Reads fails the step", "[EngineFilter]")
{
    Fixture f;
    f.Put("src\\other.cpp", "int other;\r\n");
    f.sTable = "[Step reader-ok]\r\nTiers=plan\r\nKind=script\r\nConfig=Debug\r\nExe=stub\r\nArg=reader-ok\r\n"
               "Arg=read\r\nArg=src\\a.cpp\r\nReads=src\\a.cpp\r\n\r\n"
               "[Step reader]\r\nTiers=plan\r\nKind=script\r\nConfig=Debug\r\nExe=stub\r\nArg=reader\r\n"
               "Arg=read\r\nArg=src\\other.cpp\r\nReads=src\\a.cpp\r\n";
    const GateRun r = f.Gate("plan");
    CHECK_MESSAGE(r.iCode == 1, r.sLog);
    CHECK_MESSAGE(HasLine(r, "[PASS] gate:reader-ok Win32 exit 0 "), r.sLog);
    CHECK_MESSAGE(HasLine(r, "harness: reader read src\\other.cpp, which is not in its Reads"), r.sLog);
    CHECK_MESSAGE(HasLine(r, "[FAIL] gate:reader Win32 exit 1 "), r.sLog);
    CHECK(f.Result("reader") == "FAIL");
}

TEST_CASE_NAMED(FGateRunTest15, "Unit::Gate::step order", "[EngineFilter]")
{
    StepTable kTable;
    const char* const kNames[] = {"build", "x", "y", "z", "w", "v", "u"};
    for (size_t i = 0; i < sizeof(kNames) / sizeof(kNames[0]); ++i)
    {
        Step s;
        s.sName = kNames[i];
        s.sKind = i == 0 ? "build" : "run";
        s.sConfig = "Debug";
        s.bStartsGame = false;
        kTable.kSteps.push_back(s);
    }
    RecordMap kRecords;
    const char* const kWalls[][2] = {{"build", "900"}, {"y", "50"}, {"z", "10"}, {"v", "abc"}, {"u", "10"}};
    for (size_t i = 0; i < sizeof(kWalls) / sizeof(kWalls[0]); ++i)
    {
        RecordRow k;
        k["Wall_s"] = kWalls[i][1];
        kRecords[std::string(kWalls[i][0]) + "\tWin32\tDebug"] = k;
    }
    std::vector<const Step*> kSteps;
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        kSteps.push_back(&kTable.kSteps[i]);
    }
    CHECK(JoinText(Names(OrderSteps(kSteps, kRecords, "Win32")), ",") == "build,x,w,v,z,u,y");
    CHECK(JoinText(Names(OrderSteps(kSteps, kRecords, "x64")), ",") == "build,x,y,z,w,v,u");
}

TEST_CASE_NAMED(FGateRunTest16, "Unit::Gate::a direct step is started in its work folder with absolute paths", "[EngineFilter]")
{
    Fixture f;
    f.Put("tests\\excluded.tsv", "x\r\n");
    f.sTable = "[Step direct]\r\nTiers=plan\r\nKind=run\r\nConfig=Debug\r\nWorkDir=tests\r\nExe=stub\r\n"
               "Arg=direct\r\nArg=direct\r\nArg=--runner\r\nArg=--out=$Run\r\nArg=--exclude=$Root\\tests\\excluded.tsv\r\n"
               "Reads=tests\\excluded.tsv\r\n";
    const GateRun r = f.Gate("plan");
    REQUIRE_MESSAGE(r.iCode == 0, r.sLog);
    REQUIRE(S().kSpecs.size() == 1);
    const ProcessSpec& k = S().kSpecs[0];
    const std::string sRun = JoinPath(f.sRoot, "artifacts\\gate\\20000101-000001\\Win32\\direct\\run");
    CHECK(k.sWorkDir == JoinPath(f.sRoot, "tests"));
    CHECK(ArgAfter(k.kArgs, "--out=") == sRun);
    CHECK(ArgAfter(k.kArgs, "--exclude=") == JoinPath(f.sRoot, "tests\\excluded.tsv"));
    CHECK(EnvOf(k, "OPENSWBF2_READS") == JoinPath(f.sRoot, "artifacts\\gate\\20000101-000001\\Win32\\direct\\reads.txt"));
    const std::string sEvidence = JoinPath(f.sEvidence, f.Result("direct", "Evidence"));
    std::string sParams;
    std::string sResults;
    REQUIRE(ReadTextFile(JoinPath(sEvidence, "runs\\0\\run-params.txt"), sParams));
    REQUIRE(ReadTextFile(JoinPath(sEvidence, "runs\\0\\results.txt"), sResults));
    CHECK(FileExists(JoinPath(sEvidence, "runs\\0\\index.json")));
    CHECK(sParams.find("arg=--out=" + sRun + "\r\n") != std::string::npos);
    CHECK(sParams.find("tests_exe=stub\r\n") != std::string::npos);
    CHECK(sParams.find("whea_before=0\r\n") != std::string::npos);
    CHECK(sParams.find("whea_delta=0\r\n") != std::string::npos);
    CHECK(sResults.find("[PASS] Unit::Stub::one\r\n") != std::string::npos);
    CHECK(sResults.find("  PASS     1\r\n") != std::string::npos);
    const GateRun c = f.Gate("plan");
    CheckCarried(c, "direct");
    f.sTable = "[Step direct]\r\nTiers=plan\r\nKind=run\r\nConfig=Debug\r\nWorkDir=tests\r\nExe=stub\r\n"
               "Arg=direct\r\nArg=direct\r\nArg=--runner\r\nArg=--out=$Run\r\nArg=--exclude=$Root\\tests\\excluded.tsv\r\n"
               "Reads=src\\a.cpp\r\n";
    const GateRun x = f.Gate("plan");
    CHECK(x.iCode == 1);
    CHECK_MESSAGE(HasLine(x, "harness: direct read tests\\excluded.tsv, which is not in its Reads"), x.sLog);
}

TEST_CASE_NAMED(FGateRunTest17, "Unit::Gate::the judge command runs after a run and on archived evidence", "[EngineFilter]")
{
    Fixture f;
    f.sTable = "[Step cap]\r\nTiers=plan\r\nKind=capture\r\nConfig=Debug\r\nJudgeKinds=compare\r\n"
               "Outputs=artifacts\\gate\\$Stamp\\cap\r\nExe=stub\r\nArg=cap\r\nArg=capture\r\n"
               "Arg=$Root\\artifacts\\gate\\$Stamp\\cap\r\nJudgeExe=judge\r\nJudgeArg=$Out\r\nReads=src\\a.cpp\r\n";
    SUBCASE("after a run and on a carry")
    {
        const GateRun r = f.Gate("plan");
        CHECK_MESSAGE(r.iCode == 0, r.sLog);
        REQUIRE(S().kJudgeOuts.size() == 1);
        CHECK(S().kJudgeOuts[0] == JoinPath(f.sRoot, "artifacts\\gate\\20000101-000001\\cap"));
        const GateRun c = f.Gate("plan");
        CheckCarried(c, "cap");
        REQUIRE(S().kJudgeOuts.size() == 2);
        CHECK(S().kJudgeOuts[1] == JoinPath(f.sRoot, "artifacts\\gate\\20000101-000002\\judge\\Win32-cap-1\\out\\0"));
        S().iJudgeCode = 4;
        const GateRun j = f.Gate("plan");
        CHECK_MESSAGE(HasLine(j, "gate: cap Win32 run (evidence judged FAIL: judge command exited 4)"), j.sLog);
        CHECK(j.iCode == 1);
    }
    SUBCASE("a failing judge fails the run")
    {
        S().iJudgeCode = 3;
        const GateRun r = f.Gate("plan");
        CHECK(r.iCode == 1);
        CHECK_MESSAGE(HasLine(r, "[FAIL] gate:cap Win32 exit 3 "), r.sLog);
        CHECK(f.Result("cap") == "FAIL");
    }
}

TEST_CASE_NAMED(FGateRunTest18, "Unit::Gate::the step table is valid", "[EngineFilter]")
{
    std::string sText;
    REQUIRE(ReadRepoText("tests\\gate\\steps.ini", sText));
    StepTable kTable;
    std::vector<std::string> kProblems;
    ParseSteps(sText, kTable, kProblems);
    ValidateSteps(kTable, kProblems);
    const std::string sProblems = JoinText(kProblems, "; ");
    CHECK_MESSAGE(kProblems.empty(), sProblems);
    const char* const kOld[] = {
        "build-Debug",       "build-Release",   "build-Test",       "build-Asan",        "build-Test-ref",
        "build-Asan-ref",    "build-Test-game-ref", "rider-link",   "hardening-binary-Release",
        "hardening-binary-Test", "hardening-binary-Asan", "t1-Debug", "t1-Test",           "t1-Asan",
        "corpus-Asan",       "standard-Release", "standard-Test",   "standard-Asan",     "boot-Release",
        "vendor-Release",    "twice-Release",   "crash-Release",    "crash-Test",        "dedicated-Release",
        "boot-Asan",         "dedicated-Asan",  "soak-Asan",        "perturb-Test",      "all-Release-short",
        "all-Release",       "all-Test-short",  "savecross-Test",   "luac-compare",      "x87-inventory",
        "capture-collect",   "capture-map",     "debug-digests",    "verify-v143",       "verify-v143-x64",
        "hardening-source",  "build-luac-Win32", "build-luac-x64",  "laa-compare"};
    CHECK(kTable.kSteps.size() == sizeof(kOld) / sizeof(kOld[0]));
    for (size_t i = 0; i < sizeof(kOld) / sizeof(kOld[0]); ++i)
    {
        const bool bFound = FindStep(kTable, kOld[i]) != 0;
        CHECK_MESSAGE(bFound, kOld[i]);
    }
    const char* const kPlatforms[] = {"Win32", "x64"};
    for (size_t p = 0; p < 2; ++p)
    {
        for (size_t t = 0; t < kGateTierCount; ++t)
        {
            CHECK_MESSAGE(!SelectSteps(kTable, kGateTiers[t], kPlatforms[p], std::vector<std::string>()).empty(),
                          kGateTiers[t]);
        }
    }
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        const Step& s = kTable.kSteps[i];
        const bool bUnit = s.sName.compare(0, 3, "t1-") == 0;
        for (size_t h = 0; h < s.kHashes.size(); ++h)
        {
            const bool bTestsExe = s.kHashes[h].find("OpenSWBF2Tests.exe") != std::string::npos;
            CHECK_MESSAGE((bUnit || !bTestsExe), s.sName);
        }
        for (size_t a = 0; a < s.kArgs.size(); ++a)
        {
            if (s.kArgs[a].compare(0, 10, "--exclude=") == 0)
            {
                CHECK_MESSAGE(InputMatches("tests\\excluded.tsv", s.kReads), s.sName);
            }
        }
        if (IsDirectStep(s))
        {
            CHECK_MESSAGE(s.sWorkDir == "tests", s.sName);
            CHECK_MESSAGE(s.sExe.compare(0, 5, "$Root") == 0, s.sName);
        }
        if (!bUnit && s.sKind != "build" && s.kHashes.size() && s.bStartsGame)
        {
            CHECK_MESSAGE(!s.kSources.empty(), s.sName);
        }
    }
    const Step* pRider = FindStep(kTable, "rider-link");
    const bool bRider = pRider != 0;
    REQUIRE(bRider);
    CHECK(pRider->sExe == "tools\\rider-link\\gradlew.bat");
    CHECK(JoinText(pRider->kArgs, " ") == "-p tools\\rider-link --offline --no-daemon test buildPlugin");
    const Step* pLuac = FindStep(kTable, "luac-compare");
    const bool bLuac = pLuac != 0;
    REQUIRE(bLuac);
    CHECK(pLuac->kMustRun.size() == 2);
    CHECK(pLuac->kRequires.size() == 3);
}

TEST_CASE_NAMED(FGateRunTest19, "Unit::Gate::an evidence root inside the repo is refused", "[EngineFilter]")
{
    CHECK(EvidenceInsideRepo("C:\\Dev\\OpenSWBF2", "C:\\Dev\\OpenSWBF2"));
    CHECK(EvidenceInsideRepo("c:\\dev\\openswbf2\\artifacts\\ev", "C:\\Dev\\OpenSWBF2\\"));
    CHECK_FALSE(EvidenceInsideRepo("C:\\Dev\\OpenSWBF2-evidence", "C:\\Dev\\OpenSWBF2"));
    CHECK_FALSE(EvidenceInsideRepo("C:\\Users\\x\\AppData\\Local\\OpenSWBF2\\evidence", "C:\\Dev\\OpenSWBF2"));
}

TEST_CASE_NAMED(FGateRunTest20, "Unit::Gate::a batch step is started through cmd with its arguments as a list", "[EngineFilter]")
{
    Fixture f;
    f.Put("tools\\probe.bat", "@echo off\r\necho probe %1 %2 %3\r\nexit /b 3\r\n");
    std::vector<std::string> kArgs;
    kArgs.push_back("one");
    kArgs.push_back("two words");
    kArgs.push_back("a&b");
    ProcessSpec kSpec = NewProcessSpec("tools\\probe.bat", kArgs);
    kSpec.sWorkDir = f.sRoot;
    const ProcessResult k = RunProcess(kSpec);
    CHECK(k.bStarted);
    CHECK(k.uCode == 3);
    CHECK_MESSAGE(k.sOutput.find("probe \"one\" \"two words\" \"a&b\"") != std::string::npos, k.sOutput);
}

namespace
{

struct CaseFilter
{
    std::string sTags;
    std::string sName;
    bool bGameDir;
    bool bData;
};

CaseFilter FilterOf(const std::vector<std::string>& kArgs)
{
    CaseFilter k;
    k.bGameDir = false;
    k.bData = false;
    for (size_t i = 0; i < kArgs.size(); ++i)
    {
        if (kArgs[i].compare(0, 7, "--tags=") == 0)
        {
            k.sTags = kArgs[i].substr(7);
        }
        else if (kArgs[i].compare(0, 7, "--name=") == 0)
        {
            k.sName = kArgs[i].substr(7);
        }
        k.bGameDir = k.bGameDir || kArgs[i].compare(0, 11, "--game-dir=") == 0;
        k.bData = k.bData || kArgs[i].compare(0, 7, "--data=") == 0;
    }
    return k;
}

std::vector<RunnerCase> FilterCases(const std::vector<RunnerCase>& kAll, const CaseFilter& kFilter)
{
    std::vector<RunnerCase> kOrdinary;
    std::vector<RunnerCase> kAfter;
    SelectRunCases(kAll, kFilter.sTags, kFilter.sName, kFilter.bGameDir, kFilter.bData, kOrdinary, kAfter);
    kOrdinary.insert(kOrdinary.end(), kAfter.begin(), kAfter.end());
    return kOrdinary;
}

std::string CaseSource(const std::string& sFile)
{
    std::string s = sFile;
    std::replace(s.begin(), s.end(), '/', '\\');
    const std::string sRoot = RepoRoot() + "\\";
    return StartsWithNoCase(s, sRoot) ? s.substr(sRoot.size()) : s;
}

bool RunsTestsExe(const std::string& sExe)
{
    return sExe.find("OpenSWBF2Tests.exe") != std::string::npos;
}

StepTable RealSteps()
{
    std::string sText;
    const bool bRead = ReadRepoText("tests\\gate\\steps.ini", sText);
    REQUIRE(bRead);
    StepTable kTable;
    std::vector<std::string> kProblems;
    ParseSteps(sText, kTable, kProblems);
    return kTable;
}

}

TEST_CASE_NAMED(FGateRunTest21, "Unit::Gate::every case a step selects is in its Sources", "[EngineFilter]")
{
    std::vector<RunnerCase> kAll;
    std::string sError;
    const bool bCases = RegisteredCases(kAll, sError);
    REQUIRE_MESSAGE(bCases, sError);
    const StepTable kTable = RealSteps();
    size_t uChecked = 0;
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        const Step& s = kTable.kSteps[i];
        if (s.kSources.empty())
        {
            continue;
        }
        std::vector<std::vector<std::string> > kCommands;
        if (RunsTestsExe(s.sExe))
        {
            kCommands.push_back(s.kArgs);
        }
        if (RunsTestsExe(s.sJudgeExe))
        {
            kCommands.push_back(s.kJudgeArgs);
        }
        for (size_t c = 0; c < kCommands.size(); ++c)
        {
            const std::vector<RunnerCase> kSelected = FilterCases(kAll, FilterOf(kCommands[c]));
            for (size_t k = 0; k < kSelected.size(); ++k)
            {
                const std::string sSource = CaseSource(kSelected[k].sFile);
                const bool bKeyed = InputMatches(sSource, s.kSources);
                CHECK_MESSAGE(bKeyed, s.sName << " selects " << kSelected[k].sName << " from " << sSource);
                ++uChecked;
            }
        }
    }
    CHECK(uChecked > 0);
}

TEST_CASE_NAMED(FGateRunTest22, "Unit::Gate::no unit step selects an on-request or after-all case", "[EngineFilter]")
{
    std::vector<RunnerCase> kAll;
    std::string sError;
    const bool bCases = RegisteredCases(kAll, sError);
    REQUIRE_MESSAGE(bCases, sError);
    const StepTable kTable = RealSteps();
    const char* const kUnitSteps[] = {"t1-Debug", "t1-Test", "t1-Asan"};
    for (size_t i = 0; i < 3; ++i)
    {
        const Step* pStep = FindStep(kTable, kUnitSteps[i]);
        const bool bStep = pStep != 0;
        REQUIRE_MESSAGE(bStep, kUnitSteps[i]);
        const std::vector<RunnerCase> kSelected = FilterCases(kAll, FilterOf(pStep->kArgs));
        CHECK_MESSAGE(kSelected.size() > 100, kUnitSteps[i]);
        for (size_t k = 0; k < kSelected.size(); ++k)
        {
            const bool bOnRequest = kSelected[k].sTags.find("[OnRequest]") != std::string::npos;
            const bool bAfterAll = kSelected[k].sTags.find("[AfterAll]") != std::string::npos;
            CHECK_MESSAGE(!bOnRequest, kUnitSteps[i] << " selects " << kSelected[k].sName);
            CHECK_MESSAGE(!bAfterAll, kUnitSteps[i] << " selects " << kSelected[k].sName);
        }
    }
}

TEST_CASE_NAMED(FGateRunTest23, "Unit::Gate::build properties reach MSBuild", "[EngineFilter]")
{
    Fixture f;
    f.sTable = "[Step build]\r\nTiers=plan\r\nKind=build\r\nConfig=Release\r\nProject=LuaCompiler.vcxproj\r\n"
               "ProjectPlatform=x64\r\nProperty=SWBF2_NOSTAGE=true\r\n";
    const GateRun r = f.Gate("plan", "build");
    REQUIRE_MESSAGE(r.iCode == 0, r.sLog);
    REQUIRE(S().kSpecs.size() == 1);
    const ProcessSpec& k = S().kSpecs[0];
    CHECK(k.sExe == "msbuild");
    CHECK(JoinText(k.kArgs, " ") == JoinPath(f.sRoot, "LuaCompiler.vcxproj") +
                                        " /p:Configuration=Release /p:Platform=x64 /p:SWBF2_NOSTAGE=true /m /v:minimal /nologo");
}

TEST_CASE_NAMED(FGateRunTest24, "Unit::Gate::no step calls a script", "[EngineFilter]")
{
    const StepTable kTable = RealSteps();
    REQUIRE(!kTable.kSteps.empty());
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        const Step& s = kTable.kSteps[i];
        std::vector<std::string> kWords(1, s.sExe);
        kWords.insert(kWords.end(), s.kArgs.begin(), s.kArgs.end());
        kWords.push_back(s.sJudgeExe);
        kWords.insert(kWords.end(), s.kJudgeArgs.begin(), s.kJudgeArgs.end());
        for (size_t w = 0; w < kWords.size(); ++w)
        {
            const std::string sLower = LowerText(TrimText(kWords[w]));
            const bool bScript = (sLower.size() >= 4 && sLower.compare(sLower.size() - 4, 4, ".ps1") == 0) ||
                                 sLower == "powershell" || sLower == "powershell.exe" || sLower == "pwsh" ||
                                 sLower == "pwsh.exe";
            CHECK_MESSAGE(!bScript, s.sName << " names " << kWords[w]);
        }
    }
}

TEST_CASE_NAMED(FGateRunTest25, "Unit::Gate::a scanner source is keyed by hardening-source and the unit steps only", "[EngineFilter]")
{
    const char* const pcScanner = "tests\\hardening\\hardening_source.cpp";
    const StepTable kTable = RealSteps();
    const std::string sRoot = RepoRoot();
    const std::string sPlatform = RunnerPlatform();
    std::vector<std::string> kPaths;
    std::string sError;
    const bool bListed = GitTreeList(sRoot, kPaths, sError);
    REQUIRE_MESSAGE(bListed, sError);
    std::map<std::string, std::string> kTree;
    for (size_t i = 0; i < kPaths.size(); ++i)
    {
        std::string sRel = kPaths[i];
        std::replace(sRel.begin(), sRel.end(), '/', '\\');
        kTree[LowerText(sRel)] = "x";
    }
    REQUIRE(kTree.count(pcScanner) == 1);
    ExpandVars kVars;
    kVars.push_back(std::make_pair(std::string("$Platform"), sPlatform));
    std::vector<std::string> kKeyed;
    std::vector<std::string> kExpected;
    for (size_t i = 0; i < kTable.kSteps.size(); ++i)
    {
        const Step& s = kTable.kSteps[i];
        if (s.sKind == "build" || !StepOnPlatform(s, sPlatform))
        {
            continue;
        }
        KeyFilesInput kIn;
        kIn.pTree = &kTree;
        kIn.kReads = s.kReads;
        kIn.kSources = s.kSources;
        kIn.pRead = 0;
        kIn.pCommands = 0;
        kIn.pHash = 0;
        ReadTlog kRead;
        if (!s.kSources.empty())
        {
            const std::string sTlog = JoinPath(JoinPath(sRoot, ExpandText(s.sTlog, kVars)), "CL.read.1.tlog");
            if (!FileExists(sTlog))
            {
                SkipCase((s.sName + " has no tlog: " + sTlog).c_str());
            }
            REQUIRE_MESSAGE(LoadReadTlog(sTlog, sRoot, kRead), sTlog);
            kIn.pRead = &kRead;
        }
        std::map<std::string, std::string> kFiles;
        std::string sState;
        KeyFiles(kIn, kFiles, sState);
        if (kFiles.count(pcScanner))
        {
            kKeyed.push_back(s.sName);
        }
        if (s.sName == "hardening-source" || s.sName.compare(0, 3, "t1-") == 0)
        {
            kExpected.push_back(s.sName);
        }
    }
    CHECK(!kExpected.empty());
    CHECK(JoinText(kKeyed, ", ") == JoinText(kExpected, ", "));
}
