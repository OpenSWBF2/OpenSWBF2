#include <windows.h>
#include <stdio.h>

#include <string>
#include <vector>

#include "third_party/doctest/doctest.h"
#include "harness/tags.h"
#include "testutil.h"

#include "gate/gate_machine.h"
#include "hardening/hardening_vendor.h"

namespace
{

std::string PinValue(const std::string& sPins, const std::string& sName)
{
    const std::string sOpen = "<" + sName + ">";
    const size_t uAt = sPins.find(sOpen);
    const size_t uEnd = uAt == std::string::npos ? std::string::npos : sPins.find("</" + sName + ">", uAt);
    return uEnd == std::string::npos ? std::string() : Gate::TrimText(sPins.substr(uAt + sOpen.size(), uEnd - uAt - sOpen.size()));
}

std::string Replaced(const std::string& sText, const std::string& sFrom, const std::string& sTo)
{
    std::string sOut = sText;
    for (size_t uAt = sOut.find(sFrom); uAt != std::string::npos; uAt = sOut.find(sFrom, uAt + sTo.size()))
    {
        sOut.replace(uAt, sFrom.size(), sTo);
    }
    return sOut;
}

struct StagedCase
{
    const char* pcName;
    const char* pcStatus;
    bool bData;
    const char* pcDll;
    const char* pcLine;
};

}

TEST_CASE_NAMED(FHardeningVendorUnitTest1, "Unit::Hardening::d3dx9-staged", "[EngineFilter]")
{
    std::string sPins;
    REQUIRE(ReadRepoText("lib\\vendor-pins.props", sPins));
    const std::string sX64Pin = PinValue(sPins, "D3dxX64Sha256");
    REQUIRE(sX64Pin.size() == 64);
    const std::string sGoodPin = Gate::TextSha("good");
    const std::string sStagedPins = Replaced(sPins, sX64Pin, sGoodPin);
    REQUIRE(sStagedPins != sPins);
    wchar_t acTemp[MAX_PATH];
    GetTempPathW(MAX_PATH, acTemp);
    char acTail[96];
    _snprintf(acTail, sizeof(acTail), "openswbf2-d3dx9-staged-%lu-%llu", GetCurrentProcessId(),
              (unsigned long long)GetTickCount64());
    acTail[sizeof(acTail) - 1] = 0;
    const std::string sDir = Gate::JoinPath(Gate::WideToUtf8(acTemp), acTail);
    const StagedCase kCases[] = {
        {"a staged folder whose copy matches the pin", "PASS", true, "good", "[PASS] vendor:d3dx9-staged"},
        {"a staged folder whose copy has another hash", "FAIL", true, "bad", "pin is"},
        {"a staged folder without the copy", "FAIL", true, 0, "missing"},
        {"an output folder that was not staged", "SKIP", false, 0, "[SKIP] vendor:d3dx9-staged - not staged"},
    };
    for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i)
    {
        const StagedCase& k = kCases[i];
        CAPTURE(k.pcName);
        char acCase[16];
        _snprintf(acCase, sizeof(acCase), "c%u", (unsigned int)(i + 1));
        acCase[sizeof(acCase) - 1] = 0;
        const std::string sCase = Gate::JoinPath(sDir, acCase);
        REQUIRE(Gate::MakeDirs(sCase));
        if (k.bData)
        {
            REQUIRE(Gate::MakeDirs(Gate::JoinPath(sCase, "data")));
        }
        if (k.pcDll)
        {
            REQUIRE(Gate::WriteTextFile(Gate::JoinPath(sCase, "d3dx9_43.dll"), k.pcDll));
        }
        const Hardening::D3dxStaged c = Hardening::D3dxStagedCheck(sStagedPins, "x64", sCase);
        const std::string sLine = Hardening::D3dxStagedLine(c);
        CHECK_MESSAGE(c.sStatus == k.pcStatus, sLine);
        CHECK_MESSAGE(sLine.find(k.pcLine) != std::string::npos, sLine);
        if (c.sStatus == "PASS" || c.sStatus == "SKIP")
        {
            CHECK(sLine == k.pcLine);
        }
    }
    const std::string sBad = Gate::JoinPath(sDir, "c2\\d3dx9_43.dll");
    CHECK(Hardening::D3dxStagedLine(Hardening::D3dxStagedCheck(sStagedPins, "x64", Gate::JoinPath(sDir, "c2"))) ==
          "[FAIL] vendor:d3dx9-staged - " + sBad + " is " + Gate::TextSha("bad") + ", pin is " + sGoodPin);
    CHECK(Hardening::D3dxStagedLine(Hardening::D3dxStagedCheck(sStagedPins, "x64", Gate::JoinPath(sDir, "c3"))) ==
          "[FAIL] vendor:d3dx9-staged - " + Gate::JoinPath(sDir, "c3\\d3dx9_43.dll") + " missing");
    const Hardening::D3dxStaged kWin32 = Hardening::D3dxStagedCheck(sStagedPins, "Win32", Gate::JoinPath(sDir, "c1"));
    const std::string sWin32 = Hardening::D3dxStagedLine(kWin32);
    CHECK_MESSAGE(kWin32.sStatus == "FAIL", sWin32);
    std::string sError;
    Gate::RemoveTree(sDir, sError);
}
