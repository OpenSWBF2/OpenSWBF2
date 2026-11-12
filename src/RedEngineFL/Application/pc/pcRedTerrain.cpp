#include "RedEngineFL/Application/Renderable.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Graphics/PC/pcRedAutoSettings.h"

#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedIndexBuffer.h"
#include "RedEngineFL/Graphics/PC/pcRedVertexBuffer.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblFile.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcTerrainShader.h"
#include "RedEngineFL/Graphics/PC/Shaders/pcTerrainShader2.h"
#include "RedEngineFL/Memory/engineAlloc.h"
#include "RedEngineFL/Application/pc/pcRenderHelpers.h"
#include "RedEngineFL/Graphics/RedCamera.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "RedEngineFL/Graphics/RedLight.h"
#include "RedEngineFL/Util/RedCommandConsole.h"
#include "RedEngineFL/Application/RedTerrain.h"
#include "pcFLRender.h"

#include <math.h>

// VTABLE: SWBF2 0x00773070
class TerrainRenderable : public Renderable
{
public:
    struct LowResPatch
    {
        pcRedPrimitive* pPrim;
        PblBox bbox;
    };

    struct Patch
    {
        unsigned int uiNumPasses;
        pcRedPrimitive** apPrim;
        pcTerrainShader** apShader;
        pcTerrainShader2** apShader2;
        short iHeightMin;
        short iHeightMax;
    };

    TerrainRenderable();
    virtual ~TerrainRenderable();

    void Activate();

    virtual void Deactivate();

    virtual void Render(unsigned int value) const;

    void Deinit();
    void Read(PblFileChunk& chunk);

    void _Reset();

    void Init();
    void _SetQuality();
    void _InitPixelLighting(bool bEnable);

    void SetFarTerrainMaxDistance(float fDist) { m_fFarTerrainMaxDistance = fDist; }

    void SetBumpDetailTexture(unsigned int uiHash, float fScale, float fStrength)
    {
        m_fBumpLuminosityScale = fScale;
        m_bBumpTextureAvailable = uiHash != 0;
        m_uiBumpLuminosityTextureHash = uiHash;
        m_fStaticBumpFactor = fStrength;
    }

    void SetFarTerrainTexture(unsigned int uiHash, float fA, unsigned int uiB, float fC);

    static int _CalcPatchX(float fX);
    static int _CalcPatchZ(float fZ);

    void _RenderLowRes(unsigned int uiFlags) const;

    Patch* m_aPatches;
    LowResPatch* m_aLowResPatches;
    pcRedIndexBuffer* m_pIBDefault;
    pcTerrainShader* m_pFarShader;
    pcTerrainShader2* m_pFarShader2;
    bool m_bEnableFarDetail;
    float m_fFarTerrainMaxDistance;
    float m_fLowResMapSize;
    int m_iLowResPatchSize;
    int m_iLowResPatchesPerDimension;
    float m_fLowResPointsPerMeter;
    float m_fDetailMaxDistance;
    bool m_bEnablePixelLighting;
    bool m_bBumpTextureAvailable;
    unsigned int m_uiBumpLuminosityTextureHash;
    float m_fBumpLuminosityScale;
    float m_fStaticBumpFactor;
    pcTerrainShader* m_pPrePixelLightingShader;
    pcTerrainShader2* m_pPrePixelLightingShader2;
    QualityT m_eQuality;
    bool m_bEnableBumpMapping;
};





 extern TerrainRenderable _TerrainRenderable;

// FUNCTION: SWBF2 0x006d5d60
void RedTerrain::SetBumpDetailTexture(unsigned int uiHash, float fScale, float fStrength)
{
    _TerrainRenderable.SetBumpDetailTexture(uiHash, fScale, fStrength);
}

// FUNCTION: SWBF2 0x006d5d90
void RedTerrain::SetBumpSpecularTexture(unsigned int, float)
{
}

// FUNCTION: SWBF2 0x006d60e0
void TerrainRenderable::_RenderLowRes(unsigned int uiFlags) const
{
    if (m_fFarTerrainMaxDistance <= 0.0f)
    {
        return;
    }

    const RedCamera* pCurrent = RedRenderer::GetCamera();
    RedCamera camera(*pCurrent);
    float fZoom = pCurrent->GetZoom();
    float fNear, fFar, fWidth, fHeight;
    pCurrent->GetFrustum(fNear, fFar, fWidth, fHeight);
    camera.SetZoom(fZoom);
    float fFarPlane = m_fFarTerrainMaxDistance;
    camera.SetNearFarPlane(fZoom * (uiFlags & 0x200000 ? FLRenderer::GetNearPlane()
                                                       : FLRenderer::GetNearSceneFadeStart()),
                           fFarPlane);

    int xStart = 0;
    int xEnd = m_iLowResPatchesPerDimension;
    int xStep = 1;
    if (-pCurrent->GetMatrix().m[2][0] < 0.0f)
    {
        xStart = m_iLowResPatchesPerDimension - 1;
        xEnd = -1;
        xStep = -1;
    }
    int zStart = 0;
    int zEnd = m_iLowResPatchesPerDimension;
    int zStep = 1;
    if (-pCurrent->GetMatrix().m[2][2] < 0.0f)
    {
        zStart = m_iLowResPatchesPerDimension - 1;
        zEnd = -1;
        zStep = -1;
    }
    for (int z = zStart; z != zEnd; z += zStep)
    {
        for (int x = xStart; x != xEnd; x += xStep)
        {
            const LowResPatch& patch = m_aLowResPatches[x + z * m_iLowResPatchesPerDimension];
            if (!patch.pPrim)
            {
                continue;
            }
            const PblVector3& vMin = patch.bbox._vMin;
            const PblVector3& vMax = patch.bbox._vMax;
            PblVector3 vExtent = (vMax - vMin) * 0.5f;
            PblVector3 vCenter = (vMax + vMin) * 0.5f;
            if (camera.ClassifyAABox(vCenter, vExtent) == RedCamera::VISIBILITY_NONE)
            {
                continue;
            }
            if (pcTerrainShader2::IsEnabled())
            {
                RedRenderer::pcRenderPrimitive(patch.pPrim,
                                               m_pFarShader2,
                                               gMatrixIdentity,
                                               RedColor::WHITE,
                                               uiFlags | 0x1000000, 0, 0, 0);
            }
            else
            {
                RedRenderer::pcRenderPrimitive(patch.pPrim,
                                               m_pFarShader,
                                               gMatrixIdentity,
                                               RedColor::WHITE,
                                               uiFlags | 0x1000000, 0, 0, 0);
            }
        }
    }
}

// GLOBAL: SWBF2 0x007df330
bool bTerrainEnable = true;

// GLOBAL: SWBF2 0x007df331
bool bForceFullDetail = true;

// SYNTHETIC: SWBF2 0x004081a0
// `dynamic initializer for '_TerrainRenderable''
// SYNTHETIC: SWBF2 0x00769980
// `dynamic atexit destructor for '_TerrainRenderable''

// GLOBAL: SWBF2 0x00998240
// _TerrainRenderable
TerrainRenderable _TerrainRenderable;

// SYNTHETIC: SWBF2 0x004081c0
// `dynamic initializer for 'cvForceFullDetail''
static RedCommandConsole cvForceFullDetail("terrain.ForceFullDetail", MultiVar(&bForceFullDetail));

// FUNCTION: SWBF2 0x006d34f0
TerrainRenderable::TerrainRenderable()
{
    Init();
}

// FUNCTION: SWBF2 0x006d3510
TerrainRenderable::~TerrainRenderable()
{
    Deinit();
}

// SYNTHETIC: SWBF2 0x006d3570
// TerrainRenderable::`scalar deleting destructor'

// FUNCTION: SWBF2 0x006d35a0
void RedTerrain::Activate()
{
    if (RedTerrain::_bEnable)
    {
        _TerrainRenderable.Activate();
    }
}

// FUNCTION: SWBF2 0x006d35c0
void TerrainRenderable::Activate()
{
    QualityT eQuality = RedRenderer::pcGetRenderOptions().terrainQuality;
    bool bBumpMapping = RedRenderer::pcGetRenderOptions().bumpMapping;
    if (m_eQuality != eQuality || m_bEnableBumpMapping != bBumpMapping)
    {
        m_eQuality = eQuality;
        m_bEnableBumpMapping = bBumpMapping;
        RedTerrain::RenderDetail = eQuality > QUALITY_LOW;
        _InitPixelLighting(RedRenderer::pcGetRenderCaps().pipeline != PIPELINE_FIXED &&
                           m_eQuality == QUALITY_HIGH);
    }
}

// FUNCTION: SWBF2 0x006d6480
void TerrainRenderable::_SetQuality()
{
    QualityT eQuality = RedRenderer::pcGetRenderOptions().terrainQuality;
    bool bBumpMapping = RedRenderer::pcGetRenderOptions().bumpMapping;
    if (m_eQuality != eQuality || m_bEnableBumpMapping != bBumpMapping)
    {
        m_eQuality = eQuality;
        m_bEnableBumpMapping = bBumpMapping;
        RedTerrain::RenderDetail = eQuality > QUALITY_LOW;
        _InitPixelLighting(RedRenderer::pcGetRenderCaps().pipeline != PIPELINE_FIXED &&
                           m_eQuality == QUALITY_HIGH);
    }
}

// FUNCTION: SWBF2 0x006d3620
void RedTerrain::Deactivate()
{
    _TerrainRenderable.Deinit();
    _TerrainRenderable.Init();
}

// FUNCTION: SWBF2 0x006d3640
void TerrainRenderable::Deactivate()
{
}

// FUNCTION: SWBF2 0x006d3650
void TerrainRenderable::Deinit()
{
    if (m_aPatches)
    {
        for (int i = 0; i < RedTerrain::_iNumPatches; ++i)
        {
            for (unsigned int j = 0; j < m_aPatches[i].uiNumPasses; ++j)
            {
                if (m_aPatches[i].apPrim[j])
                {
                    m_aPatches[i].apPrim[j]->Release();
                }
                if (m_aPatches[i].apShader[j])
                {
                    m_aPatches[i].apShader[j]->Release();
                }
                if (m_aPatches[i].apShader2[j])
                {
                    m_aPatches[i].apShader2[j]->Release();
                }
            }
            delete[] m_aPatches[i].apPrim;
            delete[] m_aPatches[i].apShader;
            delete[] m_aPatches[i].apShader2;
        }
        delete[] m_aPatches;
        m_aPatches = 0;
    }
    if (m_aLowResPatches)
    {
        int iNum = m_iLowResPatchesPerDimension * m_iLowResPatchesPerDimension;
        for (int i = 0; i < iNum; ++i)
        {
            if (m_aLowResPatches[i].pPrim)
            {
                m_aLowResPatches[i].pPrim->Release();
            }
        }
        delete[] m_aLowResPatches;
        m_aLowResPatches = 0;
    }
    if (m_pIBDefault)
    {
        m_pIBDefault->Release();
        m_pIBDefault = 0;
    }
    if (m_pFarShader)
    {
        m_pFarShader->Release();
        m_pFarShader = 0;
    }
    if (m_pFarShader2)
    {
        m_pFarShader2->Release();
        m_pFarShader2 = 0;
    }
    if (m_pPrePixelLightingShader)
    {
        m_pPrePixelLightingShader->Release();
        m_pPrePixelLightingShader = 0;
    }
    if (m_pPrePixelLightingShader2)
    {
        m_pPrePixelLightingShader2->Release();
        m_pPrePixelLightingShader2 = 0;
    }
}

// FUNCTION: SWBF2 0x006d3910
void TerrainRenderable::Init()
{
    m_aPatches = 0;
    m_aLowResPatches = 0;
    m_pIBDefault = 0;
    m_pFarShader = 0;
    m_pPrePixelLightingShader = 0;
    m_pPrePixelLightingShader2 = 0;
    m_fDetailMaxDistance = 32.0f;
    m_fLowResMapSize = 0.0f;
    m_iLowResPatchSize = 0;
    m_iLowResPatchesPerDimension = 0;
    m_fLowResPointsPerMeter = 0.0f;
    m_bEnablePixelLighting = false;
    m_bBumpTextureAvailable = false;
    m_bEnableFarDetail = false;
    m_fFarTerrainMaxDistance = 0.0f;
    m_uiBumpLuminosityTextureHash = 0;
    m_fBumpLuminosityScale = 1.0f;
    m_fStaticBumpFactor = 0.5f;
    m_eQuality = (QualityT)-1;
}

// FUNCTION: SWBF2 0x006d4ff0
void RedTerrain::Render(unsigned int uiFlags)
{
    if (bTerrainEnable)
    {
        _TerrainRenderable.Render((int)uiFlags);
    }
}

// FUNCTION: SWBF2 0x006d5010
void TerrainRenderable::Render(unsigned int value) const
{
    unsigned int uiFlags = (unsigned int)value;
    if (!bTerrainEnable || !RedTerrain::_bEnable || !m_aPatches)
    {
        return;
    }

    const_cast<TerrainRenderable*>(this)->_SetQuality();
    const RedCamera* pCamera = RedRenderer::GetCamera();
    if (!RedRenderer::pcIsNearScene() || (uiFlags & 0x20000))
    {
        if (!bForceFullDetail && !pcTerrainShader2::IsEnabled())
        {
            _RenderLowRes(uiFlags);
            return;
        }
        RedCamera farCamera(*pCamera);
        float fZoom = pCamera->GetZoom();
        float fFarDistance = m_fFarTerrainMaxDistance;
        farCamera.SetNearFarPlane(((uiFlags & 0x200000) ? FLRenderer::GetNearPlane() : FLRenderer::GetNearSceneFadeStart()) * fZoom,
                                  fFarDistance);
    }

    float fNear, fFar, fWidth, fHeight;
    pCamera->GetFrustum(fNear, fFar, fWidth, fHeight);
    const PblMatrix& mCamera = pCamera->GetMatrix();
    const PblVector3& vRight = mCamera.GetXAxis();
    const PblVector3& vUp = mCamera.GetYAxis();
    const PblVector3& vAt = mCamera.GetZAxis();
    PblVector3 vPos = mCamera.GetTranslation();

    float fHalfWidth = fFar * 0.5f * fWidth / fNear;
    float fHalfHeight = fFar * 0.5f * fHeight / fNear;

    const PblVector3 vFrustrumPoints[5] = {
        vPos - vAt * fFar + vRight * fHalfWidth + vUp * fHalfHeight,
        vPos,
        vPos,
        vPos,
        vPos
    };

    const PblVector3 vFrustrumPlanes[5] = {
        -vAt,
        vAt * fHalfWidth + vRight * fFar,
        vAt * fHalfWidth - vRight * fFar,
        vAt * fHalfHeight + vUp * fFar,
        vAt * fHalfHeight - vUp * fFar
    };

    const float fFrustrumPlaneDistances[5] = {
        DotProduct(vFrustrumPlanes[0], vFrustrumPoints[0]),
        DotProduct(vFrustrumPlanes[1], vFrustrumPoints[1]),
        DotProduct(vFrustrumPlanes[2], vFrustrumPoints[2]),
        DotProduct(vFrustrumPlanes[3], vFrustrumPoints[3]),
        DotProduct(vFrustrumPlanes[4], vFrustrumPoints[4])
    };

    const PblBox& box = pCamera->GetCameraBox();
    int iMinX = _CalcPatchX(box._vMin.x);
    int iMaxX = _CalcPatchX(box._vMax.x);
    int iMinZ = _CalcPatchZ(box._vMin.z);
    int iMaxZ = _CalcPatchZ(box._vMax.z);
    if (iMaxX < 0 || iMinX >= RedTerrain::_iPatchesPerDimension || iMaxZ < 0 ||
        iMinZ >= RedTerrain::_iPatchesPerDimension)
    {
        return;
    }
    if (iMinX < 0)
    {
        iMinX = 0;
    }
    if (iMaxX >= RedTerrain::_iPatchesPerDimension)
    {
        iMaxX = RedTerrain::_iPatchesPerDimension - 1;
    }
    if (iMinZ < 0)
    {
        iMinZ = 0;
    }
    if (iMaxZ >= RedTerrain::_iPatchesPerDimension)
    {
        iMaxZ = RedTerrain::_iPatchesPerDimension - 1;
    }

    int iStepX = 1;
    int iStepZ = 1;
    if (-pCamera->GetMatrix().GetZAxis().x < 0.0f)
    {
        int iTemp = iMinX;
        iMinX = iMaxX;
        iMaxX = iTemp;
        iStepX = -1;
    }
    iMaxX += iStepX;
    if (-pCamera->GetMatrix().GetZAxis().z < 0.0f)
    {
        int iTemp = iMinZ;
        iMinZ = iMaxZ;
        iMaxZ = iTemp;
        iStepZ = -1;
    }
    iMaxZ += iStepZ;

    if (RedShadowMap::IsShadowMapEnabled() && !(uiFlags & 0x10))
    {
        if (!pcTerrainShader2::IsEnabled())
        {
            pcTerrainShader::s_uiOptions |= 0x10000;
        }
        pcTerrainShader2::_InitShadowMap(RedShadowMap::GetShadowMapTexture(),
                                         RedShadowMap::GetShadowMapWrapMode(),
                                         RedShadowMap::_vShadowMapTransformU,
                                         RedShadowMap::_vShadowMapTransformV,
                                         RedShadowMap::_vShadowMapTransformW);
    }
    else if (!pcTerrainShader2::IsEnabled())
    {
        pcTerrainShader::s_uiOptions &= ~0x10000;
    }

    if (RedRenderer::pcGetRenderCaps().pipeline == PIPELINE_FIXED)
    {
        uiFlags |= 0x800;
    }
    if (!m_bEnablePixelLighting)
    {
        RedRenderer::SetLightingState(*RedLight::GetLightingState(true));
    }

    float fHalfPatch = (float)RedTerrain::_iPatchPointsPerDimension * RedTerrain::_fMetersPerPoint * 0.5f;

    for (int iZ = iMinZ; iZ != iMaxZ; iZ += iStepZ)
    {
        float fZ = RedTerrain::_CalcWorldPosZ(
            (RedTerrain::_iPatchPointsPerDimension >> 1) + (iZ << RedTerrain::_iPatchPointsPerDimensionLog2));
        float afRowDist[5];
        for (int i = 0; i < 5; ++i)
        {
            afRowDist[i] = fFrustrumPlaneDistances[i] -
                           (vFrustrumPlanes[i].z * fZ - (float)fabs(vFrustrumPlanes[i].z * fHalfPatch) -
                            (float)fabs(vFrustrumPlanes[i].x * fHalfPatch));
        }

        int iLastPlane = 0;
        for (int iX = iMinX; iX != iMaxX; iX += iStepX)
        {
            Patch& patch = m_aPatches[iZ * RedTerrain::_iPatchesPerDimension + iX];
            float fX = RedTerrain::_CalcWorldPosX(
                (RedTerrain::_iPatchPointsPerDimension >> 1) + (iX << RedTerrain::_iPatchPointsPerDimensionLog2));
            if (!patch.apPrim[0])
            {
                continue;
            }

            float fCenterY = (float)(patch.iHeightMin + patch.iHeightMax) * 0.5f * RedTerrain::_fHeightScale;
            float fExtentY = (float)(patch.iHeightMax - patch.iHeightMin) * 0.5f * RedTerrain::_fHeightScale;

            const PblVector3& vLast = vFrustrumPlanes[iLastPlane];
            if (vLast.x * fX + vLast.y * fCenterY - (float)fabs(vLast.y) * fExtentY > afRowDist[iLastPlane])
            {
                continue;
            }
            int iPlane;
            for (iPlane = 0; iPlane < 5; ++iPlane)
            {
                const PblVector3& vNormal = vFrustrumPlanes[iPlane];
                if (iPlane != iLastPlane &&
                    vNormal.x * fX + fCenterY * vNormal.y - (float)fabs(vNormal.y) * fExtentY > afRowDist[iPlane])
                {
                    break;
                }
            }
            if (iPlane < 5)
            {
                iLastPlane = iPlane;
                continue;
            }

            if (m_bEnablePixelLighting)
            {
                RedRenderer::SetLightingState(
                    *RedLight::GetLightingState(patch.apPrim[0]->m_bbox, 4, true, true, false));
            }
            patch.apShader[0]->EnableDetail(true);

            if (RedRenderer::pcGetRenderCaps().pipeline == PIPELINE_FIXED)
            {
                PblMatrix mWorld = gMatrixIdentity;
                const PblBox& bbox = patch.apPrim[0]->m_bbox;
                mWorld.SetTranslation(PblVector3(bbox._vMin.x, 0.0f, bbox._vMin.z));
                for (unsigned int uiPass = 0; uiPass < patch.uiNumPasses; ++uiPass)
                {
                    RedRenderer::pcRenderPrimitive(patch.apPrim[uiPass],
                                                   patch.apShader[uiPass],
                                                   mWorld,
                                                   RedColor::WHITE, uiFlags, 0,
                                                   0, 0);
                }
            }
            else if (!pcTerrainShader2::IsEnabled())
            {
                if (m_bEnablePixelLighting)
                {
                    RedRenderer::pcRenderPrimitive(patch.apPrim[0],
                                                   m_pPrePixelLightingShader,
                                                   gMatrixIdentity,
                                                   RedColor::WHITE,
                                                   uiFlags | 0x1000000, 0, 0, 0);
                }
                RedRenderer::pcRenderPrimitive(patch.apPrim[0],
                                               patch.apShader[0],
                                               gMatrixIdentity,
                                               RedColor::WHITE,
                                               uiFlags | 0x1000000, 0, 0, 0);
            }
            else
            {
                RedRenderer::pcRenderPrimitive(patch.apPrim[0],
                                               patch.apShader2[0],
                                               gMatrixIdentity,
                                               RedColor::WHITE,
                                               uiFlags | 0x1000000, 0, 0, 0);
            }
        }
    }
}

// FUNCTION: SWBF2 0x006d5da0
void RedTerrain::SetFarTerrainMaxDistance(float fDist)
{
    _TerrainRenderable.SetFarTerrainMaxDistance(fDist);
}

// FUNCTION: SWBF2 0x006d5dc0
void RedTerrain::SetFarTerrainTexture(unsigned int uiHash, float fA, unsigned int uiB, float fC)
{
    _TerrainRenderable.SetFarTerrainTexture(uiHash, fA, uiB, fC);
}

// FUNCTION: SWBF2 0x006d5df0
void TerrainRenderable::SetFarTerrainTexture(unsigned int uiHash, float fA, unsigned int uiB, float fC)
{
    float fScale = fA / ((float)RedTerrain::_iPointsPerDimension * RedTerrain::_fMetersPerPoint);
    float fMinX = RedTerrain::_fMetersPerPoint * 0.5f * fScale - 0.5f;
    float fMinY = -0.5f - RedTerrain::_fMetersPerPoint * 0.5f * fScale;
    if (!pcTerrainShader2::IsEnabled())
    {
        pcTerrainShader::_InitFarRendering(uiHash, fScale, fScale, fMinX, fMinY);
    }
    float fDetailScale = RedTerrain::_fPointsPerMeter * fC;
    if (!pcTerrainShader2::IsEnabled())
    {
        pcTerrainShader::_InitFarDetail(uiB, fDetailScale, fDetailScale, 0.0f, 0.0f);
    }
    bool bEnableDetail = uiB != 0;
    m_bEnableFarDetail = bEnableDetail;
    pcTerrainShader* pFarShader = m_pFarShader;
    if (pFarShader)
    {
        pFarShader->EnableFarRendering(true, bEnableDetail);
    }
}

// FUNCTION: SWBF2 0x006d5f10
int TerrainRenderable::_CalcPatchX(float fX)
{
    return (int)((RedTerrain::_fTerrainCenterX + fX) * RedTerrain::_fPointsPerMeter) >>
           RedTerrain::_iPatchPointsPerDimensionLog2;
}

// FUNCTION: SWBF2 0x006d5f40
int TerrainRenderable::_CalcPatchZ(float fZ)
{
    return (int)((RedTerrain::_fTerrainCenterZ + fZ) * RedTerrain::_fPointsPerMeter) >>
           RedTerrain::_iPatchPointsPerDimensionLog2;
}

// FUNCTION: SWBF2 0x006d5f70
void TerrainRenderable::_InitPixelLighting(bool bEnable)
{
    m_bEnablePixelLighting = bEnable;
    if (m_bBumpTextureAvailable && m_bEnableBumpMapping)
    {
        float fScale = m_fBumpLuminosityScale * RedTerrain::_fPointsPerMeter;
        PblVector4 vScale1 = { 0.0f, 0.0f, fScale, 0.0f };
        PblVector4 vScale0 = { fScale, 0.0f, 0.0f, 0.0f };
        pcTerrainShader2::_InitBumpLuminosityPixelLighting(0x785887DA /* "attenuation_volume" */, m_uiBumpLuminosityTextureHash,
                                                           vScale0, vScale1, m_fStaticBumpFactor);
    }
    else
    {
        pcTerrainShader2::_InitPixelLighting(0x785887DA /* "attenuation_volume" */, 0x63DD533C /* "cube_normalizationmap" */);
    }

    if (!pcTerrainShader2::IsEnabled())
    {
        if (bEnable)
        {
            pcTerrainShader::s_uiOptions |= 0x20000;
        }
        else
        {
            pcTerrainShader::s_uiOptions &= ~0x20000;
        }
    }

    if (!m_pPrePixelLightingShader)
    {
        unsigned int uiFlags = RedRenderer::pcGetGeometryMode() == GEOMETRYMODE_COMPRESSED ? 0x5122 : 0x122;
        m_pPrePixelLightingShader2 = (pcTerrainShader2*)pcRedShader::Create(
            PblHash("Terrain2"), uiFlags, 0, 0, 0, RedColor::WHITE, RedColor::BLACK, 32.0f);
        m_pPrePixelLightingShader = (pcTerrainShader*)pcRedShader::Create(
            0x74DBC80E /* "Terrain" */, uiFlags, 0, 0, 0, RedColor::WHITE, RedColor::BLACK, 32.0f);
    }
    m_pPrePixelLightingShader->EnablePrePixelLightingPass(true);
    m_pPrePixelLightingShader->EnableDetail(true);
}

// FUNCTION: SWBF2 0x006d60c0
void RedTerrain::_ReadPatchInfo(PblFileChunk& chunk)
{
    _TerrainRenderable.Read(chunk);
}

// FUNCTION: SWBF2 0x006d6440
void TerrainRenderable::_Reset()
{
    Deinit();
    m_fDetailMaxDistance = 32.0f;
    m_fLowResMapSize = 0.0f;
    m_iLowResPatchSize = 0;
    m_iLowResPatchesPerDimension = 0;
    m_fLowResPointsPerMeter = 0.0f;
    m_bEnablePixelLighting = false;
    m_bEnableFarDetail = false;
    m_eQuality = (QualityT)-1;
}

// FUNCTION: SWBF2 0x006d3990
void TerrainRenderable::Read(PblFileChunk& chunk)
{
    _Reset();
    pcTerrainShader2::_InitMaps((float)RedTerrain::_iPointsPerDimension * RedTerrain::_fMetersPerPoint);

    m_aPatches = new Patch[RedTerrain::_iNumPatches];
    RedTerrain::AdjustTerrainMemoryUsage(RedTerrain::_iNumPatches * sizeof(Patch));

    for (int i = 0; i < RedTerrain::_iNumLayers; ++i)
    {
        float s = 1.0f / RedTerrain::_fLayerMetersPerTexture[i];
        switch (RedTerrain::_uiLayerTextureOrientation[i])
        {
        case 1:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(s, 0.0f, 0.0f, 0.0f),
                                         PblVector4(0.0f, s, 0.0f, 0.0f));
            break;
        case 2:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(0.0f, s, 0.0f, 0.0f),
                                         PblVector4(0.0f, 0.0f, s, 0.0f));
            break;
        case 3:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(0.0f, 0.0f, s, 0.0f),
                                         PblVector4(s, 0.0f, 0.0f, 0.0f));
            break;
        case 4:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(0.0f, s, 0.0f, 0.0f),
                                         PblVector4(s, 0.0f, 0.0f, 0.0f));
            break;
        case 5:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(0.0f, 0.0f, s, 0.0f),
                                         PblVector4(0.0f, s, 0.0f, 0.0f));
            break;
        case 6:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(-s, 0.0f, 0.0f, 0.0f),
                                         PblVector4(0.0f, 0.0f, s, 0.0f));
            break;
        case 7:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(-s, 0.0f, 0.0f, 0.0f),
                                         PblVector4(0.0f, s, 0.0f, 0.0f));
            break;
        case 8:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(0.0f, -s, 0.0f, 0.0f),
                                         PblVector4(0.0f, 0.0f, s, 0.0f));
            break;
        case 9:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(0.0f, 0.0f, -s, 0.0f),
                                         PblVector4(s, 0.0f, 0.0f, 0.0f));
            break;
        case 10:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(0.0f, -s, 0.0f, 0.0f),
                                         PblVector4(s, 0.0f, 0.0f, 0.0f));
            break;
        case 11:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(0.0f, 0.0f, -s, 0.0f),
                                         PblVector4(0.0f, s, 0.0f, 0.0f));
            break;
        default:
            pcTerrainShader2::_InitLayer(i, RedTerrain::_uiLayerTextureHash[i], PblVector4(s, 0.0f, 0.0f, 0.0f),
                                         PblVector4(0.0f, 0.0f, s, 0.0f));
            break;
        }
    }

    m_fDetailMaxDistance = (float)RedTerrain::_iPatchPointsPerDimension * RedTerrain::_fMetersPerPoint;
    if (m_fDetailMaxDistance > 48.0f)
    {
        m_fDetailMaxDistance = 48.0f;
    }
    else if (m_fDetailMaxDistance < 16.0f)
    {
        m_fDetailMaxDistance = 16.0f;
    }
    pcTerrainShader2::_InitDetail(RedTerrain::_uiDetailTextureHash,
                                  PblVector4(RedTerrain::_fPointsPerMeter, 0.0f, 0.0f, 0.0f),
                                  PblVector4(0.0f, 0.0f, RedTerrain::_fPointsPerMeter, 0.0f), m_fDetailMaxDistance);

    int iPatch = 0;
    unsigned int uiDefaultStartIndex = 0;
    unsigned int uiDefaultNumIndices = 0;
    PblBox lowResBox;

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();

        switch (child._iId)
        {
        case CHUNK_ID('C', 'O', 'M', 'N'):
        {
            while (!child.IsDone())
            {
                PblFileChunk leaf = child.ReadNextChild();
                if (leaf._iId == CHUNK_ID('I', 'B', 'U', 'F'))
                {
                    uiDefaultNumIndices = leaf.Read32();
                    m_pIBDefault = pcRedIndexBuffer::Create(uiDefaultNumIndices, true, true);
                    leaf.ReadData(m_pIBDefault->Lock(uiDefaultNumIndices, uiDefaultStartIndex),
                                  uiDefaultNumIndices * sizeof(unsigned short));
                    m_pIBDefault->Unlock();
                }
            }
            break;
        }
        case CHUNK_ID('P', 'T', 'C', 'H'):
        {
            unsigned int auiLayer[4] = { 0, 0, 0, 0 };
            unsigned int auiFormat[3] = { 0, 0, 0 };
            unsigned int auiNumVertices[3] = { 0, 0, 0 };
            unsigned int auiStartVertex[3] = { 0, 0, 0 };
            pcRedVertexBuffer* apVB[3] = { 0, 0, 0 };
            unsigned int auiNumIndices[3] = { 0, 0, 0 };
            pcRedIndexBuffer* apIB[3] = { 0, 0, 0 };
            unsigned int auiStartIndex[3] = { 0, 0, 0 };
            unsigned int uiNumLayers = 0;
            unsigned int uiNumPasses = 0;
            int iNumVB = 0;
            int iNumUncompressedVB = 0;
            int iHeightShift = 0;
            Patch& patch = m_aPatches[iPatch];

            while (!child.IsDone())
            {
                PblFileChunk leaf = child.ReadNextChild();
                switch (leaf._iId)
                {
                case CHUNK_ID('I', 'N', 'F', 'O'):
                {
                    uiNumLayers = leaf.Read8();
                    for (unsigned char j = 0; j < uiNumLayers; ++j)
                    {
                        unsigned char uiLayer = leaf.Read8();
                        if (uiLayer < 16)
                        {
                            auiLayer[j] = uiLayer;
                        }
                    }
                    if (RedRenderer::pcGetRenderCaps().pipeline == PIPELINE_FIXED && uiNumLayers > 2)
                    {
                        uiNumPasses = uiNumLayers - 1;
                    }
                    else
                    {
                        uiNumPasses = 1;
                    }
                    patch.uiNumPasses = uiNumPasses;
                    patch.apPrim = new pcRedPrimitive*[uiNumPasses];
                    RedTerrain::AdjustTerrainMemoryUsage(uiNumPasses * sizeof(pcRedPrimitive));
                    patch.apShader = new pcTerrainShader*[uiNumPasses];
                    patch.apShader2 = new pcTerrainShader2*[uiNumPasses];
                    RedTerrain::AdjustTerrainMemoryUsage(uiNumPasses * sizeof(pcTerrainShader));
                    patch.iHeightMin = leaf.Read16();
                    patch.iHeightMax = leaf.Read16();
                    iHeightShift = leaf.Read16();
                    break;
                }
                case CHUNK_ID('V', 'B', 'U', 'F'):
                {
                    unsigned int uiNumVertices = leaf.Read32();
                    unsigned int uiVertexSize = leaf.Read32();
                    unsigned int uiFormat = leaf.Read32();
                    bool bUncompressed = (uiFormat & 0xF000) == 0;
                    if (bUncompressed)
                    {
                        ++iNumUncompressedVB;
                    }
                    if ((RedRenderer::pcGetGeometryMode() == GEOMETRYMODE_UNCOMPRESSED) == bUncompressed)
                    {
                        auiNumVertices[iNumVB] = uiNumVertices;
                        auiFormat[iNumVB] = uiFormat;
                        if (uiNumVertices)
                        {
                            pcRedVertexBuffer* pVB = pcRedVertexBuffer::Create(uiNumVertices, uiVertexSize, true, true);
                            apVB[iNumVB] = pVB;
                            leaf.ReadData(pVB->Lock(uiNumVertices, auiStartVertex[iNumVB]),
                                                  uiNumVertices * uiVertexSize);
                            pVB->Unlock();
                        }
                        ++iNumVB;
                    }
                    break;
                }
                case CHUNK_ID('I', 'B', 'U', 'F'):
                {
                    if (iNumVB == iNumUncompressedVB)
                    {
                        unsigned int uiNumIndices = leaf.Read32();
                        auiNumIndices[iNumVB - 1] = uiNumIndices;
                        if (uiNumIndices)
                        {
                            apIB[iNumVB - 1] = pcRedIndexBuffer::Create(uiNumIndices, true, true);
                            leaf.ReadData(apIB[iNumVB - 1]->Lock(uiNumIndices, auiStartIndex[iNumVB - 1]),
                                          uiNumIndices * sizeof(unsigned short));
                            apIB[iNumVB - 1]->Unlock();
                        }
                    }
                    break;
                }
                }
            }

            float fSize = (float)RedTerrain::_iPatchPointsPerDimension * RedTerrain::_fMetersPerPoint;
            float fX = RedTerrain::_CalcWorldPosX(
                (iPatch & RedTerrain::_iPatchesPerDimensionMask) * RedTerrain::_iPatchPointsPerDimension);
            float fZ = RedTerrain::_CalcWorldPosZ(
                (iPatch >> RedTerrain::_iPatchesPerDimensionLog2) * RedTerrain::_iPatchPointsPerDimension);
            float fHeightMin = (float)patch.iHeightMin;
            PblVector3 vMin(fX, fHeightMin * RedTerrain::_fHeightScale, fZ);
            PblVector3 vMax(vMin.x + fSize,
                            (65535.0f / (float)(1 << iHeightShift) + fHeightMin) * RedTerrain::_fHeightScale,
                            vMin.z + fSize);
            PblBox box(vMin, vMax);

            for (unsigned int uiPass = 0; uiPass < uiNumPasses; ++uiPass)
            {
                RedRenderer::pcGetRenderCaps();
                if (!apVB[uiPass])
                {
                    patch.apPrim[uiPass] = 0;
                    patch.apShader[uiPass] = 0;
                    patch.apShader2[uiPass] = 0;
                    continue;
                }

                if (apIB[uiPass])
                {
                    patch.apPrim[uiPass] = pcRedPrimitive::Create(
                        D3DPT_TRIANGLESTRIP, auiNumIndices[uiPass] - 2, auiFormat[uiPass], auiNumVertices[uiPass],
                        auiStartVertex[uiPass], apVB[uiPass], auiStartIndex[uiPass], apIB[uiPass]);
                    apVB[uiPass]->Release();
                    apIB[uiPass]->Release();
                }
                else
                {
                    patch.apPrim[uiPass] = pcRedPrimitive::Create(
                        D3DPT_TRIANGLESTRIP, uiDefaultNumIndices - 2, auiFormat[uiPass], auiNumVertices[uiPass],
                        auiStartVertex[uiPass], apVB[uiPass], uiDefaultStartIndex, m_pIBDefault);
                    apVB[uiPass]->Release();
                }
                patch.apPrim[uiPass]->SetBoundingBox(box);

                if (uiPass)
                {
                    patch.apShader2[uiPass] = (pcTerrainShader2*)pcRedShader::Create(
                        PblHash("Terrain2"), auiFormat[uiPass], &auiLayer[uiPass + 1], 1, 0, RedColor::WHITE,
                        RedColor::BLACK, 32.0f);
                    patch.apShader[uiPass] = (pcTerrainShader*)pcRedShader::Create(
                        0x74DBC80E /* "Terrain" */, auiFormat[uiPass], &auiLayer[uiPass + 1], 1, 0, RedColor::WHITE,
                        RedColor::BLACK, 32.0f);
                    patch.apShader[uiPass]->SetMultiLayer(uiPass == 1 ? 1 : 2);
                }
                else
                {
                    unsigned int uiNumParams =
                        (RedRenderer::pcGetRenderCaps().pipeline == PIPELINE_FIXED && uiNumLayers >= 2) ? 2 : uiNumLayers;
                    patch.apShader2[0] = (pcTerrainShader2*)pcRedShader::Create(
                        PblHash("Terrain2"), auiFormat[0], auiLayer, uiNumParams, 0, RedColor::WHITE,
                        RedColor::BLACK, 32.0f);
                }
                unsigned int uiNumParams =
                    (RedRenderer::pcGetRenderCaps().pipeline == PIPELINE_FIXED && uiNumLayers >= 2) ? 2 : uiNumLayers;
                patch.apShader[0] = (pcTerrainShader*)pcRedShader::Create(
                    0x74DBC80E /* "Terrain" */, auiFormat[0], auiLayer, uiNumParams, 0, RedColor::WHITE, RedColor::BLACK,
                    32.0f);
            }
            ++iPatch;
            break;
        }
        case CHUNK_ID('L', 'O', 'W', 'R'):
        {
            while (!child.IsDone())
            {
                PblFileChunk sub = child.ReadNextChild();
                if (sub._iId == CHUNK_ID('I', 'N', 'F', 'O'))
                {
                    m_iLowResPatchesPerDimension = sub.Read32();
                    m_iLowResPatchSize = sub.Read32();
                    m_fLowResMapSize = sub.ReadFloat();
                    m_fLowResPointsPerMeter =
                        (float)(m_iLowResPatchSize * m_iLowResPatchesPerDimension) / m_fLowResMapSize;
                    float fMinY = sub.ReadFloat();
                    float fMaxY = sub.ReadFloat();
                    lowResBox = PblBox(-RedTerrain::_fTerrainCenterX, fMinY, -RedTerrain::_fTerrainCenterZ,
                                       m_fLowResMapSize - RedTerrain::_fTerrainCenterX, fMaxY,
                                       m_fLowResMapSize - RedTerrain::_fTerrainCenterZ);
                    m_aLowResPatches = new LowResPatch[m_iLowResPatchesPerDimension * m_iLowResPatchesPerDimension];
                    RedTerrain::AdjustTerrainMemoryUsage(
                        m_iLowResPatchesPerDimension * m_iLowResPatchesPerDimension * sizeof(LowResPatch));
                }
                else if (sub._iId == CHUNK_ID('P', 'C', 'H', 'S'))
                {
                    pcRedIndexBuffer* pSharedIB = 0;
                    unsigned int uiSharedStartIndex = 0;
                    unsigned int uiSharedNumIndices = 0;
                    int iNumLowResPatches = 0;
                    while (!sub.IsDone())
                    {
                        PblFileChunk lp = sub.ReadNextChild();
                        if (lp._iId == CHUNK_ID('L', 'P', 'T', 'C'))
                        {
                            unsigned int uiFormat = 0;
                            pcRedIndexBuffer* pIB = 0;
                            unsigned int uiStartVertex = 0;
                            unsigned int uiNumVertices = 0;
                            unsigned int uiStartIndex = 0;
                            pcRedVertexBuffer* pVB = 0;
                            unsigned int uiNumIndices = 0;
                            while (!lp.IsDone())
                            {
                                PblFileChunk leaf = lp.ReadNextChild();
                                switch (leaf._iId)
                                {
                                case CHUNK_ID('I', 'N', 'F', 'O'):
                                {
                                    float fXMin = leaf.ReadFloat();
                                    float fXMax = leaf.ReadFloat();
                                    float fYMin = leaf.ReadFloat();
                                    float fYMax = leaf.ReadFloat();
                                    float fZMin = leaf.ReadFloat();
                                    float fZMax = leaf.ReadFloat();
                                    m_aLowResPatches[iNumLowResPatches].bbox = PblBox(fXMin, fYMin, fZMin, fXMax, fYMax, fZMax);
                                    break;
                                }
                                case CHUNK_ID('V', 'B', 'U', 'F'):
                                {
                                    unsigned int uiCount = leaf.Read32();
                                    unsigned int uiVertexSize = leaf.Read32();
                                    unsigned int uiVBFormat = leaf.Read32();
                                    if (pVB)
                                    {
                                        continue;
                                    }
                                    if (RedRenderer::pcGetGeometryMode() == GEOMETRYMODE_UNCOMPRESSED &&
                                        (uiVBFormat & 0xF000))
                                    {
                                        continue;
                                    }
                                    uiNumVertices = uiCount;
                                    uiFormat = uiVBFormat;
                                    if (uiCount)
                                    {
                                        pVB = pcRedVertexBuffer::Create(uiCount, uiVertexSize, true, true);
                                        leaf.ReadData(pVB->Lock(uiCount, uiStartVertex), uiCount * uiVertexSize);
                                        pVB->Unlock();
                                    }
                                    break;
                                }
                                case CHUNK_ID('I', 'B', 'U', 'F'):
                                {
                                    uiNumIndices = leaf.Read32();
                                    if (uiNumIndices)
                                    {
                                        pIB = pcRedIndexBuffer::Create(uiNumIndices, true, true);
                                        leaf.ReadData(pIB->Lock(uiNumIndices, uiStartIndex),
                                                      uiNumIndices * sizeof(unsigned short));
                                        pIB->Unlock();
                                    }
                                    break;
                                }
                                }
                            }

                            if (pIB)
                            {
                                m_aLowResPatches[iNumLowResPatches].pPrim = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, uiNumIndices - 2, uiFormat,
                                                                        uiNumVertices, uiStartVertex, pVB, uiStartIndex,
                                                                        pIB);
                                pIB->Release();
                            }
                            else
                            {
                                m_aLowResPatches[iNumLowResPatches].pPrim = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, uiSharedNumIndices - 2,
                                                                        uiFormat, uiNumVertices, uiStartVertex, pVB,
                                                                        uiSharedStartIndex, pSharedIB);
                            }
                            pVB->Release();
                            if (m_aLowResPatches[iNumLowResPatches].pPrim)
                            {
                                m_aLowResPatches[iNumLowResPatches].pPrim->SetBoundingBox(lowResBox);
                            }
                            ++iNumLowResPatches;
                        }
                        else if (lp._iId == CHUNK_ID('C', 'O', 'M', 'N'))
                        {
                            while (!lp.IsDone())
                            {
                                PblFileChunk leaf = lp.ReadNextChild();
                                if (leaf._iId == CHUNK_ID('I', 'B', 'U', 'F'))
                                {
                                    uiSharedNumIndices = leaf.Read32();
                                    pSharedIB = pcRedIndexBuffer::Create(uiSharedNumIndices, true, true);
                                    leaf.ReadData(pSharedIB->Lock(uiSharedNumIndices, uiSharedStartIndex),
                                                  uiSharedNumIndices * sizeof(unsigned short));
                                    pSharedIB->Unlock();
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        }
    }

    unsigned int uiFarFlags2 = RedRenderer::pcGetGeometryMode() == GEOMETRYMODE_COMPRESSED ? 0x5122 : 0x122;
    m_pFarShader2 = (pcTerrainShader2*)pcRedShader::Create(PblHash("Terrain2"), uiFarFlags2, 0, 0, 0,
                                                           RedColor::WHITE, RedColor::BLACK, 32.0f);
    unsigned int uiFarFlags = RedRenderer::pcGetGeometryMode() == GEOMETRYMODE_COMPRESSED ? 0x1002 : 0x2;
    m_pFarShader = (pcTerrainShader*)pcRedShader::Create(0x74DBC80E /* "Terrain" */, uiFarFlags, 0, 0, 0, RedColor::WHITE,
                                                         RedColor::BLACK, 32.0f);
    if (m_pFarShader)
    {
        m_pFarShader->EnableFarRendering(true, m_bEnableFarDetail);
    }
}
