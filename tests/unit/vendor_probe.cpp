#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string.h>
#include "Steam/gfVendor.h"

static const char* Reason(bool bFilePresent, unsigned long dwError, char* acOut, size_t uSize)
{
    memset(acOut, 0x7F, uSize);
    gf::vendor::LoadFailureReason(bFilePresent, dwError, acOut, uSize);
    return acOut;
}

TEST_CASE_NAMED(FVendorProbeTest1, "Unit::VendorProbe::vendor load failure reason text", "[SmokeFilter]")
{
    char acOut[64];
    CHECK(strcmp(Reason(false, 126, acOut, sizeof(acOut)), "missing") == 0);
    CHECK(strcmp(Reason(true, 193, acOut, sizeof(acOut)), "wrong bitness") == 0);
    CHECK(strcmp(Reason(false, 193, acOut, sizeof(acOut)), "wrong bitness") == 0);
    CHECK(strcmp(Reason(true, 126, acOut, sizeof(acOut)), "load failed (error 126)") == 0);
    CHECK(strcmp(Reason(true, 127, acOut, sizeof(acOut)), "load failed (error 127)") == 0);
    char acSmall[8];
    CHECK(strcmp(Reason(true, 126, acSmall, sizeof(acSmall)), "load fa") == 0);
}

TEST_CASE_NAMED(FVendorProbeTest2, "Unit::VendorProbe::vendor dll name follows pointer width", "[SmokeFilter]")
{
    const char* pcName = gf::vendor::DllName("steam_api.dll", "steam_api64.dll");
    CHECK(strcmp(pcName, sizeof(void*) == 8 ? "steam_api64.dll" : "steam_api.dll") == 0);
}

TEST_CASE_NAMED(FVendorProbeTest3, "Unit::VendorProbe::lobby off text names the 64-bit build only on x64", "[SmokeFilter]")
{
    CHECK(strcmp(gf::vendor::LobbyOffText(true),
                 "Online lobbies are not available in the 64-bit build. LAN and direct IP games work.") == 0);
    CHECK(strcmp(gf::vendor::LobbyOffText(false),
                 "Online lobbies are not available because Galaxy could not be loaded. LAN and direct IP games work.") == 0);
}
