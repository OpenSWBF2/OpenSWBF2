#include <windows.h>

#include <string>
#include <vector>

#include "third_party/doctest/doctest.h"
#include "harness/report.h"
#include "harness/runner.h"
#include "harness/tags.h"

#include "gate/gate_evidence.h"
#include "gate/gate_ini.h"
#include "gate/gate_machine.h"

using namespace Gate;

namespace
{

const char* const kGoodTable = "[Inputs game-tests]\r\n"
                               "Path=tests\\harness\\\r\n"
                               "Path=tests\\game\\\r\n"
                               "\r\n"
                               "[Step build-Release]\r\n"
                               "Tiers=plan:Win32 phase milestone\r\n"
                               "Platforms=Win32 x64\r\n"
                               "Kind=build\r\n"
                               "Config=Release\r\n"
                               "Project=BattlefrontII.vcxproj\r\n"
                               "Project=tests\\OpenSWBF2Tests.vcxproj\r\n"
                               "\r\n"
                               "[Step standard-Release]\r\n"
                               "Tiers=plan:Win32 phase\r\n"
                               "Platforms=Win32 x64\r\n"
                               "Kind=run\r\n"
                               "Config=Release\r\n"
                               "Requires=build-Release\r\n"
                               "StartsGame=true\r\n"
                               "Group=Standard\r\n"
                               "WorkDir=tests\r\n"
                               "Exe=$Root\\bin\\Tests\\$Platform\\Release\\OpenSWBF2Tests.exe\r\n"
                               "Arg=--runner\r\n"
                               "Arg=--out=$Run\r\n"
                               "Arg=--exclude=$Root\\tests\\excluded.tsv\r\n"
                               "Hashes=bin\\$Platform\\Release\\BattlefrontII.exe\r\n"
                               "DllDir=bin\\$Platform\\Release\r\n"
                               "Sources=tests\\main.cpp tests\\harness\\\r\n"
                               "Tlog=obj\\Tests\\$Platform\\Release\\OpenSWBF2Tests.tlog\r\n"
                               "Reads=game-tests tests\\excluded.tsv\r\n"
                               "MustRun=Game::Standard::boot\r\n";

std::vector<std::string> Problems(const std::string& sText)
{
    StepTable kTable;
    std::vector<std::string> kProblems;
    ParseSteps(sText, kTable, kProblems);
    ValidateSteps(kTable, kProblems);
    return kProblems;
}

std::string Replace(const std::string& sText, const std::string& sFrom, const std::string& sTo)
{
    std::string sOut = sText;
    const size_t uAt = sOut.find(sFrom);
    REQUIRE(uAt != std::string::npos);
    sOut.replace(uAt, sFrom.size(), sTo);
    return sOut;
}

bool Has(const std::vector<std::string>& kList, const std::string& sValue)
{
    for (size_t i = 0; i < kList.size(); ++i)
    {
        if (kList[i] == sValue)
        {
            return true;
        }
    }
    return false;
}

std::string TempDir(const char* pcName)
{
    wchar_t acTemp[MAX_PATH];
    GetTempPathW(MAX_PATH, acTemp);
    char acTail[96];
    _snprintf(acTail, sizeof(acTail), "openswbf2-%s-%lu-%lu", pcName, GetCurrentProcessId(), GetTickCount());
    acTail[sizeof(acTail) - 1] = 0;
    const std::string sDir = JoinPath(WideToUtf8(acTemp), acTail);
    MakeDirs(sDir);
    return sDir;
}

bool MakeJunction(const std::string& sLink, const std::string& sTarget)
{
    std::wstring sCmd = L"cmd /c mklink /J \"" + Utf8ToWide(sLink) + L"\" \"" + Utf8ToWide(sTarget) + L"\" >NUL";
    STARTUPINFOW kStart;
    ZeroMemory(&kStart, sizeof(kStart));
    kStart.cb = sizeof(kStart);
    PROCESS_INFORMATION kProc;
    if (!CreateProcessW(NULL, &sCmd[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &kStart, &kProc))
    {
        return false;
    }
    WaitForSingleObject(kProc.hProcess, INFINITE);
    DWORD uCode = 1;
    GetExitCodeProcess(kProc.hProcess, &uCode);
    CloseHandle(kProc.hThread);
    CloseHandle(kProc.hProcess);
    return uCode == 0;
}

void PutFile(const std::string& sDir, const char* pcRel, const std::string& sText)
{
    const std::string sPath = JoinPath(sDir, pcRel);
    MakeDirs(sPath.substr(0, sPath.find_last_of('\\')));
    REQUIRE(WriteTextFile(sPath, sText));
}

void Cleanup(const std::string& sDir)
{
    std::vector<std::string> kLinks;
    std::string sError;
    FindReparsePoints(sDir, kLinks, sError);
    for (size_t i = 0; i < kLinks.size(); ++i)
    {
        RemoveDirectoryW(Utf8ToWide(kLinks[i]).c_str());
    }
    RemoveTree(sDir, sError);
}

void GoodRun(const std::string& sDir)
{
    PutFile(sDir, "exit.txt", "0\r\n");
    PutFile(sDir, "stdout.txt", "stub run-a\r\n");
    PutFile(sDir, "runs\\r1\\results.txt", "[PASS] stub run-a\r\n");
    PutFile(sDir, "runs\\r1\\index.json", "{ \"failed\": 0, \"tests\": [] }\r\n");
}

std::string DirectIndex(const char* pcFirst, const char* pcSecond)
{
    std::string s = "{ \"failed\": 0, \"tests\": [";
    s += std::string("{ \"fullTestPath\": \"") + pcFirst + "\", \"state\": \"Success\" }";
    if (pcSecond)
    {
        s += std::string(", { \"fullTestPath\": \"") + pcSecond + "\", \"state\": \"Success\" }";
    }
    return s + "] }\r\n";
}

PreflightFacts GoodFacts()
{
    PreflightFacts k;
    k.sSteam = "yes";
    k.sOverlay = "off";
    k.kRulePaths.push_back("C:\\Dev\\OpenSWBF2\\bin\\Win32\\Release\\BattlefrontII.exe");
    k.kGamePlatforms.push_back("Win32");
    k.kPlatforms.push_back("Win32");
    k.kAllowRules.insert("c:\\dev\\openswbf2\\bin\\win32\\release\\battlefrontii.exe");
    k.iFreeRamKB = kRamFloorKB;
    k.sRepoDrive = "C:";
    k.uRepoFree = (unsigned long long)kDiskFloorBytes;
    k.sGameData = "G:\\GameData";
    ToolchainFacts t;
    t.sPlatform = "Win32";
    t.bMSBuild = true;
    t.bDumpbin = true;
    t.sRuntimeDir = "C:\\Windows\\SysWOW64";
    k.kToolchain.push_back(t);
    return k;
}

const PreflightResult& Verdict(const std::vector<PreflightResult>& kAll, const char* pcCheck)
{
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (kAll[i].sCheck == pcCheck)
        {
            return kAll[i];
        }
    }
    REQUIRE(false);
    return kAll[0];
}

bool FakeCount(bool bSince, unsigned long long, long long& iCount)
{
    iCount = bSince ? 2 : 5;
    return true;
}

bool BrokenCount(bool, unsigned long long, long long&)
{
    return false;
}

}

TEST_CASE_NAMED(FGateMachineTest1, "Unit::Gate::a step with no Reads is a definition error", "[EngineFilter]")
{
    StepTable kTable;
    std::vector<std::string> kProblems;
    REQUIRE(ParseSteps(kGoodTable, kTable, kProblems));
    ValidateSteps(kTable, kProblems);
    REQUIRE(kProblems.empty());
    const Step* pStep = FindStep(kTable, "standard-Release");
    REQUIRE(pStep);
    CHECK(pStep->kReads.size() == 3);
    CHECK(pStep->kReads[0] == "tests\\harness\\");
    CHECK(pStep->kReads[2] == "tests\\excluded.tsv");
    CHECK(pStep->bStartsGame);
    CHECK(pStep->kArgs.size() == 3);
    CHECK(StepInTier(*pStep, "plan", "Win32"));
    CHECK_FALSE(StepInTier(*pStep, "plan", "x64"));
    CHECK(StepInTier(*pStep, "phase", "x64"));
    CHECK_FALSE(StepInTier(*pStep, "milestone", "Win32"));
    CHECK(StepOnPlatform(*pStep, "X64"));
    const ExpandVars kVars = GateVars("C:\\Dev\\OpenSWBF2", "x64", "20261125-1", "4", "C:\\o", "C:\\r");
    CHECK(ExpandText(pStep->sExe, kVars) == "C:\\Dev\\OpenSWBF2\\bin\\Tests\\x64\\Release\\OpenSWBF2Tests.exe");
    CHECK(ExpandText(pStep->kArgs[1], kVars) == "--out=C:\\r");
    const std::string sDef = StepDefinitionText(*pStep, "x64");
    CHECK(sDef.find("exe\t$Root\\bin\\Tests\\x64\\Release\\OpenSWBF2Tests.exe") != std::string::npos);
    CHECK(sDef.find("mustrun\tGame::Standard::boot") != std::string::npos);
    StepTable kChanged;
    std::vector<std::string> kIgnored;
    ParseSteps(Replace(kGoodTable, "Arg=--runner", "Arg=--runner2"), kChanged, kIgnored);
    CHECK(StepDefinitionText(*FindStep(kChanged, "standard-Release"), "x64") != sDef);

    SUBCASE("no Reads")
    {
        const std::vector<std::string> k = Problems(Replace(kGoodTable, "Reads=game-tests tests\\excluded.tsv\r\n", ""));
        CHECK(Has(k, "standard-Release: no Reads"));
    }
    SUBCASE("a malformed line")
    {
        const std::vector<std::string> k = Problems(Replace(kGoodTable, "Group=Standard", "Group"));
        CHECK(Has(k, "line 20: malformed line 'Group'"));
        CHECK(Has(Problems(std::string("Kind=run\r\n") + kGoodTable), "line 1: 'Kind=run' is outside a section"));
        CHECK(Has(Problems(Replace(kGoodTable, "[Step build-Release]", "[Stage build-Release]")),
                  "line 5: malformed section '[Stage build-Release]'"));
        CHECK(Has(Problems(Replace(kGoodTable, "StartsGame=true", "StartsGame=yes")),
                  "line 19: StartsGame 'yes' is not true or false"));
    }
    SUBCASE("an unknown key")
    {
        CHECK(Has(Problems(Replace(kGoodTable, "Group=Standard", "Colour=red")),
                  "line 20: unknown key 'Colour' in [Step standard-Release]"));
        CHECK(Has(Problems(Replace(kGoodTable, "Path=tests\\game\\", "File=tests\\game\\")),
                  "line 3: unknown key 'File' in [Inputs game-tests]"));
        CHECK(Has(Problems(Replace(kGoodTable, "Group=Standard", "Kind=run")),
                  "line 20: duplicate key 'Kind' in [Step standard-Release]"));
    }
    SUBCASE("a duplicate step")
    {
        CHECK(Has(Problems(Replace(kGoodTable, "[Step standard-Release]", "[Step build-Release]")),
                  "duplicate step build-Release"));
    }
    SUBCASE("a Requires that is not a build")
    {
        CHECK(Has(Problems(Replace(kGoodTable, "Requires=build-Release", "Requires=standard-Release")),
                  "standard-Release: Requires 'standard-Release' is not a build step"));
        CHECK(Has(Problems(Replace(kGoodTable, "Requires=build-Release", "Requires=build-Nope")),
                  "standard-Release: Requires 'build-Nope' is not a build step"));
    }
    SUBCASE("kind and tier values")
    {
        CHECK(Has(Problems(Replace(kGoodTable, "Kind=run", "Kind=compare")), "standard-Release: Kind 'compare'"));
        CHECK(Has(Problems(Replace(kGoodTable, "Tiers=plan:Win32 phase\r\n", "Tiers=weekly\r\n")),
                  "standard-Release: tier 'weekly'"));
        CHECK(Has(Problems(Replace(kGoodTable, "Tiers=plan:Win32 phase\r\n", "Tiers=plan:arm\r\n")),
                  "standard-Release: tier 'plan:arm'"));
        CHECK(Has(Problems(Replace(kGoodTable, "Tiers=plan:Win32 phase\r\n", "Tiers=plan:Win32:x\r\n")),
                  "standard-Release: tier 'plan:Win32:x'"));
        CHECK(Has(Problems(Replace(kGoodTable, "Tiers=plan:Win32 phase\r\n", "")), "standard-Release: no Tiers"));
        CHECK(Has(Problems(Replace(kGoodTable, "Project=BattlefrontII.vcxproj\r\nProject=tests\\OpenSWBF2Tests.vcxproj\r\n", "")),
                  "build-Release: no Project"));
        CHECK(Has(Problems(Replace(kGoodTable, "Tlog=obj\\Tests\\$Platform\\Release\\OpenSWBF2Tests.tlog\r\n", "")),
                  "standard-Release: Sources needs Tlog"));
        CHECK(Has(Problems(Replace(kGoodTable, "Kind=run", "Kind=capture")), "standard-Release: Kind capture needs Outputs"));
        CHECK(Has(Problems(Replace(kGoodTable, "Arg=--out=$Run", "Arg=--out=x")),
                  "standard-Release: Kind run needs an Arg with $Run"));
        CHECK(Has(Problems(Replace(kGoodTable, "Kind=run", "Kind=census")), "standard-Release: Kind 'census'"));
    }
    SUBCASE("a compare judge kind with no judge command")
    {
        const std::string sText = Replace(kGoodTable, "MustRun=", "JudgeKinds=compare\r\nOutputs=artifacts\\$Platform\\x\r\nMustRun=");
        CHECK(Has(Problems(sText), "standard-Release: JudgeKinds compare needs JudgeExe"));
        CHECK(Problems(Replace(sText, "MustRun=", "JudgeExe=judge\r\nJudgeArg=--data\r\nMustRun=")).empty());
    }
    SUBCASE("an output folder shared by two platforms")
    {
        const std::string sText =
            Replace(kGoodTable, "MustRun=", "JudgeKinds=compare\r\nOutputs=artifacts\\x\r\nJudgeExe=judge\r\nMustRun=");
        CHECK(Has(Problems(sText), "standard-Release: Outputs 'artifacts\\x' needs $Platform on more than one platform"));
        CHECK(Has(Problems(Replace(sText, "Platforms=Win32 x64\r\nKind=run", "Kind=run")),
                  "standard-Release: Outputs 'artifacts\\x' needs $Platform on more than one platform"));
        CHECK(Problems(Replace(sText, "Platforms=Win32 x64\r\nKind=run", "Platforms=Win32\r\nKind=run")).empty());
    }
}

TEST_CASE_NAMED(FGateMachineTest2, "Unit::Gate::x64 steps need an x64 configuration in the platform props", "[EngineFilter]")
{
    CHECK(PlatformPropsHasX64("  <ItemGroup>\r\n    <ProjectConfiguration Include=\"Release|x64\">\r\n"));
    CHECK(PlatformPropsHasX64("<projectconfiguration include=\"Asan|X64\" />"));
    CHECK_FALSE(PlatformPropsHasX64("<ProjectConfiguration Include=\"Release|Win32\">\r\n"));
    CHECK_FALSE(PlatformPropsHasX64("<ProjectConfiguration Include=\"Release|x64-other\">"));
    CHECK_FALSE(PlatformPropsHasX64(""));
}

TEST_CASE_NAMED(FGateMachineTest3, "Unit::Gate::overlay state from localconfig vdf", "[EngineFilter]")
{
    const std::string sHead = "\"UserLocalConfigStore\"\n{\n\t\"apps\"\n\t{\n\t\t\"319630\"\n\t\t{\n"
                              "\t\t\t\"OverlayAppEnable\"\t\t\"0\"\n\t\t}\n";
    const std::string sTail = "\t}\n\t\"friends\"\n\t{\n\t\t\"name\"\t\t\"a \\\"quoted\\\" name\"\n\t}\n}\n";
    SUBCASE("on")
    {
        CHECK(OverlayFromVdf(sHead + "\t\t\"6060\"\n\t\t{\n\t\t\t\"OverlayAppEnable\"\t\t\"1\"\n\t\t}\n" + sTail, "6060") == "on");
    }
    SUBCASE("off")
    {
        CHECK(OverlayFromVdf(sHead + "\t\t\"6060\"\n\t\t{\n\t\t\t\"overlayappenable\"\t\t\"0\"\n\t\t}\n" + sTail, "6060") == "off");
    }
    SUBCASE("app absent")
    {
        CHECK(OverlayFromVdf(sHead + sTail, "6060") == "on");
        CHECK(OverlayFromVdf(sHead + "\t\t\"6060\"\n\t\t{\n\t\t}\n" + sTail, "6060") == "on");
    }
    SUBCASE("malformed")
    {
        CHECK(OverlayFromVdf(sHead, "6060") == "-");
        CHECK(OverlayFromVdf(sHead + sTail + "}\n", "6060") == "-");
        CHECK(OverlayFromVdf("{\n}\n", "6060") == "-");
        CHECK(OverlayFromVdf(sHead + "\t\t\"6060\"\n\t\t{\n\t\t\t\"OverlayAppEnable\"\t\t\"2\"\n\t\t}\n" + sTail, "6060") == "-");
    }
}

TEST_CASE_NAMED(FGateMachineTest4, "Unit::Gate::preflight verdicts", "[EngineFilter]")
{
    const std::vector<PreflightResult> kGood = PreflightVerdicts(GoodFacts());
    REQUIRE(kGood.size() == 7);
    const char* const kOrder[] = {"steam", "overlay", "firewall", "wer", "ram", "disk", "toolchain"};
    for (size_t i = 0; i < 7; ++i)
    {
        CHECK(kGood[i].sCheck == kOrder[i]);
        const std::string sLine = PreflightLine(kGood[i]);
        CHECK_MESSAGE(kGood[i].bPass, sLine);
    }
    CHECK(PreflightLine(kGood[0]) == "[PASS] preflight:steam Steam running with a logged-in user");
    CHECK(kGood[2].sText == "1 rule paths have an inbound allow rule");
    CHECK(kGood[4].sText == "free 5098436 KB, floor 5098436 KB");
    CHECK(kGood[5].sText == "C: free 15483357102, floor 15483357102; GameData G:\\GameData");
    CHECK(kGood[6].sText == "MSBuild, dumpbin and runtimes present for Win32");

    PreflightFacts k = GoodFacts();
    k.sSteam = "no";
    CHECK(PreflightLine(Verdict(PreflightVerdicts(k), "steam")) ==
          "[FAIL] preflight:steam Steam is not running with a logged-in user (state 'no')");
    k = GoodFacts();
    k.sOverlay = "on";
    CHECK(Verdict(PreflightVerdicts(k), "overlay").sText ==
          "app 6060 overlay is 'on', not off: set OverlayAppEnable to 0 under apps\\6060 in localconfig.vdf with "
          "Steam shut down");
    k = GoodFacts();
    k.kAllowRules.clear();
    CHECK(Verdict(PreflightVerdicts(k), "firewall").sText ==
          "1 of 1 rule paths have no enabled inbound allow rule, first "
          "C:\\Dev\\OpenSWBF2\\bin\\Win32\\Release\\BattlefrontII.exe: run tests\\gate.cmd firewall elevated");
    k.kRulePaths.clear();
    CHECK(Verdict(PreflightVerdicts(k), "firewall").sText == "no rule paths for this tier");
    k.kGamePlatforms.clear();
    CHECK(Verdict(PreflightVerdicts(k), "firewall").bPass);
    CHECK(Verdict(PreflightVerdicts(k), "firewall").sText ==
          "no selected step starts the game exe on Win32, so no rule is needed");
    k = GoodFacts();
    k.kWer.push_back("a");
    k.kWer.push_back("b");
    CHECK(PreflightLine(Verdict(PreflightVerdicts(k), "wer")) == "[FAIL] preflight:wer a; b");
    k = GoodFacts();
    k.iFreeRamKB = kRamFloorKB - 1;
    CHECK(Verdict(PreflightVerdicts(k), "ram").sText == "free physical memory 5098435 KB is below the floor 5098436 KB");
    k = GoodFacts();
    k.uRepoFree = (unsigned long long)kDiskFloorBytes - 1;
    k.sGameData.clear();
    CHECK(Verdict(PreflightVerdicts(k), "disk").sText ==
          "C: free 15483357101 bytes is below the floor 15483357102 bytes; retail GameData not found (set "
          "SWBF2_GAMEDATA or install Steam App 6060; is its drive attached?)");
    k = GoodFacts();
    k.kToolchain[0].bMSBuild = false;
    k.kToolchain[0].bDumpbin = false;
    k.kToolchain[0].kMissingRuntimes.push_back("msvcr120.dll");
    k.kToolchain[0].kMissingRuntimes.push_back("msvcp120.dll");
    CHECK(Verdict(PreflightVerdicts(k), "toolchain").sText ==
          "Win32: MSBuild not found; Win32: dumpbin not found; Win32: missing from C:\\Windows\\SysWOW64: "
          "msvcr120.dll, msvcp120.dll");
    k.kToolchain.clear();
    CHECK(Verdict(PreflightVerdicts(k), "toolchain").sText == "no platform toolchain checked");

    std::vector<std::string> kGroups;
    kGroups.push_back("Standard");
    CHECK(GameRuntimes("Win32", kGroups).size() == 4);
    CHECK(GameRuntimes("x64", kGroups).size() == 2);
    kGroups[0] = "Vendor";
    CHECK(GameRuntimes("Win32", kGroups).size() == 2);
    CHECK(PlatformRuntimes(T1Runtimes(), "x64").back() == "vcruntime140_1.dll");
    CHECK(DefaultJobs(16) == 8);
    CHECK(DefaultJobs(6) == 3);
    CHECK(DefaultJobs(1) == 1);
}

TEST_CASE_NAMED(FGateMachineTest5, "Unit::Gate::firewall rule paths cover the other platform's exe", "[EngineFilter]")
{
    Step kCross;
    kCross.sName = "savecross-Test";
    kCross.sConfig = "Test";
    kCross.bStartsGame = true;
    kCross.kHashes.push_back("bin\\$Platform\\Test\\BattlefrontII.exe");
    kCross.kHashes.push_back("bin\\Win32\\Test\\BattlefrontII.exe");
    std::vector<const Step*> kSteps(1, &kCross);
    const std::string sRoot = "C:\\Dev\\OpenSWBF2";
    CHECK(StepConfigs(kSteps) == std::vector<std::string>(1, "Test"));
    const std::vector<std::string> kX64 = PreflightRulePaths(sRoot, kSteps, "x64", 2);
    REQUIRE(kX64.size() == 6);
    CHECK(kX64[0] == "C:\\Dev\\OpenSWBF2\\bin\\x64\\Test\\BattlefrontII.exe");
    CHECK(kX64[2] == "C:\\Dev\\OpenSWBF2\\artifacts\\smoke-workers\\x64\\Test\\smoke\\w2\\BattlefrontII.exe");
    CHECK(kX64[3] == "C:\\Dev\\OpenSWBF2\\bin\\Win32\\Test\\BattlefrontII.exe");
    CHECK(kX64[5] == "C:\\Dev\\OpenSWBF2\\artifacts\\smoke-workers\\Win32\\Test\\smoke\\w2\\BattlefrontII.exe");
    CHECK(PreflightRulePaths(sRoot, kSteps, "Win32", 2).size() == 3);
    kCross.bStartsGame = false;
    CHECK(PreflightRulePaths(sRoot, kSteps, "x64", 2).empty());
    std::string sPath;
    std::string sError;
    CHECK(WorkerPath(sRoot, "Win32", "Asan", "smoke", 1, sPath, sError));
    CHECK(sPath == "C:\\Dev\\OpenSWBF2\\artifacts\\smoke-workers\\Win32\\Asan\\smoke\\w1");
    CHECK_FALSE(WorkerPath(sRoot, "arm", "Asan", "smoke", 1, sPath, sError));
    CHECK_FALSE(WorkerPath(sRoot, "Win32", "Asan", "smoke", 0, sPath, sError));
}

TEST_CASE_NAMED(FGateMachineTest6, "Unit::Gate::evidence is judged", "[EngineFilter]")
{
    const std::string sDir = TempDir("gatejudge");
    GoodRun(sDir);
    JudgeSpec kRun;
    kRun.sKind = "run";
    kRun.bDirect = false;
    CHECK(JudgeEvidenceFolder(sDir, kRun) == "");

    SUBCASE("a results.txt failure")
    {
        PutFile(sDir, "runs\\r1\\results.txt", "[PASS] a\r\n[FAIL] stub run-a\r\n");
        CHECK(JudgeEvidenceFolder(sDir, kRun) == "run r1: results.txt has a failure line: [FAIL] stub run-a");
    }
    SUBCASE("an index.json failure")
    {
        PutFile(sDir, "runs\\r1\\index.json", "{ \"failed\": 1, \"tests\": [] }\r\n");
        CHECK(JudgeEvidenceFolder(sDir, kRun) == "run r1: index.json counts 1 failed");
        PutFile(sDir, "runs\\r1\\index.json", "{ \"failed\": 0, \"tests\": [{ \"fullTestPath\": \"A::b\", \"state\": \"Fail\" }] }");
        CHECK(JudgeEvidenceFolder(sDir, kRun) == "run r1: index.json test A::b is Fail");
        PutFile(sDir, "runs\\r1\\index.json", "{ \"failed\": 0 }");
        CHECK(JudgeEvidenceFolder(sDir, kRun) == "run r1: index.json has no failed count or test list");
        PutFile(sDir, "runs\\r1\\index.json", "{ \"failed\": 0, ");
        CHECK(JudgeEvidenceFolder(sDir, kRun) == "run r1: index.json does not parse");
    }
    SUBCASE("a unit index.json failure")
    {
        PutFile(sDir, "runs\\r1\\unit\\index.json", "{ \"failed\": 1, \"tests\": [] }\r\n");
        CHECK(JudgeEvidenceFolder(sDir, kRun) == "run r1: unit\\index.json counts 1 failed");
    }
    SUBCASE("a direct step that ran no test")
    {
        JudgeSpec kDirect = kRun;
        kDirect.bDirect = true;
        CHECK(JudgeEvidenceFolder(sDir, kDirect) == "run r1: index.json lists no test");
        DeleteFileW(Utf8ToWide(JoinPath(sDir, "runs\\r1\\index.json")).c_str());
        CHECK(JudgeEvidenceFolder(sDir, kDirect) == "run r1: no index.json");
    }
    SUBCASE("a MustRun case missing or twice")
    {
        JudgeSpec kDirect = kRun;
        kDirect.bDirect = true;
        kDirect.kMustRun.push_back("Unit::A::one");
        PutFile(sDir, "runs\\r1\\index.json", DirectIndex("Unit::A::two", 0));
        CHECK(JudgeEvidenceFolder(sDir, kDirect) == "run r1: index.json runs Unit::A::one 0 times");
        PutFile(sDir, "runs\\r1\\index.json", DirectIndex("Unit::A::one", "Unit::A::one"));
        CHECK(JudgeEvidenceFolder(sDir, kDirect) == "run r1: index.json runs Unit::A::one 2 times");
        PutFile(sDir, "runs\\r1\\index.json", DirectIndex("Unit::A::one", "Unit::A::two"));
        CHECK(JudgeEvidenceFolder(sDir, kDirect) == "");
    }
    SUBCASE("a MustRun case that skipped")
    {
        JudgeSpec kDirect = kRun;
        kDirect.bDirect = true;
        kDirect.kMustRun.push_back("Unit::A::one");
        PutFile(sDir, "runs\\r1\\index.json",
                "{ \"failed\": 0, \"tests\": [{ \"fullTestPath\": \"Unit::A::one\", \"state\": \"Skipped\" }, "
                "{ \"fullTestPath\": \"Unit::A::two\", \"state\": \"Skipped\" }] }\r\n");
        CHECK(JudgeEvidenceFolder(sDir, kDirect) == "run r1: index.json runs Unit::A::one with state Skipped");
        kDirect.kMustRun.clear();
        CHECK(JudgeEvidenceFolder(sDir, kDirect) == "");
    }
    SUBCASE("exit code and failure lines of each kind")
    {
        PutFile(sDir, "exit.txt", "1\r\n");
        CHECK(JudgeEvidenceFolder(sDir, kRun) == "exit.txt is 1");
        PutFile(sDir, "exit.txt", "0\r\n");
        JudgeSpec kScript = kRun;
        kScript.sKind = "script";
        PutFile(sDir, "stdout.txt", "OK a\r\nFAILED b: why\r\n");
        CHECK(JudgeEvidenceFolder(sDir, kScript) == "stdout.txt has a failure line: FAILED");
        JudgeSpec kCapture = kRun;
        kCapture.sKind = "capture";
        PutFile(sDir, "stdout.txt", "x [FAIL] y\r\n[FAIL] z\r\n");
        CHECK(JudgeEvidenceFolder(sDir, kCapture) == "stdout.txt has a failure line: [FAIL] ");
        PutFile(sDir, "stdout.txt", "x [FAIL] y\r\n");
        CHECK(JudgeEvidenceFolder(sDir, kCapture) == "");
        std::string sGone;
        CHECK(RemoveTree(JoinPath(sDir, "runs"), sGone));
        CHECK(JudgeEvidenceFolder(sDir, kRun) == "no archived run folder");
    }
    SUBCASE("saved evidence is intact and judged")
    {
        const std::string sRoot = JoinPath(sDir, "evidence-root");
        const std::string sWork = JoinPath(sDir, "work");
        PutFile(sWork, "evidence\\stdout.txt", "stub run-a\r\n");
        KeyPairs kRuns;
        kRuns.push_back(std::make_pair(std::string("0"), JoinPath(sDir, "runs\\r1")));
        std::string sHash;
        std::string sError;
        REQUIRE_MESSAGE(SaveEvidence(sRoot, sWork, 0, true, kRuns, std::vector<std::string>(), sHash, sError), sError);
        CHECK(IsEvidenceName(sHash));
        CHECK(EvidenceIntact(sRoot, sHash) == "");
        CHECK(EvidenceHash(JoinPath(sRoot, sHash)) == sHash);
        CHECK(JudgeArchived(sRoot, sHash, JoinPath(sDir, "judge1"), kRun) == "");
        CHECK(EvidenceIntact(sRoot, std::string(64, 'a')) == "evidence " + std::string(64, 'a') + " is missing");
        CHECK(EvidenceIntact(sRoot, "-") == "record names no evidence (-)");
        PutFile(JoinPath(sRoot, sHash), "extra.txt", "x");
        CHECK(EvidenceIntact(sRoot, sHash) == "evidence " + sHash + " no longer hashes to its name");
        std::string sRunsTxt;
        CHECK(ReadTextFile(JoinPath(sWork, "runs.txt"), sRunsTxt));
        CHECK(sRunsTxt == JoinPath(sDir, "runs\\r1") + "\r\n");
    }
    Cleanup(sDir);
}

TEST_CASE_NAMED(FGateMachineTest7, "Unit::Gate::evidence copy refuses a folder with a reparse point", "[EngineFilter]")
{
    const std::string sDir = TempDir("gatereparse");
    const std::string sTarget = JoinPath(sDir, "target");
    PutFile(sTarget, "sentinel.txt", "keep");
    PutFile(sDir, "run\\results.txt", "[PASS] a\r\n");
    PutFile(sDir, "out\\table.txt", "row\r\n");
    PutFile(sDir, "work\\evidence\\stdout.txt", "ok\r\n");
    const std::string sRoot = JoinPath(sDir, "evidence-root");
    KeyPairs kRuns;
    kRuns.push_back(std::make_pair(std::string("r1"), JoinPath(sDir, "run")));
    std::vector<std::string> kOutputs(1, JoinPath(sDir, "out"));
    std::string sHash;
    std::string sError;

    SUBCASE("a run folder")
    {
        REQUIRE(MakeJunction(JoinPath(sDir, "run\\link"), sTarget));
        CHECK_FALSE(SaveEvidence(sRoot, JoinPath(sDir, "work"), 0, true, kRuns, kOutputs, sHash, sError));
        CHECK(sError == "run folder " + JoinPath(sDir, "run") + " holds a reparse point");
    }
    SUBCASE("an output folder")
    {
        REQUIRE(MakeJunction(JoinPath(sDir, "out\\link"), sTarget));
        CHECK_FALSE(SaveEvidence(sRoot, JoinPath(sDir, "work"), 0, true, kRuns, kOutputs, sHash, sError));
        CHECK(sError == "output folder " + JoinPath(sDir, "out") + " holds a reparse point");
    }
    std::vector<std::string> kLeft;
    std::string sScan;
    CHECK(FindReparsePoints(sRoot, kLeft, sScan));
    CHECK(kLeft.empty());
    WIN32_FIND_DATAW kData;
    HANDLE hFind = FindFirstFileW(Utf8ToWide(JoinPath(sRoot, "stage-*")).c_str(), &kData);
    CHECK(hFind == INVALID_HANDLE_VALUE);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
    }
    CHECK(FileExists(JoinPath(sTarget, "sentinel.txt")));
    Cleanup(sDir);
}

TEST_CASE_NAMED(FGateMachineTest8, "Unit::Gate::index json reader", "[EngineFilter]")
{
    Report kReport;
    kReport.sCreatedOn = "2026.11.25-12.00.00";
    kReport.fTotalDuration = 1.5;
    ReportTest kPass;
    kPass.sDisplayName = "one";
    kPass.sFullPath = "Unit::A::one";
    kPass.kTags.push_back("[SmokeFilter]");
    kPass.sState = "Success";
    kPass.fDuration = 0.25;
    kPass.sDateTime = "2026.11.25-12.00.00";
    ReportTest kFail = kPass;
    kFail.sFullPath = "Unit::A::a \"quoted\" \\ name\twith tab";
    kFail.sState = "Fail";
    ReportEntry kEntry = {"Error", "line one\nline two"};
    kFail.kEntries.push_back(kEntry);
    kFail.kArtifacts.push_back("C:\\out\\case.txt");
    kReport.kTests.push_back(kPass);
    kReport.kTests.push_back(kFail);
    const std::string sDir = TempDir("gateindex");
    const std::string sFile = JoinPath(sDir, "index.json");
    REQUIRE(WriteReport(sFile, kReport));
    std::string sText;
    REQUIRE(ReadTextFile(sFile, sText));
    IndexJson kIndex;
    std::string sError;
    REQUIRE_MESSAGE(ReadIndexJson(sText, kIndex, sError), sError);
    CHECK(kIndex.bHasFailed);
    CHECK(kIndex.sFailed == "1");
    CHECK(kIndex.bHasTests);
    REQUIRE(kIndex.kTests.size() == 2);
    CHECK(kIndex.kTests[0].sFullPath == "Unit::A::one");
    CHECK(kIndex.kTests[0].sState == "Success");
    CHECK(kIndex.kTests[1].sFullPath == kFail.sFullPath);
    CHECK(kIndex.kTests[1].sState == "Fail");

    Report kEmpty;
    kEmpty.fTotalDuration = 0;
    REQUIRE(WriteReport(sFile, kEmpty));
    REQUIRE(ReadTextFile(sFile, sText));
    REQUIRE(ReadIndexJson(sText, kIndex, sError));
    CHECK(kIndex.sFailed == "0");
    CHECK(kIndex.kTests.empty());

    const char* const kBad[] = {"",
                                "{",
                                "{ \"failed\": 0, }",
                                "[1]",
                                "{} x",
                                "{ \"a\": \"x\ny\" }",
                                "{ \"a\": \"\\q\" }",
                                "{ \"a\": 01 }",
                                "{ \"a\": tru }",
                                "{ \"a\": [1, ] }"};
    for (size_t i = 0; i < sizeof(kBad) / sizeof(kBad[0]); ++i)
    {
        CHECK_MESSAGE(!ReadIndexJson(kBad[i], kIndex, sError), kBad[i]);
    }
    std::string sDeep;
    for (int i = 0; i < 100; ++i)
    {
        sDeep += "[";
    }
    CHECK_FALSE(ReadIndexJson("{ \"a\": " + sDeep + " }", kIndex, sError));
    DeleteFileW(Utf8ToWide(sFile).c_str());
    RemoveDirectoryW(Utf8ToWide(sDir).c_str());
}

TEST_CASE_NAMED(FGateMachineTest9, "Unit::Gate::a direct step gets results and run params", "[EngineFilter]")
{
    const std::string sOutput = "runner: 3 cases\r\n[PASS] Unit::A::one (12 ms)\r\n[FAIL] Unit::A::two - why\r\n"
                                "  ERROR: detail\r\n[N/A] Unit::A::three\r\n[SKIP] Unit::A::four\r\nnoise [PASS] x\r\n";
    const std::vector<std::string> kLines = VerdictLines(sOutput);
    REQUIRE(kLines.size() == 4);
    CHECK(kLines[2] == "[N/A] Unit::A::three");
    CHECK(ResultsText(kLines, true, "") ==
          "[PASS] Unit::A::one (12 ms)\r\n[FAIL] Unit::A::two - why\r\n[N/A] Unit::A::three\r\n[SKIP] Unit::A::four\r\n"
          "\r\nResults\r\n  PASS     1\r\n  FAIL     1\r\n  SKIP     2\r\n  STALL    0\r\n  CRASH    0\r\n"
          "ASan covers stack, globals, CRT heap and the game heaps and pools\r\n");
    CHECK(ResultsText(std::vector<std::string>(), false, "note") ==
          "\r\nResults\r\n  PASS     0\r\n  FAIL     0\r\n  SKIP     0\r\n  STALL    0\r\n  CRASH    0\r\nnote\r\n");

    RunParamsInput kIn;
    kIn.kArgs.push_back("--runner");
    kIn.kArgs.push_back("--config=Release");
    kIn.sPlatform = "Win32";
    kIn.sConfiguration = "Release";
    kIn.sSmokeConfiguration = "Release";
    kIn.bWerLocalDumps = true;
    kIn.sSteam = "yes";
    kIn.sTestsExe = "C:\\Dev\\OpenSWBF2\\bin\\Tests\\Win32\\Release\\OpenSWBF2Tests.exe";
    kIn.kExes.push_back(std::make_pair(std::string("BattlefrontII.exe"), std::string(64, 'b')));
    kIn.kDlls.push_back(std::make_pair(std::string("steam_api.dll"), std::string(64, 'c')));
    const std::vector<std::string> kParams = RunParamsLines(kIn);
    const char* const kExpected[] = {"arg=--runner",
                                     "arg=--config=Release",
                                     "platform=Win32",
                                     "configuration=Release",
                                     "smoke_configuration=Release",
                                     "wer_localdumps=1",
                                     "steam=yes",
                                     "tests_exe=C:\\Dev\\OpenSWBF2\\bin\\Tests\\Win32\\Release\\OpenSWBF2Tests.exe"};
    REQUIRE(kParams.size() == 10);
    for (size_t i = 0; i < 8; ++i)
    {
        CHECK(kParams[i] == kExpected[i]);
    }
    CHECK(kParams[8] == "exe.BattlefrontII.exe=" + std::string(64, 'b'));
    CHECK(kParams[9] == "dll.steam_api.dll=" + std::string(64, 'c'));
    const std::map<std::string, std::string> kMap = ReadKeyValueText(LinesText(kParams));
    CHECK(kMap.find("platform")->second == "Win32");
    CHECK(kMap.find("smoke_configuration")->second == "Release");
    CHECK(kMap.find("exe.BattlefrontII.exe")->second == std::string(64, 'b'));
}

TEST_CASE_NAMED(FGateMachineTest10, "Unit::Gate::host fault counts are written at start and end", "[EngineFilter]")
{
    std::vector<std::string> kLines;
    CHECK(HostFaultLines("start", FakeCount, 0, kLines));
    CHECK(HostFaultLines("end", FakeCount, 1234, kLines));
    REQUIRE(kLines.size() == 3);
    CHECK(kLines[0] == "whea_before=5");
    CHECK(kLines[1] == "whea_after=5");
    CHECK(kLines[2] == "whea_delta=2");
    std::vector<std::string> kBroken;
    CHECK(HostFaultLines("end", BrokenCount, 0, kBroken));
    CHECK(kBroken[0] == "whea_after=unavailable");
    CHECK(kBroken[1] == "whea_delta=unavailable");
    CHECK_FALSE(HostFaultLines("middle", FakeCount, 0, kBroken));
    const std::map<std::string, std::string> kParams = ReadKeyValueText(LinesText(kLines));
    CHECK(HostFaultNote(kParams, 1) ==
          "Suspected hardware: 2 host fault event(s) (WHEA-Logger, Kernel-Power 41, EventLog 6008) in this run window, "
          "whea_before=5 whea_after=5; the failures above stay FAIL");
    CHECK(HostFaultNote(kParams, 0) == "");
    CHECK(HostFaultNote(ReadKeyValueText("whea_delta=0\r\n"), 1) == "");
    CHECK(HostFaultNote(ReadKeyValueText("whea_delta=unavailable\r\n"), 1) == "");
    long long iCount = -1;
    CHECK(HostFaultCount(false, 0, iCount));
    CHECK(iCount >= 0);
}

TEST_CASE_NAMED(FGateMachineTest11, "Unit::Gate::tree removal refuses a reparse point a repository and a file", "[EngineFilter]")
{
    const std::string sDir = TempDir("gatetree");
    const std::string sData = JoinPath(sDir, "retail\\data");
    PutFile(sData, "sentinel.txt", "keep");
    std::string sError;

    SUBCASE("a reparse point")
    {
        const std::string sLink = JoinPath(sDir, "link");
        REQUIRE(MakeJunction(sLink, sData));
        CHECK_FALSE(RemoveTree(sLink, sError));
        CHECK(sError == "REFUSING to remove " + sLink + ": it is a reparse point");
        const std::string sHolder = JoinPath(sDir, "holder");
        PutFile(sHolder, "a.txt", "a");
        REQUIRE(MakeJunction(JoinPath(sHolder, "other"), sData));
        CHECK_FALSE(RemoveTree(sHolder, sError));
        CHECK(sError == "REFUSING to remove " + sHolder + ": reparse points: " + JoinPath(sHolder, "other"));
        CHECK(FileExists(JoinPath(sHolder, "a.txt")));
    }
    SUBCASE("a repository")
    {
        const std::string sRepo = JoinPath(sDir, "repo");
        PutFile(sRepo, ".git\\HEAD", "ref: refs/heads/main\n");
        CHECK_FALSE(RemoveTree(sRepo, sError));
        CHECK(sError == "REFUSING to remove " + sRepo + ": it holds a .git folder");
        DeleteFileW(Utf8ToWide(JoinPath(sRepo, ".git\\HEAD")).c_str());
        RemoveDirectoryW(Utf8ToWide(JoinPath(sRepo, ".git")).c_str());
    }
    SUBCASE("a file")
    {
        const std::string sFile = JoinPath(sDir, "file.txt");
        PutFile(sDir, "file.txt", "x");
        CHECK_FALSE(RemoveTree(sFile, sError));
        CHECK(sError == "REFUSING to remove " + sFile + ": not a folder");
    }
    SUBCASE("a tree that holds a data junction")
    {
        const std::string sWorker = JoinPath(sDir, "w1");
        PutFile(sWorker, "BattlefrontII.exe", "x");
        PutFile(sWorker, "sub\\deep.txt", "y");
        REQUIRE(MakeJunction(JoinPath(sWorker, "data"), sData));
        CHECK_MESSAGE(RemoveTree(sWorker, sError), sError);
        CHECK_FALSE(DirExists(sWorker));
        CHECK(FileExists(JoinPath(sData, "sentinel.txt")));
        CHECK(RemoveTree(sWorker, sError));
        CHECK(RemoveTree("", sError) == false);
        CHECK(sError == "Remove-HarnessTree needs a folder");
    }
    SUBCASE("a tree whose data junction target is gone")
    {
        const std::string sGone = JoinPath(sDir, "gone\\data");
        PutFile(sGone, "x.txt", "x");
        const std::string sWorker = JoinPath(sDir, "w2");
        PutFile(sWorker, "BattlefrontII.exe", "x");
        REQUIRE(MakeJunction(JoinPath(sWorker, "data"), sGone));
        REQUIRE(RemoveTree(JoinPath(sDir, "gone"), sError));
        CHECK_MESSAGE(RemoveTree(sWorker, sError), sError);
        CHECK_FALSE(DirExists(sWorker));
    }
    CHECK(FileExists(JoinPath(sData, "sentinel.txt")));
    Cleanup(sDir);
}

TEST_CASE_NAMED(FGateMachineTest12, "Unit::Gate::arguments are quoted as the runner quotes them", "[EngineFilter]")
{
    const char* const kArgs[] = {"",
                                 "a",
                                 "a b",
                                 "a\\",
                                 "a\\\\b",
                                 "a\"b",
                                 "a\\\"b",
                                 "C:\\path with space\\",
                                 "--exclude=C:\\Dev\\OpenSWBF2\\tests\\excluded.tsv",
                                 "--tags=*[Standard]*"};
    for (size_t i = 0; i < sizeof(kArgs) / sizeof(kArgs[0]); ++i)
    {
        CHECK_MESSAGE(Gate::QuoteArg(kArgs[i]) == ::QuoteArg(kArgs[i]), kArgs[i]);
    }
    std::vector<std::string> kList;
    kList.push_back("--runner");
    kList.push_back("a b");
    CHECK(CommandLine("C:\\x\\t.exe", kList) == "\"C:\\x\\t.exe\" \"--runner\" \"a b\"");
    wchar_t acSystem[MAX_PATH];
    REQUIRE(GetSystemDirectoryW(acSystem, MAX_PATH) > 0);
    ProcessSpec kSpec = NewProcessSpec("where", std::vector<std::string>(1, "$OPENSWBF2_GATE_PROBE:cmd.exe"));
    kSpec.kEnv.push_back(std::make_pair(std::string("OPENSWBF2_GATE_PROBE"), WideToUtf8(acSystem)));
    const ProcessResult kFound = RunProcess(kSpec);
    CHECK(kFound.bStarted);
    CHECK(kFound.uCode == 0);
    CHECK(LowerText(kFound.sOutput).find("cmd.exe") != std::string::npos);
    kSpec.kArgs[0] = "$OPENSWBF2_GATE_PROBE:no-such-file.exe";
    kSpec.bMergeErr = false;
    const ProcessResult kMissing = RunProcess(kSpec);
    CHECK(kMissing.bStarted);
    CHECK(kMissing.uCode == 1);
    CHECK(kMissing.sOutput.empty());
    const ProcessResult kNone = RunProcess(NewProcessSpec("openswbf2-no-such-program", std::vector<std::string>()));
    CHECK_FALSE(kNone.bStarted);
}

TEST_CASE_NAMED(FGateMachineTest13, "Unit::Gate::clean removes staged output and artifact junctions", "[EngineFilter]")
{
    const std::string sDir = TempDir("gateclean");
    const std::string sRoot = JoinPath(sDir, "repo");
    const std::string sData = JoinPath(sDir, "retail\\data");
    PutFile(sData, "sentinel.txt", "keep");
    const char* const kFolders[] = {"bin\\Win32\\Release", "bin\\Tests\\Win32\\Debug", "bin\\Win32\\Test",
                                    "bin\\Tests\\Win32\\Test", "bin\\x64\\Release",       "bin\\Win32\\Debug"};
    for (size_t i = 0; i < sizeof(kFolders) / sizeof(kFolders[0]); ++i)
    {
        PutFile(JoinPath(sRoot, kFolders[i]), "BattlefrontII.exe", "x");
    }
    REQUIRE(MakeJunction(JoinPath(sRoot, "bin\\Win32\\Release\\data"), sData));
    REQUIRE(MakeJunction(JoinPath(sRoot, "bin\\Win32\\Test\\data"), sData));
    const std::string sWorker = JoinPath(sRoot, "artifacts\\smoke-workers\\Win32\\Release\\smoke\\w1");
    PutFile(sWorker, "BattlefrontII.exe", "x");
    REQUIRE(MakeJunction(JoinPath(sWorker, "data"), sData));
    const std::vector<std::string> kWin32(1, "Win32");
    std::vector<std::string> kLines;
    std::string sError;
    REQUIRE_MESSAGE(CleanStaged(sRoot, kWin32, std::vector<std::string>(), kLines, sError), sError);
    CHECK(JoinText(kLines, "|") == "Removed " + JoinPath(sRoot, "bin\\Win32\\Release") + "|Removed " +
                                       JoinPath(sRoot, "bin\\Tests\\Win32\\Debug") + "|Removed 1 junction(s) under " +
                                       JoinPath(sRoot, "artifacts"));
    CHECK_FALSE(DirExists(JoinPath(sRoot, "bin\\Win32\\Release")));
    CHECK_FALSE(DirExists(JoinPath(sRoot, "bin\\Tests\\Win32\\Debug")));
    CHECK(DirExists(JoinPath(sRoot, "bin\\Win32\\Test")));
    CHECK(DirExists(JoinPath(sRoot, "bin\\x64\\Release")));
    CHECK(DirExists(JoinPath(sRoot, "bin\\Win32\\Debug")));
    CHECK(FileExists(JoinPath(sWorker, "BattlefrontII.exe")));
    CHECK_FALSE(DirExists(JoinPath(sWorker, "data")));
    CHECK(FileExists(JoinPath(sData, "sentinel.txt")));
    kLines.clear();
    const std::vector<std::string> kTest(1, "Test");
    REQUIRE_MESSAGE(CleanStaged(sRoot, kWin32, kTest, kLines, sError), sError);
    CHECK(JoinText(kLines, "|") == "Removed " + JoinPath(sRoot, "bin\\Win32\\Test") + "|Removed " +
                                       JoinPath(sRoot, "bin\\Tests\\Win32\\Test"));
    CHECK_FALSE(DirExists(JoinPath(sRoot, "bin\\Win32\\Test")));
    CHECK(DirExists(JoinPath(sRoot, "bin\\x64\\Release")));
    CHECK(FileExists(JoinPath(sData, "sentinel.txt")));
    kLines.clear();
    REQUIRE(CleanStaged(sRoot, kWin32, kTest, kLines, sError));
    CHECK(kLines.empty());
    const std::string sLinked = JoinPath(sRoot, "bin\\x64\\Asan");
    REQUIRE(MakeJunction(sLinked, sData));
    CHECK_FALSE(CleanStaged(sRoot, std::vector<std::string>(1, "x64"), std::vector<std::string>(1, "Asan"), kLines, sError));
    CHECK(sError == "REFUSING to remove " + sLinked + ": it is a reparse point");
    CHECK(FileExists(JoinPath(sData, "sentinel.txt")));
    std::vector<std::string> kBoth;
    kBoth.push_back("Win32");
    kBoth.push_back("x64");
    CHECK(JoinText(CleanFolders("C:\\R", kBoth, std::vector<std::string>()), "|") ==
          "C:\\R\\bin\\Win32\\Release|C:\\R\\bin\\Tests\\Win32\\Debug|C:\\R\\bin\\x64\\Release|C:\\R\\bin\\Tests\\x64\\Debug");
    Cleanup(sDir);
}

namespace
{

std::vector<std::string>& FakePaths()
{
    static std::vector<std::string> sk;
    return sk;
}

bool FakeExists(const std::string& sPath)
{
    for (size_t i = 0; i < FakePaths().size(); ++i)
    {
        if (EqualNoCase(FakePaths()[i], sPath))
        {
            return true;
        }
    }
    return false;
}

FirewallRule Rule(const std::string& sProgram)
{
    FirewallRule k;
    k.sName = "OpenSWBF2 test " + sProgram;
    k.sProgram = sProgram;
    return k;
}

std::string RuleNames(const std::vector<FirewallRule>& kRules)
{
    std::vector<std::string> k;
    for (size_t i = 0; i < kRules.size(); ++i)
    {
        k.push_back(kRules[i].sProgram);
    }
    return JoinText(k, "|");
}

}

TEST_CASE_NAMED(FGateMachineTest14, "Unit::Gate::firewall plan", "[EngineFilter]")
{
    const std::string sRoot = "C:\\Dev\\OpenSWBF2";
    std::vector<std::string> kPlatforms;
    kPlatforms.push_back("Win32");
    kPlatforms.push_back("x64");
    const std::vector<std::string> kConfigs(kWorkerConfigurations, kWorkerConfigurations + 4);
    const std::vector<std::string> kWant = FirewallRulePaths(sRoot, kPlatforms, kConfigs, 2);
    REQUIRE(kWant.size() == 24);
    std::vector<FirewallRule> kRules;
    for (size_t i = 2; i < kWant.size(); ++i)
    {
        kRules.push_back(Rule(kWant[i]));
    }
    kRules.push_back(Rule(UpperText(kWant[1])));
    const std::string sWorker = "C:\\DEV\\OPENSWBF2\\ARTIFACTS\\SMOKE-WORKERS\\Win32\\Release\\smoke\\w3\\BattlefrontII.exe";
    const std::string sGone = sRoot + "\\artifacts\\tests\\20260101-000000\\BattlefrontII.exe";
    const std::string sKeptStale = sRoot + "\\artifacts\\laa-compare\\x\\BattlefrontII.exe";
    const std::string sOutside = "D:\\Games\\GameData\\BattlefrontII.exe";
    const std::string sOutsideGone = "D:\\Old\\BattlefrontII.exe";
    const std::string sClaude = "C:\\Users\\u\\.claude\\w\\BattlefrontII.exe";
    const std::string sClaudeKept = "C:\\Users\\u\\AppData\\Local\\Temp\\claude\\y\\BattlefrontII.exe";
    kRules.push_back(Rule(sWorker));
    kRules.push_back(Rule(sGone));
    kRules.push_back(Rule(sKeptStale));
    kRules.push_back(Rule(sOutside));
    kRules.push_back(Rule(sOutsideGone));
    kRules.push_back(Rule(sClaude));
    kRules.push_back(Rule(sClaudeKept));
    FakePaths().clear();
    FakePaths().push_back(sWorker);
    FakePaths().push_back(sKeptStale);
    FakePaths().push_back(sOutside);
    FakePaths().push_back(sClaudeKept);
    const FirewallPlan kPlan = PlanFirewall(std::vector<std::string>(1, sRoot), 2, kRules, FakeExists);
    REQUIRE(kPlan.kRoots.size() == 1);
    const FirewallRootPlan& k = kPlan.kRoots[0];
    CHECK(k.uWanted == 24);
    CHECK(JoinText(k.kAdd, "|") == kWant[0]);
    CHECK(RuleNames(k.kRemove) == sWorker + "|" + sGone);
    CHECK(RuleNames(kPlan.kStrays) == sClaude);
    const std::vector<std::string> kList = FirewallPlanLines(kPlan, true);
    REQUIRE(kList.size() == 6);
    CHECK(kList[0] == "ADD " + kWant[0]);
    CHECK(kList[1] == "REMOVE OpenSWBF2 test " + sWorker + " " + sWorker);
    CHECK(kList[3] == "REMOVE OpenSWBF2 test " + sClaude + " " + sClaude);
    CHECK(kList[4] == "root " + sRoot + ": 24 rule paths, missing 1, stale 2; would add 1 rules, would remove 2 rules");
    CHECK(kList[5] == "would add 1 rules, would remove 3 rules");
    CHECK(FirewallPlanLines(kPlan, false)[5] == "added 1 rules, removed 3 rules");
    FakePaths().clear();
}
