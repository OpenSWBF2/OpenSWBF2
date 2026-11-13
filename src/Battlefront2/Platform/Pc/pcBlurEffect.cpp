#include "Battlefront2/Source/BlurEffect.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedTexture.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcClearShader.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcFilterCopyShader.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"

namespace
{
struct BlurVertex
{
    PosXYZW pos;
    TexCoord uv0;
};

struct FilterVertex
{
    PosXYZ pos;
    TexCoord uv0;
};

}

// GLOBAL: SWBF2 0x007b2930
// __xmm@41f00000000000000000000000000000

// GLOBAL: SWBF2 0x007e5820
FilterCopyShaderSample BoxFilter[8] = {
    { 0.125f, -0.5f, -0.5f }, { 0.125f, -0.5f, 0.5f },
    { 0.125f, 0.5f, 0.5f },   { 0.125f, 0.5f, -0.5f },
    { 0.125f, -1.5f, 0.0f },  { 0.125f, 0.0f, -1.5f },
    { 0.125f, 1.5f, 0.0f },   { 0.125f, 0.0f, 1.5f },
};

// FUNCTION: SWBF2 0x0040f7a0
void BlurEffect::PlatformInit()
{
    m_constantBlend = 0.25f;
    m_downSizeFactor = 0.25f;
    m_pFilterShader = (pcFilterCopyShader*)pcRedShader::Create(
        0xfb9eb106 /* "FilterCopy" */, 0x202, 0, 0, 0, RedColor::WHITE, RedColor::BLACK, 32.0f);
    m_pClearShader = (pcClearShader*)pcRedShader::Create(
        0x5c6e1222 /* "clear" */, 0x203, 0, 0, 4, RedColor::WHITE, RedColor::BLACK, 32.0f);
}

// FUNCTION: SWBF2 0x0040f810
void BlurEffect::PlatformCleanup()
{
    if (m_pFilterShader)
    {
        m_pFilterShader->Release();
        m_pFilterShader = 0;
    }
    if (m_pClearShader)
    {
        m_pClearShader->Release();
        m_pClearShader = 0;
    }
}

// FUNCTION: SWBF2 0x0040f890
void BlurEffect::PlatformReadConfigData(PblConfig::Data& data)
{
    const unsigned int uiId = data.GetId();
    if (uiId > 0x7E6C7187u /* "DownSizeFactor" */)
    {
        return;
    }
    if (uiId != 0x7E6C7187u /* "DownSizeFactor" */)
    {
        if (uiId == 0x055CEA38u /* "MinMaxDepth" */)
        {
            (void)data.GetFloatArg(0);
        }
        else if (uiId == 0x7E1194B8u /* "ConstantBlend" */)
        {
            m_constantBlend = data.GetFloatArg(0);
        }
    }
    else
    {
        m_downSizeFactor = data.GetFloatArg(0);
    }
}

// FUNCTION: SWBF2 0x0040f8d0
void BlurEffect::Render(unsigned int value) const
{
    if ((unsigned int)value & 0x200000)
    {
        return;
    }
    if (RedRenderer::pcGetRenderCaps().pipeline == PIPELINE_FIXED)
    {
        return;
    }

    RedColor color = RedColor::WHITE;
    pcCopyBackBuffer();

    pcRedPrimitive* pPrimitive =
        pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x202, 20, 4, 0, false);
    FilterVertex* pFilterVerts = (FilterVertex*)pPrimitive->BeginVertexData();
    if (!pFilterVerts)
    {
        return;
    }

    RedViewport viewport;
    RedRenderer::pcGetViewport(viewport);
    float left, top, bottom, right;
    RedRenderer::pcGetViewportExtents(left, top, right, bottom);

    const float fSkyWidth = (float)(RedRenderer::pcGetBuffer(PC_BUFFER_SKY))->m_uiWidth;
    float fU1 = (right - left) * m_downSizeFactor / fSkyWidth;
    const float fSkyHeight = (float)(RedRenderer::pcGetBuffer(PC_BUFFER_SKY))->m_uiHeight;
    float fV1 = (bottom - top) * m_downSizeFactor / fSkyHeight;
    const float fTempWidth = (float)(RedRenderer::pcGetBuffer(PC_BUFFER_TEMP))->m_uiWidth;
    float fU0 = (right - left) * m_downSizeFactor / fTempWidth;
    const float fTempHeight = (float)(RedRenderer::pcGetBuffer(PC_BUFFER_TEMP))->m_uiHeight;
    float fV0 = (bottom - top) * m_downSizeFactor / fTempHeight;

    pFilterVerts[0].pos.SetPos(-1.0f, 1.0f, 0.0f);
    pFilterVerts[0].uv0.SetUV(0.0f, 0.0f);
    pFilterVerts[1].pos.SetPos(-1.0f, 1.0f - fV1 * 2.0f, 0.0f);
    pFilterVerts[1].uv0.SetUV(0.0f, fV0);
    pFilterVerts[2].pos.SetPos(fU1 * 2.0f - 1.0f, 1.0f, 0.0f);
    pFilterVerts[2].uv0.SetUV(fU0, 0.0f);
    pFilterVerts[3].pos.SetPos(fU1 * 2.0f - 1.0f, 1.0f - fV1 * 2.0f, 0.0f);
    pFilterVerts[3].uv0.SetUV(fU0, fV0);
    pPrimitive->EndVertexData();

    m_pFilterShader->SetTextures(RedRenderer::pcGetBuffer(PC_BUFFER_TEMP),
                                 RedRenderer::pcGetBuffer(PC_BUFFER_SKY));
    m_pFilterShader->SetSamples(8, BoxFilter);
    RedRenderer::pcRenderPrimitive(pPrimitive, m_pFilterShader, gMatrixIdentity,
                                   RedColor::WHITE, 0x200, 0, 0, 0);
    pPrimitive->Release();

    m_pClearShader->m_pTex = RedRenderer::pcGetBuffer(PC_BUFFER_SKY);
    color.a = (unsigned char)(int)(m_constantBlend * 255.0f);
    m_pClearShader->SetBlendMode(pcClearShader::BLENDMODE_CONSTANT_BLEND);

    pPrimitive = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x203, 24, 4, 0, false);
    BlurVertex* pBlurVerts = (BlurVertex*)pPrimitive->BeginVertexData();
    if (!pBlurVerts)
    {
        return;
    }

    RedRenderer::pcGetViewport(viewport);
    RedRenderer::pcGetViewportExtents(left, top, right, bottom);

    pBlurVerts[0].pos.SetPos(left, top, 0.0f, 1.0f);
    pBlurVerts[0].uv0.SetUV(0.0f, 0.0f);
    pBlurVerts[1].pos.SetPos(left, bottom, 0.0f, 1.0f);
    pBlurVerts[1].uv0.SetUV(0.0f, fV1);
    pBlurVerts[2].pos.SetPos(right, top, 0.0f, 1.0f);
    pBlurVerts[2].uv0.SetUV(fU1, 0.0f);
    pBlurVerts[3].pos.SetPos(right, bottom, 0.0f, 1.0f);
    pBlurVerts[3].uv0.SetUV(fU1, fV1);
    pPrimitive->EndVertexData();

    RedRenderer::pcRenderPrimitive(pPrimitive, m_pClearShader, gMatrixIdentity,
                                   color, 0x200, 0, 0, 0);
    pPrimitive->Release();
}

// FUNCTION: SWBF2 0x0040fd10
void BlurEffect::pcCopyBackBuffer() const
{
    float fMinX, fMinY, fMaxX, fMaxY;
    RedRenderer::pcGetViewportExtents(fMinX, fMinY, fMaxX, fMaxY);

    RECT dest;
    dest.left = 0;
    dest.top = 0;
    RECT src;
    src.left = (int)fMinX;
    src.top = (int)fMinY;
    src.right = (int)fMaxX;
    src.bottom = (int)fMaxY;
    dest.right = (int)((fMaxX - fMinX) * m_downSizeFactor);
    dest.bottom = (int)((fMaxY - fMinY) * m_downSizeFactor);

    RedTexture* pBuffer = RedRenderer::pcGetBuffer(PC_BUFFER_TEMP);
    IDirect3DSurface9* pBackBuffer;
    RedRenderer::pcGetDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    IDirect3DSurface9* pDest;
    ((IDirect3DTexture9*)pBuffer->m_pTexture)->GetSurfaceLevel(0, &pDest);
    RedRenderer::pcGetDevice()->StretchRect(pBackBuffer, &src, pDest, &dest, D3DTEXF_NONE);
    pBackBuffer->Release();
    pDest->Release();
}
