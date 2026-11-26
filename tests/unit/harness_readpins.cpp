#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <set>
#include <string>
#include <vector>

#include "testutil.h"

namespace
{

struct RawOpenAllow
{
    const char* pcFile;
    const char* pcFunction;
    const char* pcOpens;
};

const RawOpenAllow kRawOpenAllow[] = {
    {"game\\capture.cpp", "ReadBytes", "the built game exe under bin"},
    {"game\\capture.cpp", "FLaaTest1", "writes the exe copy without the LAA flag into the run folder"},
    {"game\\riderlink_client.cpp", "ReadWholeFile", "the game's port file under LOCALAPPDATA"},
    {"game\\riderlink_client.cpp", "RiderLinkClient::~RiderLinkClient", "port files under LOCALAPPDATA"},
    {"game\\riderlink_client.cpp", "RiderLinkClient::ReadPortFile", "port files under LOCALAPPDATA"},
    {"game\\riderlink_client.cpp", "RiderLinkClient::PortFileExists", "port files under LOCALAPPDATA"},
    {"game\\savecross.cpp", "ReadAll", "save files and sidecars in the worker folders of a run"},
    {"game\\standard_case.cpp", "ReadRunLog", "a game run's run.log in the run folder"},
    {"game\\standard_case.cpp", "ReadRunFile", "a game run's files in the run folder"},
    {"gate\\gate_keys.cpp", "FileSha", "the gate's hash of build outputs and evidence files"},
    {"gate\\gate_keys.cpp", "ReadFileBytes", "the gate's step table, tlogs, records and evidence; in the tests exe temp fixtures and build outputs"},
    {"gate\\gate_keys.cpp", "PutFile", "writes gate records, ledgers and evidence files"},
    {"gate\\gate_machine.cpp", "NulHandle", "the NUL device as a child's stdin"},
    {"gate\\gate_machine.cpp", "HarnessDataLink", "a data junction under bin or artifacts"},
    {"gate\\gate_machine.cpp", "RunProcess", "writes a child's output file"},
    {"harness\\gamerun.cpp", "ReadFileText", "files of a game run in the run and worker folders"},
    {"harness\\gamerun.cpp", "ListFiles", "worker folders and WER dump folders"},
    {"harness\\gamerun.cpp", "RemoveTree", "a worker folder under artifacts"},
    {"harness\\gamerun.cpp", "GameRun::RunInWorker", "writes worker files and command.txt"},
    {"harness\\report.cpp", "WriteReport", "writes index.json into the run folder"},
    {"harness\\runner.cpp", "ReadOutput", "a child's output.txt in the run folder"},
    {"harness\\runner.cpp", "ReadAsanReport", "a child's asan reports in the run folder"},
    {"harness\\runner.cpp", "ReadTextFile", "the exclude list named by --exclude, which the gate counts as read"},
    {"harness\\runner.cpp", "RunChild", "creates a child's output.txt in the run folder"},
    {"net\\corpus.cpp", "SaveCorpus", "writes a new corpus file"},
    {"net\\fuzz.cpp", "FFuzzTest2", "writes the --fuzz-out file"},
    {"tables\\tables_cases.cpp", "ReadDataFile", "files of the data folder, a run folder"},
    {"tables\\tables_cases.cpp", "ListDataFolder", "folders of the data folder, a run folder"},
    {"tables\\tables_cases.cpp", "WriteOutFile", "writes incoming tables into the run's out folder"},
    {"tables\\tables_cases.cpp", "ReadBuildFile", "the linker map under obj"},
    {"tables\\tables_main.cpp", "ReadPathText", "run folder files given to --repin and --capture-commit"},
    {"tables\\tables_main.cpp", "WritePathText", "writes the re-pinned table and the commit capture files"},
    {"tables\\tables_main.cpp", "FindCommitLogs", "the commit-flat run folder of a capture"},
    {"testutil.cpp", "OpenRepoFile", "the choke point itself"},
    {"testutil.cpp", "ListRepoFolder", "the choke point itself"},
    {"testutil.cpp", "ChildLogText", "a child's log in the temp folder"},
    {"testutil.cpp", "RunWin32Child", "creates a child's log in the temp folder"},
    {"unit\\crt_trig.cpp", "CrtTrigWriteFile", "writes a child's output file in the temp folder"},
    {"unit\\crt_trig.cpp", "CrtTrigReadFile", "a child's output file in the temp folder"},
    {"unit\\crt_trig_ref_win32.cpp", "CrtSimWritePins", "writes generated pins into the generator's out folder"},
    {"unit\\d3dx_ref_win32.cpp", "FileSha256", "the d3dx9 DLL loaded beside the exe under bin"},
    {"unit\\d3dx_ref_win32.cpp", "D3dxRefWritePins", "writes generated pins into the generator's out folder"},
    {"unit\\d3dx_ref_win32.cpp", "D3dxRefWriteFile", "writes a child's output file in the temp folder"},
    {"unit\\d3dx_ref_x64.cpp", "D3dxReferenceOpen", "a Win32 child's output file in the temp folder"},
    {"unit\\display_settings.cpp", "WriteFile", "writes a scratch ini in the work folder"},
    {"unit\\display_settings.cpp", "ReadText", "a scratch ini in the work folder"},
    {"unit\\float_corpus.cpp", "WriteLines", "writes generated corpus files into the generator's out folder"},
    {"unit\\float_corpus.cpp", "ListLvlFiles", "GameData lvl folders"},
    {"unit\\float_corpus.cpp", "OpenUcfb", "GameData lvl files"},
    {"unit\\float_corpus.cpp", "ReadFileBytes", "generated corpus files in the temp folder"},
    {"unit\\gamedata.cpp", "FGamedataTest1", "GameData core.lvl"},
    {"unit\\harness_expect.cpp", "RunProbe", "a child's output file in the temp folder"},
    {"unit\\harness_gate.cpp", "SetFileAge", "a fixture file in the temp folder"},
    {"unit\\harness_gate.cpp", "FGateRunTest13", "a fixture gate folder in the temp folder"},
    {"unit\\harness_gate_machine.cpp", "FGateMachineTest7", "a fixture evidence folder in the temp folder"},
    {"unit\\harness_readpins.cpp", "ReadsFileText", "a reads list in the temp folder"},
    {"unit\\harness_readpins.cpp", "FRunnerReadsTest1", "writes a reads list in the temp folder"},
    {"unit\\harness_report.cpp", "FHarnessReportTest2", "a report written in the temp folder"},
    {"unit\\lua_bytecode_header.cpp", "FLuaBytecodeHeaderTest4", "GameData shell.lvl"},
    {"unit\\lua_bytecode_retail.cpp", "ListFilesRecursive", "GameData lvl folders"},
    {"unit\\lua_bytecode_retail.cpp", "FLuaBytecodeRetailTest2", "GameData lvl files"},
    {"unit\\lua_bytecode_retail.cpp", "FLuaBytecodeRetailTest4", "writes the --fuzz-out file"},
    {"unit\\lua_script_hardening.cpp", "WriteTempScript", "writes a script in the temp folder"},
    {"unit\\luac_compare.cpp", "CompareFiles", "LuaCompiler outputs in the temp folder"},
    {"unit\\luac_compare.cpp", "RemoveTree", "a temp folder of LuaCompiler outputs"},
    {"unit\\movie_corpus.cpp", "FMovieCorpusTest1", "GameData movie folder"},
    {"unit\\movie_fuzz.cpp", "DecoderBases", "GameData movie folder"},
    {"unit\\movie_fuzz.cpp", "DumpInput", "writes the --fuzz-out file"},
    {"unit\\movie_retail.cpp", "ScanMvs", "GameData movie files"},
    {"unit\\movie_retail.cpp", "ListMvsSegments", "GameData movie files"},
    {"unit\\movie_retail.cpp", "ReadMvsBytes", "GameData movie files"},
    {"unit\\movie_retail.cpp", "FileMovieSource::Open", "GameData movie files"},
    {"unit\\movie_retail.cpp", "FMovieRetailTest2", "GameData movie folder"},
    {"unit\\redwarning_log.cpp", "ReadWholeFile", "a log written in the temp folder"},
    {"unit\\redwarning_log.cpp", "FRedwarningLogTest3", "writes a log in the temp folder"},
    {"unit\\redwarning_log.cpp", "FRedwarningLogTest4", "writes scratch logs in the work folder"},
    {"unit\\save_round_trip.cpp", "TempSaveDir", "a temp save folder"},
    {"unit\\save_round_trip.cpp", "ReadFileBytes", "save files in a temp save folder"},
    {"unit\\save_round_trip.cpp", "WriteFileBytes", "writes save files in a temp save folder"},
};

bool IdentChar(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

const char* const kOpenCalls[] = {"f" "open", "_wf" "open", "Create" "FileA", "Create" "FileW", "FindFirst" "FileA",
                                  "FindFirst" "FileW"};

bool HasCall(const std::string& sLine, const std::string& sName, bool bParen)
{
    for (size_t u = sLine.find(sName); u != std::string::npos; u = sLine.find(sName, u + 1))
    {
        const size_t uEnd = u + sName.size();
        if (u > 0 && IdentChar(sLine[u - 1]))
        {
            continue;
        }
        if (bParen ? (uEnd < sLine.size() && sLine[uEnd] == '(') : (uEnd >= sLine.size() || !IdentChar(sLine[uEnd])))
        {
            return true;
        }
    }
    return false;
}

bool IsOpenLine(const std::string& sLine)
{
    for (size_t i = 0; i < sizeof(kOpenCalls) / sizeof(kOpenCalls[0]); ++i)
    {
        if (HasCall(sLine, kOpenCalls[i], true))
        {
            return true;
        }
    }
    return HasCall(sLine, "if" "stream", false);
}

std::string ScopeName(const std::string& sHeader)
{
    const char* const apcMacros[] = {"TEST_CASE_NAMED(", "COMPLEX_TEST_CASE_NAMED(", "TEST_CASE("};
    for (size_t i = 0; i < sizeof(apcMacros) / sizeof(apcMacros[0]); ++i)
    {
        const std::string sMacro = apcMacros[i];
        if (sHeader.compare(0, sMacro.size(), sMacro) == 0)
        {
            const size_t uComma = sHeader.find_first_of(",)", sMacro.size());
            return sHeader.substr(sMacro.size(), uComma == std::string::npos ? std::string::npos : uComma - sMacro.size());
        }
    }
    const char* const apcTypes[] = {"struct ", "class "};
    for (size_t i = 0; i < 2; ++i)
    {
        const std::string sType = apcTypes[i];
        if (sHeader.compare(0, sType.size(), sType) == 0)
        {
            size_t uEnd = sType.size();
            while (uEnd < sHeader.size() && IdentChar(sHeader[uEnd]))
            {
                ++uEnd;
            }
            return sHeader.substr(sType.size(), uEnd - sType.size());
        }
    }
    size_t uParen = sHeader.find('(');
    while (uParen > 0 && sHeader[uParen - 1] == ' ')
    {
        --uParen;
    }
    size_t uStart = uParen;
    while (uStart > 0 && (IdentChar(sHeader[uStart - 1]) || sHeader[uStart - 1] == ':' || sHeader[uStart - 1] == '~'))
    {
        --uStart;
    }
    return sHeader.substr(uStart, uParen - uStart);
}

bool IsScopeHeader(const std::string& sLine)
{
    if (sLine.empty() || !(isalpha((unsigned char)sLine[0]) || sLine[0] == '_'))
    {
        return false;
    }
    size_t uEnd = sLine.find_last_not_of(" \t");
    if (uEnd == std::string::npos || sLine[uEnd] == ';')
    {
        return false;
    }
    if (sLine.compare(0, 7, "struct ") == 0 || sLine.compare(0, 6, "class ") == 0)
    {
        return true;
    }
    return sLine.find('(') != std::string::npos && sLine.compare(0, 10, "namespace ") != 0;
}

std::vector<std::string> RawOpenSites(const std::string& sFile, const std::string& sText)
{
    std::vector<std::string> kSites;
    std::string sScope;
    size_t uLine = 0;
    size_t uStart = 0;
    while (uStart <= sText.size())
    {
        size_t uEnd = sText.find('\n', uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uStart, uEnd - uStart);
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        ++uLine;
        if (IsScopeHeader(sLine))
        {
            sScope = ScopeName(sLine);
        }
        if (IsOpenLine(sLine))
        {
            char acLine[32];
            _snprintf(acLine, sizeof(acLine), "%u", (unsigned int)uLine);
            acLine[sizeof(acLine) - 1] = 0;
            kSites.push_back(sFile + "|" + sScope + "|" + acLine);
        }
        uStart = uEnd + 1;
    }
    return kSites;
}

std::vector<std::string> CheckRawOpens(const std::vector<std::string>& kSites, const RawOpenAllow* pAllow,
                                       size_t uAllow)
{
    std::vector<std::string> kProblems;
    std::set<size_t> kUsed;
    for (size_t i = 0; i < kSites.size(); ++i)
    {
        const size_t uBar = kSites[i].find('|');
        const size_t uBar2 = kSites[i].find('|', uBar + 1);
        const std::string sFile = kSites[i].substr(0, uBar);
        const std::string sScope = kSites[i].substr(uBar + 1, uBar2 - uBar - 1);
        bool bListed = false;
        for (size_t j = 0; j < uAllow; ++j)
        {
            if (_stricmp(sFile.c_str(), pAllow[j].pcFile) == 0 && sScope == pAllow[j].pcFunction)
            {
                bListed = true;
                kUsed.insert(j);
            }
        }
        if (!bListed)
        {
            kProblems.push_back("tests\\" + sFile + ":" + kSites[i].substr(uBar2 + 1) + " opens a file in " + sScope +
                                " outside the read list; use ReadRepoText, ReadRepoBytes, OpenRepoFile or ListRepoFolder");
        }
    }
    for (size_t j = 0; j < uAllow; ++j)
    {
        if (!kUsed.count(j))
        {
            kProblems.push_back(std::string("unused entry tests\\") + pAllow[j].pcFile + " " + pAllow[j].pcFunction);
        }
    }
    return kProblems;
}

void WalkTests(const std::string& sRel, std::vector<std::string>& kFiles)
{
    std::vector<RepoEntry> kEntries;
    ListRepoFolder("tests" + (sRel.empty() ? std::string() : "\\" + sRel), "*", kEntries);
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        const std::string sChild = sRel.empty() ? kEntries[i].sName : sRel + "\\" + kEntries[i].sName;
        if (kEntries[i].bDir)
        {
            if (_stricmp(sChild.c_str(), "third_party") != 0)
            {
                WalkTests(sChild, kFiles);
            }
            continue;
        }
        const size_t uDot = sChild.find_last_of('.');
        const std::string sExt = uDot == std::string::npos ? std::string() : sChild.substr(uDot);
        if (_stricmp(sExt.c_str(), ".cpp") == 0 || _stricmp(sExt.c_str(), ".h") == 0)
        {
            kFiles.push_back(sChild);
        }
    }
}

std::string JoinLines(const std::vector<std::string>& kLines)
{
    std::string s;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        s += kLines[i] + "\n";
    }
    return s;
}

std::string ReadsFileText(const std::string& sPath)
{
    std::string sText;
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return sText;
    }
    char acBuf[1024];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    return sText;
}

std::string TempReadsPath()
{
    char acTemp[MAX_PATH];
    const DWORD uTemp = GetTempPathA(MAX_PATH, acTemp);
    char acName[64];
    _snprintf(acName, sizeof(acName), "owb2_reads_%lu.txt", GetCurrentProcessId());
    acName[sizeof(acName) - 1] = 0;
    return uTemp > 0 && uTemp < MAX_PATH ? std::string(acTemp) + acName : std::string(acName);
}

std::vector<std::string> ReadsLines(const std::string& sText)
{
    std::vector<std::string> kOut;
    size_t uStart = 0;
    while (uStart < sText.size())
    {
        size_t uEnd = sText.find('\n', uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uStart, uEnd - uStart);
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        if (!sLine.empty())
        {
            kOut.push_back(sLine);
        }
        uStart = uEnd + 1;
    }
    return kOut;
}

}

TEST_CASE_NAMED(FRunnerReadsTest1, "Unit::Runner::a read of a repo file is noted", "[EngineFilter]")
{
    const std::string sReads = TempReadsPath();
    DeleteFileA(sReads.c_str());
    const std::string sRoot = RepoRoot();
    REQUIRE(!sRoot.empty());
    SUBCASE("each choke-point function notes the repo path")
    {
        FILE* pList = fopen(sReads.c_str(), "wb");
        REQUIRE(pList != (FILE*)0);
        fclose(pList);
        REQUIRE(SetEnvironmentVariableA("OPENSWBF2_READS", sReads.c_str()));
        std::string sText;
        CHECK(ReadRepoText("tests\\smoke\\missions-all.txt", sText));
        CHECK(ReadRepoBytes("tests\\excluded.tsv", sText));
        FILE* pFile = OpenRepoFile("tests\\gate\\steps.ini");
        CHECK(pFile != (FILE*)0);
        if (pFile)
        {
            fclose(pFile);
        }
        std::vector<RepoEntry> kEntries;
        CHECK(ListRepoFolder("tests\\baseline", "*", kEntries));
        SetEnvironmentVariableA("OPENSWBF2_READS", 0);
        const std::vector<std::string> kLines = ReadsLines(ReadsFileText(sReads));
        REQUIRE(kLines.size() == 4);
        CHECK(_stricmp(kLines[0].c_str(), (sRoot + "\\tests\\smoke\\missions-all.txt").c_str()) == 0);
        CHECK(_stricmp(kLines[1].c_str(), (sRoot + "\\tests\\excluded.tsv").c_str()) == 0);
        CHECK(_stricmp(kLines[2].c_str(), (sRoot + "\\tests\\gate\\steps.ini").c_str()) == 0);
        CHECK(_stricmp(kLines[3].c_str(), (sRoot + "\\tests\\baseline").c_str()) == 0);
    }
    SUBCASE("nothing is written when the variable is unset")
    {
        FILE* pList = fopen(sReads.c_str(), "wb");
        REQUIRE(pList != (FILE*)0);
        fclose(pList);
        SetEnvironmentVariableA("OPENSWBF2_READS", 0);
        std::string sText;
        CHECK(ReadRepoText("tests\\smoke\\missions-all.txt", sText));
        CHECK(ReadsFileText(sReads).empty());
    }
    DeleteFileA(sReads.c_str());
}

TEST_CASE_NAMED(FRunnerReadsTest2, "Unit::Runner::no test opens a tracked file outside the read list", "[EngineFilter]")
{
    SUBCASE("a raw open outside the list fails")
    {
        const std::string sFixture = "#include <stdio.h>\n"
                                     "\n"
                                     "bool ReadPins(const char* pcPath)\n"
                                     "{\n"
                                     "    FILE* pFile = f" "open(\"corpus\\\\pins.txt\", \"rb\");\n"
                                     "    return pFile != 0;\n"
                                     "}\n"
                                     "\n"
                                     "TEST_CASE_NAMED(FFixtureTest1, \"Unit::Fixture::one\", \"[EngineFilter]\")\n"
                                     "{\n"
                                     "    std::if" "stream kIn(\"x\");\n"
                                     "    HANDLE h = Create" "FileW(L\"x\", 0, 0, 0, 0, 0, 0);\n"
                                     "    char* p = my_f" "open(\"x\");\n"
                                     "}\n";
        const std::vector<std::string> kSites = RawOpenSites("unit\\fixture.cpp", sFixture);
        REQUIRE(kSites.size() == 3);
        CHECK(kSites[0] == "unit\\fixture.cpp|ReadPins|5");
        CHECK(kSites[1] == "unit\\fixture.cpp|FFixtureTest1|11");
        CHECK(kSites[2] == "unit\\fixture.cpp|FFixtureTest1|12");
        const RawOpenAllow kAllow[] = {{"unit\\fixture.cpp", "FFixtureTest1", "a temp file"},
                                       {"unit\\fixture.cpp", "Gone", "a temp file"}};
        const std::vector<std::string> kProblems = CheckRawOpens(kSites, kAllow, 2);
        REQUIRE(kProblems.size() == 2);
        CHECK(kProblems[0] ==
              "tests\\unit\\fixture.cpp:5 opens a file in ReadPins outside the read list; use ReadRepoText, "
              "ReadRepoBytes, OpenRepoFile or ListRepoFolder");
        CHECK(kProblems[1] == "unused entry tests\\unit\\fixture.cpp Gone");
    }
    SUBCASE("a member function and a struct method are named")
    {
        const std::string sFixture = "class TempDir\n"
                                     "{\n"
                                     "    void Clear()\n"
                                     "    {\n"
                                     "        FindFirst" "FileA(\"x\", 0);\n"
                                     "    }\n"
                                     "};\n"
                                     "\n"
                                     "bool Client::~Client()\n"
                                     "{\n"
                                     "    _wf" "open(L\"x\", L\"rb\");\n"
                                     "}\n";
        const std::vector<std::string> kSites = RawOpenSites("game\\fixture.cpp", sFixture);
        REQUIRE(kSites.size() == 2);
        CHECK(kSites[0] == "game\\fixture.cpp|TempDir|5");
        CHECK(kSites[1] == "game\\fixture.cpp|Client::~Client|11");
    }
    SUBCASE("every tests source opens tracked files only through the choke point")
    {
        std::vector<std::string> kFiles;
        WalkTests(std::string(), kFiles);
        REQUIRE(kFiles.size() > 100);
        std::vector<std::string> kSites;
        for (size_t i = 0; i < kFiles.size(); ++i)
        {
            std::string sText;
            REQUIRE(ReadRepoText(("tests\\" + kFiles[i]).c_str(), sText));
            const std::vector<std::string> kOne = RawOpenSites(kFiles[i], sText);
            kSites.insert(kSites.end(), kOne.begin(), kOne.end());
        }
        const std::vector<std::string> kProblems =
            CheckRawOpens(kSites, kRawOpenAllow, sizeof(kRawOpenAllow) / sizeof(kRawOpenAllow[0]));
        const std::string sProblems = JoinLines(kProblems);
        CHECK_MESSAGE(kProblems.empty(), sProblems);
    }
}
