#include "third_party/doctest/doctest.h"

#include <stdio.h>
#include <string>

#include "testutil.h"

namespace
{

bool ReadSource(const char* pcPath, std::string& sOut)
{
    FILE* pFile = fopen(pcPath, "rb");
    if (!pFile)
    {
        return false;
    }
    char acBuf[4096];
    size_t uiRead;
    while ((uiRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sOut.append(acBuf, uiRead);
    }
    fclose(pFile);
    return true;
}

}

TEST_CASE("window mode source pins" * doctest::test_suite("unit"))
{
    std::string sMain;
    std::string sMouse;
    std::string sProfile;
    std::string sWindow;
    if (!ReadSource("../src/Battlefront2/Source/pcMain.cpp", sMain) ||
        !ReadSource("../src/RedEngineFL/input/pc/pcRedMouse.cpp", sMouse) ||
        !ReadSource("../src/Battlefront2/Source/PlayerProfile.cpp", sProfile) ||
        !ReadSource("../src/RedEngineFL/Application/pc/pcWindow.cpp", sWindow))
    {
        SkipCase("source files not found");
    }

    CHECK(sWindow.find("if (IsIconic(hGlobalWnd))") != std::string::npos);
    CHECK(sWindow.find("GWL_STYLE) & kStateBits)") != std::string::npos);
    CHECK(sMain.find("PcWindow::ApplyPending();") != std::string::npos);
    CHECK(sWindow.find("pMouse->Exists() && pMouse->m_connected") != std::string::npos);
    CHECK(sWindow.find("if (sLevel == DisplayLogic::kDpiLevelPmV2)") != std::string::npos);
    CHECK(sWindow.find("if (sAlready && sLevel != DisplayLogic::kDpiLevelOff)") != std::string::npos);
    size_t uiDpiChanged = sWindow.find("void OnDpiChanged(");
    REQUIRE(uiDpiChanged != std::string::npos);
    CHECK(sWindow.find("FixClientSize(hWnd, 0, rc, sClientW, sClientH", uiDpiChanged) !=
          std::string::npos);

    size_t uiMs = sProfile.find("RedRenderer::pcMultiSampleSupported(gAdapter, msType");
    REQUIRE(uiMs != std::string::npos);
    CHECK(uiMs < sProfile.find("RedRenderer::pcSetDisplayMode(uiWidth"));
    CHECK(sMain.find("DisplayLogic::MinimizeOnDeactivate(") != std::string::npos);
    CHECK(sMain.find("HTCLIENT") != std::string::npos);
    CHECK(sMain.find("!pcWindowed && !g_bNoRender") == std::string::npos);
    CHECK(sMain.find("pcWindowed ?") == std::string::npos);
    size_t uiDpi = sMain.find("PcWindow::InitDpi(");
    size_t uiClass = sMain.find("RegisterClassA(");
    CHECK(uiDpi != std::string::npos);
    CHECK(uiClass != std::string::npos);
    CHECK(uiDpi < uiClass);

    CHECK(sMouse.find("gAppActive") != std::string::npos);
    size_t uiMap = sMouse.find("MapBackBufferToClient(m_X, m_Y,");
    size_t uiToScreen = sMouse.find("ClientToScreen(hGlobalWnd, &pt);");
    size_t uiWarp = sMouse.find("::SetCursorPos(pt.x, pt.y);");
    CHECK(uiMap != std::string::npos);
    CHECK(uiToScreen != std::string::npos);
    CHECK(uiWarp != std::string::npos);
    CHECK(uiMap < uiToScreen);
    CHECK(uiToScreen < uiWarp);
    CHECK(sMouse.find("::SetCursorPos(iX, iY)") == std::string::npos);
    CHECK(sMouse.find("::SetCursorPos(m_X, m_Y)") == std::string::npos);

    CHECK(sProfile.find("pcWindowed ? true : mWindowed") == std::string::npos);
    CHECK(sProfile.find("ResolveMode(") != std::string::npos);
}

TEST_CASE("runtime reset restyle source pins" * doctest::test_suite("unit"))
{
    std::string sRenderer;
    if (!ReadSource("../src/RedEngineFL/Graphics/PC/pcRedRenderer.cpp", sRenderer))
    {
        SkipCase("source files not found");
    }

    size_t uiBegin = sRenderer.find("void RedRenderer::internalReset()");
    size_t uiEnd = sRenderer.find("void RedRenderer::internalResetState()");
    REQUIRE(uiBegin != std::string::npos);
    REQUIRE(uiEnd != std::string::npos);
    REQUIRE(uiBegin < uiEnd);
    std::string sBody = sRenderer.substr(uiBegin, uiEnd - uiBegin);
    CHECK(sBody.find("if (bRestyle)") != std::string::npos);
    size_t uiRevert = sBody.find("m_clientH = m_clientHLast;");
    REQUIRE(uiRevert != std::string::npos);
    CHECK(sBody.find("ApplyCurrentMode(", uiRevert) != std::string::npos);
}
