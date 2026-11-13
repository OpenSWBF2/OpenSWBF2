#include <d3dx9math.h>
#include "pcRedParticle.h"

#include <float.h>

#include "PebbleFL/Common/PblMath.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"
#include "RedEngineFL/Application/pc/pcRenderStateHelpers.h"
#include "RedEngineFL/Graphics/RedCamera.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcParticleShader.h"








struct BlurParticleVertexT
{
    PosXYZ pos;
    NormalXYZ normal;
    RedColor color;
    TexCoord uv;
};

struct ParticleVertexT
{
    float x;
    float y;
    float z;
    RedColor color;
    float tu;
    float tv;
};

 
 

// GLOBAL: SWBF2 0x009661b4
RedParticleRenderer* RedParticleRenderer::currentCache;
// GLOBAL: SWBF2 0x009661b8
int RedParticleRenderer::s_cacheIndex;
// GLOBAL: SWBF2 0x009661bc
bool RedParticleRenderer::s_blurEnable;

// SYNTHETIC: SWBF2 0x00408180
// `dynamic initializer for 'kuiParticleShader''

// GLOBAL: SWBF2 0x009661dc
// kuiParticleShader
unsigned int kuiParticleShader = PblHash("particle");

// SYNTHETIC: SWBF2 0x006d0bf0
// RedParticleRenderer::RedParticleRenderer

// SYNTHETIC: SWBF2 0x00408110
// `dynamic initializer for 'RedParticleRenderer::s_caches''

// GLOBAL: SWBF2 0x009661e0
// RedParticleRenderer::s_caches
RedParticleRenderer RedParticleRenderer::s_caches[15];

// SYNTHETIC: SWBF2 0x004080f0
// `dynamic initializer for 'RedParticleRenderer::_Instance''
// SYNTHETIC: SWBF2 0x00769970
// `dynamic atexit destructor for 'RedParticleRenderer::_Instance''

// GLOBAL: SWBF2 0x00998210
// RedParticleRenderer::_Instance
RedParticleRenderer::Renderable RedParticleRenderer::_Instance;
// SYNTHETIC: SWBF2 0x004080d0
// `dynamic initializer for 'RedParticleRenderer::_FarInstance''
// SYNTHETIC: SWBF2 0x00769960
// `dynamic atexit destructor for 'RedParticleRenderer::_FarInstance''

// GLOBAL: SWBF2 0x009661c0
// RedParticleRenderer::_FarInstance
RedParticleRenderer::Renderable RedParticleRenderer::_FarInstance;

// FUNCTION: SWBF2 0x006d0c00
RedParticleRenderer::Renderable::Renderable()
{
}

// FUNCTION: SWBF2 0x006d0c20
RedParticleRenderer::Renderable::~Renderable()
{
}

// SYNTHETIC: SWBF2 0x006d0c30
// RedParticleRenderer::Renderable::`scalar deleting destructor'





static inline float Dot(const PblVector3& a, const PblVector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline PblVector3* Cross(PblVector3* pOut, const PblVector3& a, const PblVector3& b)
{
    PblVector3 v(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    *pOut = v;
    return pOut;
}

static inline bool IsZero(const PblVector3& v)
{
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}

static inline void SetXYZ(ParticleVertexT& out, const PblVector3& v)
{
    out.x = v.x;
    out.y = v.y;
    out.z = v.z;
}

static inline void SetUV(ParticleVertexT& out, RedColor color, float u, float v)
{
    out.color = color;
    out.tu = u;
    out.tv = v;
}

static float GetCos(float fAngle);
static float GetSin(float fAngle);
static int numIndicesAdded(ParticleInfoType eType);
static int numVertsAdded(ParticleInfoType eType);

static const int MAX_PARTICLES = 200;

// FUNCTION: SWBF2 0x006d0c60
bool RedParticleRenderer::AddParticle(ParticleInfoType eType, const PblVector3& vPos, const RedColor& color,
                                      float fSize, float fRotation, TexCoords texCoords,
                                      const PblVector3& vRight, const PblVector3& vUp)
{
    if (m_particleIndex >= MAX_PARTICLES)
    {
        return false;
    }

    RedParticleInfoType& info = m_particleCache[m_particleIndex++];
    info.mPit = eType;
    info.mPos = vPos;
    info.mColor = color;
    info.mSize = fSize;
    info.mRotation = fRotation;
    info.u[0] = texCoords.u0 * (1.0f / 256.0f);
    info.v[0] = texCoords.v0 * (1.0f / 256.0f);
    info.u[1] = texCoords.u1 * (1.0f / 256.0f);
    info.v[1] = texCoords.v1 * (1.0f / 256.0f);
    info.mRight = vRight;
    info.mUp = vUp;
    return true;
}

// FUNCTION: SWBF2 0x006d0da0
void RedParticleRenderer::AddParticleToBuffers(void* pVertices, unsigned short* pIndices)
{
    ParticleVertexT* pVerts = (ParticleVertexT*)pVertices;
    BlurParticleVertexT* pBlurVerts = (BlurParticleVertexT*)pVertices;
    const PblMatrix& mCamera = RedRenderer::GetCamera()->GetMatrix();

    m_blurBounds.SetEmpty();

    bool bStreakStart = false;
    unsigned int iBaseVertex = 0;

    for (int i = 0; i < m_particleIndex; ++i)
    {
        const RedParticleInfoType& info = m_particleCache[i];
        const unsigned short b = (unsigned short)iBaseVertex;
        int nVerts = 0;
        int nIndices = 0;

        switch (info.mPit)
        {
        case PIT_PARTICLE:
        case PIT_ANIMATED:
            if (m_blendMode == BLEND_BLUR)
            {
                pBlurVerts[0].color = info.mColor;
                pBlurVerts[1].color = info.mColor;
                pBlurVerts[2].color = info.mColor;
                pBlurVerts[3].color = info.mColor;

                pBlurVerts[0].uv.SetUV(info.u[1], info.v[0]);
                pBlurVerts[1].uv.SetUV(info.u[0], info.v[0]);
                pBlurVerts[2].uv.SetUV(info.u[1], info.v[1]);
                pBlurVerts[3].uv.SetUV(info.u[0], info.v[1]);

                float fRotation = info.mRotation;
                const float fHalfSize = info.mSize * 0.5f;
                PblVector3 vRight;
                PblVector3 vUp;
                if (fRotation == 0.0f)
                {
                    vRight = mCamera.GetXAxis();
                }
                else
                {
                    vRight = mCamera.GetXAxis() * GetCos(fRotation) + mCamera.GetYAxis() * GetSin(fRotation);
                }
                if (fRotation == 0.0f)
                {
                    vUp = mCamera.GetYAxis();
                }
                else
                {
                    vUp = mCamera.GetYAxis() * GetCos(fRotation) - mCamera.GetXAxis() * GetSin(fRotation);
                }

                pBlurVerts[0].normal.SetNormal(info.mPos + (vRight + vUp) * (m_blurValue + fHalfSize));
                pBlurVerts[1].normal.SetNormal(info.mPos + (vUp - vRight) * (m_blurValue + fHalfSize));
                pBlurVerts[2].normal.SetNormal(info.mPos + (-(vUp - vRight)) * (m_blurValue + fHalfSize));
                pBlurVerts[3].normal.SetNormal(info.mPos + (-(vRight + vUp)) * (m_blurValue + fHalfSize));

                pBlurVerts[0].pos.SetPos(info.mPos + (vRight + vUp) * fHalfSize);
                pBlurVerts[1].pos.SetPos(info.mPos + (vUp - vRight) * fHalfSize);
                pBlurVerts[2].pos.SetPos(info.mPos + (-(vUp - vRight)) * fHalfSize);
                pBlurVerts[3].pos.SetPos(info.mPos + (-(vRight + vUp)) * fHalfSize);
            }
            else
            {
                SetUV(pVerts[0], info.mColor, info.u[1], info.v[0]);
                SetUV(pVerts[1], info.mColor, info.u[0], info.v[0]);
                SetUV(pVerts[2], info.mColor, info.u[1], info.v[1]);
                SetUV(pVerts[3], info.mColor, info.u[0], info.v[1]);

                float fRotation = info.mRotation;
                const float fHalfSize = info.mSize * 0.5f;
                PblVector3 vRight;
                PblVector3 vUp;
                if (fRotation == 0.0f)
                {
                    vRight = mCamera.GetXAxis() * fHalfSize;
                }
                else
                {
                    vRight = mCamera.GetXAxis() * (GetCos(fRotation) * fHalfSize)
                           + mCamera.GetYAxis() * (GetSin(fRotation) * fHalfSize);
                }
                if (fRotation == 0.0f)
                {
                    vUp = mCamera.GetYAxis() * fHalfSize;
                }
                else
                {
                    vUp = mCamera.GetYAxis() * (GetCos(fRotation) * fHalfSize)
                        - mCamera.GetXAxis() * (GetSin(fRotation) * fHalfSize);
                }

                SetXYZ(pVerts[0], info.mPos + vRight + vUp);
                SetXYZ(pVerts[1], info.mPos - vRight + vUp);
                SetXYZ(pVerts[2], info.mPos + vRight - vUp);
                SetXYZ(pVerts[3], info.mPos - vRight - vUp);
            }
            pIndices[0] = b + 1;
            pIndices[1] = b;
            pIndices[2] = b + 2;
            pIndices[3] = b + 1;
            pIndices[4] = b + 2;
            pIndices[5] = b + 3;
            nIndices = 6;
            nVerts = 4;
            break;

        case PIT_STREAK_HEAD:
            bStreakStart = true;
            break;

        case PIT_STREAK:
        {
            const RedParticleInfoType& prev = m_particleCache[i - 1];
            const PblVector3 vSplit = CalcStreakSplitVector(prev.mPos, prev.mPos, info.mPos, prev.mSize, mCamera);
            SetXYZ(pVerts[0], prev.mPos + vSplit);
            SetUV(pVerts[0], prev.mColor, prev.mRotation, 0.0f);
            SetXYZ(pVerts[1], prev.mPos - vSplit);
            SetUV(pVerts[1], prev.mColor, prev.mRotation, 1.0f);
            if (!bStreakStart)
            {
                pIndices[0] = b - 2;
                pIndices[1] = b;
                pIndices[2] = b - 1;
                pIndices[3] = b;
                pIndices[4] = b + 1;
                pIndices[5] = b - 1;
                nIndices = 6;
            }
            bStreakStart = false;
            nVerts = 2;
            break;
        }

        case PIT_STREAK_TAIL:
        {
            const RedParticleInfoType& prev = m_particleCache[i - 1];
            const RedParticleInfoType& prev2 = m_particleCache[i - 2];
            PblVector3 vSplit = CalcStreakSplitVector(prev2.mPos, prev.mPos, info.mPos, prev.mSize, mCamera);
            SetXYZ(pVerts[0], prev.mPos + vSplit);
            SetUV(pVerts[0], prev.mColor, prev.mRotation, 0.0f);
            SetXYZ(pVerts[1], prev.mPos - vSplit);
            SetUV(pVerts[1], prev.mColor, prev.mRotation, 1.0f);
            pIndices[0] = b - 2;
            pIndices[1] = b;
            pIndices[2] = b - 1;
            pIndices[3] = b;
            pIndices[4] = b + 1;
            pIndices[5] = b - 1;

            vSplit = CalcStreakSplitVector(prev.mPos, info.mPos, info.mPos, info.mSize, mCamera);
            SetXYZ(pVerts[2], info.mPos + vSplit);
            SetUV(pVerts[2], info.mColor, info.mRotation, 0.0f);
            SetXYZ(pVerts[3], info.mPos - vSplit);
            SetUV(pVerts[3], info.mColor, info.mRotation, 1.0f);
            pIndices[6] = b;
            pIndices[7] = b + 2;
            pIndices[8] = b + 1;
            pIndices[9] = b + 2;
            pIndices[10] = b + 3;
            pIndices[11] = b + 1;
            nIndices = 12;
            nVerts = 4;
            break;
        }

        case PIT_LASER_START:
            break;

        case PIT_LASER_END:
        {
            const RedParticleInfoType& prev = m_particleCache[i - 1];
            PblVector3 vStart = prev.mPos;
            const PblVector3 vEnd = info.mPos;
            const float fStartWidth = prev.mSize;
            const float fEndWidth = info.mSize;

            const RedCamera* pCamera = RedRenderer::GetCamera();
            const PblVector3 vCamPos = pCamera->GetMatrix().GetTranslation();
            const PblVector3 vToStart = vStart - vCamPos;
            const PblVector3 vToEnd = vEnd - vCamPos;

            PblVector3 vNormal = CrossProduct(vToEnd, vToStart);
            if (IsZero(vNormal))
            {
                vNormal = -pCamera->GetMatrix().GetXAxis();
            }
            else
            {
                vNormal = Normalize(vNormal);
            }

            PblVector3 vWidth = vNormal * fStartWidth;
            PblVector3 vSide = CrossProduct(vToStart, vWidth);
            PblVector3 vSideNorm = Normalize(vSide);
            PblVector3 vDepth = vSideNorm * fStartWidth;

            if (m_blendMode == BLEND_BLUR)
            {
                const PblVector3 vBack = vStart - vDepth;

                pBlurVerts[0].color = info.mColor;
                pBlurVerts[0].uv.SetUV(0.0f, 0.0f);
                PblVector3 vPos = vBack + vWidth;
                pBlurVerts[0].pos.SetPos(vPos);
                pBlurVerts[0].normal.SetNormal(vPos + (vNormal - vSideNorm) * m_blurValue);
                vPos = vBack - vWidth;
                pBlurVerts[1].color = info.mColor;
                pBlurVerts[1].uv.SetUV(0.0f, 1.0f);
                pBlurVerts[1].pos.SetPos(vPos);
                pBlurVerts[1].normal.SetNormal(vPos + (-vNormal - vSideNorm) * m_blurValue);
                vPos = vStart + vWidth;
                pBlurVerts[2].color = info.mColor;
                pBlurVerts[2].uv.SetUV(0.25f, 0.0f);
                pBlurVerts[2].pos.SetPos(vPos);
                pBlurVerts[2].normal.SetNormal(vPos + vNormal * m_blurValue);
                vPos = vStart - vWidth;
                pBlurVerts[3].color = info.mColor;
                pBlurVerts[3].uv.SetUV(0.25f, 1.0f);
                pBlurVerts[3].pos.SetPos(vPos);
                pBlurVerts[3].normal.SetNormal(vPos + -vNormal * m_blurValue);

                const PblVector3 vToEndCam = vEnd - mCamera.GetTranslation();
                const PblVector3 vEndWidth = vNormal * info.mSize;
                PblVector3 vEndSide = CrossProduct(vToEndCam, vEndWidth);
                PblVector3 vEndSideNorm = Normalize(vEndSide);

                const PblVector3 vZero(0.0f, 0.0f, 0.0f);
                pBlurVerts[4].color = info.mColor;
                pBlurVerts[4].uv.SetUV(0.25f, 0.5f);
                pBlurVerts[4].pos.SetPos(vStart);
                pBlurVerts[4].normal.SetNormal(vZero);
                pBlurVerts[5].color = info.mColor;
                pBlurVerts[5].uv.SetUV(1.0f, 0.5f);
                pBlurVerts[5].pos.SetPos(vEnd);
                pBlurVerts[5].normal.SetNormal(vZero);
                vPos = vEnd + vEndWidth;
                pBlurVerts[6].color = info.mColor;
                pBlurVerts[6].uv.SetUV(1.0f, 0.0f);
                pBlurVerts[6].pos.SetPos(vPos);
                pBlurVerts[6].normal.SetNormal(vPos + vNormal * m_blurValue);
                vPos = vEnd - vEndWidth;
                pBlurVerts[7].color = info.mColor;
                pBlurVerts[7].uv.SetUV(1.0f, 1.0f);
                pBlurVerts[7].pos.SetPos(vPos);
                pBlurVerts[7].normal.SetNormal(vPos + -vNormal * m_blurValue);

                static const unsigned short s_blurIndices[18] = {
                    0, 2, 1, 2, 3, 1, 2, 6, 5, 2, 5, 4, 4, 5, 3, 3, 5, 7
                };
                for (int k = 0; k < 18; ++k)
                {
                    pIndices[k] = b + s_blurIndices[k];
                }
                nVerts = 8;
                nIndices = 18;
                break;
            }

            unsigned char startAlpha = info.mColor.a;
            unsigned char midAlpha = 0;
            const PblVector3 vDir = vEnd - vStart;
            if (SquaredLength(vDir) > 0.0f)
            {
                PblVector3 vDirNorm = Normalize(vDir);
                const PblVector3 vStartToCam = vCamPos - vStart;
                const PblVector3 vOffLine = vStartToCam - vDirNorm * DotProduct(vStartToCam, vDirNorm);
                if (fStartWidth * fStartWidth > SquaredLength(vOffLine))
                {
                    startAlpha = 0;
                    vStart = (vEnd + vStart) * 0.5f;
                    vDepth = mCamera.GetXAxis() * fStartWidth;
                    vWidth = mCamera.GetYAxis() * fStartWidth;
                    midAlpha = info.mColor.a;
                }
            }

            const PblVector3 vBack = vStart - vDepth;
            SetXYZ(pVerts[0], vBack + vWidth);
            SetUV(pVerts[0], info.mColor, 0.0f, 0.0f);
            SetXYZ(pVerts[1], vBack - vWidth);
            SetUV(pVerts[1], info.mColor, 0.0f, 1.0f);
            SetXYZ(pVerts[2], vStart + vWidth);
            SetUV(pVerts[2], info.mColor, 0.25f, 0.0f);
            SetXYZ(pVerts[3], vStart - vWidth);
            SetUV(pVerts[3], info.mColor, 0.25f, 1.0f);
            pIndices[0] = b;
            pIndices[1] = b + 2;
            pIndices[2] = b + 1;
            pIndices[3] = b + 2;
            pIndices[4] = b + 3;
            pIndices[5] = b + 1;
            nVerts = 4;
            nIndices = 6;

            if (startAlpha)
            {
                RedColor color(info.mColor, startAlpha);
                const PblVector3 vEndWidth = vNormal * fEndWidth;
                SetXYZ(pVerts[4], vStart);
                SetUV(pVerts[4], prev.mColor, 0.25f, 0.5f);
                SetXYZ(pVerts[5], vEnd + vEndWidth);
                SetUV(pVerts[5], color, 1.0f, 0.0f);
                SetXYZ(pVerts[6], vEnd - vEndWidth);
                SetUV(pVerts[6], color, 1.0f, 1.0f);
                pIndices[6] = b + 2;
                pIndices[7] = b + 5;
                pIndices[8] = b + 4;
                pIndices[9] = b + 4;
                pIndices[10] = b + 5;
                pIndices[11] = b + 6;
                pIndices[12] = b + 6;
                pIndices[13] = b + 3;
                pIndices[14] = b + 4;
                nVerts = 7;
                nIndices = 15;
            }

            if (midAlpha)
            {
                RedColor color(prev.mColor, midAlpha);
                const PblVector3 vMid = vStart + vDepth;
                SetXYZ(pVerts[nVerts], vMid + vWidth);
                SetUV(pVerts[nVerts], color, 0.0f, 0.0f);
                SetXYZ(pVerts[nVerts + 1], vMid - vWidth);
                SetUV(pVerts[nVerts + 1], color, 0.0f, 1.0f);
                unsigned short* pOut = pIndices + nIndices;
                pOut[0] = b + 2;
                pOut[1] = b + nVerts;
                pOut[2] = b + nVerts + 1;
                pOut[3] = b + 2;
                pOut[4] = b + nVerts + 1;
                for (int k = 5; k < 15; ++k)
                {
                    pOut[k] = b + 3;
                }
                nVerts += 5;
                nIndices += 15;
            }
            else
            {
                for (int k = 0; k < 6; ++k)
                {
                    pIndices[nIndices + k] = b + 6;
                }
                nVerts += 2;
                nIndices += 6;
            }
            break;
        }

        case PIT_FLAT:
        {
            SetUV(pVerts[0], info.mColor, 1.0f, 0.0f);
            SetUV(pVerts[1], info.mColor, 0.0f, 0.0f);
            SetUV(pVerts[2], info.mColor, 1.0f, 1.0f);
            SetUV(pVerts[3], info.mColor, 0.0f, 1.0f);
            const PblVector3 vRight = info.mRight * info.mSize;
            const PblVector3 vUp = info.mUp * info.mSize;
            SetXYZ(pVerts[0], info.mPos + vRight + vUp);
            SetXYZ(pVerts[1], info.mPos - vRight + vUp);
            SetXYZ(pVerts[2], info.mPos + vRight - vUp);
            SetXYZ(pVerts[3], info.mPos - vRight - vUp);
            pIndices[0] = b + 1;
            pIndices[1] = b;
            pIndices[2] = b + 2;
            pIndices[3] = b + 1;
            pIndices[4] = b + 2;
            pIndices[5] = b + 3;
            nIndices = 6;
            nVerts = 4;
            break;
        }

        default:
            break;
        }

        pIndices += nIndices;
        if (m_blendMode == BLEND_BLUR)
        {
            pBlurVerts += nVerts;
        }
        else
        {
            pVerts += nVerts;
        }
        iBaseVertex += nVerts;
    }
}

// FUNCTION: SWBF2 0x006d2e80
const PblVector3 CalcStreakSplitVector(const PblVector3& vPrev, const PblVector3& vCur,
                                       const PblVector3& vNext, float fWidth,
                                       const PblMatrix& mCamera)
{
    const PblVector3 vDir = (vCur - vPrev) + (vNext - vCur);
    const PblVector3 vToCam = vCur - mCamera.GetTranslation();
    PblVector3 vSplit;
    Cross(&vSplit, vDir, vToCam);
    if (IsZero(vSplit))
    {
        return mCamera.GetYAxis() * fWidth;
    }

    PblVector3 vNorm;
    D3DXVec3Normalize(reinterpret_cast<D3DXVECTOR3*>(&vNorm), reinterpret_cast<const D3DXVECTOR3*>(&vSplit));
    return vNorm * fWidth;
}

// FUNCTION: SWBF2 0x006d2fe0
void RedParticleRenderer::Cleanup()
{
    FLRenderer::RemoveRenderable(&RedParticleRenderer::_Instance);
    FLRenderer::RemoveRenderable(&RedParticleRenderer::_FarInstance);
    currentCache = 0;
}

static const float kRadiansPerAngleStep = 6.2831855f / 512.0f;

// FUNCTION: SWBF2 0x006d3010
static float GetCos(float fAngle)
{
    return PblMath::Cos(fAngle * kRadiansPerAngleStep);
}

// FUNCTION: SWBF2 0x006d3030
static float GetSin(float fAngle)
{
    return PblMath::Sin(fAngle * kRadiansPerAngleStep);
}

// FUNCTION: SWBF2 0x006d3050
void RedParticleRenderer::Init()
{
    FLRenderer::AddRenderable(&RedParticleRenderer::_Instance, RENDERABLE_POSTWORLD, 0x12c);
    FLRenderer::AddRenderable(&RedParticleRenderer::_FarInstance, RENDERABLE_SKY_POSTWORLD, 0x12c);

    currentCache = 0;
    s_cacheIndex = 0;
}

// FUNCTION: SWBF2 0x006d3090
bool RedParticleRenderer::IsFull()
{
    RedParticleRenderer* pCache = RedParticleRenderer::currentCache;
    if (pCache)
    {
        return pCache->m_particleIndex >= 200;
    }
    return false;
}

// FUNCTION: SWBF2 0x006d30b0
void RedParticleRenderer::Renderable::Render(unsigned int value) const
{
    RenderAll();
}

// FUNCTION: SWBF2 0x006d30c0
void RedParticleRenderer::RenderAll()
{
    for (int i = 0; i < s_cacheIndex; ++i)
    {
        RedParticleRenderer& cache = s_caches[i];
        if (cache.m_blendMode == BLEND_BLUR && !RedParticleRenderer::s_blurEnable)
        {
            continue;
        }
        if (!cache.m_numVerts)
        {
            continue;
        }

        pcRedPrimitive* pPrimitive;
        if (cache.m_blendMode == BLEND_BLUR)
        {
            pPrimitive = pcRedPrimitive::Create(D3DPT_TRIANGLELIST, 0x2a2, sizeof(BlurParticleVertexT),
                                                cache.m_numVerts, cache.m_numIndices, false);
        }
        else
        {
            pPrimitive = pcRedPrimitive::Create(D3DPT_TRIANGLELIST, 0x282, sizeof(ParticleVertexT),
                                                cache.m_numVerts, cache.m_numIndices, false);
        }
        void* pVertices = pPrimitive->BeginVertexData();
        if (!pVertices)
        {
            return;
        }
        unsigned short* pIndices = pPrimitive->BeginIndexData();
        cache.AddParticleToBuffers(pVertices, pIndices);
        pPrimitive->EndVertexData();
        pPrimitive->EndIndexData();

        pcParticleShader* pShader;
        if (cache.m_blendMode == BLEND_BLUR)
        {
            pShader = (pcParticleShader*)pcRedShader::Create(kuiParticleShader, 0x2a2, &cache.m_texHash, 1, 4,
                                                             RedColor::WHITE, RedColor::BLACK, 32.0f);
            pShader->m_blurEnable = true;
            pShader->m_blurBuffer = RedRenderer::pcGetBuffer(PC_BUFFER_REFRACTION);
        }
        else
        {
            pShader = (pcParticleShader*)pcRedShader::Create(kuiParticleShader, 0x282, &cache.m_texHash, 1,
                                                             cache.m_blendMode == BLEND_NORMAL ? 4 : 0x44,
                                                             RedColor::WHITE, RedColor::BLACK, 32.0f);
        }

        RedRenderer::pcRenderPrimitive(pPrimitive, pShader,
                                       gMatrixIdentity, RedColor::WHITE,
                                       cache.m_uiRenderFlags | 0x10, 0, 0, 0);
        pShader->Release();
        pPrimitive->Release();
    }
    s_cacheIndex = 0;
    currentCache = 0;
}

// FUNCTION: SWBF2 0x006d3280
void RedParticleRenderer::SetBlurParams(float fBlurValue, int iBlurParam)
{
    if (currentCache)
    {
        currentCache->m_blurValue = fBlurValue;
        currentCache->m_blurParam = (float)iBlurParam;
    }
}

// FUNCTION: SWBF2 0x006d32b0
void RedParticleRenderer::SetCurrentCache(unsigned int uTexture, RedBlendMode eBlend, unsigned int uiRenderFlags)
{
    for (int i = 0; i < s_cacheIndex; ++i)
    {
        if (s_caches[i].m_texHash == uTexture && s_caches[i].m_blendMode == eBlend &&
            s_caches[i].m_uiRenderFlags == uiRenderFlags)
        {
            currentCache = &s_caches[i];
            return;
        }
    }
    if (s_cacheIndex >= 15)
    {
        currentCache = 0;
        return;
    }
    if (!currentCache || currentCache->m_numVerts > 0)
    {
        currentCache = &s_caches[s_cacheIndex++];
    }
    currentCache->m_texHash = uTexture;
    currentCache->m_blendMode = eBlend;
    currentCache->m_uiRenderFlags = uiRenderFlags;
    currentCache->m_numVerts = 0;
    currentCache->m_numIndices = 0;
    currentCache->m_particleIndex = 0;
}

// FUNCTION: SWBF2 0x006d3390
void RedParticleRenderer::SubmitBounds(PblVector3 const& vMin, PblVector3 const& vMax)
{
}

// FUNCTION: SWBF2 0x006d33a0
void RedParticleRenderer::SubmitParticle(ParticleInfoType eType, PblVector3 const& vPos, RedColor const& color,
                                         float fSize, float fRotation, TexCoords texCoords,
                                         PblVector3 const& vRight, PblVector3 const& vUp)
{
    if (!currentCache)
    {
        return;
    }
    if (!currentCache->AddParticle(eType, vPos, color, fSize, fRotation, texCoords, vRight, vUp))
    {
        return;
    }

    currentCache->m_numVerts += numVertsAdded(eType);
    currentCache->m_numIndices += numIndicesAdded(eType);
    if (eType == PIT_STREAK)
    {
        const int i = currentCache->m_particleIndex;
        if (i >= 2 && currentCache->m_particleCache[i - 2].mPit == PIT_STREAK_HEAD)
        {
            currentCache->m_numIndices -= 6;
        }
    }
}

// FUNCTION: SWBF2 0x006d3440
void RedParticleRenderer::SubmitTexture(unsigned int uTexture, RedBlendMode eBlend, unsigned int uiRenderFlags)
{
    SetCurrentCache(uTexture, eBlend, uiRenderFlags);
}

// FUNCTION: SWBF2 0x006d3450
static int numIndicesAdded(ParticleInfoType eType)
{
    switch (eType)
    {
    case PIT_PARTICLE:
    case PIT_STREAK:
    case PIT_FLAT:
    case PIT_ANIMATED:
        return 6;
    case PIT_STREAK_TAIL:
        return 12;
    case PIT_LASER_END:
        return 21;
    default:
        return 0;
    }
}

// FUNCTION: SWBF2 0x006d34a0
static int numVertsAdded(ParticleInfoType eType)
{
    switch (eType)
    {
    case PIT_STREAK:
        return 2;
    case PIT_LASER_END:
        return 9;
    case PIT_PARTICLE:
    case PIT_STREAK_TAIL:
    case PIT_FLAT:
    case PIT_ANIMATED:
        return 4;
    default:
        return 0;
    }
}
