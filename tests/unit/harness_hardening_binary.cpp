#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <algorithm>
#include <string>
#include <vector>

#include "gate/gate_keys.h"
#include "hardening/hardening_allow.h"
#include "hardening/hardening_binary.h"

namespace
{

using Hardening::BinaryRow;
using Hardening::ClCounts;

std::string Replace(const std::string& sText, const std::string& sFrom, const std::string& sTo)
{
    std::string sOut = sText;
    const size_t uAt = sOut.find(sFrom);
    REQUIRE_MESSAGE(uAt != std::string::npos, "fixture text lacks " << sFrom);
    for (size_t u = uAt; u != std::string::npos; u = sOut.find(sFrom, u + sTo.size()))
    {
        sOut.replace(u, sFrom.size(), sTo);
    }
    return sOut;
}

std::string Utf16(const std::vector<std::string>& kLines)
{
    std::string sOut("\xFF\xFE", 2);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string sLine = kLines[i] + "\r\n";
        for (size_t c = 0; c < sLine.size(); ++c)
        {
            sOut += sLine[c];
            sOut += '\0';
        }
    }
    return sOut;
}

std::string Utf8Bom(const std::vector<std::string>& kLines)
{
    std::string sOut = "\xEF\xBB\xBF";
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        sOut += kLines[i] + "\r\n";
    }
    return sOut;
}

std::string JoinLines(const std::vector<std::string>& kLines, const char* pcSep)
{
    return Gate::JoinText(kLines, pcSep);
}

const BinaryRow* Find(const std::vector<BinaryRow>& kRows, const std::string& sName)
{
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        if (kRows[i].sName == sName)
        {
            return &kRows[i];
        }
    }
    return 0;
}

bool Passes(const std::vector<BinaryRow>& kRows, const std::string& sName)
{
    const BinaryRow* p = Find(kRows, sName);
    REQUIRE_MESSAGE(p != nullptr, "no row " << sName);
    return p->eStatus == Hardening::kRowPass;
}

bool HasText(const std::string& sText, const std::string& sPart)
{
    return sText.find(sPart) != std::string::npos;
}

bool HasTextNoCase(const std::string& sText, const std::string& sPart)
{
    return Gate::LowerText(sText).find(Gate::LowerText(sPart)) != std::string::npos;
}

Hardening::AllowFile RealAllow()
{
    Hardening::AllowFile kAllow;
    std::string sError;
    REQUIRE_MESSAGE(Hardening::ReadAllowFile(kAllow, sError), sError);
    return kAllow;
}

std::vector<std::string> AllowList(const Hardening::AllowFile& kAllow, const std::string& sName)
{
    const std::vector<std::string>* p = Hardening::FindAllow(kAllow, sName);
    REQUIRE_MESSAGE(p != nullptr, "allow.txt has no section " << sName);
    return *p;
}

std::vector<BinaryRow> Imports(const std::string& sImports, const std::string& sPlatform)
{
    const Hardening::AllowFile kAllow = RealAllow();
    std::string sError;
    const std::vector<BinaryRow> kRows =
        Hardening::ImportChecks(sImports, sPlatform, AllowList(kAllow, "ImportStaticAllow " + sPlatform),
                                AllowList(kAllow, "ImportDelayAllow " + sPlatform), sError);
    REQUIRE_MESSAGE(sError.empty(), sError);
    return kRows;
}

ClCounts FlagBag(int iTotal, int iAlignedNew, int iInlineOff, int iInlineOn)
{
    ClCounts k;
    k.iTotal = iTotal;
    k.iSdl = iTotal;
    k.iAlignedNewOff = iAlignedNew;
    k.iInlineOff = iInlineOff;
    k.iInlineOn = iInlineOn;
    k.iWe4311 = iTotal;
    k.iWe4312 = iTotal;
    k.iWe4302 = iTotal;
    k.iWe4477 = iTotal;
    k.iWeAll = iTotal;
    return k;
}

ClCounts TlogCounts(const Hardening::MemoryFiles& kFiles, const std::vector<std::string>& kDirs)
{
    ClCounts k;
    std::string sError;
    REQUIRE_MESSAGE(Hardening::CountClTlogs(kFiles, kDirs, k, sError), sError);
    return k;
}

const std::string kGoodHdr = "    Characteristics\n         102 characteristics\n    DLL characteristics\n            8140 DLL "
                             "characteristics\n                   Dynamic base\n                   NX compatible\n          "
                             "         Application can handle large (>2GB) addresses\n   1C13000 [   38D38] RVA [size] of "
                             "Base Relocation Directory\n  .reloc name\n";
const std::string kCookieCfg = "            0085C930 Security Cookie\n";
const std::string kGoodCfg = kCookieCfg + "            008327F0 Safe Exception Handler Table\n                 7CF Safe "
                                          "Exception Handler Count\n";
const std::string kOldHdr = "    Characteristics\n         103 characteristics\n                   Relocations "
                            "stripped\n                   NX compatible\n         0 [       0] RVA [size] of Base "
                            "Relocation Directory\n";

const std::string kImpGood = "Dump of file C:\\X\\BattlefrontII.exe\n\n  Section contains the following imports:\n\n    "
                             "KERNEL32.dll\n              1000 Import Address Table\n                 1A0 "
                             "GetTickCount\n\n    api-ms-win-crt-environment-l1-1-0.dll\n                   0 getenv\n";
const std::string kImpDelay = kImpGood +
                              "\n  Section contains the following delay load imports:\n\n    steam_api.dll\n              "
                              "00000001 Characteristics\n              0041C958 Address of HMODULE\n              "
                              "0041C944 Import Address Table\n              0041B7EC Import Name Table\n              "
                              "0041B808 Bound Import Name Table\n              00000000 Unload Import Name Table\n      "
                              "               0 time date stamp\n\n          0040125A             313 "
                              "SteamAPI_Shutdown\n\n  Summary\n\n        1000 .data\n";

const char* const kFx = "C:\\fx";

}

TEST_CASE_NAMED(FHardeningBinaryTest1, "Unit::Hardening::dynamicbase nxcompat laa reloc safeseh gs sdl", "[EngineFilter]")
{
    struct Scenario
    {
        const char* pcName;
        std::string sHdr;
        std::string sCfg;
        int iTotal;
        int iGsMinus;
        int iSdl;
        std::vector<std::string> kPass;
        std::vector<std::string> kFail;
    };
    const Scenario akScenarios[] = {
        {"hardening good text passes all", kGoodHdr, kGoodCfg, 736, 0, 736,
         {"laa", "dynamicbase", "nxcompat", "reloc", "safeseh", "gs", "sdl"}, {}},
        {"hardening old exe fails dynamicbase and reloc", kOldHdr, kGoodCfg, 736, 0, 736,
         {"nxcompat", "safeseh", "gs", "sdl"}, {"dynamicbase", "reloc", "laa"}},
        {"hardening safeseh count 0 fails", kGoodHdr, kCookieCfg + "                 0 Safe Exception Handler Count\n", 736,
         0, 736, {"laa", "dynamicbase", "nxcompat", "reloc", "gs", "sdl"}, {"safeseh"}},
        {"hardening missing security cookie fails gs", kGoodHdr, "                 7CF Safe Exception Handler Count\n", 736,
         0, 736, {"laa", "dynamicbase", "nxcompat", "reloc", "safeseh", "sdl"}, {"gs"}},
        {"hardening /GS- entries fail gs", kGoodHdr, kGoodCfg, 736, 3, 736,
         {"laa", "dynamicbase", "nxcompat", "reloc", "safeseh", "sdl"}, {"gs"}},
        {"hardening empty tlog fails gs and sdl", kGoodHdr, kGoodCfg, 0, 0, 0, {"laa", "reloc", "safeseh"}, {"gs", "sdl"}},
        {"hardening missing /sdl entry fails sdl", kGoodHdr, kGoodCfg, 736, 0, 735,
         {"laa", "dynamicbase", "nxcompat", "reloc", "safeseh", "gs"}, {"sdl"}},
        {"hardening non-LAA header fails laa",
         Replace(kGoodHdr, "Application can handle large (>2GB) addresses", "Something else"), kGoodCfg, 736, 0, 736,
         {"dynamicbase", "nxcompat", "reloc", "safeseh", "gs", "sdl"}, {"laa"}},
    };
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        std::string sError;
        const std::vector<BinaryRow> kRows =
            Hardening::HardeningChecks(k.sHdr, k.sCfg, k.iTotal, k.iGsMinus, k.iSdl, sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        for (size_t i = 0; i < k.kPass.size(); ++i)
        {
            CHECK_MESSAGE(Passes(kRows, k.kPass[i]), k.kPass[i] << " should pass");
        }
        for (size_t i = 0; i < k.kFail.size(); ++i)
        {
            CHECK_MESSAGE(!Passes(kRows, k.kFail[i]), k.kFail[i] << " should fail");
        }
    }
    std::string sError;
    const std::vector<BinaryRow> kGood = Hardening::HardeningChecks(kGoodHdr, kGoodCfg, 736, 0, 736, sError);
    CHECK(Find(kGood, "gs")->sDetail == "cl entries=736 /GS-=0 security-cookie=0x0085C930");
    CHECK(Find(kGood, "reloc")->sDetail == ".reloc=True reloc-dir-size=232760 stripped=False");
    CHECK(Find(kGood, "safeseh")->sDetail == "Safe Exception Handler Count=1999");
}

TEST_CASE_NAMED(FHardeningBinaryTest2, "Unit::Hardening::flag-alignednew flag-inline flag-alignednew-tests flag-banned", "[EngineFilter]")
{
    const std::vector<std::string> kAll = {"flag-alignednew", "flag-inline", "flag-alignednew-tests", "flag-banned"};
    ClCounts kGl = FlagBag(748, 748, 0, 748);
    kGl.iGl = 1;
    ClCounts kFpFast = FlagBag(97, 97, 0, 0);
    kFpFast.iFpFast = 1;
    struct Scenario
    {
        const char* pcName;
        ClCounts kGame;
        bool bTests;
        ClCounts kTests;
        std::vector<std::string> kPass;
        std::vector<std::string> kFail;
    };
    const Scenario akScenarios[] = {
        {"flags all good pass", FlagBag(748, 748, 0, 748), true, FlagBag(97, 97, 0, 0), kAll, {}},
        {"flags alignednew 747/748 fails", FlagBag(748, 747, 0, 748), true, FlagBag(97, 97, 0, 0),
         {"flag-inline", "flag-alignednew-tests", "flag-banned"}, {"flag-alignednew"}},
        {"flags one /Zc:inline- fails", FlagBag(748, 748, 1, 748), true, FlagBag(97, 97, 0, 0),
         {"flag-alignednew", "flag-alignednew-tests", "flag-banned"}, {"flag-inline"}},
        {"flags missing /Zc:inline fails", FlagBag(748, 748, 0, 747), true, FlagBag(97, 97, 0, 0),
         {"flag-alignednew", "flag-alignednew-tests", "flag-banned"}, {"flag-inline"}},
        {"flags tests missing alignednew fails", FlagBag(748, 748, 0, 748), true, FlagBag(97, 0, 0, 0),
         {"flag-alignednew", "flag-inline", "flag-banned"}, {"flag-alignednew-tests"}},
        {"flags one /GL fails banned", kGl, true, FlagBag(97, 97, 0, 0),
         {"flag-alignednew", "flag-inline", "flag-alignednew-tests"}, {"flag-banned"}},
        {"flags one tests /fp:fast fails banned", FlagBag(748, 748, 0, 748), true, kFpFast,
         {"flag-alignednew", "flag-inline", "flag-alignednew-tests"}, {"flag-banned"}},
        {"flags empty game tlog fails", FlagBag(0, 0, 0, 0), false, ClCounts(), {"flag-banned"},
         {"flag-alignednew", "flag-inline"}},
    };
    const Hardening::AllowFile kAllow = RealAllow();
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        std::string sError;
        const std::vector<BinaryRow> kRows =
            Hardening::FlagChecks(k.kGame, k.bTests ? &k.kTests : 0, "", "", 0, "Win32", kAllow, sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        for (size_t i = 0; i < k.kPass.size(); ++i)
        {
            CHECK_MESSAGE(Passes(kRows, k.kPass[i]), k.kPass[i] << " should pass");
        }
        for (size_t i = 0; i < k.kFail.size(); ++i)
        {
            CHECK_MESSAGE(!Passes(kRows, k.kFail[i]), k.kFail[i] << " should fail");
        }
        if (!k.bTests)
        {
            CHECK_MESSAGE(Find(kRows, "flag-alignednew-tests") == nullptr,
                          "flag-alignednew-tests must be omitted without a tests tlog");
        }
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest3, "Unit::Hardening::UTF-16 tlog flag counts", "[EngineFilter]")
{
    Hardening::MemoryFiles kFiles;
    kFiles.Add(std::string(kFx) + "\\tlog\\CL.command.1.tlog",
               Utf16({"^C:\\X\\A.CPP", "/c /IC:\\X\\GL\\INC /Zc:alignedNew- /Zc:inline- /sdl /GS C:\\X\\A.CPP",
                      "^C:\\X\\B.CPP",
                      "/c /IC:\\X\\GL\\INC /Zc:alignedNew- /Zc:inline /sdl /GS /Fo\"C:\\X\\GL\\B.OBJ\" C:\\X\\B.CPP"}));
    const ClCounts k = TlogCounts(kFiles, {std::string(kFx) + "\\tlog"});
    CHECK(k.iTotal == 2);
    CHECK(k.iAlignedNewOff == 2);
    CHECK(k.iInlineOff == 1);
    CHECK(k.iInlineOn == 1);
    CHECK_MESSAGE(k.iGl == 0, "Gl matched a path");
    CHECK(k.iSdl == 2);
    CHECK(k.iGsMinus == 0);
    CHECK(k.kMissing.empty());
}

TEST_CASE_NAMED(FHardeningBinaryTest4, "Unit::Hardening::mimalloc-defines", "[EngineFilter]")
{
    const std::string sGood = "/c /ISRC\\MIMALLOC\\INCLUDE /D MI_PAGE_CACHE_OFFSET=0 /D MI_DEBUG=2 /D MI_SECURE=0 /D "
                              "MI_NO_GETENV=1 /D MI_USE_RTLGENRANDOM /GS C:\\X\\SRC\\MIMALLOC\\SRC\\STATIC.C";
    const std::string sRelease = Replace(sGood, "MI_DEBUG=2 /D MI_SECURE=0", "MI_DEBUG=0 /D MI_SECURE=4");
    struct Scenario
    {
        const char* pcName;
        std::string sLine;
        const char* pcConfig;
        bool bOk;
        bool bTests;
    };
    const Scenario akScenarios[] = {
        {"mimalloc-defines good static.c line passes", sGood, "Debug", true, true},
        {"mimalloc-defines quoted asan line passes",
         "/c /fsanitize=address /D \"MI_PAGE_CACHE_OFFSET=0\" /D \"MI_DEBUG=0\" /D \"MI_SECURE=0\" /D "
         "\"MI_TRACK_ASAN=1\" /D \"MI_NO_GETENV=1\" /D \"MI_USE_RTLGENRANDOM\" C:\\X\\SRC\\MIMALLOC\\SRC\\STATIC.C",
         "Asan", true, true},
        {"mimalloc-defines missing MI_NO_GETENV fails", Replace(sGood, " /D MI_NO_GETENV=1", ""), "Debug", false, true},
        {"mimalloc-defines missing MI_PAGE_CACHE_OFFSET=0 fails", Replace(sGood, " /D MI_PAGE_CACHE_OFFSET=0", ""),
         "Debug", false, true},
        {"mimalloc-defines MI_PAGE_CACHE_OFFSET=1 fails",
         Replace(sGood, "MI_PAGE_CACHE_OFFSET=0", "MI_PAGE_CACHE_OFFSET=1"), "Debug", false, true},
        {"mimalloc-defines asan without MI_TRACK_ASAN fails", Replace(sGood, "/c ", "/c /fsanitize=address "), "Debug",
         false, true},
        {"mimalloc-defines MI_TRACK_ASAN without asan fails", Replace(sGood, "/c ", "/c /D MI_TRACK_ASAN=1 "), "Debug",
         false, true},
        {"mimalloc-defines release line passes", sRelease, "Release", true, true},
        {"mimalloc-defines release with MI_SECURE=0 fails", Replace(sRelease, "MI_SECURE=4", "MI_SECURE=0"), "Release",
         false, true},
        {"mimalloc-defines debug values under release fail", sGood, "Release", false, true},
        {"mimalloc-defines without tests tlog checks the game only", sGood, "Debug", true, false},
    };
    const Hardening::AllowFile kAllow = RealAllow();
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        Hardening::MemoryFiles kFiles;
        kFiles.Add(std::string(kFx) + "\\mi\\CL.command.1.tlog",
                   Utf16({"^C:\\X\\A.CPP", "/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\\X\\A.CPP",
                          "^C:\\X\\SRC\\MIMALLOC\\SRC\\STATIC.C", k.sLine}));
        const ClCounts kBag = TlogCounts(kFiles, {std::string(kFx) + "\\mi"});
        std::string sError;
        const std::vector<BinaryRow> kRows =
            Hardening::FlagChecks(kBag, k.bTests ? &kBag : 0, k.pcConfig, k.pcConfig, 0, "Win32", kAllow, sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        const BinaryRow* p = Find(kRows, "mimalloc-defines");
        REQUIRE(p != nullptr);
        CHECK_MESSAGE((p->eStatus == Hardening::kRowPass) == k.bOk, p->sDetail);
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest5, "Unit::Hardening::library tlogs", "[EngineFilter]")
{
    const std::string sLibGood = "/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\\X\\LIB\\B.CPP";
    struct Scenario
    {
        const char* pcName;
        std::string sLine;
        bool bMimalloc;
        std::vector<std::string> kFail;
    };
    const Scenario akScenarios[] = {
        {"library tlog all good passes", sLibGood, true, {}},
        {"library tlog /Zc:inline- fails flag-inline", Replace(sLibGood, "/Zc:inline ", "/Zc:inline- "), true,
         {"flag-inline"}},
        {"library tlog /GS- fails flag-banned and gs", Replace(sLibGood, "/GS ", "/GS- "), true, {"flag-banned", "gs"}},
        {"library tlog without /sdl fails sdl", Replace(sLibGood, "/sdl ", ""), true, {"sdl"}},
        {"library tlog without /Zc:alignedNew- fails flag-alignednew", Replace(sLibGood, "/Zc:alignedNew- ", ""), true,
         {"flag-alignednew"}},
        {"missing mimalloc tlog dir fails mimalloc-defines", sLibGood, false,
         {"mimalloc-defines", "flag-alignednew", "flag-inline"}},
    };
    const std::vector<std::string> kJudged = {"flag-alignednew", "flag-inline", "flag-banned", "mimalloc-defines", "gs",
                                              "sdl"};
    const Hardening::AllowFile kAllow = RealAllow();
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        const std::string sExeTl = std::string(kFx) + "\\BattlefrontII.tlog";
        const std::string sLibTl = std::string(kFx) + "\\lib\\SoundFL\\SoundFL.tlog";
        const std::string sMiTl = std::string(kFx) + "\\lib\\mimalloc\\mimalloc.tlog";
        Hardening::MemoryFiles kFiles;
        kFiles.Add(sExeTl + "\\CL.command.1.tlog",
                   Utf16({"^C:\\X\\A.CPP", "/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\\X\\A.CPP"}));
        kFiles.Add(sLibTl + "\\CL.command.1.tlog", Utf16({"^C:\\X\\LIB\\B.CPP", k.sLine}));
        if (k.bMimalloc)
        {
            kFiles.Add(sMiTl + "\\CL.command.1.tlog",
                       Utf16({"^C:\\X\\SRC\\MIMALLOC\\SRC\\STATIC.C",
                              "/c /Zc:alignedNew- /Zc:inline /sdl /GS /D MI_PAGE_CACHE_OFFSET=0 /D MI_DEBUG=2 /D "
                              "MI_SECURE=0 /D MI_NO_GETENV=1 /D MI_USE_RTLGENRANDOM C:\\X\\SRC\\MIMALLOC\\SRC\\STATIC.C"}));
        }
        const ClCounts kBag = TlogCounts(kFiles, {sExeTl, sLibTl, sMiTl});
        std::string sError;
        std::vector<BinaryRow> kRows = Hardening::FlagChecks(kBag, 0, "Debug", "Debug", 0, "Win32", kAllow, sError);
        const std::vector<BinaryRow> kHard =
            Hardening::HardeningChecks(kGoodHdr, kGoodCfg, kBag.iTotal, kBag.iGsMinus, kBag.iSdl, sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        kRows.insert(kRows.end(), kHard.begin(), kHard.end());
        for (size_t j = 0; j < kJudged.size(); ++j)
        {
            const BinaryRow* p = Find(kRows, kJudged[j]);
            REQUIRE(p != nullptr);
            const bool bWant = std::find(k.kFail.begin(), k.kFail.end(), kJudged[j]) == k.kFail.end();
            CHECK_MESSAGE((p->eStatus == Hardening::kRowPass) == bWant, p->sName << ": " << p->sDetail);
        }
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest6, "Unit::Hardening::x64-truncation", "[EngineFilter]")
{
    const std::string sWeGood = "/c /Zc:alignedNew- /Zc:inline /sdl /GS /we4311 /we\"4312\" /we4302 /we4477 C:\\X\\A.CPP";
    const std::vector<std::string> kWeLuac = {"^C:\\X\\L.CPP", Replace(sWeGood, "A.CPP", "L.CPP")};
    struct Scenario
    {
        const char* pcName;
        const char* pcPlatform;
        std::vector<std::string> kGame;
        bool bTests;
        int iOk;
        const char* pcDetail;
        std::vector<std::string> kLuac;
    };
    const Scenario akScenarios[] = {
        {"x64-truncation passes plain and quoted /we on every line", "x64", {"^C:\\X\\A.CPP", sWeGood}, true, 1, 0,
         kWeLuac},
        {"x64-truncation fails a line without /we4477 and names it", "x64",
         {"^C:\\X\\A.CPP", sWeGood, "^C:\\X\\B.CPP", Replace(Replace(sWeGood, " /we4477", ""), "A.CPP", "B.CPP")}, true,
         0, "first missing C:\\X\\B.CPP", kWeLuac},
        {"x64-truncation fails a /we44770 lookalike", "x64", {"^C:\\X\\A.CPP", Replace(sWeGood, "/we4477", "/we44770")},
         true, 0, "first missing C:\\X\\A.CPP", kWeLuac},
        {"x64-truncation fails without a tests tlog", "x64", {"^C:\\X\\A.CPP", sWeGood}, false, 0, "tests cl entries=0",
         kWeLuac},
        {"x64-truncation fails a LuaCompiler line without /we4302 and names it", "x64", {"^C:\\X\\A.CPP", sWeGood}, true, 0,
         "luac cl entries=1 /we4311=1 /we4312=1 /we4302=0 /we4477=1 all four=0 first missing C:\\X\\L.CPP",
         {"^C:\\X\\L.CPP", Replace(Replace(sWeGood, " /we4302", ""), "A.CPP", "L.CPP")}},
        {"x64-truncation fails without a LuaCompiler tlog", "x64", {"^C:\\X\\A.CPP", sWeGood}, true, 0,
         "luac cl entries=0", {}},
        {"x64-truncation is not emitted on Win32", "Win32", {"^C:\\X\\A.CPP", sWeGood}, true, -1, 0, {}},
    };
    const Hardening::AllowFile kAllow = RealAllow();
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        Hardening::MemoryFiles kFiles;
        kFiles.Add(std::string(kFx) + "\\we\\CL.command.1.tlog", Utf16(k.kGame));
        const ClCounts kBag = TlogCounts(kFiles, {std::string(kFx) + "\\we"});
        const ClCounts kTests = FlagBag(97, 97, 0, 0);
        ClCounts kLuac;
        if (!k.kLuac.empty())
        {
            Hardening::MemoryFiles kLuacFiles;
            kLuacFiles.Add(std::string(kFx) + "\\luac\\CL.command.1.tlog", Utf16(k.kLuac));
            kLuac = TlogCounts(kLuacFiles, {std::string(kFx) + "\\luac"});
        }
        std::string sError;
        const std::vector<BinaryRow> kRows =
            Hardening::FlagChecks(kBag, k.bTests ? &kTests : 0, "Debug", "Debug", k.kLuac.empty() ? 0 : &kLuac,
                                  k.pcPlatform, kAllow, sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        const BinaryRow* p = Find(kRows, "x64-truncation");
        if (k.iOk < 0)
        {
            CHECK_MESSAGE(p == nullptr, "emitted");
            continue;
        }
        REQUIRE(p != nullptr);
        CHECK_MESSAGE((p->eStatus == Hardening::kRowPass) == (k.iOk == 1), p->sDetail);
        if (k.pcDetail)
        {
            CHECK_MESSAGE(HasText(p->sDetail, k.pcDetail), "detail lacks '" << k.pcDetail << "': " << p->sDetail);
        }
    }
}

namespace
{

std::string ObjDir(const char* pcPlatform)
{
    return std::string(kFx) + "\\obj\\" + pcPlatform + "\\Test";
}

std::string Project(const std::vector<std::string>& kItems)
{
    std::vector<std::string> kLines = {"<?xml version=\"1.0\" encoding=\"utf-8\"?>",
                                       "<Project xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">"};
    kLines.insert(kLines.end(), kItems.begin(), kItems.end());
    kLines.push_back("</Project>");
    return Utf8Bom(kLines);
}

void AddLink(Hardening::MemoryFiles& kFiles, const char* pcPlatform, const std::vector<std::string>& kLinked)
{
    kFiles.Add(ObjDir(pcPlatform) + "\\BattlefrontII.tlog\\link.command.1.tlog",
               Utf16({"^" + JoinLines(kLinked, "|"), "/OUT:\"X.EXE\""}));
}

BinaryRow Objects(const Hardening::MemoryFiles& kFiles, const char* pcPlatform)
{
    BinaryRow kRow;
    std::string sError;
    REQUIRE_MESSAGE(Hardening::X64ObjectsCheck(kFiles, kFx, pcPlatform, "Test", kRow, sError), sError);
    return kRow;
}

}

TEST_CASE_NAMED(FHardeningBinaryTest7, "Unit::Hardening::x64-objects", "[EngineFilter]")
{
    struct Scenario
    {
        const char* pcName;
        std::vector<std::string> kBuilt;
        std::vector<std::string> kLinked;
        bool bOk;
        const char* pcDetail;
    };
    const Scenario akScenarios[] = {
        {"x64-objects passes when every unit is built and linked", {"One", "Two"}, {"One", "Two"}, true, 0},
        {"x64-objects fails a unit with no object and names it", {"One"}, {"One", "Two"}, false, "TWO.OBJ not built"},
        {"x64-objects fails a stale object the link did not read", {"One", "Two"}, {"One"}, false, "TWO.OBJ not linked"},
    };
    const std::string sInt = ObjDir("x64");
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        Hardening::MemoryFiles kFiles;
        kFiles.Add(std::string(kFx) + "\\BattlefrontII.vcxproj",
                   Project({"  <ItemGroup>", "    <ClCompile Include=\"src\\a\\One.cpp\" />",
                            "    <ClCompile Include=\"Two.cpp\" />", "  </ItemGroup>"}));
        const std::string sOne = sInt + "\\src\\a\\ONE.OBJ";
        const std::string sTwo = sInt + "\\TWO.OBJ";
        for (size_t i = 0; i < k.kBuilt.size(); ++i)
        {
            kFiles.Add(k.kBuilt[i] == "One" ? sOne : sTwo, "obj");
        }
        std::vector<std::string> kLinked;
        for (size_t i = 0; i < k.kLinked.size(); ++i)
        {
            kLinked.push_back(k.kLinked[i] == "One" ? sOne : sTwo);
        }
        kLinked.push_back(sInt + "\\BATTLEFRONTII.RES");
        AddLink(kFiles, "x64", kLinked);
        const BinaryRow kRow = Objects(kFiles, "x64");
        CHECK_MESSAGE((kRow.eStatus == Hardening::kRowPass) == k.bOk, kRow.sDetail);
        CHECK_MESSAGE(kRow.sDetail.compare(0, 8, "units=2 ") == 0, kRow.sDetail);
        if (k.pcDetail)
        {
            CHECK_MESSAGE(HasTextNoCase(kRow.sDetail, k.pcDetail), "detail lacks '" << k.pcDetail << "': " << kRow.sDetail);
        }
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest8, "Unit::Hardening::x64-objects masm", "[EngineFilter]")
{
    struct Scenario
    {
        const char* pcName;
        const char* pcPlatform;
        std::vector<std::string> kBuilt;
        std::vector<std::string> kLinked;
        bool bOk;
        const char* pcMasm;
        const char* pcDetail;
    };
    const Scenario akScenarios[] = {
        {"x64-objects counts an x64 MASM item that is built and linked", "x64", {"One", "Fast"}, {"One", "Fast"}, true,
         "masm=1", 0},
        {"x64-objects fails an x64 MASM object the link did not read", "x64", {"One", "Fast"}, {"One"}, false, "masm=1",
         "FAST.OBJ not linked"},
        {"x64-objects fails an x64 MASM item with no object", "x64", {"One"}, {"One"}, false, "masm=1",
         "FAST.OBJ not built"},
        {"x64-objects skips an x64 MASM item on Win32", "Win32", {"One"}, {"One"}, true, "masm=0", 0},
    };
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        const std::string sInt = ObjDir(k.pcPlatform);
        Hardening::MemoryFiles kFiles;
        kFiles.Add(std::string(kFx) + "\\BattlefrontII.vcxproj",
                   Project({"  <ItemGroup>", "    <ClCompile Include=\"One.cpp\" />",
                            "    <MASM Include=\"src\\a\\Fast.asm\" Condition=\"'$(Platform)'=='x64'\" />",
                            "  </ItemGroup>"}));
        const std::string sOne = sInt + "\\ONE.OBJ";
        const std::string sFast = sInt + "\\src\\a\\FAST.OBJ";
        for (size_t i = 0; i < k.kBuilt.size(); ++i)
        {
            kFiles.Add(k.kBuilt[i] == "One" ? sOne : sFast, "obj");
        }
        std::vector<std::string> kLinked;
        for (size_t i = 0; i < k.kLinked.size(); ++i)
        {
            kLinked.push_back(k.kLinked[i] == "One" ? sOne : sFast);
        }
        AddLink(kFiles, k.pcPlatform, kLinked);
        const BinaryRow kRow = Objects(kFiles, k.pcPlatform);
        CHECK_MESSAGE((kRow.eStatus == Hardening::kRowPass) == k.bOk, kRow.sDetail);
        const std::string sCounts = std::string("units=1 missing=");
        CHECK_MESSAGE(kRow.sDetail.compare(0, sCounts.size(), sCounts) == 0, kRow.sDetail);
        CHECK_MESSAGE(HasText(kRow.sDetail, std::string(" ") + k.pcMasm), kRow.sDetail);
        if (k.pcDetail)
        {
            CHECK_MESSAGE(HasTextNoCase(kRow.sDetail, k.pcDetail), "detail lacks '" << k.pcDetail << "': " << kRow.sDetail);
        }
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest9, "Unit::Hardening::x64-objects conditions", "[EngineFilter]")
{
    const std::vector<std::string> kItems = {"  <ItemGroup>", "    <ClCompile Include=\"All.cpp\" />",
                                             "    <ClCompile Include=\"Win.cpp\" Condition=\"'$(Platform)'=='Win32'\" />",
                                             "  </ItemGroup>", "  <ItemGroup Condition=\"'$(Platform)'=='x64'\">",
                                             "    <ClCompile Include=\"Wide.cpp\" />", "  </ItemGroup>"};
    Hardening::MemoryFiles kFiles;
    kFiles.Add(std::string(kFx) + "\\BattlefrontII.vcxproj", Project(kItems));
    const char* const apcPlatforms[] = {"x64", "Win32"};
    const std::vector<std::string> akObjs[] = {{"All", "Wide"}, {"All", "Win"}};
    for (size_t p = 0; p < 2; ++p)
    {
        std::vector<std::string> kObjs;
        for (size_t i = 0; i < akObjs[p].size(); ++i)
        {
            kObjs.push_back(ObjDir(apcPlatforms[p]) + "\\" + akObjs[p][i] + ".obj");
            kFiles.Add(kObjs.back(), "obj");
        }
        AddLink(kFiles, apcPlatforms[p], kObjs);
        const BinaryRow kRow = Objects(kFiles, apcPlatforms[p]);
        CHECK_MESSAGE(kRow.eStatus == Hardening::kRowPass, apcPlatforms[p] << ": " << kRow.sDetail);
        CHECK_MESSAGE(kRow.sDetail.compare(0, 17, "units=2 missing=0") == 0, apcPlatforms[p] << ": " << kRow.sDetail);
    }
    Hardening::MemoryFiles kMutant = kFiles;
    std::vector<std::string> kPlain = kItems;
    kPlain[2] = "    <ClCompile Include=\"Win.cpp\" />";
    kPlain[4] = "  <ItemGroup>";
    kMutant.Add(std::string(kFx) + "\\BattlefrontII.vcxproj", Project(kPlain));
    const BinaryRow kMut = Objects(kMutant, "x64");
    CHECK_MESSAGE(kMut.eStatus == Hardening::kRowFail, kMut.sDetail);
    CHECK_MESSAGE(HasTextNoCase(kMut.sDetail, "\\Win.obj not built"),
                  "a project without Conditions did not report Win.obj missing on x64: " << kMut.sDetail);
    struct Cond
    {
        const char* pcCondition;
        const char* pcPlatform;
        const char* pcConfig;
        bool bWant;
    };
    const Cond akConds[] = {
        {"'$(Platform)'=='x64' and '$(Configuration)'=='Release'", "x64", "Release", true},
        {"'$(Platform)'=='x64' and '$(Configuration)'=='Release'", "x64", "Test", false},
        {"'$(Configuration)'=='Test'", "Win32", "Test", true},
        {"", "Win32", "Test", true},
    };
    for (size_t i = 0; i < sizeof(akConds) / sizeof(akConds[0]); ++i)
    {
        bool bOk = false;
        std::string sError;
        REQUIRE_MESSAGE(Hardening::MsBuildCondition(akConds[i].pcCondition, akConds[i].pcPlatform, akConds[i].pcConfig,
                                                    bOk, sError),
                        sError);
        CHECK_MESSAGE(bOk == akConds[i].bWant, "condition '" << akConds[i].pcCondition << "' on " << akConds[i].pcPlatform
                                                             << "|" << akConds[i].pcConfig);
    }
    const char* const apcBad[] = {"'$(Platform)'!='x64'", "Exists('x.dll')", "'$(Platform)'=='x64' or '$(Platform)'=='Win32'",
                                  "'$(SWBF2_VENDOR_X64)'==''"};
    for (size_t i = 0; i < sizeof(apcBad) / sizeof(apcBad[0]); ++i)
    {
        bool bOk = true;
        std::string sError;
        CHECK_MESSAGE(!Hardening::MsBuildCondition(apcBad[i], "x64", "Test", bOk, sError), apcBad[i]);
        CHECK(sError == std::string("unsupported MSBuild condition: ") + apcBad[i]);
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest10, "Unit::Hardening::mimalloc-crt-hooks", "[EngineFilter]")
{
    const std::vector<std::string> kGood = {" 0002:0000156c 00000004H .CRT$XIB                DATA",
                                            " 0002:00001578 00000004H .CRT$XLB                DATA",
                                            " 0002:0000157c 00000004H .CRT$XLY                DATA",
                                            " 0002:0000156c       __mi_crt_callback_init     0085456c     static.obj",
                                            " 0002:00001578       __mi_tls_callback_pre      00854578     static.obj",
                                            " 0002:0000157c       __mi_tls_callback_post     0085457c     static.obj"};
    const std::string sGood = JoinLines(kGood, "\r\n");
    std::vector<std::string> kNoInit = kGood;
    kNoInit.erase(kNoInit.begin() + 3);
    struct Scenario
    {
        const char* pcName;
        std::string sMap;
        bool bOk;
    };
    const Scenario akScenarios[] = {
        {"good map", sGood, true},
        {"no __mi_crt_callback_init", JoinLines(kNoInit, "\r\n"), false},
        {"XIB symbol outside .CRT$XIB",
         Replace(sGood, "0002:0000156c       __mi_crt_callback_init", "0002:00001570       __mi_crt_callback_init"), false},
        {"XLY symbol from another object", Replace(sGood, "0085457c     static.obj", "0085457c     other.obj"), false},
        {"no map", "", false},
    };
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        CAPTURE(akScenarios[s].pcName);
        std::string sError;
        const BinaryRow kRow = Hardening::MiCrtHookCheck(akScenarios[s].sMap, sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        CHECK(kRow.sName == "mimalloc-crt-hooks");
        CHECK_MESSAGE((kRow.eStatus == Hardening::kRowPass) == akScenarios[s].bOk, kRow.sDetail);
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest11, "Unit::Hardening::riderlink-map", "[EngineFilter]")
{
    const std::string sRelease =
        JoinLines({" 0002:00004a4c       ??_C@_09LGAOCJJ@riderlink@ 0087ea4c     CommandLine.obj",
                   " 0002:00004a58       ??_C@_0N@DNHDPIMN@riderlinkdir@ 0087ea58     CommandLine.obj",
                   " 0002:00004a68       ??_C@_0O@BCFKFPPB@riderlinkhold@ 0087ea68     CommandLine.obj",
                   " 0002:00024870       ??_C@_0CA@DCCPKP@?9riderlink?5is?5not?5in?5this?5build@ 0089e870     pcMain.obj",
                   " 0003:0002898c       ?gLinkSwitchIgnored@@3_NA  0092598c     CommandLine.obj"},
                  "\r\n");
    const std::string sFunction = " 0001:00050730       ?Start@RiderLink@@YAX_N@Z  00451730 f   RiderLink.obj";
    const std::string sProtocol =
        " 0001:00050d50       ?Parse@RiderLinkProtocol@@YA?AW4Message@1@PBDIPADAAI@Z 00451d50 f   RiderLinkProtocol.obj";
    const std::string sTap = " 0003:005aa9c0       ?g_fnLineTap@RedWarning@@3P6AXPBD@ZA 00eba9c0     RedWarning.obj";
    struct Scenario
    {
        const char* pcName;
        std::string sMap;
        const char* pcConfig;
        bool bOk;
    };
    const Scenario akScenarios[] = {
        {"Release map with only the lower-case literals", sRelease, "Release", true},
        {"Release map with a RiderLink function", sRelease + "\r\n" + sFunction, "Release", false},
        {"Release map with a RiderLinkProtocol function", sRelease + "\r\n" + sProtocol, "Release", false},
        {"Release map with the tap slot", sRelease + "\r\n" + sTap, "Release", false},
        {"Test map without link symbols", sRelease, "Test", false},
        {"Test map with link symbols", sRelease + "\r\n" + sFunction + "\r\n" + sProtocol + "\r\n" + sTap, "Test", true},
        {"Debug map with a link function", sRelease + "\r\n" + sFunction, "Debug", true},
        {"no map", "", "Release", false},
        {"no map on Test", "", "Test", false},
    };
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        CAPTURE(akScenarios[s].pcName);
        std::string sError;
        const BinaryRow kRow = Hardening::RiderLinkMapCheck(akScenarios[s].sMap, akScenarios[s].pcConfig, sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        CHECK(kRow.sName == "riderlink-map");
        CHECK_MESSAGE((kRow.eStatus == Hardening::kRowPass) == akScenarios[s].bOk, kRow.sDetail);
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest12, "Unit::Hardening::import-modules", "[EngineFilter]")
{
    const std::string sBad = kImpGood + "\n    bcrypt.dll\n                   0 BCryptGenRandom\n";
    const char* const apcPlatforms[] = {"Win32", "x64"};
    for (size_t p = 0; p < 2; ++p)
    {
        CAPTURE(apcPlatforms[p]);
        const std::vector<BinaryRow> kBadRows = Imports(sBad, apcPlatforms[p]);
        const std::vector<BinaryRow> kEmptyRows = Imports("", apcPlatforms[p]);
        const std::vector<BinaryRow> kGoodRows = Imports(kImpGood, apcPlatforms[p]);
        REQUIRE(Find(kGoodRows, "import-modules") != nullptr);
        CHECK(Passes(kGoodRows, "import-modules"));
        CHECK(!Passes(kBadRows, "import-modules"));
        CHECK(HasText(Find(kBadRows, "import-modules")->sDetail, "bcrypt.dll"));
        CHECK(!Passes(kEmptyRows, "import-modules"));
        CHECK(HasText(Find(kEmptyRows, "import-modules")->sDetail, "kernel32.dll not found"));
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest13, "Unit::Hardening::d3dx-imports", "[EngineFilter]")
{
    const std::string sGood = kImpGood +
                              "\n    d3dx9_43.dll\n             1404C3DB0 Import Address Table\n             1405B63E8 "
                              "Import Name Table\n                     0 time date stamp\n                     0 Index of "
                              "first forwarder reference\n\n                          B7 D3DXLoadSurfaceFromSurface\n    "
                              "                       7 D3DXCheckTextureRequirements\n\n    WS2_32.dll\n               "
                              "    0 Ordinal 23\n";
    const std::string sExtra = Replace(sGood, "7 D3DXCheckTextureRequirements\n",
                                       "7 D3DXCheckTextureRequirements\n                          C1 D3DXMatrixRotationAxis\n");
    const std::string sMissing = Replace(sGood, "                          B7 D3DXLoadSurfaceFromSurface\n", "");
    const std::vector<BinaryRow> kGood = Imports(sGood, "x64");
    const std::vector<BinaryRow> kExtra = Imports(sExtra, "x64");
    const std::vector<BinaryRow> kMissing = Imports(sMissing, "x64");
    REQUIRE(Find(kGood, "d3dx-imports") != nullptr);
    CHECK_MESSAGE(Passes(kGood, "d3dx-imports"), Find(kGood, "d3dx-imports")->sDetail);
    CHECK(!Passes(kExtra, "d3dx-imports"));
    CHECK(Find(kExtra, "d3dx-imports")->sDetail == "extra: D3DXMatrixRotationAxis");
    CHECK(!Passes(kMissing, "d3dx-imports"));
    CHECK(Find(kMissing, "d3dx-imports")->sDetail == "missing: D3DXLoadSurfaceFromSurface");
    CHECK_MESSAGE(Find(Imports(sExtra, "Win32"), "d3dx-imports") == nullptr, "Win32 has a d3dx-imports row");
}

TEST_CASE_NAMED(FHardeningBinaryTest14, "Unit::Hardening::crt-float-imports", "[EngineFilter]")
{
    const std::string sGood = kImpGood +
                              "\n    api-ms-win-crt-math-l1-1-0.dll\n             1404C4998 Import Address Table\n         "
                              "    1405B6F00 Import Name Table\n                     0 time date stamp\n                 "
                              "    0 Index of first forwarder reference\n\n                          4F asin\n           "
                              "               55 atan2\n                         117 sqrtf\n                          BF "
                              "floorf\n                          9D sin\n";
    const std::string sBad = Replace(sGood, "                          9D sin\n",
                                     "                          9D sin\n                          A0 sinf\n            "
                                     "              60 atan2f\n");
    const std::vector<BinaryRow> kGood = Imports(sGood, "x64");
    const std::vector<BinaryRow> kBad = Imports(sBad, "x64");
    const std::vector<BinaryRow> kNone = Imports(kImpGood, "x64");
    REQUIRE(Find(kGood, "crt-float-imports") != nullptr);
    CHECK_MESSAGE(Passes(kGood, "crt-float-imports"), Find(kGood, "crt-float-imports")->sDetail);
    CHECK(!Passes(kBad, "crt-float-imports"));
    CHECK(Find(kBad, "crt-float-imports")->sDetail == "float CRT imports: atan2f sinf");
    CHECK(!Passes(kNone, "crt-float-imports"));
    CHECK(Find(kNone, "crt-float-imports")->sDetail == "api-ms-win-crt-math-l1-1-0.dll not in the import table");
    CHECK_MESSAGE(Find(Imports(sBad, "Win32"), "crt-float-imports") == nullptr, "Win32 has a crt-float-imports row");
}

TEST_CASE_NAMED(FHardeningBinaryTest15, "Unit::Hardening::crt-trig-route", "[EngineFilter]")
{
    const std::vector<std::string> kObjs = {"C:\\X\\OBJ\\A.OBJ", "C:\\X\\OBJ\\PCCRTTRIGX64.OBJ", "C:\\X\\OBJ\\B.OBJ"};
    const std::string sSym = "Dump of file C:\\X\\OBJ\\A.OBJ\n\nFile Type: COFF OBJECT\n\nCOFF SYMBOL TABLE\n00A 00000000 "
                             "UNDEF  notype ()    External     | PcCrtSinX64\n00B 00000000 UNDEF  notype ()    External  "
                             "   | sinh\n\nDump of file C:\\X\\OBJ\\PCCRTTRIGX64.OBJ\n\nFile Type: COFF OBJECT\n\nCOFF "
                             "SYMBOL TABLE\n00C 00000000 UNDEF  notype       External     | __imp_sin\n\nDump of file "
                             "C:\\X\\OBJ\\B.OBJ\n\nFile Type: COFF OBJECT\n\nCOFF SYMBOL TABLE\n010 00000000 SECT4  "
                             "notype ()    External     | cos\n";
    const std::string sBad = Replace(sSym, "00B 00000000 UNDEF  notype ()    External     | sinh\n",
                                     "00B 00000000 UNDEF  notype ()    External     | sinh\n00C 00000000 UNDEF  notype "
                                     "()    External     | cos\n00D 00000000 UNDEF  notype       External     | "
                                     "__imp_tan\n");
    const std::string sMissing = sSym.substr(0, sSym.find("Dump of file C:\\X\\OBJ\\B.OBJ"));
    std::string sError;
    const BinaryRow kGood = Hardening::CrtTrigRouteCheck(kObjs, sSym, sError);
    const BinaryRow kBadRow = Hardening::CrtTrigRouteCheck(kObjs, sBad, sError);
    const BinaryRow kMiss = Hardening::CrtTrigRouteCheck(kObjs, sMissing, sError);
    const BinaryRow kNoHelper = Hardening::CrtTrigRouteCheck({"C:\\X\\OBJ\\A.OBJ"}, sSym, sError);
    REQUIRE_MESSAGE(sError.empty(), sError);
    CHECK(kGood.sName == "crt-trig-route");
    CHECK_MESSAGE(kGood.eStatus == Hardening::kRowPass, kGood.sDetail);
    CHECK(kBadRow.eStatus == Hardening::kRowFail);
    CHECK(kBadRow.sDetail == "objects=3; A.OBJ references cos,tan");
    CHECK(kMiss.eStatus == Hardening::kRowFail);
    CHECK(kMiss.sDetail == "objects=3; no symbols for 1 objects: C:\\X\\OBJ\\B.OBJ");
    CHECK(kNoHelper.eStatus == Hardening::kRowFail);
    CHECK(kNoHelper.sDetail == "objects=1; pcCrtTrigX64.obj not linked");
}

TEST_CASE_NAMED(FHardeningBinaryTest16, "Unit::Hardening::import-delay-modules", "[EngineFilter]")
{
    const Hardening::AllowFile kAllow = RealAllow();
    const char* const apcPlatforms[] = {"Win32", "x64"};
    for (size_t p = 0; p < 2; ++p)
    {
        CAPTURE(apcPlatforms[p]);
        const std::vector<std::string> kStatic = AllowList(kAllow, std::string("ImportStaticAllow ") + apcPlatforms[p]);
        const std::vector<std::string> kNone;
        std::string sError;
        const std::vector<BinaryRow> kGood = Hardening::ImportChecks(kImpGood, apcPlatforms[p], kStatic, kNone, sError);
        const std::vector<BinaryRow> kDelay = Hardening::ImportChecks(kImpDelay, apcPlatforms[p], kStatic, kNone, sError);
        const std::vector<BinaryRow> kListed = Hardening::ImportChecks(
            kImpDelay, apcPlatforms[p], kStatic, std::vector<std::string>(1, "steam_api.dll"), sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        CHECK_MESSAGE(Passes(kGood, "import-modules"), "static-only fixture");
        CHECK_MESSAGE(Passes(kGood, "import-delay-modules"), "static-only fixture");
        CHECK(!Passes(kDelay, "import-delay-modules"));
        CHECK(Find(kDelay, "import-delay-modules")->sDetail == "delay imports outside the allow list: steam_api.dll");
        CHECK_MESSAGE(Passes(kDelay, "import-modules"), "static half saw the delay import: "
                                                            << Find(kDelay, "import-modules")->sDetail);
        CHECK_MESSAGE(Passes(kListed, "import-delay-modules"), Find(kListed, "import-delay-modules")->sDetail);
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest17, "Unit::Hardening::import-modules lists are per platform", "[EngineFilter]")
{
    const std::string sVc1 = kImpGood + "\n    VCRUNTIME140_1.dll\n                   0 __CxxFrameHandler4\n";
    const std::vector<BinaryRow> kWin32 = Imports(sVc1, "Win32");
    const std::vector<BinaryRow> kX64 = Imports(sVc1, "x64");
    CHECK(Passes(kX64, "import-modules"));
    CHECK(!Passes(kWin32, "import-modules"));
    CHECK(HasText(Find(kWin32, "import-modules")->sDetail, "vcruntime140_1.dll"));
}

TEST_CASE_NAMED(FHardeningBinaryTest18, "Unit::Hardening::import-modules fails binkw64 on x64", "[EngineFilter]")
{
    const std::vector<BinaryRow> kRows = Imports(kImpGood + "\n    binkw64.dll\n                   0 BinkOpen\n", "x64");
    CHECK(!Passes(kRows, "import-modules"));
    CHECK(HasText(Find(kRows, "import-modules")->sDetail, "binkw64.dll"));
}

TEST_CASE_NAMED(FHardeningBinaryTest19, "Unit::Hardening::import-delay-modules fixture discriminates", "[EngineFilter]")
{
    const std::string sMutant =
        Replace(kImpDelay, "Section contains the following delay load imports:", "OPENSWBF2 NO SUCH DUMPBIN HEADER");
    const std::vector<BinaryRow> kRows = Imports(sMutant, "Win32");
    CHECK_MESSAGE(Passes(kRows, "import-delay-modules"), Find(kRows, "import-delay-modules")->sDetail);
}

TEST_CASE_NAMED(FHardeningBinaryTest20, "Unit::Hardening::fma3-off", "[EngineFilter]")
{
    const std::vector<std::string> kHeap = {"Startup_RedInitHeap:", "  0000000140355560: sub         rsp,48h",
                                            "  0000000140355573: call        ?Install@pcCrash@@YAXXZ",
                                            "  0000000140355578: call        ?PcMathStartup@@YAXXZ",
                                            "  000000014035557D: lea         rcx,[??_C@_08DCLPFPD@heapinit@]"};
    const std::vector<std::string> kMath = {"?PcMathStartup@@YAXXZ:", "  0000000140026D60: xor         ecx,ecx",
                                            "  0000000140026D62: jmp         qword ptr [__imp__set_FMA3_enable]",
                                            "  0000000140026D69: CC CC CC CC CC CC CC                             IIIIIII"};
    struct Scenario
    {
        const char* pcName;
        bool bOk;
        std::vector<std::string> kHeapLines;
        std::vector<std::string> kMathLines;
        const char* pcDetail;
    };
    const Scenario akScenarios[] = {
        {"the shipped tail jump with ecx zeroed", true, kHeap, kMath, 0},
        {"a direct call through the import thunk after mov ecx,0", true, kHeap,
         {kMath[0], "  0000000140026D60: sub         rsp,28h", "  0000000140026D64: mov         ecx,0",
          "  0000000140026D69: call        _set_FMA3_enable", "  0000000140026D6E: add         rsp,28h",
          "  0000000140026D72: ret"},
         0},
        {"argument 1", false, kHeap, {kMath[0], "  0000000140026D60: mov         ecx,1", kMath[2]}, "does not pass 0"},
        {"ecx set to 0 then overwritten", false, kHeap,
         {kMath[0], kMath[1], "  0000000140026D62: mov         ecx,eax", kMath[2]}, "does not pass 0"},
        {"ecx set to 0 before another call", false, kHeap,
         {kMath[0], kMath[1], "  0000000140026D62: call        ?Other@@YAXXZ", kMath[2]}, "does not pass 0"},
        {"no call to _set_FMA3_enable", false, kHeap, {kMath[0], kMath[1], "  0000000140026D62: ret"},
         "does not call _set_FMA3_enable"},
        {"another function called from Startup_RedInitHeap", false,
         {kHeap[0], kHeap[1], kHeap[2], "  0000000140355578: call        ?PcMathStartupX@@YAXXZ"}, kMath,
         "does not call PcMathStartup"},
        {"no PcMathStartup call", false, {kHeap[0], kHeap[1], kHeap[2], kHeap[4]}, kMath, "does not call PcMathStartup"},
        {"no Startup_RedInitHeap", false, {}, kMath, "Startup_RedInitHeap not found"},
        {"no PcMathStartup", false, kHeap, {}, "PcMathStartup not found"},
    };
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        std::vector<std::string> kLines = {"Dump of file x.exe", "", "File Type: EXECUTABLE IMAGE", ""};
        kLines.insert(kLines.end(), k.kHeapLines.begin(), k.kHeapLines.end());
        kLines.insert(kLines.end(), {"", "  Summary", "      4BF000 .text", "Dump of file x.exe"});
        kLines.insert(kLines.end(), k.kMathLines.begin(), k.kMathLines.end());
        kLines.insert(kLines.end(), {"?Next@@YAXXZ:", "  0000000140026D70: call        qword ptr [__imp__set_FMA3_enable]"});
        std::string sError;
        const BinaryRow kRow = Hardening::Fma3OffCheck(JoinLines(kLines, "\r\n"), sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        CHECK(kRow.sName == "fma3-off");
        CHECK_MESSAGE((kRow.eStatus == Hardening::kRowPass) == k.bOk, kRow.sDetail);
        if (k.pcDetail)
        {
            CHECK_MESSAGE(HasText(kRow.sDetail, k.pcDetail), "detail does not name '" << k.pcDetail << "': " << kRow.sDetail);
        }
        if (k.bOk)
        {
            CHECK_MESSAGE(kRow.sDetail.empty(), "passing case has detail: " << kRow.sDetail);
        }
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest21, "Unit::Hardening::steam-api64-staged", "[EngineFilter]")
{
    const std::vector<std::string> kNames = {"SteamAPI_Init",           "SteamAPI_RestartAppIfNecessary",
                                             "SteamAPI_Shutdown",       "SteamAPI_RunCallbacks",
                                             "SteamAPI_RegisterCallResult", "SteamAPI_UnregisterCallResult",
                                             "SteamAPI_GetHSteamPipe",  "SteamAPI_GetHSteamUser",
                                             "SteamInternal_ContextInit", "SteamInternal_CreateInterface"};
    std::string sDef = "LIBRARY steam_api64.dll\r\nEXPORTS\r\n";
    std::string sDelayNames;
    std::string sExp = "    ordinal hint RVA      name\n\n";
    for (size_t i = 0; i < kNames.size(); ++i)
    {
        sDef += "    " + kNames[i] + "\r\n";
        sDelayNames += (i ? "\n" : "") + std::string("                                    000000014048319F     3 ") +
                       kNames[i];
        sExp += "          1    0 00001200 " + kNames[i] + "\n";
    }
    sExp += "          9    8 00001300 SteamAPI_Other\n";
    const std::string sImp = "  Section contains the following imports:\n\n    KERNEL32.dll\n             140123456 Import "
                             "Address Table\n                         0 SteamAPI_NotDelayed\n\n  Section contains the "
                             "following delay load imports:\n\n    steam_api64.dll\n              00000001 "
                             "Characteristics\n      00000001421D9F80 Address of HMODULE\n                     0 time "
                             "date stamp\n\n" +
                             sDelayNames +
                             "\n\n    Galaxy64.dll\n              00000001 Characteristics\n\n                        "
                             "            0000000140483300     0 Galaxy_NotInSteam\n";
    const std::string sGoodSha = Gate::LowerText(Gate::Sha256Hex("good", 4));
    const std::string sBadSha = Gate::LowerText(Gate::Sha256Hex("bad", 3));
    std::string sNoCallbacks;
    {
        const std::vector<std::string> kLines = Gate::SplitText(sExp, '\n');
        std::vector<std::string> kKeep;
        for (size_t i = 0; i < kLines.size(); ++i)
        {
            if (kLines[i].find(" SteamAPI_RunCallbacks") == std::string::npos)
            {
                kKeep.push_back(kLines[i]);
            }
        }
        sNoCallbacks = JoinLines(kKeep, "\n");
    }
    struct Scenario
    {
        const char* pcName;
        Hardening::RowStatus eStatus;
        bool bStaged;
        std::string sSha;
        std::string sExports;
        std::string sImports;
        std::string sDefText;
        const char* pcDetail;
    };
    const Scenario akScenarios[] = {
        {"a staged copy with the pin and every export", Hardening::kRowPass, true, sGoodSha, sExp, sImp, sDef, 0},
        {"a staged copy with another hash", Hardening::kRowFail, true, sBadSha, sExp, sImp, sDef, "pin is"},
        {"exports without SteamAPI_RunCallbacks", Hardening::kRowFail, true, sGoodSha, sNoCallbacks, sImp, sDef,
         "SteamAPI_RunCallbacks imported by the exe is not exported, SteamAPI_RunCallbacks in steam_api64.def"},
        {"a .def name the DLL does not export", Hardening::kRowFail, true, sGoodSha, sExp, sImp,
         sDef + "    SteamAPI_Missing\r\n", "SteamAPI_Missing in steam_api64.def is not exported"},
        {"an exe without a steam_api64.dll delay import", Hardening::kRowFail, true, sGoodSha, sExp,
         Replace(sImp, "    steam_api64.dll", "    other64.dll"), sDef, "no steam_api64.dll delay import"},
        {"an output folder without the copy", Hardening::kRowSkip, false, "", sExp, sImp, sDef, "not staged"},
    };
    for (size_t s = 0; s < sizeof(akScenarios) / sizeof(akScenarios[0]); ++s)
    {
        const Scenario& k = akScenarios[s];
        CAPTURE(k.pcName);
        std::string sError;
        const BinaryRow kRow = Hardening::SteamApi64StagedCheck("C:\\fx\\steam_api64.dll", k.bStaged, k.sSha, sGoodSha,
                                                                k.sExports, k.sImports, k.sDefText, sError);
        REQUIRE_MESSAGE(sError.empty(), sError);
        CHECK(kRow.sName == "steam-api64-staged");
        CHECK_MESSAGE(kRow.eStatus == k.eStatus, kRow.sDetail);
        if (k.pcDetail)
        {
            CHECK_MESSAGE(HasText(kRow.sDetail, k.pcDetail), "detail does not name '" << k.pcDetail << "': " << kRow.sDetail);
        }
        if (k.eStatus == Hardening::kRowPass)
        {
            CHECK_MESSAGE(kRow.sDetail.empty(), kRow.sDetail);
        }
        if (k.eStatus == Hardening::kRowSkip)
        {
            CHECK(kRow.sDetail == "not staged");
        }
    }
}

TEST_CASE_NAMED(FHardeningBinaryTest22, "Unit::Hardening::unicows-staged build output", "[EngineFilter]")
{
    const std::string sOut = "C:\\fx\\out";
    std::string sError;
    const BinaryRow kClean = Hardening::UnicowsBuildCheck(sOut, false, kImpGood, sError);
    const BinaryRow kStaged = Hardening::UnicowsBuildCheck(sOut, true, kImpGood, sError);
    const BinaryRow kStatic = Hardening::UnicowsBuildCheck(
        sOut, false, kImpGood + "\n    UNICOWS.dll\n                   0 GetModuleHandleW\n", sError);
    const BinaryRow kDelay =
        Hardening::UnicowsBuildCheck(sOut, false, Replace(kImpDelay, "    steam_api.dll", "    unicows.dll"), sError);
    REQUIRE_MESSAGE(sError.empty(), sError);
    CHECK(kClean.sName == "unicows-staged");
    CHECK_MESSAGE(kClean.eStatus == Hardening::kRowPass, kClean.sDetail);
    CHECK(kClean.sDetail.empty());
    CHECK(kStaged.eStatus == Hardening::kRowFail);
    CHECK(HasText(kStaged.sDetail, "staged in "));
    CHECK(kStatic.eStatus == Hardening::kRowFail);
    CHECK(HasText(kStatic.sDetail, "static import"));
    CHECK(kDelay.eStatus == Hardening::kRowFail);
    CHECK(HasText(kDelay.sDetail, "delay import"));
}

TEST_CASE_NAMED(FHardeningBinaryTest23, "Unit::Hardening::bink-gone build output", "[EngineFilter]")
{
    const std::string sOut = "C:\\fx\\out";
    std::string sError;
    const BinaryRow kClean = Hardening::BinkwBuildCheck(sOut, false, kImpGood, sError);
    const BinaryRow kStatic = Hardening::BinkwBuildCheck(
        sOut, false, kImpGood + "\n    binkw32.dll\n                   0 _BinkOpen@8\n", sError);
    const BinaryRow kDelay =
        Hardening::BinkwBuildCheck(sOut, false, Replace(kImpDelay, "    steam_api.dll", "    binkw32.dll"), sError);
    const BinaryRow kStaged = Hardening::BinkwBuildCheck(sOut, true, kImpGood, sError);
    REQUIRE_MESSAGE(sError.empty(), sError);
    CHECK(kClean.sName == "bink-gone");
    CHECK_MESSAGE(kClean.eStatus == Hardening::kRowPass, kClean.sDetail);
    CHECK(kClean.sDetail.empty());
    CHECK(kStatic.eStatus == Hardening::kRowFail);
    CHECK(kStatic.sDetail.compare(0, 26, "static import binkw32.dll") == 0);
    CHECK(kDelay.eStatus == Hardening::kRowFail);
    CHECK(kDelay.sDetail.compare(0, 25, "delay import binkw32.dll") == 0);
    CHECK(kStaged.eStatus == Hardening::kRowFail);
    CHECK(kStaged.sDetail.compare(0, 10, "staged in ") == 0);
}

namespace
{

void Put16(std::string& s, size_t uAt, unsigned int uValue)
{
    s[uAt] = static_cast<char>(uValue & 0xFF);
    s[uAt + 1] = static_cast<char>((uValue >> 8) & 0xFF);
}

void Put32(std::string& s, size_t uAt, unsigned long uValue)
{
    Put16(s, uAt, uValue & 0xFFFF);
    Put16(s, uAt + 2, (uValue >> 16) & 0xFFFF);
}

std::string PeHeader(bool bPlus)
{
    std::string s(0x400, '\0');
    s[0] = 'M';
    s[1] = 'Z';
    Put32(s, 0x3C, 0x80);
    Put32(s, 0x80, 0x00004550);
    Put16(s, 0x80 + 22, 0x0122);
    const size_t uOpt = 0x80 + 24;
    Put16(s, uOpt, bPlus ? 0x20B : 0x10B);
    Put16(s, uOpt + 70, 0x8140);
    const size_t uCount = uOpt + (bPlus ? 108 : 92);
    const size_t uDir = uOpt + (bPlus ? 112 : 96);
    Put32(s, uCount, 16);
    Put32(s, uDir + 40, 0x1EBF000);
    Put32(s, uDir + 44, 0x3EF90);
    return s;
}

}

TEST_CASE_NAMED(FHardeningBinaryTest24, "Unit::Hardening::tests-exe ASLR bytes", "[EngineFilter]")
{
    const std::string sGood = PeHeader(false);
    const std::string sGoodPlus = PeHeader(true);
    CHECK(Hardening::PeAslr(sGood).empty());
    CHECK(Hardening::PeAslr(sGoodPlus).empty());
    std::string s = sGood;
    s[0] = 'X';
    CHECK(Hardening::PeAslr(s) == "no MZ header");
    CHECK(Hardening::PeAslr(sGood.substr(0, 0x3F)) == "no MZ header");
    s = sGood;
    Put32(s, 0x3C, 0x10);
    CHECK(Hardening::PeAslr(s) == "PE header at 0x10 is outside the first 1024 bytes");
    s = sGood;
    Put32(s, 0x3C, 0x2000);
    CHECK(Hardening::PeAslr(s) == "PE header at 0x2000 is outside the first 1024 bytes");
    s = sGood;
    Put32(s, 0x3C, 0xFFFFFFF0);
    CHECK(Hardening::PeAslr(s) == "PE header at 0xFFFFFFF0 is outside the first 1024 bytes");
    CHECK(Hardening::PeAslr(sGood.substr(0, 0x98 + 96 + 40 + 4)) ==
          "PE header at 0x80 is outside the first " + std::to_string(0x98 + 96 + 40 + 4) + " bytes");
    s = sGood;
    Put32(s, 0x80, 0x00004551);
    CHECK(Hardening::PeAslr(s) == "no PE signature");
    s = sGood;
    Put16(s, 0x98, 0x107);
    CHECK(Hardening::PeAslr(s) == "optional header magic 0x107 is not PE32 or PE32+");
    s = sGood;
    Put16(s, 0x80 + 22, 0x0123);
    CHECK(Hardening::PeAslr(s) == "IMAGE_FILE_RELOCS_STRIPPED is set");
    s = sGoodPlus;
    Put16(s, 0x98 + 70, 0x8100);
    CHECK(Hardening::PeAslr(s) == "IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE is clear");
    s = sGood;
    Put32(s, 0x98 + 92, 5);
    CHECK(Hardening::PeAslr(s) == "no base relocation data directory");
    s = sGoodPlus;
    Put32(s, 0x98 + 112 + 44, 0);
    CHECK(Hardening::PeAslr(s) == "base relocation directory is empty");
    s = sGood;
    Put32(s, 0x98 + 96 + 40, 0);
    CHECK(Hardening::PeAslr(s) == "base relocation directory is empty");
    std::string sBig = sGood + std::string(8192, '\0');
    CHECK(Hardening::PeAslr(sBig).empty());
    Put32(sBig, 0x3C, 0x1000);
    CHECK(Hardening::PeAslr(sBig) == "PE header at 0x1000 is outside the first 4096 bytes");
}

TEST_CASE_NAMED(FHardeningBinaryTest25, "Unit::Hardening::msbuild conditions and items", "[EngineFilter]")
{
    bool bOk = false;
    std::string sError;
    REQUIRE(Hardening::MsBuildCondition("  '$(Platform)'=='X64'  AND  '$(Configuration)'=='test' ", "x64", "Test", bOk,
                                        sError));
    CHECK(bOk);
    REQUIRE(Hardening::MsBuildCondition("'$(Platform)' == 'Win32'", "x64", "Test", bOk, sError));
    CHECK(!bOk);
    CHECK(!Hardening::MsBuildCondition("'$(platform)'=='x64'", "x64", "Test", bOk, sError));
    CHECK(sError == "unsupported MSBuild condition: '$(platform)'=='x64'");
    Hardening::MemoryFiles kFiles;
    kFiles.Add(std::string(kFx) + "\\BattlefrontII.vcxproj",
               Project({"  <ItemGroup>",
                        "    <ProjectReference Include=\"src\\A\\A.vcxproj\">",
                        "      <UseLibraryDependencyInputs> True </UseLibraryDependencyInputs>",
                        "    </ProjectReference>",
                        "    <ProjectReference Include=\"src\\B\\B.vcxproj\" />",
                        "    <ProjectReference Include=\"src\\C\\C.vcxproj\" Condition=\"'$(Platform)'=='x64'\">",
                        "      <UseLibraryDependencyInputs>true</UseLibraryDependencyInputs>",
                        "    </ProjectReference>",
                        "  </ItemGroup>",
                        "  <ItemGroup Condition=\"'$(Configuration)'=='Test'\">",
                        "    <ProjectReference Include=\"src\\D\\D.vcxproj\">",
                        "      <UseLibraryDependencyInputs>true</UseLibraryDependencyInputs>",
                        "    </ProjectReference>",
                        "  </ItemGroup>"}));
    std::vector<Hardening::GameProject> kProjects;
    REQUIRE_MESSAGE(Hardening::GameProjects(kFiles, kFx, "Win32", "Test", kProjects, sError), sError);
    REQUIRE(kProjects.size() == 3);
    CHECK(kProjects[0].sName == "BattlefrontII");
    CHECK(kProjects[0].sTlogDir == "C:\\fx\\obj\\Win32\\Test\\BattlefrontII.tlog");
    CHECK(kProjects[1].sName == "A");
    CHECK(kProjects[1].sVcxproj == "C:\\fx\\src\\A\\A.vcxproj");
    CHECK(kProjects[1].sIntDir == "C:\\fx\\obj\\Win32\\Test\\lib\\A");
    CHECK(kProjects[1].sTlogDir == "C:\\fx\\obj\\Win32\\Test\\lib\\A\\A.tlog");
    CHECK(kProjects[2].sName == "D");
    REQUIRE_MESSAGE(Hardening::GameProjects(kFiles, kFx, "x64", "Release", kProjects, sError), sError);
    REQUIRE(kProjects.size() == 3);
    CHECK(kProjects[1].sName == "A");
    CHECK(kProjects[2].sName == "C");
    CHECK(kProjects[2].sTlogDir == "C:\\fx\\obj\\x64\\Release\\lib\\C\\C.tlog");
}

TEST_CASE_NAMED(FHardeningBinaryTest26, "Unit::Hardening::ffmpeg rows", "[EngineFilter]")
{
    std::string sError;
    const std::string sNotice = "Source: https://github.com/OpenSWBF2/OpenSWBF2 under src/FFmpegFL\r\n";
    const std::string sChanges = "+++ b/openswbf2/ossf2_alloc.c\n+++ b/openswbf2-ffmpeg.def\n";
    const std::string sConfig = "#define CONFIG_GPL 0\n#define FFMPEG_CONFIGURATION \"--disable-everything\"\n";
    const std::string sComponents = "#define CONFIG_BINK_DECODER 1\n#define CONFIG_AAC_DECODER 0\n";
    CHECK(Hardening::FFmpegLicenceCheck(sConfig, sComponents, sNotice, sChanges, sError).eStatus == Hardening::kRowPass);
    CHECK(Hardening::FFmpegLicenceCheck(sConfig + "#define CONFIG_GPL 1\n", sComponents, sNotice, sChanges, sError)
              .sDetail == "licence flag: #define CONFIG_GPL 1");
    CHECK(Hardening::FFmpegLicenceCheck(sConfig, sComponents + "#define CONFIG_AAC_DEMUXER 1\n", sNotice, sChanges, sError)
              .sDetail == "component: #define CONFIG_AAC_DEMUXER 1");
    CHECK(Hardening::FFmpegLicenceCheck(sConfig, sComponents, "", sChanges, sError).sDetail ==
          "notice: NOTICE.txt does not name https://github.com/OpenSWBF2/OpenSWBF2 under src/FFmpegFL");
    CHECK(Hardening::FFmpegLicenceCheck("", sComponents, sNotice, "", sError).sDetail == "config.h missing or empty");
    const std::string sLine = "/c /O2 /sdl /GS /Gy /wd4146 C:\\X\\SRC\\FFMPEGFL\\LIBAVCODEC\\UTILS.C";
    Hardening::FlagMap kFlags;
    Hardening::ReadClFlags("^C:\\X\\SRC\\FFMPEGFL\\LIBAVCODEC\\UTILS.C\r\n" + sLine + "\r\n", kFlags);
    const char* const apcWd4703[] = {"LIBAVUTIL\\ENCRYPTION_INFO.C", "LIBAVFORMAT\\URL.C", "LIBAVFORMAT\\AV1.C",
                                     "LIBAVFORMAT\\AVIO.C", "LIBAVCODEC\\RAW.C"};
    for (size_t i = 0; i < 5; ++i)
    {
        Hardening::ReadClFlags(std::string("^C:\\X\\SRC\\FFMPEGFL\\") + apcWd4703[i] + "\r\n" + sLine + " /wd4703\r\n",
                               kFlags);
    }
    REQUIRE(kFlags.size() == 6);
    const Hardening::BinaryRow kFlagRow = Hardening::FFmpegFlagCheck(kFlags, "Release", "Win32", sError);
    CHECK_MESSAGE(kFlagRow.eStatus == Hardening::kRowPass, kFlagRow.sDetail);
    CHECK(kFlagRow.sDetail.compare(0, 15, "cl entries=6; e") == 0);
    const Hardening::BinaryRow kDebugRow = Hardening::FFmpegFlagCheck(kFlags, "Debug", "Win32", sError);
    CHECK(kDebugRow.eStatus == Hardening::kRowFail);
    CHECK(HasText(kDebugRow.sDetail, "libavcodec\\utils.c has '/O2' want /Od"));
    const Hardening::BinaryRow kX64Row = Hardening::FFmpegFlagCheck(kFlags, "Release", "x64", sError);
    CHECK(HasText(kX64Row.sDetail, "lacks /we4311"));
    CHECK(HasText(kX64Row.sDetail, "(+"));
    const std::string sDllHdr = "                   DLL\n                   Dynamic base\n                   NX "
                                "compatible\n   1B1000 [    4AA8] RVA [size] of Base Relocation Directory\n  .reloc name\n";
    const std::string sDllCfg = "            1009E080 Security Cookie\n                   1 Safe Exception Handler Count\n";
    CHECK(Hardening::FFmpegDllCheck(sDllHdr, sDllCfg, "Win32", sError).eStatus == Hardening::kRowPass);
    CHECK(Hardening::FFmpegDllCheck(sDllHdr, "            1009E080 Security Cookie\n", "Win32", sError).sDetail ==
          "Safe Exception Handler Count=0");
    CHECK(Hardening::FFmpegDllCheck(sDllHdr, "            1009E080 Security Cookie\n", "x64", sError).eStatus ==
          Hardening::kRowPass);
    const std::string sImp = "  Section contains the following imports:\n\n    KERNEL32.dll\n                   0 Sleep\n\n "
                             "   bcrypt.dll\n                   0 BCryptGenRandom\n\n    api-ms-win-crt-heap-l1-1-0.dll\n "
                             "                  0 malloc\n                   0 free\n\n  Summary\n";
    const std::vector<Hardening::BinaryRow> kRelease =
        Hardening::FFmpegImportChecks(sImp, "Win32", "Release", std::vector<std::string>(), sError);
    REQUIRE(kRelease.size() == 2);
    CHECK(kRelease[0].eStatus == Hardening::kRowFail);
    CHECK(kRelease[0].sDetail == "imports outside the allow list: api-ms-win-crt-heap-l1-1-0.dll; caused: bcrypt.dll "
                                 "(C-FFMPEG-BCRYPT)");
    CHECK(kRelease[1].eStatus == Hardening::kRowFail);
    CHECK(kRelease[1].sDetail == "CRT heap imports: api-ms-win-crt-heap-l1-1-0.dll!malloc "
                                 "api-ms-win-crt-heap-l1-1-0.dll!free; C-FFMPEG-ALLOC-CONFIG, C-FFMPEG-VENDOR-SCOPE");
    const std::vector<Hardening::BinaryRow> kAsan =
        Hardening::FFmpegImportChecks(sImp, "Win32", "Asan", std::vector<std::string>({"free", "malloc"}), sError);
    CHECK_MESSAGE(kAsan[0].eStatus == Hardening::kRowPass, kAsan[0].sDetail);
    CHECK_MESSAGE(kAsan[1].eStatus == Hardening::kRowPass, kAsan[1].sDetail);
    const std::vector<Hardening::BinaryRow> kAsanNone =
        Hardening::FFmpegImportChecks(sImp, "Win32", "Asan", std::vector<std::string>(), sError);
    CHECK(HasText(kAsanNone[1].sDetail, "no vcasan.obj heap imports read"));
    std::vector<std::string> kHeap;
    REQUIRE(Hardening::AsanHeapNames("010 00000000 UNDEF  notype       External     | __imp__malloc\n011 00000000 UNDEF  "
                                     "notype       External     | __imp__Sleep\n012 00000000 UNDEF  notype       External"
                                     "     | __imp__free\n",
                                     "Win32", kHeap, sError));
    CHECK(Gate::JoinText(kHeap, " ") == "free malloc");
    REQUIRE_MESSAGE(sError.empty(), sError);
}
