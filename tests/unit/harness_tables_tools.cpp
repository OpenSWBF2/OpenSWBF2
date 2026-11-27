#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <map>
#include <string>
#include <vector>

#include "tables/tables_commit.h"
#include "tables/tables_readers.h"
#include "tables/tables_repin.h"

using namespace Tables;

namespace
{

const std::string kShaA(64, 'a');
const std::string kShaB(64, 'b');

std::string Lines(const std::vector<std::string>& k)
{
    return JoinText(k, "|");
}

struct RepinFixture
{
    std::map<std::string, std::string> kFiles;
    std::string sExeSha;
    RepinInput kIn;

    RepinFixture()
    {
        sExeSha = kShaA;
        kFiles["R\\run\\run-params.txt"] = "arg=--runner\r\nplatform=Win32\r\nconfiguration=Release\r\n"
                                           "smoke_configuration=Release\r\nexe.BattlefrontII.exe=" +
                                           kShaA + "\r\n";
        kFiles["R\\tests\\baseline\\v143\\CAUSES.md"] = "# causes\r\n"
                                                        "- `smoke-all-digests geo1c_con` | 51.2 | a cause\r\n"
                                                        "- `mem-peaks geo1c_con/exit/heap/Runtime` | 51.2 | a cause\r\n"
                                                        "- `lua-bins geo1c_con/turn1500/1` | 51.2 | a cause\r\n"
                                                        "- `offthread c1` | 51.2 | a cause\r\n"
                                                        "- `digest-expected geo1c_con` | 51.2 | a cause\r\n"
                                                        "- `pool-sizes nab2g_c` without a commit\r\n";
        kFiles["R\\run\\tables\\smoke-all-digests.txt"] = "geo1c_con\tSim digest new\tnone\r\nnab2g_c\tSim digest other\tnone\r\n";
        kFiles["R\\tests\\baseline\\v143\\smoke-all-digests.txt"] =
            "cor1c_c\ta\tnone\r\ngeo1c_con\tSim digest old\tnone\r\nnab2g_c\tSim digest keep\tnone\r\n";
        kIn.sRoot = "R";
        kIn.sRunDir = "R\\run";
        kIn.sTable = "smoke-all-digests";
        kIn.kKeyArgs = {"geo1c_con"};
        Bind();
    }

    void Bind()
    {
        const std::map<std::string, std::string>* pFiles = &kFiles;
        const std::string* pSha = &sExeSha;
        kIn.fnRead = [pFiles](const std::string& sPath) {
            std::map<std::string, std::string>::const_iterator it = pFiles->find(sPath);
            return it == pFiles->end() ? Absent() : Found(it->second);
        };
        kIn.fnExeSha = [pSha](const std::string& sPath, std::string& sSha) {
            if (sPath != "R\\bin\\Win32\\Release\\BattlefrontII.exe" && sPath != "R\\other.exe")
            {
                return false;
            }
            sSha = *pSha;
            return true;
        };
    }
};

std::string Part(const char* pcState, long long llArena, long long llGameCrt, long long llGameVm)
{
    return std::string("[00:00][  1]LogMemory: Inform: Commit state=") + pcState + " bytes=1\r\n" +
           "[00:00][  1]LogMemory: Inform: Commit parts state=" + pcState + " private=1 gamecrt=" +
           std::to_string(llGameCrt) + " heaps=1 threads=1 threadcount=1 arena=" + std::to_string(llArena) +
           " crtbusy=1 crtcommitted=1 crtlarge=1 gamevm=" + std::to_string(llGameVm) + "\r\n";
}

std::string CommitLog(int iCycles, long long llLastArena)
{
    std::string s;
    for (int i = 1; i <= iCycles; ++i)
    {
        const long long llArena = i <= 8 ? 50 + i : (i == iCycles ? llLastArena : 100);
        s += Part("mission", llArena, 10, 290);
        s += Part("shell", i <= 8 ? 20 + i : 40, 5, 95);
    }
    return s;
}

CaptureRun GoodRun()
{
    CaptureRun k;
    k.bStarted = true;
    k.sRunDir = "R\\artifacts\\tests\\s1";
    k.iRunCode = 0;
    k.sSteamState = "Closed";
    k.sSeen = "no";
    k.bHog = true;
    k.kCommitFlatLogs = {CommitLog(10, 100)};
    k.sShortSha = "abcd1234";
    k.sOutFull = "C:\\scratch\\commit";
    return k;
}

CapturePreflight GoodPreflight()
{
    CapturePreflight k;
    k.sPlatform = "x64";
    k.sSteamState = "Running";
    k.bHog = true;
    k.sOutFull = "C:\\R\\artifacts\\commit";
    k.sBaselineFull = "C:\\R\\tests\\baseline";
    k.kProcesses = {{4, "System"}, {10, "OpenSWBF2Tests.exe"}, {11, "steam.exe"}};
    k.uSelfPid = 10;
    k.sSteamRunning = "yes";
    return k;
}

}

TEST_CASE_NAMED(FRepinRefusalsTest, "Unit::Repin::refusals", "[EngineFilter]")
{
    RepinFixture f;
    SUBCASE("every rule holds and only the named rows change")
    {
        const RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 0);
        CHECK(k.sWritePath == "R\\tests\\baseline\\v143\\smoke-all-digests.txt");
        CHECK(k.sWriteText == "cor1c_c\ta\tnone\r\ngeo1c_con\tSim digest new\tnone\r\nnab2g_c\tSim digest keep\tnone\r\n");
        CHECK(Lines(k.kLines) == "[PASS] repin smoke-all-digests geo1c_con (1 rows from exe " + kShaA + ")");
    }
    SUBCASE("no key")
    {
        f.kIn.kKeyArgs = {" , ", ""};
        const RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 2);
        CHECK(Lines(k.kLines) == "[FAIL] repin - no -Key given");
    }
    SUBCASE("no run-params.txt")
    {
        f.kFiles.erase("R\\run\\run-params.txt");
        const RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 2);
        CHECK(Lines(k.kLines) == "[FAIL] repin - no run-params.txt in R\\run");
    }
    SUBCASE("no platform or game configuration in run-params.txt")
    {
        f.kFiles["R\\run\\run-params.txt"] = "platform=Win32\r\nconfiguration=Release\r\nexe.BattlefrontII.exe=" + kShaA;
        RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 2);
        CHECK(Lines(k.kLines) == "[FAIL] repin - run-params.txt names no platform or smoke configuration");
        f.kFiles["R\\run\\run-params.txt"] = "Smoke_Configuration=Release\r\nexe.BattlefrontII.exe=" + kShaA;
        CHECK(Repin(f.kIn).iExit == 2);
        f.kFiles["R\\run\\run-params.txt"] = "PLATFORM=Win32\r\nSmoke_Configuration=Release\r\nexe.BattlefrontII.exe=" + kShaA;
        CHECK(Repin(f.kIn).iExit == 0);
        f.kFiles["R\\run\\run-params.txt"] = "exe.BattlefrontII.exe=" + kShaA;
        f.kIn.sExe = "R\\other.exe";
        CHECK(Repin(f.kIn).iExit == 0);
    }
    SUBCASE("no exe hash")
    {
        f.kFiles["R\\run\\run-params.txt"] = "platform=Win32\r\nsmoke_configuration=Release\r\nexe.BattlefrontII.exe=ABC";
        RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 1);
        CHECK(Lines(k.kLines) == "[FAIL] repin - run-params.txt has no exe hash ('ABC')");
        f.kFiles["R\\run\\run-params.txt"] =
            "platform=Win32\r\nsmoke_configuration=Release\r\nexe.BattlefrontII.exe=" + std::string(64, 'A');
        CHECK(Repin(f.kIn).iExit == 1);
    }
    SUBCASE("exe not found")
    {
        f.kFiles["R\\run\\run-params.txt"] =
            "platform=x64\r\nsmoke_configuration=Test\r\nexe.BattlefrontII.exe=" + kShaA + "\r\n";
        const RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 1);
        CHECK(Lines(k.kLines) == "[FAIL] repin - exe not found: R\\bin\\x64\\Test\\BattlefrontII.exe");
    }
    SUBCASE("the run's exe is not the current exe")
    {
        f.sExeSha = kShaB;
        const RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 1);
        CHECK(Lines(k.kLines) == "[FAIL] repin - the run's exe " + kShaA + " is not the current exe " + kShaB +
                                     " (R\\bin\\Win32\\Release\\BattlefrontII.exe)");
    }
    SUBCASE("no CAUSES.md line for a row")
    {
        f.kIn.kKeyArgs = {"nab2g_c,geo1c_con"};
        RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 1);
        CHECK(Lines(k.kLines) ==
              "[FAIL] repin - no cause line in R\\tests\\baseline\\v143\\CAUSES.md names 'smoke-all-digests nab2g_c'");
        f.kIn.sTable = "pool-sizes";
        f.kIn.kKeyArgs = {"nab2g_c"};
        CHECK(Repin(f.kIn).iExit == 1);
    }
    SUBCASE("a row missing from the run or from the pinned table")
    {
        f.kFiles["R\\run\\tables\\smoke-all-digests.txt"] = "nab2g_c\tSim digest other\tnone\r\n";
        RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 1);
        CHECK(Lines(k.kLines) == "[FAIL] repin - geo1c_con is not a row of R\\run\\tables\\smoke-all-digests.txt");
        f.kFiles["R\\tests\\baseline\\v143\\smoke-all-digests.txt"] = "cor1c_c\ta\tnone\r\n";
        k = Repin(f.kIn);
        CHECK(Lines(k.kLines) == "[FAIL] repin - geo1c_con is not a row of R\\run\\tables\\smoke-all-digests.txt|"
                                 "[FAIL] repin - geo1c_con is not a row of R\\tests\\baseline\\v143\\smoke-all-digests.txt");
        f.kFiles.erase("R\\run\\tables\\smoke-all-digests.txt");
        k = Repin(f.kIn);
        CHECK(k.iExit == 1);
        CHECK(Lines(k.kLines) == "[FAIL] repin - R\\run\\tables\\smoke-all-digests.txt not found");
    }
    SUBCASE("a table that is not one the script accepts")
    {
        f.kIn.sTable = "lua-hist";
        const RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 1);
        CHECK(Lines(k.kLines) == "[FAIL] repin - -Table lua-hist does not belong to the set "
                                 "smoke-all-digests,mem-peaks,pool-sizes,cloth-headroom,lua-bins,offthread,digest-expected");
    }
    SUBCASE("a digest-expected row comes from the run's digest line")
    {
        f.kIn.sTable = "digest-expected";
        f.kFiles["R\\run\\smoke\\geo1c_con\\run.log"] = "[00:00]LogSim: Inform: Sim digest turns=1500 state=0x1\r\n";
        f.kFiles["R\\tests\\smoke\\digest-expected.txt"] = "# expected\nnab2g_c\tSim digest x\ngeo1c_con\tSim digest old\n";
        RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 0);
        CHECK(k.sWritePath == "R\\tests\\smoke\\digest-expected.txt");
        CHECK(k.sWriteText == "# expected\nnab2g_c\tSim digest x\ngeo1c_con\tSim digest turns=1500 state=0x1\n");
        f.kFiles.erase("R\\run\\smoke\\geo1c_con\\run.log");
        k = Repin(f.kIn);
        CHECK(Lines(k.kLines) == "[FAIL] repin - geo1c_con has no SIM DIGEST line in R\\run\\smoke\\geo1c_con");
    }
}

TEST_CASE_NAMED(FRepinTablesTest, "Unit::Repin::tables and key columns come from the reader table", "[EngineFilter]")
{
    const std::map<std::string, int> kWant = {{"smoke-all-digests", 1}, {"mem-peaks", 4}, {"pool-sizes", 1},
                                              {"cloth-headroom", 1},    {"lua-bins", 3},  {"offthread", 1},
                                              {"digest-expected", 1}};
    REQUIRE(RepinTables().size() == kWant.size());
    for (size_t i = 0; i < RepinTables().size(); ++i)
    {
        std::string sFile;
        int iCols = 0;
        std::string sWhy;
        CHECK(RepinTableFile(RepinTables()[i], sFile, iCols, sWhy));
        CHECK(sFile == RepinTables()[i] + ".txt");
        CHECK(iCols == kWant.at(RepinTables()[i]));
    }
    std::string sFile;
    int iCols = 0;
    std::string sWhy;
    CHECK(!RepinTableFile("Mem-Peaks", sFile, iCols, sWhy));
    CHECK(sWhy == "the reader table has no key columns for Mem-Peaks.txt");
    CHECK(Lines(RepinKeys({"b, A", "a", " c ,b"})) == "A|b|c");
    CHECK(RunParam("Platform=a\r\nplatform=b\r\n=x\r\n", "PLATFORM") == "b");
    RepinFixture f;
    SUBCASE("mem-peaks rows are keyed on four columns")
    {
        f.kIn.sTable = "mem-peaks";
        f.kIn.kKeyArgs = {"geo1c_con/exit/heap/Runtime"};
        f.kFiles["R\\run\\tables\\mem-peaks.txt"] = "# commit a\r\nmission\tsource\r\n"
                                                    "geo1c_con\texit\theap\tRuntime\t9\t-\t9\t1\t1\t0\r\n"
                                                    "geo1c_con\texit\theap\tLua\t9\t-\t9\t1\t1\t0\r\n";
        f.kFiles["R\\tests\\baseline\\v143\\mem-peaks.txt"] = "# commit b\r\n"
                                                              "geo1c_con\texit\theap\tRuntime\t1\t-\t1\t1\t1\t0\r\n"
                                                              "geo1c_con\texit\theap\tLua\t1\t-\t1\t1\t1\t0";
        const RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 0);
        CHECK(k.sWriteText == "# commit b\r\ngeo1c_con\texit\theap\tRuntime\t9\t-\t9\t1\t1\t0\r\n"
                              "geo1c_con\texit\theap\tLua\t1\t-\t1\t1\t1\t0");
    }
    SUBCASE("lua-bins and offthread rows come from their own run folders")
    {
        f.kIn.sTable = "lua-bins";
        f.kIn.kKeyArgs = {"geo1c_con/turn1500/1"};
        f.kFiles["R\\run\\lua-bins\\lua-bins.txt"] = "geo1c_con\tturn1500\t1\t5\t5\t5\t5\t-\r\n";
        f.kFiles["R\\tests\\baseline\\v143\\lua-bins.txt"] = "geo1c_con\tturn1500\t1\t4\t4\t4\t4\t-\r\n";
        CHECK(Repin(f.kIn).sWriteText == "geo1c_con\tturn1500\t1\t5\t5\t5\t5\t-\r\n");
        f.kIn.sTable = "offthread";
        f.kIn.kKeyArgs = {"c1"};
        f.kFiles["R\\run\\offthread\\offthread.txt"] = "case\tcount\r\nc1\t3\t1\t0\r\nc1\t7\tRuntime\t32\t2\tf\r\n";
        f.kFiles["R\\tests\\baseline\\v143\\offthread.txt"] =
            "case\tcount\r\nc0\t0\t0\t0\r\nc1\t2\t0\t0\r\n## sound-on\r\nc1\tsound=opened\r\n";
        const RepinResult k = Repin(f.kIn);
        CHECK(k.iExit == 0);
        CHECK(k.sWriteText ==
              "case\tcount\r\nc0\t0\t0\t0\r\nc1\t3\t1\t0\r\nc1\t7\tRuntime\t32\t2\tf\r\n## sound-on\r\nc1\tsound=opened\r\n");
        CHECK(Lines(k.kLines) == "[PASS] repin offthread c1 (2 rows from exe " + kShaA + ")");
    }
}

TEST_CASE_NAMED(FCaptureCommitTest, "Unit::CaptureCommit::refusals and rows", "[EngineFilter]")
{
    SUBCASE("a capture with every rule met")
    {
        CHECK(CaptureCommitPreflight(GoodPreflight()).iExit == 0);
        CapturePreflight k = GoodPreflight();
        k.sOutFull = "C:\\R\\tests\\baselineX\\out";
        CHECK(CaptureCommitPreflight(k).iExit == 0);
    }
    SUBCASE("an out folder inside tests baseline")
    {
        CapturePreflight k = GoodPreflight();
        k.sOutFull = "c:\\r\\TESTS\\baseline\\v143-x64\\";
        CaptureVerdict v = CaptureCommitPreflight(k);
        CHECK(v.iExit == 2);
        CHECK(Lines(v.kLines) == "[FAIL] capture-commit - -OutDir c:\\r\\TESTS\\baseline\\v143-x64 is inside "
                                 "tests\\baseline; capture into a scratch folder");
        k.sOutFull = "C:\\R\\tests\\baseline";
        CHECK(CaptureCommitPreflight(k).iExit == 2);
    }
    SUBCASE("another game or tests exe or the gate running")
    {
        CapturePreflight k = GoodPreflight();
        k.kProcesses.push_back({20, "OpenSWBF2Gate.exe"});
        k.kProcesses.push_back({21, "BattlefrontII.exe"});
        k.kProcesses.push_back({22, "openswbf2tests.exe"});
        k.kProcesses.push_back({23, "BattlefrontII_old.exe"});
        const CaptureVerdict v = CaptureCommitPreflight(k);
        CHECK(v.iExit == 2);
        CHECK(Lines(v.kLines) == "[FAIL] capture-commit - a commit capture runs alone; refusing beside:|"
                                 "    game process BattlefrontII (21)|    game process BattlefrontII_old (23)|"
                                 "    harness process 20: OpenSWBF2Gate.exe|    harness process 22: openswbf2tests.exe");
    }
    SUBCASE("the Steam state not as asked")
    {
        CapturePreflight k = GoodPreflight();
        k.sSteamRunning = "no";
        CaptureVerdict v = CaptureCommitPreflight(k);
        CHECK(v.iExit == 2);
        CHECK(Lines(v.kLines) == "[FAIL] capture-commit - -SteamState Running needs Steam running and logged in; "
                                 "Get-SteamRunning is no");
        k.sSteamState = "Closed";
        CHECK(CaptureCommitPreflight(k).iExit == 0);
        CaptureRun r = GoodRun();
        r.sSeen = "yes";
        const CaptureResult c = CaptureCommitRows(r);
        CHECK(c.iExit == 1);
        CHECK(Lines(c.kLines) == "[FAIL] capture-commit - Steam was steam=yes at the run, -SteamState Closed needs steam=no");
        k.sSteamState = "steam";
        CHECK(CaptureCommitPreflight(k).iExit == 1);
    }
    SUBCASE("the hog switch on Win32 and its absence on x64")
    {
        CapturePreflight k = GoodPreflight();
        k.sPlatform = "Win32";
        CaptureVerdict v = CaptureCommitPreflight(k);
        CHECK(v.iExit == 2);
        CHECK(Lines(v.kLines) == "[FAIL] capture-commit - Game::Standard::commit-flat runs under -hogaddrspace on x64 "
                                 "only; pass -HogAddrSpace exactly with -Platform x64");
        k.bHog = false;
        CHECK(CaptureCommitPreflight(k).iExit == 0);
        k.sPlatform = "x64";
        CHECK(CaptureCommitPreflight(k).iExit == 2);
    }
    SUBCASE("the rows are the one value after cycle 8")
    {
        const CaptureResult c = CaptureCommitRows(GoodRun());
        CHECK(c.iExit == 0);
        CHECK(c.bWrite);
        CHECK(c.sCommitPath == "C:\\scratch\\commit\\commit.txt");
        CHECK(c.sCommitText ==
              "# commit abcd1234\r\nsteam\thog\tstate\tarena\trest\r\nno\t1\tmission\t100\t300\r\nno\t1\tshell\t40\t100\r\n");
        CHECK(c.sRunText == "run=R\\artifacts\\tests\\s1\r\nexit=0\r\n");
        CHECK(Lines(c.kLines) == "capture-commit: wrote C:\\scratch\\commit\\commit.txt from R\\artifacts\\tests\\s1 (test "
                                 "runner exit 0)|    no\t1\tmission\t100\t300|    no\t1\tshell\t40\t100");
    }
    SUBCASE("one value taken after cycle 8")
    {
        CaptureRun r = GoodRun();
        r.kCommitFlatLogs = {CommitLog(10, 101)};
        CaptureResult c = CaptureCommitRows(r);
        CHECK(c.iExit == 1);
        CHECK(!c.bWrite);
        CHECK(Lines(c.kLines) == "[FAIL] capture-commit - R\\artifacts\\tests\\s1|"
                                 "    mission arena after cycle 8 is not one value: 100, 101");
        r.kCommitFlatLogs = {CommitLog(8, 100)};
        c = CaptureCommitRows(r);
        CHECK(Lines(c.kLines) == "[FAIL] capture-commit - R\\artifacts\\tests\\s1|"
                                 "    mission has no COMMITPARTS rows after cycle 8|"
                                 "    shell has no COMMITPARTS rows after cycle 8");
    }
    SUBCASE("a run that did not start or left other than one log or failed")
    {
        CaptureRun r = GoodRun();
        r.bStarted = false;
        CHECK(Lines(CaptureCommitRows(r).kLines) == "[FAIL] capture-commit - the test runner did not start");
        r = GoodRun();
        r.kCommitFlatLogs.clear();
        CHECK(Lines(CaptureCommitRows(r).kLines) ==
              "[FAIL] capture-commit - 0 commit-flat run logs under R\\artifacts\\tests\\s1");
        r = GoodRun();
        r.iRunCode = 1;
        const CaptureResult c = CaptureCommitRows(r);
        CHECK(c.iExit == 1);
        CHECK(c.bWrite);
        CHECK(c.kLines.back() == "[FAIL] capture-commit - the test runner exited 1");
    }
}
