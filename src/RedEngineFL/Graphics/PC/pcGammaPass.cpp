#include "RedEngineFL/Graphics/PC/pcGammaPass.h"
#include "RedEngineFL/Graphics/PC/pcGammaPassPS.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcDisplayLogic.h"
#include "RedEngineFL/Util/RedLog.h"

static D3DGAMMARAMP sRamp;
static unsigned char sLut[3][256];
static bool sbIdentity = true;
static bool sbLutDirty;
static DisplayLogic::GammaPath sePath = DisplayLogic::kGammaUndecided;
static bool sbRegistered;
static bool sbEnsuring;
static bool sbFallbackWarned;
static unsigned int suiDrawFailures;
static const unsigned int kDrawFailLimit = 3;
static IDirect3DPixelShader9* sPixelShader;
static IDirect3DTexture9* sLutTexture;
static IDirect3DTexture9* sCopyTexture;
static IDirect3DSurface9* sCopySurface;
static IDirect3DStateBlock9* sStateBlock;

static void __cdecl GammaReset(ResetFlagsT eFlags);
static HRESULT DrawQuad(IDirect3DDevice9* pDevice);

static bool EnsureInner(IDirect3DDevice9* pDevice, HRESULT& hr)
{
    if (!sPixelShader)
    {
        IDirect3DPixelShader9* pShader = 0;
        hr = pDevice->CreatePixelShader(kGammaPassPS, &pShader);
        if (FAILED(hr) || !pShader)
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            return false;
        }
        sPixelShader = pShader;
    }
    if (!sLutTexture)
    {
        IDirect3DTexture9* pLut = 0;
        hr = pDevice->CreateTexture(256, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pLut, 0);
        if (FAILED(hr) || !pLut)
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            return false;
        }
        sLutTexture = pLut;
        sbLutDirty = true;
    }
    if (sCopyTexture)
    {
        hr = D3D_OK;
        return true;
    }
    const D3DPRESENT_PARAMETERS& pp = RedRenderer::pcGetPresentParameters();
    const D3DCAPS9& caps = RedRenderer::pcGetDeviceCaps();
    if (!DisplayLogic::SizeFitsCaps(pp.BackBufferWidth, pp.BackBufferHeight, caps.MaxTextureWidth,
                                    caps.MaxTextureHeight))
    {
        hr = D3DERR_INVALIDCALL;
        return false;
    }
    IDirect3DTexture9* pCopy = 0;
    hr = pDevice->CreateTexture(pp.BackBufferWidth, pp.BackBufferHeight, 1,
                                D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pCopy, 0);
    if (FAILED(hr) || !pCopy)
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        return false;
    }
    sCopyTexture = pCopy;
    IDirect3DSurface9* pSurface = 0;
    hr = sCopyTexture->GetSurfaceLevel(0, &pSurface);
    if (FAILED(hr) || !pSurface)
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        return false;
    }
    sCopySurface = pSurface;
    IDirect3DStateBlock9* pBlock = 0;
    hr = pDevice->CreateStateBlock(D3DSBT_ALL, &pBlock);
    if (FAILED(hr) || !pBlock)
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        return false;
    }
    sStateBlock = pBlock;
    return true;
}

static bool Ensure(HRESULT& hr)
{
    IDirect3DDevice9* pDevice = RedRenderer::pcGetDevice();
    if (!pDevice)
    {
        hr = E_FAIL;
        return false;
    }
    if (!sbRegistered)
    {
        RedRenderer::pcRegisterResetFunction(&GammaReset);
        sbRegistered = true;
    }
    sbEnsuring = true;
    bool bOk = EnsureInner(pDevice, hr);
    sbEnsuring = false;
    if (!bOk)
    {
        GammaPass::ReleaseDefault();
    }
    return bOk;
}

static void WriteLut()
{
    if (!sLutTexture)
    {
        return;
    }
    D3DLOCKED_RECT lr;
    if (FAILED(sLutTexture->LockRect(0, &lr, 0, 0)) || !lr.pBits)
    {
        return;
    }
    DWORD* pTexels = (DWORD*)lr.pBits;
    for (unsigned int i = 0; i < 256; ++i)
    {
        pTexels[i] = 0xFF000000 | (DWORD)sLut[0][i] << 16 | (DWORD)sLut[1][i] << 8 |
                     (DWORD)sLut[2][i];
    }
    if (SUCCEEDED(sLutTexture->UnlockRect(0)))
    {
        sbLutDirty = false;
    }
}

static void Fallback(HRESULT hr, const char* pcReason)
{
    GammaPass::Shutdown();
    if (sePath == DisplayLogic::kGammaPass && !sbFallbackWarned)
    {
        LOG(LogRender, Warning, "OPENSWBF2 GAMMA fallback to ramp reason=%s hr=0x%08X", pcReason,
            (unsigned int)hr);
        sbFallbackWarned = true;
    }
    if (sePath == DisplayLogic::kGammaUndecided)
    {
        LOG(LogRender, Inform, "OPENSWBF2 GAMMA path=ramp reason=%s hr=0x%08X", pcReason,
            (unsigned int)hr);
    }
    sePath = DisplayLogic::kGammaRamp;
    IDirect3DDevice9* pDevice = RedRenderer::pcGetDevice();
    if (pDevice)
    {
        pDevice->SetGammaRamp(0, 0, &sRamp);
    }
}

static void Activate(bool bInReset)
{
    if (sbIdentity || (RedRenderer::m_bResourcesReleased && !bInReset) ||
        !RedRenderer::pcGetDevice())
    {
        return;
    }
    if (!sCopyTexture)
    {
        if (!bInReset && RedRenderer::pcGetDevice()->TestCooperativeLevel() != D3D_OK)
        {
            return;
        }
        HRESULT hr = D3D_OK;
        if (!Ensure(hr))
        {
            Fallback(hr, "createfail");
            return;
        }
    }
    if (sePath == DisplayLogic::kGammaUndecided)
    {
        sePath = DisplayLogic::kGammaPass;
        LOG(LogRender, Inform, "OPENSWBF2 GAMMA path=pass");
    }
    WriteLut();
}

static void __cdecl GammaReset(ResetFlagsT eFlags)
{
    if (eFlags & RESETFLAGS_FREE_RESOURCES)
    {
        GammaPass::ReleaseDefault();
    }
    if ((eFlags & RESETFLAGS_CREATE_RESOURCES) && sePath != DisplayLogic::kGammaRamp)
    {
        Activate(true);
    }
}

void GammaPass::Route(const D3DGAMMARAMP& ramp, bool bNoRender, bool bTestKind, bool bProg2)
{
    sRamp = ramp;
    DisplayLogic::RampToLut(sRamp.red, sLut[0]);
    DisplayLogic::RampToLut(sRamp.green, sLut[1]);
    DisplayLogic::RampToLut(sRamp.blue, sLut[2]);
    sbIdentity = DisplayLogic::LutIsIdentity(sLut[0]) && DisplayLogic::LutIsIdentity(sLut[1]) &&
                 DisplayLogic::LutIsIdentity(sLut[2]);
    sbLutDirty = true;
    if (sePath == DisplayLogic::kGammaUndecided)
    {
        DisplayLogic::GammaReason eReason = DisplayLogic::GammaPathFor(bNoRender, bTestKind, bProg2);
        if (eReason == DisplayLogic::kGammaReasonNoProg2)
        {
            LOG(LogRender, Inform, "OPENSWBF2 GAMMA path=ramp reason=noprog2");
            sePath = DisplayLogic::kGammaRamp;
        }
        else if (eReason == DisplayLogic::kGammaReasonNoRender)
        {
            LOG(LogRender, Inform, "OPENSWBF2 GAMMA path=ramp reason=norender");
            sePath = DisplayLogic::kGammaRamp;
        }
    }
    if (sePath == DisplayLogic::kGammaRamp)
    {
        IDirect3DDevice9* pDevice = RedRenderer::pcGetDevice();
        if (pDevice)
        {
            pDevice->SetGammaRamp(0, 0, &sRamp);
        }
        return;
    }
    Activate(false);
}

void GammaPass::Draw()
{
    if (sePath == DisplayLogic::kGammaUndecided)
    {
        Activate(false);
    }
    if (sePath != DisplayLogic::kGammaPass)
    {
        return;
    }
    if (sbLutDirty && sLutTexture)
    {
        WriteLut();
    }
    if (!DisplayLogic::GammaPassRuns(sePath, sbIdentity, RedRenderer::m_bResourcesReleased,
                                     sCopyTexture != 0))
    {
        return;
    }
    if (RedRenderer::m_curRenderTarget != 0 || !RedRenderer::m_pBackBufferSurface)
    {
        return;
    }
    IDirect3DDevice9* pDevice = RedRenderer::pcGetDevice();
    if (!pDevice || !sStateBlock || !sCopySurface || !sPixelShader || !sLutTexture)
    {
        return;
    }
    sStateBlock->Capture();
    HRESULT hr =
        pDevice->StretchRect(RedRenderer::m_pBackBufferSurface, 0, sCopySurface, 0, D3DTEXF_NONE);
    if (SUCCEEDED(hr))
    {
        hr = pDevice->BeginScene();
    }
    if (SUCCEEDED(hr))
    {
        hr = DrawQuad(pDevice);
    }
    sStateBlock->Apply();
    if (SUCCEEDED(hr))
    {
        suiDrawFailures = 0;
        return;
    }
    if (pDevice->TestCooperativeLevel() != D3D_OK)
    {
        return;
    }
    if (++suiDrawFailures >= kDrawFailLimit)
    {
        Fallback(hr, "drawfail");
    }
}

static HRESULT DrawQuad(IDirect3DDevice9* pDevice)
{
    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
    pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    for (DWORD dwStage = 0; dwStage < 2; ++dwStage)
    {
        pDevice->SetSamplerState(dwStage, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        pDevice->SetSamplerState(dwStage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        pDevice->SetSamplerState(dwStage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        pDevice->SetSamplerState(dwStage, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        pDevice->SetSamplerState(dwStage, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        pDevice->SetSamplerState(dwStage, D3DSAMP_SRGBTEXTURE, 0);
    }
    pDevice->SetTexture(0, sCopyTexture);
    pDevice->SetTexture(1, sLutTexture);
    pDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    pDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    pDevice->SetVertexShader(0);
    pDevice->SetPixelShader(sPixelShader);
    pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    const D3DPRESENT_PARAMETERS& pp = RedRenderer::pcGetPresentParameters();
    D3DVIEWPORT9 vp = {0, 0, pp.BackBufferWidth, pp.BackBufferHeight, 0.0f, 1.0f};
    pDevice->SetViewport(&vp);
    const float fW = (float)pp.BackBufferWidth - 0.5f;
    const float fH = (float)pp.BackBufferHeight - 0.5f;
    const float afVerts[4][6] = {
        {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
        {fW, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f},
        {-0.5f, fH, 0.0f, 1.0f, 0.0f, 1.0f},
        {fW, fH, 0.0f, 1.0f, 1.0f, 1.0f}};
    const HRESULT hr = pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, afVerts, 24);
    const HRESULT hrEnd = pDevice->EndScene();
    return FAILED(hr) ? hr : hrEnd;
}

void GammaPass::ReleaseDefault()
{
    if (sStateBlock)
    {
        sStateBlock->Release();
        sStateBlock = 0;
    }
    if (sCopySurface)
    {
        sCopySurface->Release();
        sCopySurface = 0;
    }
    if (sCopyTexture)
    {
        sCopyTexture->Release();
        sCopyTexture = 0;
    }
}

void GammaPass::Shutdown()
{
    ReleaseDefault();
    if (sLutTexture)
    {
        sLutTexture->Release();
        sLutTexture = 0;
    }
    if (sPixelShader)
    {
        sPixelShader->Release();
        sPixelShader = 0;
    }
}

bool GammaPass::IsEnsuring()
{
    return sbEnsuring;
}
