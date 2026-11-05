#include <d3dx9math.h>
#include "pcLeafPatch.h"
#include "pcDynamicSprite.h"

#include "RedEngineFL/Graphics/RedCamera.h"
#include "RedEngineFL/Graphics/RedLight.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/PC/pcRedIndexBuffer.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedVertexBuffer.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"
#include <new>
#include "Battlefront2/Source/BirdEffect.h"

// FUNCTION: SWBF2 0x0041a740
LeafPatchRenderable::LeafPatchRenderable(const LeafPatchClass* pClass)
    : Renderable(2), m_fDistanceFarShader(pClass->mNearFadeDist), m_pClass(pClass)
{
    m_pPrimitive = 0;
    m_pPrimitiveFar = 0;
    m_pVertices = 0;
    if (RedRenderer::pcGetGeometryMode() == GEOMETRYMODE_UNCOMPRESSED)
    {
        m_pShader = pcRedShader::Create(0xe68b9c52, 0x2a2, &m_pClass->mTextureHash,
                                        1, 0x10002, RedColor::WHITE, RedColor::BLACK, 32.0f);
        m_pShaderFar = pcRedShader::Create(0xe68b9c52, 0x2a2, &m_pClass->mTextureHash,
                                           1, 0x10002, RedColor::WHITE, RedColor::BLACK, 32.0f);
    }
    else
    {
        m_pShader = pcRedShader::Create(0xe68b9c52, 0xd2a2, &m_pClass->mTextureHash,
                                        1, 0x10002, RedColor::WHITE, RedColor::BLACK, 32.0f);
        m_pShaderFar = pcRedShader::Create(0xe68b9c52, 0xd2a2, &m_pClass->mTextureHash,
                                           1, 0x10002, RedColor::WHITE, RedColor::BLACK, 32.0f);
    }
    m_bReflection = false;
    const int count = m_pClass->mNumParticles;
    m_pIBReverse = pcRedIndexBuffer::Create(6 * count, true, true);
    unsigned short* indices = m_pIBReverse->Lock(6 * count, m_startIndexReverse);
    for (short iReverse = (short)(count - 1); iReverse >= 0; --iReverse)
    {
        *indices++ = iReverse * 4;
        *indices++ = iReverse * 4 + 1;
        *indices++ = iReverse * 4 + 2;
        *indices++ = iReverse * 4;
        *indices++ = iReverse * 4 + 2;
        *indices++ = iReverse * 4 + 3;
    }
    m_pIBReverse->Unlock();
    m_pIBForward = pcRedIndexBuffer::Create(6 * count, true, true);
    indices = m_pIBForward->Lock(6 * count, m_startIndexForward);
    for (short i = 0; i < count; ++i)
    {
        const unsigned short v = (unsigned short)(i * 4 + 2);
        *indices++ = v - 2;
        *indices++ = v - 1;
        *indices++ = v;
        *indices++ = v - 2;
        *indices++ = v;
        *indices++ = v + 1;
    }
    m_pIBForward->Unlock();
    m_pVB = 0;
    mHeap = new PblHeap<int, float>(pClass->mNumParticles, 1000000.0f, 0);
    m_iTexCoordType = 0;
    if (pClass->mNumParts == 4)
    {
        m_iTexCoordType = 1;
    }
    Activate(0);
}

// FUNCTION: SWBF2 0x0041a9f0
LeafPatchRenderable::~LeafPatchRenderable()
{
    Deactivate();
    if (m_pPrimitive)
    {
        m_pPrimitive->Release();
    }
    if (m_pPrimitiveFar)
    {
        m_pPrimitiveFar->Release();
    }
    if (m_pIBForward)
    {
        m_pIBForward->Release();
    }
    if (m_pIBReverse)
    {
        m_pIBReverse->Release();
    }
    if (m_pVB)
    {
        m_pVB->Release();
    }
    if (m_pShader)
    {
        m_pShader->Release();
    }
    if (m_pShaderFar)
    {
        m_pShaderFar->Release();
    }
    delete mHeap;
}

// FUNCTION: SWBF2 0x0041ba90
void LeafPatchClass::Render(const PblMatrix& matrix, float fAlpha, int, unsigned int uiFlags,
                            const RedColorValue& color) const
{
    const RedColorValue faded(color, fAlpha);
    mRenderData->RenderInstance(matrix, faded, uiFlags);
}

// FUNCTION: SWBF2 0x0041bb00
void LeafPatchClass::CreateRenderData()
{
    pcDynamicSprite::Init();
    mRenderData = new LeafPatchRenderable(this);
}

// FUNCTION: SWBF2 0x0041b930
bool LeafPatchClass::RenderMaxDist(const PblMatrix& matrix, float& fDist,
                                   unsigned int uiFlags, float fMaxDist,
                                   const RedColorValue& color) const
{
    const PblMatrix& cam = RedRenderer::GetCamera()->_Matrix;
    const RedCamera* pCamera = RedRenderer::GetCamera();
    const PblVector3& vPos = matrix.GetTranslation();
    const PblVector3 vDelta = cam.GetTranslation() - vPos;
    const float distance = DotProduct(cam.GetZAxis(), vDelta);
    if (distance < -mBoundingRadius ||
        distance > pCamera->_fZoom * fMaxDist + mBoundingRadius)
    {
        return false;
    }
    if (mMaxScatterBirds && !(uiFlags & 0x200000))
    {
        BirdParticleManager::SetPotentialScatterPoint(&vPos, mMaxScatterBirds);
    }
    PblMatrix scaled = matrix;
    scaled.SetYAxis(scaled.GetYAxis() * mHeightScale);
    Render(scaled, 1.0f, 0, uiFlags, color);
    return true;
}


// GLOBAL: SWBF2 0x007eb710
// sBox
static PblBox sBox(-50.0f, -50.0f, -50.0f, 50.0f, 50.0f, 50.0f);

namespace
{

struct LeafPatchCompressedVertex
{
    CompressedPos pos;
    CompressedNormal normal;
    RedColor color;
    CompressedTexCoord uv;
};
struct LeafPatchVertex
{
    PblVector3 pos;
    PblVector3 normal;
    RedColor color;
    TexCoord uv;
};
}

// FUNCTION: SWBF2 0x0041abb0
void LeafPatchRenderable::Render(int value)
{
    m_bReflection = ((unsigned int)value & 0x200000) != 0;
    Update();
}

// FUNCTION: SWBF2 0x0041abd0
void LeafPatchRenderable::RenderInstance(const PblMatrix& matrix, const RedColor& color,
                                         unsigned int uiFlags) const
{
    if (m_bReflection != ((uiFlags & 0x200000) != 0))
    {
        return;
    }

    PblSphere sphere(matrix.GetTranslation(), m_pClass->mBoundingRadius);
    RedRenderer::SetLightingState(*RedLight::GetLightingState(sphere, 4, false, false, false));

    RedRenderer::GetCamera();
    RedRenderer::GetCamera();

    if (color.a < 0xff)
    {
        if (m_pPrimitiveFar)
        {
            RedRenderer::pcRenderPrimitive(m_pPrimitiveFar,
                                           m_pShaderFar, matrix,
                                           color, uiFlags, 0, 0, 0);
        }
    }
    else
    {
        if (m_pPrimitive)
        {
            RedRenderer::pcRenderPrimitive(m_pPrimitive, m_pShader,
                                           matrix, color,
                                           uiFlags, 0, 0, 0);
        }
    }
}

// FUNCTION: SWBF2 0x0041aca0
void LeafPatchRenderable::Update()
{
    const RedCamera* pCamera = RedRenderer::GetCamera();
    const PblVector3& vForward = pCamera->_Matrix[2];

    for (int i = 0; i < m_pClass->mNumParticles; ++i)
    {
        float fKey = -DotProduct(vForward, m_pClass->mParticle[i].mPos);
        mHeap->Push(i, fKey);
    }

    Begin();

    while (!mHeap->IsEmpty())
    {
        int iIndex = mHeap->Get(0);
        const LeafPatchClass::LeafParticle& particle = m_pClass->mParticle[iIndex];

        RedColor color;
        color.b = (unsigned char)(int)(particle.mColor.b * 255.0f);
        color.g = (unsigned char)(int)(particle.mColor.g * 255.0f);
        color.r = (unsigned char)(int)(particle.mColor.r * 255.0f);
        color.a = (unsigned char)(int)(particle.mColor.a * 255.0f);

        AddParticle(particle.mPos, particle.mSize, color, particle.mRotation, iIndex, particle.mTexture);
        mHeap->Pop();
    }

    m_pVertices = 0;
    m_pVB->Unlock();
    m_pVB->Release();
    m_pVB = 0;
}

// FUNCTION: SWBF2 0x0041ae90
void LeafPatchRenderable::Begin()
{
    unsigned int uiNumParticles = (unsigned int)m_pClass->mNumParticles;
    unsigned int uiPrimCount = 2 * uiNumParticles;
    unsigned int uiNumVertices = 4 * uiNumParticles;

    unsigned int uiVertexFormat;
    unsigned int uiVertexStride;
    float fScale;
    if (RedRenderer::pcGetGeometryMode() == GEOMETRYMODE_UNCOMPRESSED)
    {
        uiVertexFormat = 0x2A2;
        uiVertexStride = 36;
        fScale = 1.0f;
    }
    else
    {
        uiVertexFormat = 0xD2A2;
        uiVertexStride = 20;
        fScale = 655.34998f;
    }

    unsigned int uiStartVertex = 0;
    m_pVB = pcRedVertexBuffer::Create(uiNumVertices, uiVertexStride, false, true);
    m_pVertices = m_pVB->Lock(uiNumVertices, uiStartVertex);

    if (m_pPrimitive)
    {
        m_pPrimitive->Release();
        m_pPrimitive = 0;
    }
    if (m_pPrimitiveFar)
    {
        m_pPrimitiveFar->Release();
        m_pPrimitiveFar = 0;
    }

    m_pPrimitive = pcRedPrimitive::Create(D3DPT_TRIANGLELIST, uiPrimCount, uiVertexFormat, uiNumVertices,
                                          uiStartVertex, m_pVB, m_startIndexForward, m_pIBForward);
    m_pPrimitive->m_bbox = sBox;
    m_pPrimitiveFar = pcRedPrimitive::Create(D3DPT_TRIANGLELIST, uiPrimCount, uiVertexFormat, uiNumVertices,
                                             uiStartVertex, m_pVB, m_startIndexReverse, m_pIBReverse);
    m_pPrimitiveFar->m_bbox = sBox;

    const RedCamera* pCamera = RedRenderer::GetCamera();
    const PblMatrix& mat = pCamera->_Matrix;
    m_vUp = PblVector3(mat.m[1][0] * fScale, mat.m[1][1] * fScale, mat.m[1][2] * fScale);
    m_vFront = PblVector3(mat.m[2][0] * fScale, mat.m[2][1] * fScale, mat.m[2][2] * fScale);

    for (int i = 0; i < 10; ++i)
    {

        float f = (float)(i % 5) * (1.0f - (float)(i & 1) * 2.0f) * 0.3f;
        PblVector3 vOffset(mat.m[2][0] * f, mat.m[2][1] * f, mat.m[2][2] * f);
        PblVector3 vSum(mat.m[0][0] + vOffset.x, mat.m[0][1] + vOffset.y, mat.m[0][2] + vOffset.z);
        PblVector3 vDir;
        D3DXVec3Normalize(reinterpret_cast<D3DXVECTOR3*>(&vDir), reinterpret_cast<const D3DXVECTOR3*>(&vSum));
        m_avRight[i] = PblVector3(vDir.x * fScale, vDir.y * fScale, vDir.z * fScale);
    }
}

// FUNCTION: SWBF2 0x0041b140
void LeafPatchRenderable::AddParticle(const PblVector3& vPos, float fSize, const RedColor& color,
                                      float fRotation, int iParticleIndex, unsigned char ucTexture)
{
    const GeometryModeT geometryMode = RedRenderer::pcGetGeometryMode();
    const float BOX_SCALE = geometryMode == GEOMETRYMODE_UNCOMPRESSED ? 1.0f : 655.34998f;

    const PblVector3 right = m_avRight[iParticleIndex % 10] * fSize;
    const PblVector3 midPos = vPos * BOX_SCALE + m_vFront * 0.1f;
    const PblVector3 up = m_vUp * fSize * 0.5f;
    const PblVector3 pos = up + midPos + right * (fRotation - 0.5f);
    const PblVector3 pos2 = midPos - up - right * 0.5f;

    if (geometryMode == GEOMETRYMODE_UNCOMPRESSED)
    {
        LeafPatchVertex*& pVertices = reinterpret_cast<LeafPatchVertex*&>(m_pVertices);
        const TexCoord* pTexCoord = s_TexCoords[m_iTexCoordType][ucTexture];
        pVertices->pos = pos;
        pVertices->normal = Normalize(pos);
        pVertices->color = color;
        pVertices->uv = pTexCoord[2];
        ++pVertices;
        pVertices->pos = pos + right;
        pVertices->normal = Normalize(pos + right);
        pVertices->color = color;
        pVertices->uv = pTexCoord[3];
        ++pVertices;
        pVertices->pos = pos2 + right;
        pVertices->normal = Normalize(pos2 + right);
        pVertices->color = color;
        pVertices->uv = pTexCoord[1];
        ++pVertices;
        pVertices->pos = pos2;
        pVertices->normal = Normalize(pos2);
        pVertices->color = color;
        pVertices->uv = pTexCoord[0];
        ++pVertices;
    }
    else
    {
        LeafPatchCompressedVertex*& pVertices =
            reinterpret_cast<LeafPatchCompressedVertex*&>(m_pVertices);
        const CompressedTexCoord* pTexCoord = s_CompressedTexCoords[m_iTexCoordType][ucTexture];
        pVertices->pos.SetPosHex(pos.x, pos.y, pos.z);
        pVertices->normal.SetNormal(Normalize((pos) / BOX_SCALE));
        pVertices->color = color;
        pVertices->uv = pTexCoord[2];
        ++pVertices;
        pVertices->pos.SetPosHex(pos.x + right.x, pos.y + right.y, pos.z + right.z);
        pVertices->normal.SetNormal(Normalize((pos + right) / BOX_SCALE));
        pVertices->color = color;
        pVertices->uv = pTexCoord[3];
        ++pVertices;
        pVertices->pos.SetPosHex(pos2.x + right.x, pos2.y + right.y, pos2.z + right.z);
        pVertices->normal.SetNormal(Normalize((pos2 + right) / BOX_SCALE));
        pVertices->color = color;
        pVertices->uv = pTexCoord[1];
        ++pVertices;
        pVertices->pos.SetPosHex(pos2.x, pos2.y, pos2.z);
        pVertices->normal.SetNormal(Normalize((pos2) / BOX_SCALE));
        pVertices->color = color;
        pVertices->uv = pTexCoord[0];
        ++pVertices;
    }
}

// GLOBAL: SWBF2 0x007884f0
const LeafPatchRenderable::TexCoord LeafPatchRenderable::s_TexCoords[2][4][4] = {
    {
        { { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f } },
        { { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f } },
    },
    {
        { { 0.0f, 1.0f }, { 0.5f, 1.0f }, { 0.0f, 0.5f }, { 0.5f, 0.5f } },
        { { 0.5f, 1.0f }, { 1.0f, 1.0f }, { 0.5f, 0.5f }, { 1.0f, 0.5f } },
        { { 0.0f, 0.5f }, { 0.5f, 0.5f }, { 0.0f, 0.0f }, { 0.5f, 0.0f } },
        { { 0.5f, 0.5f }, { 1.0f, 0.5f }, { 0.5f, 0.0f }, { 1.0f, 0.0f } },
    },
};

// GLOBAL: SWBF2 0x007885f8
const LeafPatchRenderable::CompressedTexCoord LeafPatchRenderable::s_CompressedTexCoords[2][4][4] = {
    {
        { { 0, 2047 }, { 2047, 2047 }, { 0, 0 }, { 2047, 0 } },
        { { 2047, 2047 }, { 0, 2047 }, { 2047, 0 }, { 0, 0 } },
        { { 0, 2047 }, { 2047, 2047 }, { 0, 0 }, { 2047, 0 } },
        { { 2047, 2047 }, { 0, 2047 }, { 2047, 0 }, { 0, 0 } },
    },
    {
        { { 0, 2047 }, { 1023, 2047 }, { 0, 1024 }, { 1023, 1024 } },
        { { 1024, 2047 }, { 2047, 2047 }, { 1024, 1024 }, { 2047, 1024 } },
        { { 0, 1023 }, { 1023, 1023 }, { 0, 0 }, { 1023, 0 } },
        { { 1024, 1023 }, { 2047, 1023 }, { 1024, 0 }, { 2047, 0 } },
    },
};

// FUNCTION: SWBF2 0x0041bb80
void LeafPatchClass::DestroyRenderData()
{
    if (mRenderData)
    {
        delete mRenderData;
        mRenderData = 0;
    }
}
