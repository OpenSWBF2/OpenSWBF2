#include "Battlefront2/Source/MotionBlurEffect.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcClearShader.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedTexture.h"
#include "RedEngineFL/Application/pc/pcFLRender.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"

extern PblMatrix gMatrixIdentity;

struct MotionBlurVertex
{
    PosXYZW pos;
    TexCoord uv0;
    TexCoord uv1;
};

// GLOBAL: SWBF2 0x01f9ae54
static pcClearShader* s_pClearShader;

// FUNCTION: SWBF2 0x00423bd0
void MotionBlurEffect::PlatformInit()
{
    s_pClearShader = (pcClearShader*)pcRedShader::Create(
        0x5c6e1222, 0x403, 0, 0, 4, RedColor::WHITE, RedColor::BLACK, 32.0f);
}

// FUNCTION: SWBF2 0x00423c00
void MotionBlurEffect::PlatformCleanup()
{
    if (s_pClearShader)
    {
        s_pClearShader->Release();
        s_pClearShader = 0;
    }
}

// FUNCTION: SWBF2 0x00423c40
void MotionBlurEffect::PlatformRender(unsigned int) const
{
    const unsigned int uiView = FLRenderer::GetCurrentViewport();
    if (m_curBlurAmount[uiView] == 0.0f)
    {
        return;
    }
    if (!(RedRenderer::pcGetDeviceCaps().DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES))
    {
        return;
    }
    if (!RedRenderer::pcGetRenderOptions().motionBlur)
    {
        return;
    }

    pcRedPrimitive* pPrimitive =
        pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x403, 32, 4, 0, false);
    MotionBlurVertex* pVerts = (MotionBlurVertex*)pPrimitive->BeginVertexData();
    if (!pVerts)
    {
        return;
    }

    RedViewport viewport;
    RedRenderer::pcGetViewport(viewport);
    float fMinX, fMinY, fMaxY, fMaxX;
    RedRenderer::pcGetViewportExtents(fMinX, fMinY, fMaxX, fMaxY);

    pVerts[0].pos.SetPos(fMinX, fMinY, 0.0f, 1.0f);
    pVerts[0].uv0.SetUV(viewport.fOffsetX, viewport.fOffsetY);
    pVerts[0].uv1.SetUV(0.0f, 0.0f);
    pVerts[1].pos.SetPos(fMinX, fMaxY, 0.0f, 1.0f);
    pVerts[1].uv0.SetUV(viewport.fOffsetX, viewport.fHeight + viewport.fOffsetY);
    pVerts[1].uv1.SetUV(0.0f, 1.0f);
    pVerts[2].pos.SetPos(fMaxX, fMinY, 0.0f, 1.0f);
    pVerts[2].uv0.SetUV(viewport.fWidth + viewport.fOffsetX, viewport.fOffsetY);
    pVerts[2].uv1.SetUV(1.0f, 0.0f);
    pVerts[3].pos.SetPos(fMaxX, fMaxY, 0.0f, 1.0f);
    pVerts[3].uv0.SetUV(viewport.fWidth + viewport.fOffsetX, viewport.fHeight + viewport.fOffsetY);
    pVerts[3].uv1.SetUV(1.0f, 1.0f);
    pPrimitive->EndVertexData();

    s_pClearShader->m_pTex = FLRenderer::pcGetPreviousFrameTexture();
    s_pClearShader->m_pAlphaMaskTex = RedTexture::Find(0xF745F270);

    RedColor color;
    color.b = RedColor::WHITE.b;
    color.g = RedColor::WHITE.g;
    color.r = RedColor::WHITE.r;
    color.a = (unsigned char)(m_curBlurAmount[uiView] * 255.0f);
    RedRenderer::pcRenderPrimitive(pPrimitive, s_pClearShader, gMatrixIdentity,
                                   color, 0x200, 0, 0, 0);
    pPrimitive->Release();
}
