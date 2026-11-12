#include "Battlefront2/Source/LeafEffect.h"
#include "PebbleFL/Common/PblRef.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"
#include "pcSunFlare.h"

struct LeafVertex
{
    PblVector3 pos;
    RedColor color;
    TexCoord uv;
};

struct LeafCompressedVertex
{
    CompressedPos pos;
    RedColor color;
    CompressedTexCoord uv;
};

// FUNCTION: SWBF2 0x0041a380
pcRedPrimitive* FallingLeafParticle::CreateGeometry(float fSize, RedColor color)
{
    pcRedPrimitive* pPrimitive = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x282, 24, 4, 0, false);
    LeafVertex* pVertices = (LeafVertex*)pPrimitive->BeginVertexData();
    if (pVertices)
    {
        pVertices[0].pos.Set(-fSize, 0.0f, -fSize);
        pVertices[0].color = color;
        pVertices[0].uv.SetUV(0.0f, 0.0f);

        pVertices[1].pos.Set(-fSize, 0.0f, fSize);
        pVertices[1].color = color;
        pVertices[1].uv.SetUV(1.0f, 0.0f);

        pVertices[2].pos.Set(fSize, 0.0f, -fSize);
        pVertices[2].color = color;
        pVertices[2].uv.SetUV(0.0f, 1.0f);

        pVertices[3].pos.Set(fSize, 0.0f, fSize);
        pVertices[3].color = color;
        pVertices[3].uv.SetUV(1.0f, 1.0f);
        pPrimitive->EndVertexData();
    }
    return pPrimitive;
}

// FUNCTION: SWBF2 0x0041a470
pcRedPrimitive* FallingLeafParticle::CreateCompressedGeometry(float fSize, RedColor color)
{
    pcRedPrimitive* pPrimitive = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x9282, sizeof(LeafCompressedVertex), 4, 0, false);
    const PblBox bbox(-0.25f, -0.25f, -0.25f, 0.25f, 0.25f, 0.25f);
    pPrimitive->m_bbox = bbox;
    CompressedPos::SetBoundingBox(bbox);

    LeafCompressedVertex* pVertices = (LeafCompressedVertex*)pPrimitive->BeginVertexData();
    if (pVertices)
    {
        pVertices[0].pos.SetPos(-fSize, 0.0f, -fSize);
        pVertices[0].color = color;
        pVertices[0].uv.SetUV(0.0f, 0.0f);

        pVertices[1].pos.SetPos(-fSize, 0.0f, fSize);
        pVertices[1].color = color;
        pVertices[1].uv.SetUV(1.0f, 0.0f);

        pVertices[2].pos.SetPos(fSize, 0.0f, -fSize);
        pVertices[2].color = color;
        pVertices[2].uv.SetUV(0.0f, 1.0f);

        pVertices[3].pos.SetPos(fSize, 0.0f, fSize);
        pVertices[3].color = color;
        pVertices[3].uv.SetUV(1.0f, 1.0f);
        pPrimitive->EndVertexData();
    }
    return pPrimitive;
}

// FUNCTION: SWBF2 0x0041a2c0
void FallingLeafParticle::Render(unsigned char, float, unsigned int)
{
    unsigned char ucGrey = (unsigned char)(int)((mRandomness * 0.6f + 0.4f) * 255.0f);
    mRenderTimer = 0.0f;

    RedColor color;
    color.b = ucGrey;
    color.g = ucGrey;
    color.r = ucGrey;
    color.a = 0xff;

    pcRedPrimitive* pPrimitive;
    if (!RedRenderer::pcGetGeometryMode())
    {
        pPrimitive = CreateGeometry(0.17f, color);
    }
    else
    {
        pPrimitive = CreateCompressedGeometry(0.17f, color);
    }

    RedRenderer::pcRenderPrimitive(pPrimitive, m_pShader, mMatrix, RedColor::WHITE, 0x200, 0, 0, 0);
    pPrimitive->Release();
}

// FUNCTION: SWBF2 0x0041a280
void FallingLeafParticle::PlatformCleanup()
{
    if (m_pShader)
    {
        m_pShader->Release();
    }
}

// FUNCTION: SWBF2 0x0041a230
void FallingLeafParticle::PlatformInit()
{
    const unsigned int uiFormat = RedRenderer::pcGetGeometryMode() ? 0x9282 : 0x282;
    m_pShader = pcRedShader::Create(0x8a8743bf /* "PARTICLE" */, uiFormat, &mTextureHash, 1, 0x10004,
                                    RedColor::WHITE, RedColor::BLACK, 32.0f);
}
