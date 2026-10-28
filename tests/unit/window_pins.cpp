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

std::string StripCr(const std::string& sIn)
{
    std::string sOut;
    sOut.reserve(sIn.size());
    for (size_t i = 0; i < sIn.size(); ++i)
    {
        if (sIn[i] != '\r')
        {
            sOut += sIn[i];
        }
    }
    return sOut;
}

std::string FunctionBody(const std::string& sText, const char* pcSignature)
{
    size_t uiBegin = sText.find(pcSignature);
    if (uiBegin == std::string::npos)
    {
        return std::string();
    }
    size_t uiOpen = sText.find("\n{\n", uiBegin);
    if (uiOpen == std::string::npos || sText.find(';', uiBegin) < uiOpen)
    {
        return std::string();
    }
    size_t uiEnd = sText.find("\n}\n", uiOpen);
    if (uiEnd == std::string::npos)
    {
        return std::string();
    }
    return sText.substr(uiBegin, uiEnd + 2 - uiBegin);
}

bool Has(const std::string& sText, const char* pcNeedle)
{
    return sText.find(pcNeedle) != std::string::npos;
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

TEST_CASE("shadow quad reset source pins" * doctest::test_suite("unit"))
{
    std::string sQuadRaw;
    std::string sTextureRaw;
    if (!ReadSource("../src/RedEngineFL/Application/pc/pcShadowQuadRenderable.cpp", sQuadRaw) ||
        !ReadSource("../src/RedEngineFL/Graphics/PC/pcRedTexture.cpp", sTextureRaw))
    {
        SkipCase("source files not found");
    }
    const std::string sQuad = StripCr(sQuadRaw);
    const std::string sTexture = StripCr(sTextureRaw);

    const std::string sInit = FunctionBody(sQuad, "void pcShadowQuadRenderable::Init()");
    const std::string sCleanup = FunctionBody(sQuad, "void pcShadowQuadRenderable::Cleanup()");
    const std::string sReset = FunctionBody(sQuad, "static void __cdecl ShadowQuadReset(");
    REQUIRE(!sInit.empty());
    REQUIRE(!sCleanup.empty());
    REQUIRE(!sReset.empty());
    CHECK(Has(sInit, "DisplayLogic::ShadowQuadSharesSky("));
    CHECK(Has(sInit, "RedRenderer::pcRegisterResetFunction(&ShadowQuadReset);"));
    CHECK(Has(sCleanup, "RedRenderer::pcUnregisterResetFunction(&ShadowQuadReset);"));

    size_t uiFree = sReset.find("if (eFlags & RESETFLAGS_FREE_RESOURCES)");
    size_t uiDelete = sReset.find("delete pQuad->m_pShadowTex;");
    size_t uiCreate = sReset.find("if (eFlags & RESETFLAGS_CREATE_RESOURCES)");
    size_t uiRule = sReset.find("DisplayLogic::ShadowQuadSharesSky(pp.BackBufferWidth, pp.BackBufferHeight,");
    size_t uiNew = sReset.find("new RedTexture(pSky->m_uiWidth, pSky->m_uiHeight,");
    REQUIRE(uiFree != std::string::npos);
    REQUIRE(uiDelete != std::string::npos);
    REQUIRE(uiCreate != std::string::npos);
    REQUIRE(uiRule != std::string::npos);
    REQUIRE(uiNew != std::string::npos);
    CHECK(uiFree < uiDelete);
    CHECK(uiDelete < uiCreate);
    CHECK(uiCreate < uiRule);
    CHECK(uiRule < uiNew);
    CHECK(Has(sReset, "(unsigned int)pp.MultiSampleType,"));

    const std::string sTexReset = FunctionBody(sTexture, "void RedTexture::pcReset(");
    REQUIRE(!sTexReset.empty());
    size_t uiCreateLoop = sTexReset.find("if (eFlags & RESETFLAGS_CREATE_RESOURCES)");
    size_t uiSkip = sTexReset.find("if (it->m_pTexture)\n            {\n                continue;\n            }");
    REQUIRE(uiCreateLoop != std::string::npos);
    REQUIRE(uiSkip != std::string::npos);
    CHECK(uiCreateLoop < uiSkip);
}

TEST_CASE("display apply target failure source pins" * doctest::test_suite("unit"))
{
    std::string sTextureRaw;
    std::string sProfileRaw;
    if (!ReadSource("../src/RedEngineFL/Graphics/PC/pcRedTexture.cpp", sTextureRaw) ||
        !ReadSource("../src/Battlefront2/Source/PlayerProfile.cpp", sProfileRaw))
    {
        SkipCase("source files not found");
    }
    const std::string sTexture = StripCr(sTextureRaw);
    const std::string sProfile = StripCr(sProfileRaw);

    const std::string sInit = FunctionBody(sTexture, "void RedTexture::pcInit(unsigned int uiWidth,");
    REQUIRE(!sInit.empty());
    CHECK(Has(sInit, "IDirect3DTexture9* pTexture = 0;"));
    CHECK(Has(sInit, "s_defaultPoolTextures.Add(&m_listNode, this);\n        }\n        return;\n    }\n"
                     "    pcInit(pTexture, uiNameHash);"));

    const std::string sReset = FunctionBody(sTexture, "void RedTexture::pcReset(");
    REQUIRE(!sReset.empty());
    CHECK(Has(sReset, "IDirect3DTexture9* pTexture = 0;"));
    CHECK(Has(sReset, "{\n                    pTexture = 0;\n                }"));

    const std::string sCreated = FunctionBody(sTexture, "bool RedTexture::pcDefaultPoolCreated()");
    REQUIRE(!sCreated.empty());
    CHECK(Has(sCreated, "if (!it->m_pTexture)\n        {\n            return false;"));

    const std::string sUsable = FunctionBody(sProfile, "static bool ProfileSizeUsable(");
    REQUIRE(!sUsable.empty());
    CHECK(Has(sUsable, "RedRenderer::pcGetMaxTextureSize(uiMaxW, uiMaxH);"));
    CHECK(Has(sUsable, "DisplayLogic::SizeInBounds(uiW, uiH) &&\n"
                       "           DisplayLogic::SizeFitsCaps(uiW, uiH, uiMaxW, uiMaxH);"));
    const std::string sTarget = FunctionBody(sProfile, "static void ComputeTargetMode(");
    REQUIRE(!sTarget.empty());
    size_t uiCheck = sTarget.find("if (!ProfileSizeUsable(uiResX, uiResY))");
    size_t uiPolicy = sTarget.find("uiResX, uiResY, uiDeskW, uiDeskH);");
    REQUIRE(uiCheck != std::string::npos);
    REQUIRE(uiPolicy != std::string::npos);
    CHECK(uiCheck < uiPolicy);
    CHECK(!Has(sTarget, "rOpt.mResolutionX, rOpt.mResolutionY, uiDeskW"));
    const std::string sLive = FunctionBody(sProfile, "void PlayerProfile::ApplyLiveDisplay()");
    REQUIRE(!sLive.empty());
    size_t uiLiveCheck = sLive.find("if (!ProfileSizeUsable(rPC.mResolutionX, rPC.mResolutionY))");
    size_t uiApply = sLive.find("RedRenderer::pcApplyDisplayMode(");
    REQUIRE(uiLiveCheck != std::string::npos);
    REQUIRE(uiApply != std::string::npos);
    CHECK(uiLiveCheck < uiApply);
}

TEST_CASE("display apply snapshot source pins" * doctest::test_suite("unit"))
{
    std::string sRendererRaw;
    std::string sProfileRaw;
    if (!ReadSource("../src/RedEngineFL/Graphics/PC/pcRedRenderer.cpp", sRendererRaw) ||
        !ReadSource("../src/Battlefront2/Source/PlayerProfile.cpp", sProfileRaw))
    {
        SkipCase("source files not found");
    }
    const std::string sRenderer = StripCr(sRendererRaw);
    const std::string sProfile = StripCr(sProfileRaw);

    CHECK(!Has(sProfile, "CaptureAppliedPC"));
    const std::string sPre = FunctionBody(sProfile, "void PlayerProfile::tPCOpt::ApplyPreLoadSettings()");
    const std::string sLive = FunctionBody(sProfile, "void PlayerProfile::ApplyLiveDisplay()");
    const std::string sPoll = FunctionBody(sProfile, "void PlayerProfile::PollApplyNotice()");
    REQUIRE(!sPre.empty());
    REQUIRE(!sLive.empty());
    REQUIRE(!sPoll.empty());
    CHECK(!Has(sPre, "sAppliedPC"));
    CHECK(!Has(sLive, "sAppliedPC"));
    size_t uiStagePre = sPre.find("StageRequestTag(*this);");
    size_t uiSet = sPre.find("RedRenderer::pcSetDisplayMode(");
    REQUIRE(uiStagePre != std::string::npos);
    REQUIRE(uiSet != std::string::npos);
    CHECK(uiStagePre < uiSet);
    size_t uiStageLive = sLive.find("StageRequestTag(rPC);");
    size_t uiApply = sLive.find("RedRenderer::pcApplyDisplayMode(");
    REQUIRE(uiStageLive != std::string::npos);
    REQUIRE(uiApply != std::string::npos);
    CHECK(uiStageLive < uiApply);
    CHECK(Has(sPoll, "if (kTag.bValid)\n            {\n                sAppliedPC = kTag;\n            }"));
    CHECK(Has(sPoll, "DisplayLogic::ShouldRestoreProfile(sAppliedPC.bValid, sAppliedPC.uiOwner,"));
    CHECK(Has(sPoll, "RedRenderer::pcApplyPending()"));
    const std::string sStage = FunctionBody(sProfile, "static void StageRequestTag(");
    REQUIRE(!sStage.empty());
    CHECK(Has(sStage, "kTag.uiOwner = PlayerProfile::GetCurProfileIndex();"));

    const std::string sPromote = FunctionBody(sRenderer, "static void PromoteApplied()");
    const std::string sBegin = FunctionBody(sRenderer, "bool RedRenderer::BeginFrame()");
    const std::string sApplyMode = FunctionBody(sRenderer, "bool RedRenderer::pcApplyDisplayMode(");
    const std::string sInit = FunctionBody(sRenderer, "void RedRenderer::Init()");
    const std::string sNote = FunctionBody(sRenderer, "static void NoteInitApplied(bool bApplied)\n");
    REQUIRE(!sPromote.empty());
    REQUIRE(!sBegin.empty());
    REQUIRE(!sApplyMode.empty());
    REQUIRE(!sInit.empty());
    REQUIRE(!sNote.empty());
    CHECK(Has(sPromote, "sNoticeTag = sInFlightTag;"));
    CHECK(Has(sBegin, "sInFlightTag = sPendingApply.kTag;"));
    CHECK(Has(sApplyMode, "sPendingApply.kTag = sStagedTag;"));
    CHECK(Has(sInit, "NoteInitApplied(!bReverted);"));
    CHECK(Has(sInit, "bRevertUsed = true;\n            bReverted = true;"));
    CHECK(Has(sNote, "if (bApplied && sStagedTag.bValid)"));
}

TEST_CASE("display apply cleanup source pins" * doctest::test_suite("unit"))
{
    std::string sRendererRaw;
    std::string sStateRaw;
    if (!ReadSource("../src/RedEngineFL/Graphics/PC/pcRedRenderer.cpp", sRendererRaw) ||
        !ReadSource("../src/Battlefront2/Source/GameState.cpp", sStateRaw))
    {
        SkipCase("source files not found");
    }
    const std::string sRenderer = StripCr(sRendererRaw);
    const std::string sState = StripCr(sStateRaw);

    const std::string sCleanup = FunctionBody(sRenderer, "void RedRenderer::Cleanup()");
    REQUIRE(!sCleanup.empty());
    CHECK(Has(sCleanup, "sPendingApply.bSet = false;"));
    CHECK(Has(sCleanup, "sApplyNotice = 0;"));
    CHECK(Has(sCleanup, "sNoticeTag.bValid = false;"));

    const std::string sUpdate = FunctionBody(sState, "bool GameState::Update()");
    REQUIRE(!sUpdate.empty());
    size_t uiPoll = sUpdate.find("PlayerProfile::PollApplyNotice();");
    size_t uiChange = sUpdate.find("if (_bDoChangeState)");
    REQUIRE(uiPoll != std::string::npos);
    REQUIRE(uiChange != std::string::npos);
    CHECK(uiPoll < uiChange);
    CHECK(sUpdate.find("PlayerProfile::PollApplyNotice();", uiPoll + 1) == std::string::npos);
}

TEST_CASE("display apply source pins" * doctest::test_suite("unit"))
{
    std::string sRendererRaw;
    std::string sLuaRaw;
    if (!ReadSource("../src/RedEngineFL/Graphics/PC/pcRedRenderer.cpp", sRendererRaw) ||
        !ReadSource("../src/Battlefront2/Source/LuaCallbacks_PC.cpp", sLuaRaw))
    {
        SkipCase("source files not found");
    }
    const std::string sRenderer = StripCr(sRendererRaw);
    const std::string sLua = StripCr(sLuaRaw);

    const std::string sApply = FunctionBody(sRenderer, "bool RedRenderer::pcApplyDisplayMode(");
    REQUIRE(!sApply.empty());
    CHECK(Has(sApply, "pcSetDisplayMode("));
    CHECK(!Has(sApply, "internalReset("));
    CHECK(!Has(sApply, "Reset("));

    const std::string sRequest = FunctionBody(sRenderer, "bool RedRenderer::pcRequestAdapter(");
    const std::string sAdapterCb =
        FunctionBody(sLua, "int Lua_Callbacks::ScriptCB_SetPCDisplayAdapter(");
    REQUIRE(!sRequest.empty());
    REQUIRE(!sAdapterCb.empty());
    const char* apcForbidden[] = {"Reset(", "pcApplyDisplayMode", "internalReset", "CreateDevice"};
    for (size_t i = 0; i < sizeof(apcForbidden) / sizeof(apcForbidden[0]); ++i)
    {
        CHECK(!Has(sRequest, apcForbidden[i]));
        CHECK(!Has(sAdapterCb, apcForbidden[i]));
    }
    CHECK(Has(sRequest, "AdapterDecision"));
    CHECK(Has(sAdapterCb, "pcRequestAdapter"));

    CHECK(!Has(sLua, "Reset("));
    CHECK(!Has(sLua, "pcApplyDisplayMode"));
    CHECK(Has(sLua, "sPCCallbacks[37]"));
    CHECK(Has(sLua, "i < 37"));
    const std::string sVideo = FunctionBody(sLua, "int ScriptCB_SetPCVideoOptions(");
    REQUIRE(!sVideo.empty());
    size_t uiChanged = sVideo.find("if (bChanged)\n    {\n        pProfile->UpdateProfCRC();\n"
                                   "        rPC.ApplyPostLoadSettings();\n    }\n");
    size_t uiLive = sVideo.find("PlayerProfile::ApplyLiveDisplay();");
    size_t uiReturn = sVideo.rfind("return 0;");
    REQUIRE(uiChanged != std::string::npos);
    REQUIRE(uiLive != std::string::npos);
    REQUIRE(uiReturn != std::string::npos);
    CHECK(uiChanged < uiLive);
    CHECK(uiLive < uiReturn);
    CHECK(sVideo.find("return 0;", uiLive) == uiReturn);

    const std::string sBegin = FunctionBody(sRenderer, "bool RedRenderer::BeginFrame()");
    REQUIRE(!sBegin.empty());
    size_t uiPending = sBegin.find(
        "if ((sPendingApply.bSet || sForceReset) && !m_bResourcesReleased && hrCoop == D3D_OK)");
    size_t uiRecover = sBegin.find("if (m_bResourcesReleased || hrCoop == D3DERR_DEVICENOTRESET)");
    REQUIRE(uiPending != std::string::npos);
    REQUIRE(uiRecover != std::string::npos);
    CHECK(uiPending < uiRecover);

    const std::string sInit = FunctionBody(sRenderer, "void RedRenderer::Init()");
    const std::string sResize = FunctionBody(sRenderer, "static void ResizeBuffersToBackBuffer()");
    REQUIRE(!sInit.empty());
    REQUIRE(!sResize.empty());
    CHECK(Has(sInit, "DisplayLogic::BufferSizes"));
    CHECK(Has(sResize, "DisplayLogic::BufferSizes"));

    const std::string sReset = FunctionBody(sRenderer, "void RedRenderer::internalReset()");
    REQUIRE(!sReset.empty());
    size_t uiResize = sReset.find("ResizeBuffersToBackBuffer();");
    size_t uiFree = sReset.find("m_resetFunctions[i](RESETFLAGS_FREE_RESOURCES);");
    REQUIRE(uiResize != std::string::npos);
    REQUIRE(uiFree != std::string::npos);
    CHECK(uiResize < uiFree);
    CHECK(!Has(sReset, "delete m_buffers"));
    CHECK(!Has(sApply, "delete m_buffers"));
}
