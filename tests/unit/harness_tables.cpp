#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "gate/gate_keys.h"
#include "tables/tables_build.h"
#include "tables/tables_compare.h"
#include "tables/tables_readers.h"
#include "tables/tables_verify.h"
#include "testutil.h"

using namespace Tables;

namespace
{

const char* const kClothBase = "# commit abc\r\n"
                               "mission\tfree\tenabled\tlow\thigh\r\n"
                               "geo1c_con\t10\t1\t2\t3\r\n"
                               "nab2g_c\t-\t-\t-\t-\r\n";

std::string Lines(const std::vector<std::string>& k)
{
    return JoinText(k, "|");
}

const BuiltTable* Table(const BuildResult& k, const std::string& sName)
{
    for (size_t i = 0; i < k.kTables.size(); ++i)
    {
        if (k.kTables[i].sName == sName)
        {
            return &k.kTables[i];
        }
    }
    return 0;
}

CaseLog Case(const std::string& sName, const std::string& sLog)
{
    CaseLog k;
    k.sName = sName;
    k.kRunLog = Found(sLog);
    k.kFinalScore = Absent();
    return k;
}

CaseLog NoLog(const std::string& sName)
{
    CaseLog k;
    k.sName = sName;
    k.kRunLog = Absent();
    k.kFinalScore = Absent();
    return k;
}

std::string L(const std::string& s)
{
    return "[00:00][  1]LogTest: Inform: " + s + "\r\n";
}

std::string Upper(std::string s)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] >= 'a' && s[i] <= 'f')
        {
            s[i] = (char)(s[i] - 'a' + 'A');
        }
    }
    return s;
}

RepoTextFn MapFn(const std::map<std::string, std::string>& k)
{
    return [k](const std::string& sRel) {
        std::map<std::string, std::string>::const_iterator it = k.find(sRel);
        return it == k.end() ? Absent() : Found(it->second);
    };
}

std::map<std::string, std::vector<std::string> > ReaderFiles()
{
    std::map<std::string, std::vector<std::string> > k;
    const std::vector<Reader>& kAll = BaselineReaders();
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        std::vector<std::string>& kList = k[ReaderFolder(kAll[i])];
        if (std::find(kList.begin(), kList.end(), kAll[i].pcFile) == kList.end())
        {
            kList.push_back(kAll[i].pcFile);
        }
    }
    return k;
}

std::map<std::string, std::string> ReaderSources()
{
    std::map<std::string, std::string> k;
    k["tests\\unit\\memory_baseline.cpp"] = "a(\"tests\\\\baseline\\\\v143\\\\CAUSES.md\"); b(\"x\\\\movie-goldens.txt\");";
    k["tests\\unit\\movie_retail.cpp"] = "c(\"tests\\\\baseline\\\\v143\\\\movie-frames.txt\");";
    k["tests\\game\\crash.cpp"] =
        "d(\"tests\\\\baseline\\\\v143-x64\\\\mem-peaks.txt\" : \"tests\\\\baseline\\\\v143\\\\mem-peaks.txt\");";
    k["tests\\game\\standard.cpp"] = "e(\"tests\\\\baseline\\\\v143-x64\\\\commit.txt\");";
    k["tests\\game\\standard_case.cpp"] = "f(\"tests\\\\baseline\\\\v143\\\\smoke-all-digests.txt\"); "
                                          "g(\"tests\\\\baseline\\\\v143-x64\\\\offthread.txt\" : "
                                          "\"tests\\\\baseline\\\\v143\\\\offthread.txt\");";
    k["tests\\x87\\x87_cases.cpp"] = "h(\"tests\\\\baseline\\\\v143\\\\x87-inventory.tsv\");";
    return k;
}

std::string Trace1600(const char* pcState)
{
    std::string s;
    for (int i = 1; i <= 1600; ++i)
    {
        s += L("Digest trace turn=" + std::to_string(i) + " state=" + pcState + " rand=1/2");
    }
    return s;
}

std::string MemPeaksX86()
{
    return "# commit a\r\n"
           "mission\tsource\tkind\tname\tcap_or_count\tsize\tpeak\tused\tallocs_or_grown\tfailed\r\n"
           "geo\texit\theap\tRuntime\t1000\t-\t500\t1\t1\t0\r\n"
           "geo\texit\tpool\tLua1\t4\t16\t3\t-\t0\t-\r\n"
           "nab\texit\theap\tRuntime\t1000\t-\t400\t1\t1\t0\r\n";
}

std::string LuaBinsX64()
{
    return "# commit a\r\n"
           "mission\tsource\tbin\tcount\tsize\tpeak\tgrown\tspill\r\n"
           "geo\texit\t1\t4\t16\t3\t0\t-\r\n"
           "nab\texit\t2\t4\t16\t3\t0\t-\r\n";
}

std::string MemPeaksX64Good()
{
    return "# commit b\r\n"
           "geo\texit\theap\tRuntime\t2000\t-\t900\t1\t1\t0\r\n"
           "geo\texit\tpool\tLua1\t4\t16\t3\t-\t0\t-\r\n"
           "nab\texit\theap\tRuntime\t2000\t-\t800\t1\t1\t0\r\n"
           "nab\texit\tpool\tLua2\t4\t16\t3\t-\t0\t-\r\n";
}

TableVerdict MemPeaks(const std::string& sNew, const std::string& sBins = LuaBinsX64(),
                      const std::string& sPair = MemPeaksX86())
{
    return CompareMemPeaksKeys("tables:mem-peaks-x64", "mem-peaks.txt", "mem-peaks.txt", "lua-bins.txt", "X86",
                               Found(sPair), "T", Found(sNew), "X64", Found(sBins));
}

std::string Heap(const char* pcMission, const char* pcHeap, const char* pcCap, const char* pcPeak)
{
    return std::string(pcMission) + "\texit\theap\t" + pcHeap + "\t" + pcCap + "\t-\t" + pcPeak + "\t1\t1\t0\r\n";
}

const char* const kOffMap = " BattlefrontII\r\n"
                            "\r\n"
                            " Timestamp is 0123abcd (Tue Nov 25 00:00:00 2026)\r\n"
                            "\r\n"
                            " Preferred load address is 00400000\r\n"
                            "\r\n"
                            " Start         Length     Name                   Class\r\n"
                            " 0001:00000000 00001000H .text$mn                CODE\r\n"
                            " 0002:00000000 00000100H .rdata                  DATA\r\n"
                            "\r\n"
                            "  Address         Publics by Value              Rva+Base       Lib:Object\r\n"
                            "\r\n"
                            " 0000:00000000       ___guard_flags             00000000     <absolute>\r\n"
                            " 0001:00000000       _main                      00401000 f   main.obj\r\n"
                            " 0001:00000010       a-b                        00401010 f   a.obj\r\n"
                            " 0001:00000010       ab                         00401010 f   a.obj\r\n"
                            " 0001:00000100       _zed                       00401100 f   b.obj\r\n";

const char* const kInitMap = " Start         Length     Name                   Class\r\n"
                             " 0002:00000000 00000004H .CRT$XCA                DATA\r\n"
                             " 0002:00000004 00000010H .CRT$XCU                DATA\r\n"
                             " 0002:00000014 00000004H .CRT$XCZ                DATA\r\n"
                             "\r\n"
                             "  Address         Publics by Value              Rva+Base       Lib:Object\r\n"
                             "\r\n"
                             " 0002:00000000       ___xc_a                    00402000     MSVCRT:initializers.obj\r\n"
                             " 0002:00000008       ?x$initializer$@@3P6AXXZA  00402008     a.obj\r\n"
                             " 0002:00000004       ?y$initializer$@@3P6AXXZA  00402004     b.obj\r\n"
                             " 0002:00000100       ?z$initializer$@@3P6AXXZA  00402100     c.obj\r\n"
                             " 0003:00000010       ??__Ebar@@YAXXZ            00403010 f i b.obj\r\n"
                             " 0001:00000020       ??__Efoo@@YAXXZ            00401020 f   a.obj\r\n";

}

TEST_CASE_NAMED(FTablesOrderTest, "Unit::Tables::order follows PowerShell Sort-Object", "[EngineFilter]")
{
    std::vector<std::string> k = {"b", "_a", "A", "a", "?x", "@y", "a-b", "ab", "a_b", "Z", "??_C", "?_C"};
    CultureSort(k, false);
    CHECK(JoinText(k, " ") == "??_C ?_C ?x @y _a a A a_b ab a-b b Z");
    std::vector<std::string> kNoCase = {"b", "_a", "a", "?x", "@y", "a-b", "ab", "a_b", "Z", "??_C", "?_C"};
    CultureSort(kNoCase, true);
    CHECK(JoinText(kNoCase, " ") == "??_C ?_C ?x @y _a a a_b ab a-b b Z");
    CHECK(Lines(ReadAllLines("a\r\nb\nc\rd")) == "a|b|c|d");
    CHECK(Lines(ReadAllLines("\xEF\xBB\xBFx\r\n\r\n")) == "x|");
    CHECK(ReadAllLines("").empty());
}

TEST_CASE_NAMED(FTablesKeyedTest, "Unit::Tables::keyed compare", "[EngineFilter]")
{
    SUBCASE("the reader table has the 37 entries of the script in its order")
    {
        const std::vector<Reader>& k = BaselineReaders();
        REQUIRE(k.size() == 37);
        CHECK(std::string(k[0].pcLabel) == "tables:cloth");
        CHECK(std::string(k[3].pcLabel) == "tables:mem-peaks-x64");
        CHECK(std::string(k[3].pcPair) == "mem-peaks.txt");
        CHECK(std::string(k[3].pcBins) == "lua-bins.txt");
        CHECK(std::string(k[13].pcLabel) == "tables:offthread-x64");
        CHECK(std::string(k[29].pcLabel) == "debug:digests-debug");
        CHECK(std::string(k[30].pcCpp) == "tests\\x87\\x87_cases.cpp");
        CHECK(std::string(k[36].pcFile) == "MANIFEST.md");
        CHECK(std::string(k[36].pcVerify) == "v143-x64");
    }
    SUBCASE("each reader keeps its key columns and skip pattern")
    {
        std::map<std::string, int> kWant = {{"tables:cloth", 1},     {"tables:cloth-x64", 1},
                                            {"tables:mem-peaks", 4}, {"tables:pool-sizes", 1},
                                            {"tables:pool-sizes-x64", 1}, {"tables:digests", 1},
                                            {"tables:lua-bins", 3},  {"tables:lua-bins-x64", 3},
                                            {"tables:lua-hist", 2},  {"tables:lua-hist-x64", 2},
                                            {"tables:offthread", 1}, {"tables:offthread-x64", 1}};
        const std::vector<Reader>& k = BaselineReaders();
        int iKeyed = 0;
        for (size_t i = 0; i < k.size(); ++i)
        {
            if (k[i].iKeyCols > 0)
            {
                ++iKeyed;
                CHECK(kWant[k[i].pcLabel] == k[i].iKeyCols);
            }
        }
        CHECK(iKeyed == 12);
        int iCols = 0;
        CHECK(KeyColumnsFor("mem-peaks.txt", iCols));
        CHECK(iCols == 4);
        CHECK(KeyColumnsFor("lua-bins.txt", iCols));
        CHECK(iCols == 3);
        CHECK(!KeyColumnsFor("digests.txt", iCols));
        CHECK(std::string(k[7].pcSkipRx) == "^(crash-.+|fatal-init|pool-full|pool-full-strict|dedicated)$");
        CHECK(k[12].bOffThread);
        CHECK(k[13].bOffThread);
    }
    SUBCASE("equal tables pass")
    {
        const TableVerdict v = CompareTable("tables:cloth", "cloth-headroom.txt", 1, false, "", "B", Found(kClothBase), "T",
                                            Found(kClothBase));
        CHECK(v.bOk);
        CHECK(v.sLine == "[PASS] tables:cloth keys=2");
    }
    SUBCASE("a changed value fails")
    {
        std::string sNew = kClothBase;
        sNew.replace(sNew.find("10\t1"), 2, "11");
        const TableVerdict v =
            CompareTable("tables:cloth", "cloth-headroom.txt", 1, false, "", "B", Found(kClothBase), "T", Found(sNew));
        CHECK(!v.bOk);
        CHECK(v.sLine == "[FAIL] tables:cloth keys=2");
        CHECK(Lines(v.kProblems) == "tables 'geo1c_con\t11\t1\t2\t3' baseline 'geo1c_con\t10\t1\t2\t3'");
        CHECK(VerdictText(v) == "[FAIL] tables:cloth keys=2\n    tables 'geo1c_con\t11\t1\t2\t3' baseline "
                                "'geo1c_con\t10\t1\t2\t3'\n");
    }
    SUBCASE("a missing row and an extra row fail")
    {
        const std::string sNew = "geo1c_con\t10\t1\t2\t3\r\nkas2c_uber\t1\t1\t1\t1\r\n";
        const TableVerdict v =
            CompareTable("tables:cloth", "cloth-headroom.txt", 1, false, "", "B", Found(kClothBase), "T", Found(sNew));
        CHECK(Lines(v.kProblems) == "nab2g_c missing from tables|kas2c_uber not in baseline");
    }
    SUBCASE("a second row for a key is compared line by line")
    {
        const std::string sNew = std::string(kClothBase) + "nab2g_c\t1\t1\t1\t1\r\n";
        const TableVerdict v =
            CompareTable("tables:cloth", "cloth-headroom.txt", 1, false, "", "B", Found(kClothBase), "T", Found(sNew));
        CHECK(Lines(v.kProblems) == "tables 'nab2g_c\t1\t1\t1\t1' baseline '<none>'");
    }
    SUBCASE("a row skipped by the pattern is not compared")
    {
        const char* pcRx = "^(crash-.+|fatal-init|pool-full|pool-full-strict|dedicated)$";
        const std::string sBase = "geo\tSim digest a\tnone\r\ncrash-x\tSim digest b\tnone\r\npool-full\tx\tnone\r\n";
        const std::string sNew = "geo\tSim digest a\tnone\r\ncrash-x\tSim digest c\tnone\r\npool-full-strictx\ty\tnone\r\n";
        const TableVerdict v =
            CompareTable("tables:digests", "smoke-all-digests.txt", 1, false, pcRx, "B", Found(sBase), "T", Found(sNew));
        CHECK(v.sLine == "[FAIL] tables:digests keys=1");
        CHECK(Lines(v.kProblems) == "pool-full-strictx not in baseline");
        CHECK(CompareTable("tables:digests", "smoke-all-digests.txt", 1, false, pcRx, "B", Found(sBase), "T",
                           Found("geo\tSim digest a\tnone\r\ncrash-y\tz\r\n"))
                  .bOk);
        const TableVerdict kAll = CompareTable("tables:digests", "smoke-all-digests.txt", 1, false, pcRx, "B",
                                               Found("crash-x\ta\r\n"), "T", Found("crash-x\tb\r\n"));
        CHECK(Lines(kAll.kProblems) == "no baseline rows to compare");
    }
    SUBCASE("offthread rows drop the tid column and stop at sound-on")
    {
        const std::string sBase = "# commit a\r\ncase\tcount\tsources\tdropped\r\n"
                                  "c1\t2\t1\t0\r\nc1\t7\tRuntime\t32\t2\tf0\r\n## sound-on\r\nc1\t9\t9\t9\r\n";
        const std::string sNew = "# commit b\r\nc1\t2\t1\t0\r\nc1\t8\tRuntime\t32\t2\tf0\r\n";
        CHECK(CompareTable("tables:offthread", "offthread.txt", 1, true, "", "B", Found(sBase), "T", Found(sNew)).bOk);
        const std::string sBad = "c1\t2\t1\t0\r\nc1\t8\tRuntime\t32\t3\tf0\r\n";
        const TableVerdict v =
            CompareTable("tables:offthread", "offthread.txt", 1, true, "", "B", Found(sBase), "T", Found(sBad));
        CHECK(Lines(v.kProblems) == "tables 'c1\tRuntime\t32\t3\tf0' baseline 'c1\tRuntime\t32\t2\tf0'");
    }
    SUBCASE("a missing table or one with no rows stops the compare")
    {
        TableVerdict v = CompareTable("tables:cloth", "cloth-headroom.txt", 1, false, "", "B", Absent(), "T",
                                      Found(kClothBase));
        CHECK(v.bThrown);
        CHECK(v.sLine == "cloth-headroom.txt not found in B");
        v = CompareTable("tables:cloth", "cloth-headroom.txt", 1, false, "", "B", Found(kClothBase), "T",
                         Found("# only\r\nmission\tx\r\n\r\n"));
        CHECK(v.bThrown);
        CHECK(v.sLine == "cloth-headroom.txt in T has no rows");
    }
    SUBCASE("the x64 folder is used when the file is there and v143 otherwise")
    {
        CompareInput kIn;
        kIn.sPlatform = "x64";
        kIn.sBaseX86 = "R\\v143";
        kIn.sBaseX64 = "R\\v143-x64";
        kIn.sTables = "T";
        kIn.fnRead = MapFn({{"R\\v143-x64\\mem-peaks.txt", "x"}});
        CHECK(BaselineDirFor("mem-peaks.txt", kIn) == "R\\v143-x64");
        CHECK(BaselineDirFor("smoke-all-digests.txt", kIn) == "R\\v143");
        kIn.sPlatform = "Win32";
        CHECK(BaselineDirFor("mem-peaks.txt", kIn) == "R\\v143");
    }
    SUBCASE("a mode reads only its entries and a folder entry replaces its v143 entry on x64")
    {
        std::vector<std::string> kLabels;
        const std::vector<Reader> kX64 = SelectReaders(kReadTables, "x64");
        for (size_t i = 0; i < kX64.size(); ++i)
        {
            kLabels.push_back(kX64[i].pcLabel);
        }
        CHECK(Lines(kLabels) == "tables:cloth-x64|tables:mem-peaks|tables:pool-sizes-x64|tables:digests");
        kLabels.clear();
        const std::vector<Reader> kWin32 = SelectReaders(kReadTables, "Win32");
        for (size_t i = 0; i < kWin32.size(); ++i)
        {
            kLabels.push_back(kWin32[i].pcLabel);
        }
        CHECK(Lines(kLabels) == "tables:cloth|tables:mem-peaks|tables:pool-sizes|tables:digests");
        CompareInput kIn;
        kIn.sPlatform = "Win32";
        kIn.fnRead = MapFn({});
        const std::vector<TableVerdict> v = CompareMode(kReadMemPeaksX64, kIn);
        REQUIRE(v.size() == 1);
        CHECK(v[0].sLine == "[FAIL] tables:MemPeaksX64 - no reader map entry for -MemPeaksX64 on Win32");
        CHECK(ModeFailed(v));
    }
    SUBCASE("a mode compares each entry and stops at a missing baseline")
    {
        CompareInput kIn;
        kIn.sPlatform = "Win32";
        kIn.sBaseX86 = "B";
        kIn.sBaseX64 = "X";
        kIn.sTables = "T";
        kIn.fnRead = MapFn({{"B\\lua-hist.txt", "geo\t1\t2\t3\r\n"}, {"T\\lua-hist.txt", "geo\t1\t2\t3\r\n"}});
        std::vector<TableVerdict> v = CompareMode(kReadLuaHist, kIn);
        REQUIRE(v.size() == 1);
        CHECK(v[0].sLine == "[PASS] tables:lua-hist keys=1");
        CHECK(!ModeFailed(v));
        v = CompareMode(kReadCollect, kIn);
        REQUIRE(v.size() == 1);
        CHECK(v[0].bThrown);
        CHECK(v[0].sLine == "digests.txt not found in B");
    }
    SUBCASE("byte compares")
    {
        TableVerdict v = CompareTableBytes("collect:digests", "digests.txt", "B", Found("a\r\nb\r\n"), "T",
                                           Found("a\r\nb\r\n"));
        CHECK(v.sLine == "[PASS] collect:digests lines=2");
        v = CompareTableBytes("collect:digests", "digests.txt", "B", Found("a\r\nb\r\n"), "T", Found("a\r\nc\r\n"));
        CHECK(v.sLine == "[FAIL] collect:digests lines=2");
        CHECK(Lines(v.kProblems) == "line 2: tables 'c' baseline 'b'");
        v = CompareTableBytes("collect:digests", "digests.txt", "B", Found("a\r\nb\r\n"), "T", Found("a\nb\n"));
        CHECK(Lines(v.kProblems) ==
              "bytes differ with equal lines (line endings or final newline): tables 4 bytes, baseline 6 bytes");
        v = CompareTableBytes("collect:digests", "digests.txt", "B", Found("a\r\n"), "T", Found("a\r\nb\r\n"));
        CHECK(Lines(v.kProblems) == "line 2: tables 'b' baseline '<none>'");
        v = CompareTableBytes("collect:digests", "digests.txt", "B", Found("a\r\n"), "T", Absent());
        CHECK(v.sLine == "[FAIL] collect:digests");
        CHECK(Lines(v.kProblems) == "digests.txt not found in T");
        v = CompareTableBytes("collect:digests", "digests.txt", "B", Found(""), "T", Found(""));
        CHECK(Lines(v.kProblems) == "digests.txt in B is empty");
        v = CompareTableBytes("collect:digests", "digests.txt", "B", Absent(), "T", Found(""));
        CHECK(v.bThrown);
        CHECK(v.sLine == "digests.txt not found in B");
    }
}

TEST_CASE_NAMED(FTablesMemPeaksTest, "Unit::Tables::mem-peaks x64 key shape", "[EngineFilter]")
{
    SUBCASE("a good table passes")
    {
        const TableVerdict v = MemPeaks(MemPeaksX64Good());
        CHECK(v.bOk);
        CHECK(v.sLine == "[PASS] tables:mem-peaks-x64 keys=2 missions=2");
    }
    SUBCASE("x64 lua-bins with no bins")
    {
        const TableVerdict v = MemPeaks(MemPeaksX64Good(), "geo\texit\tover\t1\r\n");
        CHECK(v.sLine == "[FAIL] tables:mem-peaks-x64 keys=2");
        CHECK(Lines(v.kProblems).find("x64 lua-bins.txt has no bins") == 0);
    }
    SUBCASE("a Lua pool that is not a bin")
    {
        std::string s = MemPeaksX64Good() + "geo\texit\tpool\tLua9\t1\t1\t1\t-\t0\t-\r\n";
        CHECK(Lines(MemPeaks(s).kProblems) == "geo\texit\tpool\tLua9 not a bin of x64 lua-bins.txt");
    }
    SUBCASE("a Lua pool with two rows")
    {
        std::string s = MemPeaksX64Good() + "geo\texit\tpool\tLua1\t1\t1\t1\t-\t0\t-\r\n";
        CHECK(Lines(MemPeaks(s).kProblems) == "geo\texit\tpool\tLua1 has 2 rows in x64 mem-peaks.txt");
    }
    SUBCASE("a bin with no Lua pool row")
    {
        std::string s = MemPeaksX64Good();
        s.erase(s.find("nab\texit\tpool\tLua2"));
        CHECK(Lines(MemPeaks(s).kProblems) == "nab\texit\tpool\tLua2 missing from x64 mem-peaks.txt");
    }
    SUBCASE("a row without ten columns and a heap row without a cap and peak")
    {
        std::string s = MemPeaksX64Good();
        s.replace(s.find("nab\texit\theap\tRuntime\t2000"), std::string("nab\texit\theap\tRuntime\t2000").size(),
                  "nab\texit\theap\tRuntime\t-");
        s += "nab\texit\tpool\tFoo\t1\r\n";
        CHECK(Lines(MemPeaks(s).kProblems) ==
              "x64 mem-peaks.txt heap row without a cap and peak: nab\texit\theap\tRuntime\t-\t-\t800\t1\t1\t0|"
              "x64 mem-peaks.txt row has 5 columns: nab\texit\tpool\tFoo\t1|"
              "nab\texit\tpool\tFoo not in x86 mem-peaks.txt");
    }
    SUBCASE("missions that differ and a key missing from x64")
    {
        std::string s = MemPeaksX64Good();
        s.erase(s.find("nab\texit\theap"), std::string("nab\texit\theap\tRuntime\t2000\t-\t800\t1\t1\t0\r\n").size());
        CHECK(Lines(MemPeaks(s).kProblems) ==
              "x64 mem-peaks.txt covers 1 missions and x86 mem-peaks.txt 2|"
              "nab\texit\theap\tRuntime missing from x64 mem-peaks.txt");
    }
    SUBCASE("a key with another row count and a key not in x86")
    {
        std::string s = MemPeaksX64Good() + Heap("geo", "Runtime", "2000", "901") + Heap("geo", "Extra", "1", "1");
        CHECK(Lines(MemPeaks(s).kProblems) ==
              "geo\texit\theap\tRuntime has 2 rows in x64 mem-peaks.txt and 1 in x86 mem-peaks.txt|"
              "geo\texit\theap\tExtra not in x86 mem-peaks.txt");
    }
    SUBCASE("a missing table stops the compare")
    {
        const TableVerdict v = CompareMemPeaksKeys("l", "mem-peaks.txt", "mem-peaks.txt", "lua-bins.txt", "X86",
                                                   Found(MemPeaksX86()), "T", Found(MemPeaksX64Good()), "X64", Absent());
        CHECK(v.bThrown);
        CHECK(v.sLine == "lua-bins.txt not found in X64");
    }
}

TEST_CASE_NAMED(FTablesHeapCapsTest, "Unit::Tables::heap caps x64", "[EngineFilter]")
{
    const std::string s86 = "# c\r\n" + Heap("geo", "Runtime", "1048576", "524288") + Heap("nab", "Runtime", "1048576", "262144") +
                            Heap("geo", "Lua", "65536", "1000");
    const std::string s64 = "# c\r\n" + Heap("geo", "Runtime", "2000000", "800000") + Heap("geo", "Lua", "99", "0");
    SUBCASE("caps derived from the x86 cap and the peak ratio")
    {
        const TableVerdict v = CompareHeapCapsX64("tables:heap-caps-x64", "T\\mem-peaks.txt", Found(s64),
                                                  "B\\mem-peaks.txt", Found(s86), "", Absent());
        CHECK(v.bOk);
        CHECK(v.sLine == "[PASS] tables:heap-caps-x64 Lua=65536 Runtime=1638400");
    }
    SUBCASE("logged caps are checked against the derived caps")
    {
        const std::string sLog = L("Memory report heap=Runtime cap=1638400 peak=1 used=1") +
                                 L("Memory report heap=Lua cap=65536 peak=1 used=1");
        CHECK(CompareHeapCapsX64("l", "T", Found(s64), "B", Found(s86), "run.log", Found(sLog)).bOk);
        const std::string sBad = L("Memory report heap=Runtime cap=1638401 peak=1") + L("Memory report heap=Other cap=5 x") +
                                 L("Memory report heap=Other cap=5 x");
        const TableVerdict v = CompareHeapCapsX64("l", "T", Found(s64), "B", Found(s86), "run.log", Found(sBad));
        CHECK(v.sLine == "[FAIL] l heaps=2");
        CHECK(Lines(v.kProblems) == "Runtime logged cap 1638401 derived 1638400|Other logged cap 5 has no derived cap|"
                                    "Lua derived 65536 not logged");
    }
    SUBCASE("a run log that is missing or has no heap lines")
    {
        CHECK(Lines(CompareHeapCapsX64("l", "T", Found(s64), "B", Found(s86), "run.log", Absent()).kProblems) ==
              "run.log not found");
        CHECK(Lines(CompareHeapCapsX64("l", "T", Found(s64), "B", Found(s86), "run.log", Found("x\r\n")).kProblems) ==
              "run.log has no MEMREPORT heap lines");
    }
    SUBCASE("an x86 heap with two caps and heaps missing on either side")
    {
        const std::string s86b = s86 + Heap("kas", "Runtime", "2097152", "1") + Heap("geo", "Debug", "1", "1");
        const std::string s64b = s64 + Heap("geo", "Gamespy", "1", "1");
        const TableVerdict v = CompareHeapCapsX64("l", "T", Found(s64b), "B", Found(s86b), "", Absent());
        CHECK(Lines(v.kProblems) == "Debug has no heap rows in T|Runtime has 2 caps in B: 1048576, 2097152|"
                                    "Gamespy has no heap rows in B");
    }
    SUBCASE("a heap row without a cap and peak stops the compare")
    {
        const TableVerdict v = CompareHeapCapsX64("l", "T", Found(Heap("geo", "Runtime", "-", "1")), "B", Found(s86), "",
                                                  Absent());
        CHECK(v.bThrown);
        CHECK(v.sLine == "T heap row without a cap and peak: geo\texit\theap\tRuntime\t-\t-\t1\t1\t1\t0");
        CHECK(CompareHeapCapsX64("l", "T", Absent(), "B", Found(s86), "", Absent()).sLine == "T not found");
    }
}

TEST_CASE_NAMED(FTablesBuildTest, "Unit::Tables::tables are built from run logs", "[EngineFilter]")
{
    SUBCASE("smoke-all digests pool sizes mem peaks and cloth headroom")
    {
        CaseLog kGeo = Case("geo1c_con", L("Script memory entry=e1 label=lab one requested=-1 applied=5 pool=p1") +
                                             L("Memory report source=turn100") +
                                             L("Memory report heap=Runtime cap=100 peak=50 used=40 allocs=7 failed=0  ") +
                                             L("Memory report pool=Foo count=4 size=16 peak=3 grown=0") +
                                             L("Memory report luaspill pool=Lua1 count=2") +
                                             L("Memory report source=exit") +
                                             L("Memory report heap=Runtime cap=100 peak=60 used=41 allocs=8 failed=0") +
                                             L("Cloth free=12 enabled=1 low=-2 high=9") +
                                             L("Sim digest turns=10 state=0x0000AAAA") +
                                             L("SIM DIGEST turns=1500 state=0x0000ABCD rand=1/2") + L("Soak turns=10") +
                                             L("Memory report heap=Late cap=1 peak=1 used=1 allocs=1 failed=0"));
        kGeo.kFinalScore = Found("123\tScore A\r\nsecond\r\n");
        std::vector<CaseLog> kCases = {NoLog("nab2g_c"), kGeo};
        const BuildResult k = BuildSmokeAllTables(kCases, "geo1c_con  ctf\r\n  nab2g_c\r\n\r\n", "abc123");
        REQUIRE(k.bOk);
        CHECK(Lines(Table(k, "smoke-all-digests.txt")->kLines) ==
              "geo1c_con\tSIM DIGEST turns=1500 state=0x0000ABCD rand=1/2\tScore A|nab2g_c\tnone\tnone");
        CHECK(Lines(Table(k, "pool-sizes.txt")->kLines) ==
              "# commit abc123|mission\tentry\tlabel\trequested\tapplied\tpool|geo1c_con\te1\tlab one\t-1\t5\tp1|"
              "nab2g_c\t-\t-\t-\t-\t-");
        CHECK(Lines(Table(k, "mem-peaks.txt")->kLines) ==
              "# commit abc123|mission\tsource\tkind\tname\tcap_or_count\tsize\tpeak\tused\tallocs_or_grown\tfailed|"
              "geo1c_con\texit\theap\tRuntime\t100\t-\t60\t41\t8\t0|geo1c_con\tturn100\tpool\tFoo\t4\t16\t3\t-\t0\t-|"
              "nab2g_c\t-\t-\t-\t-\t-\t-\t-\t-\t-");
        CHECK(Lines(Table(k, "cloth-headroom.txt")->kLines) ==
              "# commit abc123|mission\tfree\tenabled\tlow\thigh|geo1c_con\t12\t1\t-2\t9|nab2g_c\t-\t-\t-\t-");
    }
    SUBCASE("an x64 crash-delayload-x64 folder writes the crash-delayload digest row")
    {
        const BuildResult k = BuildSmokeAllTables({NoLog("crash-delayload-x64"), NoLog("crash-abort")}, "", "c");
        REQUIRE(k.bOk);
        CHECK(Lines(Table(k, "smoke-all-digests.txt")->kLines) == "crash-abort\tnone\tnone|crash-delayload\tnone\tnone");
    }
    SUBCASE("smoke-all failures")
    {
        CHECK(BuildSmokeAllTables({}, "", "c").sWhy == "no case dirs under -SmokeAllDir\\smoke");
        CHECK(BuildSmokeAllTables({Case("m", L("Script memory entry=x"))}, "m", "c").sWhy ==
              "bad SCRIPTMEM line in m: Script memory entry=x");
        CHECK(BuildSmokeAllTables({Case("m", L("Memory report heap=a cap=1"))}, "m", "c").sWhy ==
              "MEMREPORT line before source= in m: Memory report heap=a cap=1");
        CHECK(BuildSmokeAllTables({Case("m", L("Memory report source=exit") + L("Memory report odd"))}, "m", "c").sWhy ==
              "bad MEMREPORT line in m: Memory report odd");
        CHECK(BuildSmokeAllTables({Case("m", L("Cloth free=x"))}, "m", "c").sWhy == "bad CLOTH line in m: Cloth free=x");
    }
    SUBCASE("lua bins and lua hist")
    {
        const std::string sLog = L("Memory report source=turn100") +
                                 L("Memory report pool=Lua1 count=1 size=1 peak=1 grown=0") +
                                 L("Memory report source=turn1500") +
                                 L("Memory report pool=Lua1 count=2 size=16 peak=3 grown=1") +
                                 L("Memory report pool=Lua2 count=4 size=32 peak=5 grown=0") +
                                 L("Memory report luaspill pool=Lua1 count=7") +
                                 L("Memory report luahist bin=over peak=1 calls=2") +
                                 L("Memory report luahist bin=10 peak=3 calls=4") +
                                 L("Memory report luahist bin=2 peak=5 calls=6");
        const std::vector<CaseLog> kCases = {Case("geo", sLog), Case("exitonly", L("Memory report source=exit")),
                                             Case("none", L("nothing"))};
        const BuildResult k = BuildLuaTables(kCases, "geo\r\nexitonly\r\nnone\r\n", "c1");
        REQUIRE(k.bOk);
        CHECK(Lines(Table(k, "lua-bins.txt")->kLines) ==
              "# commit c1|mission\tsource\tbin\tcount\tsize\tpeak\tgrown\tspill|geo\tturn1500\t1\t2\t16\t3\t1\t7|"
              "geo\tturn1500\t2\t4\t32\t5\t0\t-|exitonly\texit\t-\t-\t-\t-\t-\t-|none\t-\t-\t-\t-\t-\t-\t-");
        CHECK(Lines(Table(k, "lua-hist.txt")->kLines) ==
              "# commit c1|mission\tbin\tpeak\tcalls|geo\t2\t5\t6|geo\t10\t3\t4|geo\tover\t1\t2|exitonly\t-\t-\t-|"
              "none\t-\t-\t-");
    }
    SUBCASE("lua failures and a missing mission")
    {
        CHECK(BuildLuaTables({Case("a", "")}, "a\r\nb\r\nc\r\nd\r\ne\r\nf\r\ng\r\n", "c").sWhy ==
              "no run.log under -LuaBinsDir for 6 missions: b, c, d, e, f");
        CHECK(BuildLuaTables({Case("a", L("Memory report source=exit") + L("Memory report luaspill x"))}, "a", "c").sWhy ==
              "bad luaspill line in a: Memory report luaspill x");
        CHECK(BuildLuaTables({Case("a", L("Memory report source=exit") + L("Memory report luahist x"))}, "a", "c").sWhy ==
              "bad luahist line in a: Memory report luahist x");
        CHECK(BuildLuaTables({Case("a", L("Memory report source=exit") + L("Memory report luahist bin=1 peak=1 calls=1") +
                                            L("Memory report luahist bin=1 peak=2 calls=2"))},
                             "a", "c")
                  .sWhy == "duplicate luahist bin 1 in a exit");
    }
    SUBCASE("offthread rows from the map")
    {
        MapSymbols kSym;
        std::string sWhy;
        REQUIRE(ReadMapSymbols(kOffMap, kSym, sWhy));
        CHECK(kSym.sTimestamp == "0123ABCD");
        CHECK(kSym.ullTextEnd == 0x402000ULL);
        CHECK(Lines(kSym.kNames) == "_main|ab|_zed");
        const std::string sLog = L("log start game build 0123ABCD99") +
                                 L("Memory report offthread count=2 sources=1 dropped=0") +
                                 L("Memory report offthread tid=7 heap=Runtime size=32 count=2 "
                                   "stack=BattlefrontII.exe+0x1015,ntdll.dll+0x20,battlefrontii.exe+0x1000");
        std::vector<OffRow> kRows = OffThreadHeader("c9");
        REQUIRE(OffThreadRows({Case("c1", sLog), Case("c0", L("x"))}, "S", kSym, kRows, sWhy));
        CHECK(Lines(OffThreadNames(kRows)) == "ab|_main");
        const std::map<std::string, std::string> kDem = {{"ab", "void ab(void)"}};
        CHECK(Lines(FormatOffThreadRows(kRows, kDem)) ==
              "# commit c9|case\tcount\tsources\tdropped|case\ttid\theap\tsize\tcount\tframe0..frame7|c0\t-\t-\t-|"
              "c1\t2\t1\t0|c1\t7\tRuntime\t32\t2\tvoid ab(void)+0x5\tntdll.dll+0x20\t_main");
    }
    SUBCASE("offthread failures")
    {
        MapSymbols kSym;
        std::string sWhy;
        REQUIRE(ReadMapSymbols(kOffMap, kSym, sWhy));
        std::vector<OffRow> kRows;
        CHECK(!OffThreadRows({}, "S\\smoke", kSym, kRows, sWhy));
        CHECK(sWhy == "no case dirs under S\\smoke");
        CHECK(!OffThreadRows({Case("c", L("log start x build 0123ABCE"))}, "S", kSym, kRows, sWhy));
        CHECK(sWhy == "map timestamp mismatch in c: build 0123ABCE map 0123ABCD");
        CHECK(!OffThreadRows({Case("c", L("Memory report offthread tid=1"))}, "S", kSym, kRows, sWhy));
        CHECK(sWhy == "bad offthread line in c: Memory report offthread tid=1");
        CHECK(!OffThreadRows({Case("c", L("Memory report offthread count=1 sources=1 dropped=0") +
                                            L("Memory report offthread tid=1 heap=a size=1 count=1 stack=-"))},
                             "S", kSym, kRows, sWhy));
        CHECK(sWhy == "no build id in c; cannot verify the map");
        Frame f;
        CHECK(!FindMapSymbol(kSym, "BattlefrontII.exe+0x5", f, sWhy));
        CHECK(sWhy == "frame 'BattlefrontII.exe+0x5' is below the first map symbol");
        CHECK(!FindMapSymbol(kSym, "BattlefrontII.exe+0x2000", f, sWhy));
        CHECK(sWhy == "frame 'BattlefrontII.exe+0x2000' is past the end of .text");
        CHECK(!FindMapSymbol(kSym, "nohex", f, sWhy));
        CHECK(sWhy == "bad stack frame 'nohex'");
        MapSymbols kBad;
        CHECK(!ReadMapSymbols("x\r\n", kBad, sWhy));
        CHECK(sWhy == "no Timestamp line in the map");
        CHECK(!ReadMapSymbols(" Timestamp is 1\r\nx\r\n", kBad, sWhy));
        CHECK(sWhy == "no Preferred load address line in the map");
        CHECK(!ReadMapSymbols(" Timestamp is 1\r\nPreferred load address is 400000\r\nPublics by Value\r\n", kBad, sWhy));
        CHECK(sWhy == "no code symbols in the map");
        CHECK(!ReadMapSymbols(" Timestamp is 1\r\nPreferred load address is 400000\r\nPublics by Value\r\n"
                              " 0001:00000000       _main                      00401000 f   main.obj\r\n",
                              kBad, sWhy));
        CHECK(sWhy == "no 0001 CODE section lines in the map");
    }
    SUBCASE("debug digests")
    {
        const std::vector<CaseLog> kCases = {Case("twice-digest-1", L("Sim digest a")),
                                             Case("digest-hot1g_con", L("Sim digest b")),
                                             Case("digest-spa3c_ass", L("Sim digest c"))};
        const BuildResult k = BuildDebugDigests(kCases);
        REQUIRE(k.bOk);
        CHECK(Lines(Table(k, "digests-debug.txt")->kLines) ==
              "twice-digest-1\tSim digest a|digest-hot1g_con\tSim digest b|digest-spa3c_ass\tSim digest c");
        CHECK(BuildDebugDigests({kCases[0]}).sWhy == "no digest line for digest-hot1g_con, digest-spa3c_ass");
    }
    SUBCASE("the twelve collect files and the hog digest rule")
    {
        const char* const apcFull[] = {"geo1c_con", "nab2g_c", "hot1g_con", "spa3c_ass"};
        std::vector<CaseLog> kCases;
        for (size_t i = 0; i < 4; ++i)
        {
            const std::string sDigest = L(std::string("Sim digest turns=1600 state=0x0000000") + (char)('1' + i));
            kCases.push_back(Case(apcFull[i], Trace1600("0x1") + L("Factory a " + std::to_string(i)) +
                                                  "Factory notlogged\r\n" + sDigest));
            kCases.push_back(Case(std::string("hog-") + apcFull[i], sDigest));
        }
        kCases.push_back(Case("dag1c_ctf", Trace1600("0x2")));
        kCases.push_back(Case("kas2c_uber", Trace1600("0x3")));
        BuildResult k = BuildCollect(kCases, 0);
        REQUIRE(k.bOk);
        REQUIRE(k.kTables.size() == 12);
        CHECK(Lines(Table(k, "factory-nab2g_c.txt")->kLines) == "Factory a 1");
        CHECK(Table(k, "digest-trace-kas2c_uber.txt")->kLines.size() == 1600);
        CHECK(Table(k, "digest-trace-kas2c_uber.txt")->kLines[0] == "Digest trace turn=1 state=0x3 rand=1/2");
        CHECK(Lines(Table(k, "digests.txt")->kLines) ==
              "geo1c_con\tSim digest turns=1600 state=0x00000001|nab2g_c\tSim digest turns=1600 state=0x00000002|"
              "hot1g_con\tSim digest turns=1600 state=0x00000003|spa3c_ass\tSim digest turns=1600 state=0x00000004");
        CHECK(Lines(Table(k, "digests-hog.txt")->kLines) == Lines(Table(k, "digests.txt")->kLines));
        CHECK(Lines(k.kVerdicts) == "[PASS] capture:hog-digest same");
        CHECK(k.kInfo[0] == "INFO: collect geo1c_con trace=1600 factory=1 Sim digest turns=1600 state=0x00000001");
        CHECK(k.kInfo[1] == "INFO: collect hog-geo1c_con Sim digest turns=1600 state=0x00000001");
        kCases[1].kRunLog = Found(L("Sim digest turns=1600 state=0x0000000F"));
        kCases[2].kRunLog = Found(Trace1600("0x1"));
        kCases[8].kRunLog = Absent();
        k = BuildCollect(kCases, 3);
        CHECK(!k.bOk);
        CHECK(Lines(k.kVerdicts) == "[FAIL] capture:hog-digest geo1c_con base='Sim digest turns=1600 state=0x00000001' "
                                    "hog='Sim digest turns=1600 state=0x0000000F'|"
                                    "[FAIL] capture:hog-digest nab2g_c base='' hog='Sim digest turns=1600 state=0x00000002'");
        CHECK(k.sWhy == "the test runner exited 3; nab2g_c has no SIM DIGEST line; nab2g_c has no FACTORY lines; "
                        "dag1c_ctf has 0 DIGEST TRACE lines, expected 1600");
        kCases[2].kRunLog = Found(L("Sim digest x"));
        CHECK(BuildCollect(kCases, 0).sWhy.find("nab2g_c has 0 DIGEST TRACE lines, expected 1600; nab2g_c has no FACTORY "
                                                "lines") != std::string::npos);
    }
    SUBCASE("baseline text is CRLF scrubbed and refuses the user name")
    {
        Scrub kScrub;
        kScrub.sRoot = "C:\\Dev\\Repo";
        kScrub.sProfile = "C:\\Users\\someone\\";
        kScrub.sUser = "someone";
        std::string sText;
        std::string sWhy;
        REQUIRE(BaselineText({"c:\\dev\\repo\\bin\\x", "C:/Users/SomeOne/y"}, kScrub, "t.txt", sText, sWhy));
        CHECK(sText == ".\\bin\\x\r\n%USERPROFILE%/y\r\n");
        CHECK(!BaselineText({"user someone here"}, kScrub, "t.txt", sText, sWhy));
        CHECK(sWhy == "user name left in t.txt after scrubbing");
        REQUIRE(BaselineText({}, kScrub, "t.txt", sText, sWhy));
        CHECK(sText.empty());
    }
}

TEST_CASE_NAMED(FTablesMapTest, "Unit::Tables::initializer tables from a map", "[EngineFilter]")
{
    MapTables k;
    std::string sWhy;
    REQUIRE(MapInitializers(kInitMap, k, sWhy));
    std::map<std::string, std::string> kDem;
    const std::vector<std::string> kArgs = UndnameArgLines(MapNames(k));
    REQUIRE(kArgs.size() == 1);
    CHECK(kArgs[0] == "\"___xc_a\" \"?y$initializer$@@3P6AXXZA\" \"?x$initializer$@@3P6AXXZA\" "
                      "\"?z$initializer$@@3P6AXXZA\" \"??__Efoo@@YAXXZ\" \"??__Ebar@@YAXXZ\"");
    ParseUndname("Microsoft (R) C++ Name Undecorator\r\n"
                 "Undecoration of :- \"??__Efoo@@YAXXZ\"\r\n"
                 "is :- \"void __cdecl `dynamic initializer for 'foo''(void)\"\r\n\r\n"
                 "Undecoration of :- \"?y$initializer$@@3P6AXXZA\"\n"
                 "is :- \"void (__cdecl* y$initializer$)(void)\"\n",
                 kDem);
    CHECK(kDem.size() == 2);
    CHECK(Lines(FormatMapRows(k.kInRange, kDem)) ==
          "0\t___xc_a\t___xc_a\tMSVCRT:initializers.obj|"
          "1\t?y$initializer$@@3P6AXXZA\tvoid (__cdecl* y$initializer$)(void)\tb.obj|"
          "2\t?x$initializer$@@3P6AXXZA\t?x$initializer$@@3P6AXXZA\ta.obj");
    CHECK(Lines(FormatMapRows(k.kDiscarded, kDem)) == "0\t?z$initializer$@@3P6AXXZA\t?z$initializer$@@3P6AXXZA\tc.obj");
    CHECK(Lines(FormatMapRows(k.kDi, kDem)) ==
          "0\t??__Efoo@@YAXXZ\tvoid __cdecl `dynamic initializer for 'foo''(void)\ta.obj|"
          "1\t??__Ebar@@YAXXZ\t??__Ebar@@YAXXZ\tb.obj");
    CHECK(MapInfoLine(k, kDem.size()) == "INFO: map XCU entries=4 initializers=3 discarded=1 di=2 demangled=2");
    MapTables kRound = k;
    kRound.ullXcuSize = 6;
    CHECK(MapInfoLine(kRound, 0).find("entries=2 ") != std::string::npos);
    kRound.ullXcuSize = 10;
    CHECK(MapInfoLine(kRound, 0).find("entries=2 ") != std::string::npos);
    std::vector<std::string> kMany;
    for (int i = 0; i < 205; ++i)
    {
        kMany.push_back("n" + std::to_string(i));
    }
    kMany.push_back("n0");
    const std::vector<std::string> kChunks = UndnameArgLines(kMany);
    REQUIRE(kChunks.size() == 3);
    CHECK(kChunks[0].find("\"n0\" \"n1\" ") == 0);
    CHECK(kChunks[2] == "\"n200\" \"n201\" \"n202\" \"n203\" \"n204\"");
    std::string sMap = kInitMap;
    CHECK(!MapInitializers(sMap.substr(0, sMap.find("  Address")), k, sWhy));
    CHECK(sWhy == "no Publics by Value table in the map");
    sMap.erase(sMap.find(" 0002:00000014"), std::string(" 0002:00000014 00000004H .CRT$XCZ                DATA\r\n").size());
    CHECK(!MapInitializers(sMap, k, sWhy));
    CHECK(sWhy == ".CRT$XCZ missing from the map section table");
    CHECK(!MapInitializers(" 0002:00000000 00000004H .CRT$XCA x\r\n 0002:00000004 00000010H .CRT$XCU x\r\n"
                           " 0002:00000014 00000004H .CRT$XCZ x\r\nPublics by Value\r\n",
                           k, sWhy));
    CHECK(sWhy == "no symbols inside the .CRT$XCA..XCZ range");
}

TEST_CASE_NAMED(FBaselineReadersTest, "Unit::Baseline::every baseline file has a reader", "[EngineFilter]")
{
    const std::vector<Reader>& kAll = BaselineReaders();
    const RepoTextFn fnSources = MapFn(ReaderSources());
    SUBCASE("the fixture tree passes")
    {
        CHECK(Lines(BaselineReadersCheck(kAll, ReaderFiles(), fnSources)) == "");
    }
    SUBCASE("a file with no reader")
    {
        std::map<std::string, std::vector<std::string> > kFiles = ReaderFiles();
        kFiles["v143"].push_back("extra-table.txt");
        CHECK(Lines(BaselineReadersCheck(kAll, kFiles, fnSources)) == "v143\\extra-table.txt has no reader");
        kFiles = ReaderFiles();
        kFiles["v143-x64"].push_back("extra-table.txt");
        CHECK(Lines(BaselineReadersCheck(kAll, kFiles, fnSources)) == "v143-x64\\extra-table.txt has no reader");
    }
    SUBCASE("an entry with no file")
    {
        std::vector<Reader> k = kAll;
        k[0].pcFile = 0;
        CHECK(Lines(BaselineReadersCheck(k, ReaderFiles(), fnSources)) ==
              "reader map entry without File and Reader: System.Collections.Hashtable|"
              "v143\\cloth-headroom.txt has no reader");
    }
    SUBCASE("an entry twice")
    {
        std::vector<Reader> k = kAll;
        k.push_back(kAll[5]);
        CHECK(Lines(BaselineReadersCheck(k, ReaderFiles(), fnSources)) == "v143\\pool-sizes.txt is in the reader map twice");
    }
    SUBCASE("a folder that is not a baseline folder")
    {
        std::vector<Reader> k = kAll;
        Reader r = kAll[0];
        r.pcFolder = "vs2013";
        k.push_back(r);
        CHECK(Lines(BaselineReadersCheck(k, ReaderFiles(), fnSources)) ==
              "cloth-headroom.txt names folder vs2013, which is not a baseline folder");
    }
    SUBCASE("a C++ reader that does not name its file")
    {
        std::map<std::string, std::string> kSrc = ReaderSources();
        kSrc["tests\\game\\crash.cpp"] = "d(\"tests\\\\baseline\\\\v143\\\\mem-peaks.txt\");";
        CHECK(Lines(BaselineReadersCheck(kAll, ReaderFiles(), MapFn(kSrc))) ==
              "v143-x64\\mem-peaks.txt C++ reader tests\\game\\crash.cpp does not name it");
        kSrc.erase("tests\\game\\standard.cpp");
        CHECK(Lines(BaselineReadersCheck(kAll, ReaderFiles(), MapFn(kSrc))) ==
              "v143-x64\\mem-peaks.txt C++ reader tests\\game\\crash.cpp does not name it|"
              "v143-x64\\commit.txt C++ reader tests\\game\\standard.cpp does not name it");
    }
    SUBCASE("a unit-test reader that does not name its file")
    {
        std::map<std::string, std::string> kSrc = ReaderSources();
        kSrc["tests\\unit\\movie_retail.cpp"] = "movie-frames.txt";
        CHECK(Lines(BaselineReadersCheck(kAll, ReaderFiles(), MapFn(kSrc))) ==
              "v143\\movie-frames.txt reader tests\\unit\\movie_retail.cpp does not name it");
    }
    SUBCASE("an entry for a file that is not there and a folder that is not there")
    {
        std::map<std::string, std::vector<std::string> > kFiles = ReaderFiles();
        std::vector<std::string>& kV = kFiles["v143"];
        kV.erase(std::find(kV.begin(), kV.end(), "digests.txt"));
        CHECK(Lines(BaselineReadersCheck(kAll, kFiles, fnSources)) ==
              "v143\\digests.txt is in the reader map but not in tests\\baseline");
        kFiles = ReaderFiles();
        kFiles.erase("v143-x64");
        const std::vector<std::string> kHits = BaselineReadersCheck(kAll, kFiles, fnSources);
        REQUIRE(!kHits.empty());
        CHECK(kHits[0] == "tests\\baseline\\v143-x64 not found");
        CHECK(kHits.back() == "v143-x64\\pool-sizes.txt is in the reader map but not in tests\\baseline");
    }
    SUBCASE("the real tree passes")
    {
        std::map<std::string, std::vector<std::string> > kFiles;
        const char* const apcFolders[] = {"v143", "v143-x64"};
        for (size_t i = 0; i < 2; ++i)
        {
            std::vector<std::string>& kList = kFiles[apcFolders[i]];
            std::vector<std::string> kDirs(1, std::string());
            while (!kDirs.empty())
            {
                const std::string sSub = kDirs.back();
                kDirs.pop_back();
                std::vector<RepoEntry> kEntries;
                const std::string sDir = std::string("tests\\baseline\\") + apcFolders[i] + (sSub.empty() ? "" : "\\" + sSub);
                REQUIRE(ListRepoFolder(sDir, "*", kEntries));
                for (size_t j = 0; j < kEntries.size(); ++j)
                {
                    const std::string sRel = sSub.empty() ? kEntries[j].sName : sSub + "\\" + kEntries[j].sName;
                    if (kEntries[j].bDir)
                    {
                        kDirs.push_back(sRel);
                    }
                    else
                    {
                        kList.push_back(sRel);
                    }
                }
            }
        }
        const RepoTextFn fnRepo = [](const std::string& sRel) {
            std::string s;
            return ReadRepoText(sRel.c_str(), s) ? Found(s) : Absent();
        };
        CHECK(Lines(BaselineReadersCheck(kAll, kFiles, fnRepo)) == "");
    }
}

TEST_CASE_NAMED(FBaselineCheckpointsTest, "Unit::Baseline::the digest checkpoints are still judged", "[EngineFilter]")
{
    const RepoTextFn fnRepo = [](const std::string& sRel) {
        std::string s;
        return ReadRepoText(sRel.c_str(), s) ? Found(s) : Absent();
    };
    CHECK(Lines(AllCheckpointsCheck(fnRepo)) == "");
    std::map<std::string, std::string> kFix;
    kFix["tests\\game\\missions.cpp"] =
        "-digesttrace 1500\" HasDigestTrace(sLog, 100) HasDigestTrace(sLog, 1500) CheckDigestRow(";
    kFix["tests\\game\\standard_case.cpp"] =
        "\" -digesttrace 1500\" HasDigestTrace(sLog, 100) hasdigesttrace(sLog, 1500) CheckDigestRow(";
    kFix["tests\\game\\twice.cpp"] = "StandardArgs(";
    CHECK(Lines(AllCheckpointsCheck(MapFn(kFix))) ==
          "tests\\game\\standard_case.cpp lacks HasDigestTrace(sLog, 1500)|tests\\game\\twice.cpp lacks CheckStandardRun(");
    kFix.erase("tests\\game\\missions.cpp");
    CHECK(AllCheckpointsCheck(MapFn(kFix))[0] == "tests\\game\\missions.cpp not found");
}

TEST_CASE_NAMED(FBaselineVerifyTest, "Unit::Baseline::manifest verify", "[EngineFilter]")
{
    const std::string sA = "alpha\r\n";
    const std::string sB = "beta\r\n";
    const std::string sManifest = "# v143 baseline manifest\r\n\r\n## Text artifacts\r\n\r\n"
                                  "| File | SHA-256 | Produced by |\r\n|------|---------|-------------|\r\n"
                                  "| `a.txt` | " +
                                  Upper(Gate::TextSha(sA)) + " | x |\r\n| `sub\\b.txt` | " + Upper(Gate::TextSha(sB)) +
                                  " | y |\r\n";
    std::vector<BaselineFile> kFiles = {{"a.txt", sA}, {"sub\\b.txt", sB}, {"MANIFEST.md", sManifest}};
    SUBCASE("a matching folder passes")
    {
        const VerifyResult k = ManifestVerify("v143", Found(sManifest), kFiles, "someone", "src/x.cpp\n");
        CHECK(k.bOk);
        CHECK(Lines(k.kLines) == "[PASS] verify:a.txt|[PASS] verify:sub\\b.txt|[PASS] capture:verify");
    }
    SUBCASE("a wrong hash and a missing file")
    {
        kFiles[0].sText = "alpha\n";
        kFiles.erase(kFiles.begin() + 1);
        const VerifyResult k = ManifestVerify("v143", Found(sManifest), kFiles, "", "");
        CHECK(!k.bOk);
        CHECK(Lines(k.kLines) == "[FAIL] verify:a.txt - hash mismatch|[FAIL] verify:sub\\b.txt - missing|"
                                 "[FAIL] capture:verify - hash mismatch a.txt|[FAIL] capture:verify - missing sub\\b.txt");
    }
    SUBCASE("a file not in the manifest")
    {
        kFiles.push_back({"c.txt", "x"});
        CHECK(Lines(ManifestVerify("v143", Found(sManifest), kFiles, "", "").kProblems) == "not in manifest c.txt");
    }
    SUBCASE("the user name in a file")
    {
        kFiles[1].sText = "path C:\\Users\\SomeOne\\x";
        CHECK(Lines(ManifestVerify("v143", Found(sManifest), kFiles, "someone", "").kProblems) ==
              "hash mismatch sub\\b.txt|user name in b.txt");
    }
    SUBCASE("a tracked binary")
    {
        CHECK(Lines(ManifestVerify("v143", Found(sManifest), kFiles, "",
                                   "bin/Win32/Release/BattlefrontII.exe\nobj/x.MAP\nobj/x.mapx\n")
                        .kProblems) == "tracked binary bin/Win32/Release/BattlefrontII.exe|tracked binary obj/x.MAP");
    }
    SUBCASE("an archive section and an empty or missing manifest")
    {
        const std::string sArchive = sManifest + "\r\n## Archive\r\n\r\n| `BattlefrontII.exe` | " + Upper(Gate::TextSha(sA)) +
                                     " |\r\n";
        CHECK(Lines(ManifestVerify("v143-x64", Found(sArchive), kFiles, "", "").kProblems) ==
              "archive section in the v143-x64 manifest");
        CHECK(Lines(ManifestVerify("v143", Found("# x\r\n"), {}, "", "").kProblems) == "no text artifacts listed");
        const VerifyResult k = ManifestVerify("v143", Absent(), kFiles, "", "");
        CHECK(Lines(k.kLines) == "[FAIL] capture:verify - MANIFEST.md not found in the output dir");
    }
}
