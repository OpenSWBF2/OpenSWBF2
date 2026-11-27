#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string>

#include "harness/gamerun.h"
#include "hardening/hardening_vendor.h"
#include "testutil.h"

TEST_CASE_NAMED(FHardeningVendorTest1, "Hardening::Binary::d3dx9-staged", "[EngineFilter][Vendor][AfterAll]")
{
    const std::string sGameDir = GameRun::GameDir();
    REQUIRE_MESSAGE(!sGameDir.empty(), "the Vendor run gave no --game-dir");
    std::string sPins;
    ReadRepoText("lib\\vendor-pins.props", sPins);
    const Hardening::D3dxStaged k = Hardening::D3dxStagedCheck(sPins, RunnerPlatform(), sGameDir);
    const std::string sLine = Hardening::D3dxStagedLine(k);
    if (k.sStatus == "SKIP")
    {
        SkipCase(sLine.c_str());
    }
    CHECK_MESSAGE(k.sStatus == "PASS", sLine);
}
