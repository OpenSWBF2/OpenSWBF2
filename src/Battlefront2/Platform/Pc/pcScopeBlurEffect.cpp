#include "Battlefront2/Source/ScopeBlurEffect.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/PC/pcRedTexture.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcClearShader.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcFilterCopyShader.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"

namespace FLRenderer
{

unsigned int GetCurrentViewport();
}

namespace
{
struct BlurVertex
{
    PosXYZW pos;
    TexCoord uv0;
    TexCoord uv1;
};

struct FilterVertex
{
    PosXYZ pos;
    TexCoord uv0;
};

}

// GLOBAL: SWBF2 0x007e5e78
static FilterCopyShaderSample BoxFilter[9] = {
    { 0.125f, -3.0f, 3.0f },  { 0.125f, -2.0f, 2.0f },  { 0.125f, -1.0f, 1.0f },
    { 0.125f, 3.0f, -3.0f },  { 0.0f, 0.0f, 0.0f },     { 0.125f, -4.0f, 4.0f },
    { 0.125f, 1.0f, -1.0f },  { 0.125f, 2.0f, -2.0f },  { 0.125f, 4.0f, -4.0f },
};

void ScopeBlurEffect::PlatformReadConfigData(PblConfig::Data& data)
{
    (void)data;
}

// FUNCTION: SWBF2 0x004267b0
void ScopeBlurEffect::PlatformInit()
{
    m_pClearShader = (pcClearShader*)pcRedShader::Create(
        0x5c6e1222 /* "clear" */, 0x403, 0, 0, 4, RedColor::WHITE, RedColor::BLACK, 32.0f);
    m_pFilterShader = (pcFilterCopyShader*)pcRedShader::Create(
        0xfb9eb106 /* "FilterCopy" */, 0x202, 0, 0, 0, RedColor::WHITE, RedColor::BLACK, 32.0f);
    m_pBlurTexture = new RedTexture(256, 256, D3DFMT_A8R8G8B8, 1, D3DUSAGE_RENDERTARGET,
                                    D3DPOOL_DEFAULT, 0);
}

// FUNCTION: SWBF2 0x00426880
void ScopeBlurEffect::PlatformCleanup()
{
    if (m_pClearShader)
    {
        m_pClearShader->Release();
        m_pClearShader = 0;
    }
    if (m_pFilterShader)
    {
        m_pFilterShader->Release();
        m_pFilterShader = 0;
    }
    delete m_pBlurTexture;
}

// FUNCTION: SWBF2 0x00426910
void ScopeBlurEffect::Render(unsigned int value) const
{
    unsigned int uiViewport = FLRenderer::GetCurrentViewport();
    if ((unsigned int)value & 0x200000)
    {
        return;
    }
    if (!m_alpha[uiViewport])
    {
        return;
    }

    RedColor color = RedColor::WHITE;
    float fU;
    float left, top, bottom, right;
    float fV;
    RedViewport viewport;
    pcRedPrimitive* pPrimitive;

    if ((RedRenderer::pcGetDeviceCaps().DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES) &&
        RedRenderer::pcGetRenderOptions().distortion)
    {
        pcCopyBackBuffer();

        pPrimitive = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x202, 20, 4, 0, false);
        FilterVertex* pFilterVerts = (FilterVertex*)pPrimitive->BeginVertexData();
        if (!pFilterVerts)
        {
            return;
        }

        RedRenderer::pcGetViewport(viewport);
        RedRenderer::pcGetViewportExtents(left, top, right, bottom);

        fU = (float)m_pBlurTexture->GetWidth() /
             (float)(RedRenderer::pcGetBuffer(PC_BUFFER_TEMP))->GetWidth();
        fV = (float)m_pBlurTexture->GetHeight() /
             (float)(RedRenderer::pcGetBuffer(PC_BUFFER_TEMP))->GetHeight();

        pFilterVerts[0].pos.SetPos(-1.0f, 1.0f, 0.0f);
        pFilterVerts[0].uv0.SetUV(0.0f, 0.0f);
        pFilterVerts[1].pos.SetPos(-1.0f, 1.0f - fV * 2.0f, 0.0f);
        pFilterVerts[1].uv0.SetUV(0.0f, 1.0f);
        pFilterVerts[2].pos.SetPos(fU * 2.0f - 1.0f, 1.0f, 0.0f);
        pFilterVerts[2].uv0.SetUV(1.0f, 0.0f);
        pFilterVerts[3].pos.SetPos(fU * 2.0f - 1.0f, 1.0f - fV * 2.0f, 0.0f);
        pFilterVerts[3].uv0.SetUV(1.0f, 1.0f);
        pPrimitive->EndVertexData();

        m_pFilterShader->SetTextures(m_pBlurTexture, RedRenderer::pcGetBuffer(PC_BUFFER_TEMP));
        m_pFilterShader->SetSamples(9, BoxFilter);
        RedRenderer::pcRenderPrimitive(pPrimitive, m_pFilterShader, gMatrixIdentity,
                                       RedColor::WHITE, 0x200, 0, 0, 0);
        pPrimitive->Release();

        m_pClearShader->m_pTex = RedRenderer::pcGetBuffer(PC_BUFFER_TEMP);
        color.a = (unsigned char)m_alpha[uiViewport];
    }
    else
    {
        m_pClearShader->m_pTex = RedTexture::Find(0xDE020766 /* "white" */);
        fU = 1.0f;
        color = RedColor::BLACK;
        fV = 1.0f;
        color.a = (unsigned char)(int)((float)m_alpha[uiViewport] * 0.6f);
    }

    m_pClearShader->m_pAlphaMaskTex = RedTexture::Find(0xF745F270 /* "blurmask" */);

    pPrimitive = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x403, 32, 4, 0, false);
    BlurVertex* pVerts = (BlurVertex*)pPrimitive->BeginVertexData();
    if (!pVerts)
    {
        return;
    }

    RedRenderer::pcGetViewport(viewport);
    RedRenderer::pcGetViewportExtents(left, top, right, bottom);

    pVerts[0].pos.SetPos(left, top, 0.0f, 1.0f);
    pVerts[0].uv0.SetUV(viewport.fOffsetX * fU, viewport.fOffsetY * fV);
    pVerts[0].uv1.SetUV(0.0f, 0.0f);
    pVerts[1].pos.SetPos(left, bottom, 0.0f, 1.0f);
    pVerts[1].uv0.SetUV(viewport.fOffsetX * fU, (viewport.fHeight + viewport.fOffsetY) * fV);
    pVerts[1].uv1.SetUV(0.0f, 1.0f);
    pVerts[2].pos.SetPos(right, top, 0.0f, 1.0f);
    pVerts[2].uv0.SetUV((viewport.fWidth + viewport.fOffsetX) * fU, viewport.fOffsetY * fV);
    pVerts[2].uv1.SetUV(1.0f, 0.0f);
    pVerts[3].pos.SetPos(right, bottom, 0.0f, 1.0f);
    pVerts[3].uv0.SetUV((viewport.fWidth + viewport.fOffsetX) * fU, (viewport.fHeight + viewport.fOffsetY) * fV);
    pVerts[3].uv1.SetUV(1.0f, 1.0f);
    pPrimitive->EndVertexData();

    RedRenderer::pcRenderPrimitive(pPrimitive, m_pClearShader, gMatrixIdentity,
                                   color, 0x200, 0, 0, 0);
    pPrimitive->Release();
}

// FUNCTION: SWBF2 0x00426e10
void ScopeBlurEffect::pcCopyBackBuffer() const
{
    float left, top, right, bottom;
    RedRenderer::pcGetViewportExtents(left, top, right, bottom);

    RECT srcRect;
    srcRect.left = (LONG)left;
    srcRect.top = (LONG)top;
    srcRect.right = (LONG)right;
    srcRect.bottom = (LONG)bottom;
    RECT destRect;
    destRect.left = 0;
    destRect.top = 0;
    destRect.right = m_pBlurTexture->GetWidth();
    destRect.bottom = m_pBlurTexture->GetHeight();

    IDirect3DSurface9* pBackBuffer;
    RedRenderer::pcGetDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    IDirect3DSurface9* pSurface;
    ((IDirect3DTexture9*)m_pBlurTexture->m_pTexture)->GetSurfaceLevel(0, &pSurface);
    RedRenderer::pcGetDevice()->StretchRect(pBackBuffer, &srcRect, pSurface, &destRect,
                                            D3DTEXF_NONE);
    pBackBuffer->Release();
    pSurface->Release();
}
