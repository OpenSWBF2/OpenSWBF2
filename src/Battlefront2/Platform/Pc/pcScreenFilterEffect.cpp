#include "Battlefront2/Source/ScreenFilterEffect.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcClearShader.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedTexture.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"

namespace FLRenderer
{

unsigned int GetCurrentViewport();
}

struct ScreenVertex
{
    PosXYZW pos;
    TexCoord uv;
};

// GLOBAL: SWBF2 0x01f9ae58
static pcClearShader* s_pClearShader;

// FUNCTION: SWBF2 0x00426ed0
void ScreenFilterEffect::PlatformInit()
{
    s_pClearShader = (pcClearShader*)pcRedShader::Create(
        0x5c6e1222, 0x203, 0, 0, 4, RedColor::WHITE, RedColor::BLACK, 32.0f);
}

// FUNCTION: SWBF2 0x00426f00
void ScreenFilterEffect::PlatformCleanup()
{
    if (s_pClearShader)
    {
        s_pClearShader->Release();
        s_pClearShader = 0;
    }
}

// FUNCTION: SWBF2 0x00426f40
void ScreenFilterEffect::PlatformRender(unsigned int) const
{
    unsigned int uiViewport = FLRenderer::GetCurrentViewport();
    pcRedPrimitive* pPrimitive =
        pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x203, 24, 4, 0, false);
    ScreenVertex* pVerts = (ScreenVertex*)pPrimitive->BeginVertexData();
    if (!pVerts)
    {
        return;
    }

    RedViewport viewport;
    RedRenderer::pcGetViewport(viewport);
    float left, top, right, bottom;
    RedRenderer::pcGetViewportExtents(left, top, right, bottom);

    pVerts[0].pos.SetPos(left, top, 0.0f, 1.0f);
    pVerts[0].uv.SetUV(0.0f, 0.0f);
    pVerts[1].pos.SetPos(left, bottom, 0.0f, 1.0f);
    pVerts[1].uv.SetUV(0.0f, 1.0f);
    pVerts[2].pos.SetPos(right, top, 0.0f, 1.0f);
    pVerts[2].uv.SetUV(1.0f, 0.0f);
    pVerts[3].pos.SetPos(right, bottom, 0.0f, 1.0f);
    pVerts[3].uv.SetUV(1.0f, 1.0f);
    pPrimitive->EndVertexData();

    const FilterData& data = m_data[uiViewport];
    s_pClearShader->m_pTex = RedTexture::Find(data.uiTextureHash);
    RedRenderer::pcRenderPrimitive(pPrimitive, s_pClearShader, gMatrixIdentity,
                                   RedColor(data.filterColor, (unsigned char)(int)(data.filterColor.a * data.fIntensity)),
                                   0x200, 0, 0, 0);

    pPrimitive->Release();
}
