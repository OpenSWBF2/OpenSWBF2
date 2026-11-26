#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>

#include <string>
#include <vector>

#include "gate/gate_keys.h"
#include "gate/gate_machine.h"
#include "harness/runner.h"
#include "testutil.h"
#include "x87/x87_inventory.h"

namespace
{

const char* const kPinnedRel = "tests\\baseline\\v143\\x87-inventory.tsv";
const char* const kCausesRel = "tests\\baseline\\v143\\CAUSES.md";
const char* const kIncomingName = "x87-inventory.tsv";

void SkipUnlessWin32Release()
{
    if (RunnerPlatform() != "Win32" || RunnerConfig() != "Release")
    {
        SkipCase(("the x87 inventory is pinned for the Win32 Release exe; this run is " + RunnerPlatform() + " " +
                  RunnerConfig())
                     .c_str());
    }
}

std::vector<X87::Row> ReadPinned()
{
    std::string sText;
    const bool bFound = ReadRepoBytes(kPinnedRel, sText);
    std::vector<X87::Row> kRows;
    std::string sError;
    const bool bRead = X87::ReadInventoryText(bFound, sText, kPinnedRel, kRows, sError);
    REQUIRE_MESSAGE(bRead, sError);
    return kRows;
}

void JudgeAgainstPinned(const std::vector<X87::Row>& kPinned, const std::vector<X87::Row>& kFresh)
{
    std::vector<std::string> kBad;
    std::string sError;
    const bool bCompared = X87::CompareInventory(kPinned, kFresh, kBad, sError);
    REQUIRE_MESSAGE(bCompared, sError);
    if (kBad.empty())
    {
        printf("[PASS] x87-inventory rows=%u\n", (unsigned int)kPinned.size());
    }
    else
    {
        printf("[FAIL] x87-inventory rows=%u problems=%u\n", (unsigned int)kPinned.size(), (unsigned int)kBad.size());
        for (size_t i = 0; i < kBad.size(); ++i)
        {
            printf("    %s\n", kBad[i].c_str());
        }
    }
    fflush(stdout);
    const size_t uBad = kBad.size();
    CHECK_MESSAGE(uBad == 0, "x87-inventory has " << uBad << " problems against " << kPinnedRel);
}

}

TEST_CASE_NAMED(FX87InventoryTest1, "X87::Inventory::the Release exe matches the pinned inventory",
                "[EngineFilter][X87][OnRequest]")
{
    SkipUnlessWin32Release();
    const ULONGLONG uStart = GetTickCount64();
    const std::string sRoot = RepoRoot();
    const Hardening::DiskFiles kFiles;
    std::string sCauses;
    const bool bCauses = ReadRepoText(kCausesRel, sCauses);
    REQUIRE_MESSAGE(bCauses, "x87-inventory: " << kCausesRel << " not found");
    X87::SourceRules kRules;
    std::string sError;
    const bool bSources = X87::PrepareSources(kFiles, sRoot, sRoot + "\\BattlefrontII.vcxproj", sCauses, kRules, sError);
    REQUIRE_MESSAGE(bSources, sError);
    const std::string sMapRel = "obj\\Win32\\Release\\BattlefrontII.map";
    std::string sMap;
    const bool bMapRead = ReadRepoBytes(sMapRel, sMap);
    REQUIRE_MESSAGE(bMapRead, "x87-inventory: " << sMapRel << " not found");
    X87::MapObjects kMap;
    const bool bMap =
        X87::ReadMapObjects(Gate::DecodeText(std::vector<unsigned char>(sMap.begin(), sMap.end())), kMap, sError);
    REQUIRE_MESSAGE(bMap, sError);
    const std::string sExe = Gate::FullPath(sRoot + "\\bin\\Win32\\Release\\BattlefrontII.exe");
    std::set<long long> kBounds;
    std::vector<X87::DisasmFunc> kFuncs;
    const std::vector<std::string> kArgs = {"/nologo", "/disasm:nobytes", sExe};
    const bool bDisasm = X87::RunDisasm("Win32", kArgs, &kBounds, kFuncs, sError);
    REQUIRE_MESSAGE(bDisasm, sError);
    const X87::DecodeFn fnDecode = [&sExe](long long iFrom, long long iTo, std::vector<X87::DisasmFunc>& kOut,
                                           std::string& sWhy) {
        const std::vector<std::string> kRange = {"/nologo", "/disasm:nobytes",
                                                 "/range:0x" + X87::Hex8(iFrom) + ",0x" + X87::Hex8(iTo), sExe};
        return X87::RunDisasm("Win32", kRange, 0, kOut, sWhy);
    };
    std::vector<X87::Row> kRows;
    const bool bBuilt = X87::BuildInventory(kRules, kMap, kFuncs, kBounds, fnDecode, kRows, sError);
    REQUIRE_MESSAGE(bBuilt, sError);
    const std::vector<X87::Row> kPinned = ReadPinned();
    const bool bKept = X87::KeepHandVerdicts(kRows, kPinned, sError);
    REQUIRE_MESSAGE(bKept, sError);
    const std::string sOut = RunnerOut() + "\\" + kIncomingName;
    Gate::MakeDirs(RunnerOut());
    const bool bWritten = Gate::WriteTextFile(sOut, X87::InventoryText(kRows));
    REQUIRE_MESSAGE(bWritten, "x87-inventory: cannot write " << sOut);
    printf("%s seconds=%d\n", X87::CountsLine(kRows).c_str(), (int)((GetTickCount64() - uStart) / 1000));
    fflush(stdout);
    JudgeAgainstPinned(kPinned, kRows);
}

TEST_CASE_NAMED(FX87InventoryTest2, "X87::Inventory::an archived inventory matches the pinned inventory",
                "[EngineFilter][X87][AfterAll]")
{
    SkipUnlessWin32Release();
    const std::string sPath = RunnerData() + "\\" + kIncomingName;
    const Hardening::DiskFiles kFiles;
    std::string sText;
    const bool bFound = kFiles.Read(Gate::FullPath(sPath), sText);
    std::vector<X87::Row> kFresh;
    std::string sError;
    const bool bRead = X87::ReadInventoryText(bFound, sText, sPath, kFresh, sError);
    REQUIRE_MESSAGE(bRead, sError);
    JudgeAgainstPinned(ReadPinned(), kFresh);
}
