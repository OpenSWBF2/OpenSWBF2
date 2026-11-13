#include <d3dx9math.h>
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/RedCamera.h"
#include "Battlefront2/Platform/Pc/pcLightBeam.h"
const RedColor Interpolate(const RedColor& a, const RedColor& b, float t);
#include "Battlefront2/Source/LightsaberTrailEffect.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblMath.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

// FUNCTION: SWBF2 0x004213e0
void LightsaberTrailEffect::_PlatformInit()
{
    if (RedRenderer::pcGetGeometryMode() == GEOMETRYMODE_COMPRESSED)
    {
        m_pShader = pcRedShader::Create(
            0xE8651328u /* "LightBeam" */, 0x1082,
            0, 0, 0x10040, RedColor::WHITE, RedColor::BLACK, 32.0f);
    }
    else
    {
        m_pShader = pcRedShader::Create(
            0xE8651328u /* "LightBeam" */, 0x82,
            0, 0, 0x10040, RedColor::WHITE, RedColor::BLACK, 32.0f);
    }
}

// FUNCTION: SWBF2 0x00421440
void LightsaberTrailEffect::_PlatformCleanup()
{
    m_pShader->Release();
    m_pShader = 0;
}


#pragma pack(push, 1)
struct VertexU
{
    PblVector3 pos;
    RedColor color;

    void SetPos(const PblVector3& v) { pos = v; }
    void SetColor(const RedColor& c) { color = c; }
};
#pragma pack(pop)

// FUNCTION: SWBF2 0x00421480
void LightsaberTrailEffect::RenderLightsaberTrail(const LightsaberTrail* pTrail, unsigned int uiFlags) const
{
    unsigned char auiNumSteps[LightsaberTrail::MAX_SAMPLES_PER_TRAIL];
    float fTotalDist = 0.0f;
    unsigned int uiNumSteps = pTrail->GetTotalNumSteps(m_fLIGHTSABER_TRAIL_MAX_TIME_PER_STEP,
                                         m_fLIGHTSABER_TRAIL_MAX_DISTANCE_PER_STEP,
                                         auiNumSteps, &fTotalDist);
    if (!uiNumSteps)
    {
        return;
    }
    if (pTrail->IsTrailOn())
    {
        uiNumSteps += 2;
    }

    const LightsaberTrailClass* pClass = pTrail->m_pClass;
    RedColor colorTrail;
    colorTrail.a = 0xff;
    const RedColor colorHead = pClass->m_colorHead;
    const float fWidth = pClass->m_fWidth;
    const float fLength = pClass->m_fLength;
    colorTrail.b = (unsigned char)(int)((pClass->m_colorTrail.b * pClass->m_colorTrail.a + 0.5f) * (1.0f / 255.0f));
    colorTrail.g = (unsigned char)(int)((pClass->m_colorTrail.g * pClass->m_colorTrail.a + 0.5f) * (1.0f / 255.0f));
    colorTrail.r = (unsigned char)(int)((pClass->m_colorTrail.r * pClass->m_colorTrail.a + 0.5f) * (1.0f / 255.0f));
    PblBox bbox = pTrail->GetBoundingBox(fLength);
    const PblVector3& vCamZ = RedRenderer::GetCamera()->GetMatrix().GetZAxis();
    const float fPersistTime = m_fLIGHTSABER_TRAIL_PERSIST_TIME;
    const float fFadeTime = m_fLIGHTSABER_TRAIL_FADE_TIME;
    const float fFadeTimeInv = 1.0f / (fPersistTime - fFadeTime);
    const float fFadeDist = m_fLIGHTSABER_TRAIL_FADE_DISTANCE;
    const float fFadeDistInv = 1.0f / fFadeDist;
    const unsigned int uiNumVerts = 6 * uiNumSteps - 2;

    const int iGeometryMode = RedRenderer::pcGetGeometryMode();
    pcRedPrimitive* pPrim;
    if (iGeometryMode == GEOMETRYMODE_COMPRESSED)
    {
        pPrim = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x1082, sizeof(VertexC), uiNumVerts, 0, false);
        CompressedPos::SetBoundingBox(bbox);
    }
    else
    {
        pPrim = pcRedPrimitive::Create(D3DPT_TRIANGLESTRIP, 0x82, sizeof(VertexU), uiNumVerts, 0, false);
    }
    pPrim->m_bbox = bbox;
    RedColor colorEdge = colorTrail;
    colorEdge.a = 0;
    RedColor color = colorTrail;
    float fTime;
    PblVector3 vPos;
    PblVector3 vPosEnd;
    PblVector3 vCap;
    if (iGeometryMode == GEOMETRYMODE_COMPRESSED)
    {
        VertexC* pFront = (VertexC*)pPrim->BeginVertexData();
        if (!pFront)
        {
            return;
        }
        VertexC* pBack = pFront + 4 * uiNumSteps - 2;
        VertexC* pThird = pBack + 1;
        color.a = 0;

        vPos = pTrail->GetPos(0);
        vPosEnd = vPos + pTrail->GetDir(0) * fLength;
        vCap = pTrail->GetDir(0) - vCamZ * DotProduct(pTrail->GetDir(0), vCamZ);
        if (SquaredLength(vCap) > 0.01f)
        {
            vCap = Normalize(vCap);
        }
        else
        {
            vCap *= 10.0f;
        }
        pFront->SetPos(vPos - vCap * fWidth);
        pFront->SetColor(colorEdge);
        ++pFront;
        pFront->SetPos(vPos);
        pFront->SetColor(color);
        ++pFront;
        pBack->SetPos(vPosEnd);
        pBack->SetColor(color);
        --pBack;
        pBack->SetPos(vPos);
        pBack->SetColor(color);
        --pBack;
        pThird->SetPos(vPosEnd + vCap * fWidth);
        pThird->SetColor(colorEdge);
        ++pThird;

        float fTimePrev = pTrail->GetTime(0);
        float fAgePrev = pTrail->GetTimer() - fTimePrev;
        bool bCoreDone = !pTrail->IsTrailOn();
        float fDist = 0.0f;
        for (unsigned int i = 1; i < pTrail->GetNumSamples(); ++i)
        {
            const float fSampleDist = pTrail->GetDist(i);
            const bool bMoved = fSampleDist > 0.0f;
            bool bNextVisible;
            if (i == pTrail->GetNumSamples() - 1)
            {
                bNextVisible = pTrail->IsTrailOn();
            }
            else
            {
                bNextVisible = auiNumSteps[i + 1] != 0;
            }
            const float fSampleTime = pTrail->GetTime(i);
            const int iNumSteps = auiNumSteps[i];
            const float fStepInv = 1.0f / iNumSteps;
            const float fTimeStep = (fSampleTime - fTimePrev) * fStepInv;
            const float fDistStep = fStepInv * fSampleDist;
            for (int j = 0; j < iNumSteps; ++j)
            {
                fTime = fTimePrev + fTimeStep;
                fDist += fDistStep;
                color = colorTrail;
                const float fAge = pTrail->GetTimer() - fTime;
                const PblVector3 vDir = pTrail->GetInterpolatedDir(fTime);
                vCap = vDir - vCamZ * DotProduct(vDir, vCamZ);
                if (SquaredLength(vCap) > 0.01f)
                {
                    vCap = Normalize(vCap);
                }
                else
                {
                    vCap *= 10.0f;
                }

                if (!bCoreDone && fAge <= m_fLIGHTSABER_TRAIL_CORE_MAX_TIME &&
                    fDist > fTotalDist - m_fLIGHTSABER_TRAIL_CORE_MAX_DISTANCE)
                {
                    const PblVector3 vPosPrev = vPos;
                    const PblVector3 vPosEndPrev = vPosEnd;
                    const PblVector3 vPosNew = pTrail->GetInterpolatedPos(fTime);
                    const PblVector3 vPosEndNew = vPosPrev + vDir * fLength;
                    const float fPosMove = Length(vPosNew - vPosPrev);
                    const float fTipMove = Length(vPosEndNew - vPosEndPrev);
                    float fCore;
                    if (fAgePrev > m_fLIGHTSABER_TRAIL_CORE_MAX_TIME)
                    {
                        fCore = (m_fLIGHTSABER_TRAIL_CORE_MAX_TIME - fAge) / (fAgePrev - fAge);
                    }
                    else
                    {
                        fCore = (fDist - (fTotalDist - m_fLIGHTSABER_TRAIL_CORE_MAX_DISTANCE)) /
                                (fPosMove > fTipMove ? fPosMove : fTipMove);
                        if (fCore > 1.0f)
                        {
                            fCore = 1.0f;
                        }
                    }
                    float fPosWidth;
                    if (fPosMove > 0.01f)
                    {
                        fPosWidth = m_fLIGHTSABER_TRAIL_CORE_WIDTH * fWidth * 0.5f / fPosMove;
                    }
                    else
                    {
                        fPosWidth = 0.0f;
                    }
                    float fTipWidth;
                    if (fTipMove > 0.01f)
                    {
                        fTipWidth = m_fLIGHTSABER_TRAIL_CORE_WIDTH * fWidth * 0.5f / fTipMove;
                    }
                    else
                    {
                        fTipWidth = 0.0f;
                    }

                    float fPos = fPosWidth + fCore;
                    PblVector3 vPosNext = Interpolate(vPosNew, vPosPrev, fPos);
                    float fTip = fTipWidth + fCore;
                    PblVector3 vPosEndNext = Interpolate(vPosEndNew, vPosEndPrev, fTip);
                    pFront[0].pos.SetPos(vPosNext - vCap * fWidth);
                    pFront[0].SetColor(colorEdge);
                    pFront[1].pos.SetPos(vPosNext);
                    pFront[1].SetColor(colorTrail);
                    pBack[0].pos.SetPos(vPosEndNext);
                    pBack[0].SetColor(colorTrail);
                    pBack[-1].pos.SetPos(vPosNext);
                    pBack[-1].SetColor(colorTrail);
                    pBack -= 2;
                    pThird[0].pos.SetPos(vPosEndNext);
                    pThird[0].SetColor(colorTrail);
                    pThird[1].pos.SetPos(vPosEndNext + vCap * fWidth);
                    pThird[1].SetColor(colorEdge);
                    pThird += 2;

                    if (i == pTrail->GetNumSamples() - 1 && j == iNumSteps - 1)
                    {
                        if (fPosWidth > fCore)
                        {
                            fPosWidth = fCore;
                        }
                        if (fTipWidth > fCore)
                        {
                            fTipWidth = fCore;
                        }
                    }
                    fPos = fCore - fPosWidth;
                    fTip = fCore - fTipWidth;
                    vPos = Interpolate(vPosNew, vPosPrev, fPos);
                    vPosEnd = Interpolate(vPosEndNew, vPosEndPrev, fTip);
                    color = colorHead;
                    pFront[2].pos.SetPos(vPos - vCap * fWidth);
                    pFront[2].SetColor(colorEdge);
                    pFront[3].pos.SetPos(vPos);
                    pFront[3].SetColor(color);
                    pFront += 4;
                    pBack[0].pos.SetPos(vPosEnd);
                    pBack[0].SetColor(color);
                    pBack[-1].pos.SetPos(vPos);
                    pBack[-1].SetColor(color);
                    pBack -= 2;
                    pThird[0].pos.SetPos(vPosEnd);
                    pThird[0].SetColor(color);
                    pThird[1].pos.SetPos(vPosEnd + vCap * fWidth);
                    pThird[1].SetColor(colorEdge);
                    pThird += 2;

                    if (fPos > 0.0f)
                    {
                        vPos = vPosNew;
                    }
                    if (fTip > 0.0f)
                    {
                        vPosEnd = vPosEndNew;
                    }
                    bCoreDone = true;
                }
                else
                {
                    vPos = pTrail->GetInterpolatedPos(fTime);
                    vPosEnd = vPos + vDir * fLength;
                }

                if (bCoreDone && pTrail->IsTrailOn())
                {
                    color = colorHead;
                }
                else if (fAge >= fPersistTime || !bMoved || (!bNextVisible && j == iNumSteps - 1))
                {
                    color.a = 0;
                }
                else
                {
                    float fFade = 1.0f;
                    if (fAge > fFadeTime)
                    {
                        fFade = (fPersistTime - fAge) * fFadeTimeInv * ((fPersistTime - fAge) * fFadeTimeInv);
                    }
                    if (fFadeDist > fDist)
                    {
                        fFade = fDist * fFadeDistInv * fFade;
                    }
                    if (fFade < 1.0f)
                    {
                        color.a = (unsigned char)(int)(color.a * fFade);
                    }
                }

                pFront[0].pos.SetPos(vPos - vCap * fWidth);
                pFront[0].SetColor(colorEdge);
                pFront[1].pos.SetPos(vPos);
                pFront[1].SetColor(color);
                pFront += 2;
                pBack[0].pos.SetPos(vPosEnd);
                pBack[0].SetColor(color);
                pBack[-1].pos.SetPos(vPos);
                pBack[-1].SetColor(color);
                pBack -= 2;
                pThird[0].pos.SetPos(vPosEnd);
                pThird[0].SetColor(color);
                pThird[1].pos.SetPos(vPosEnd + vCap * fWidth);
                pThird[1].SetColor(colorEdge);
                pThird += 2;

                fAgePrev = fAge;
                fTimePrev = fTime;
            }
            fTimePrev = fSampleTime;
        }
    }
    else
    {
        VertexU* pFront = (VertexU*)pPrim->BeginVertexData();
        if (!pFront)
        {
            return;
        }
        VertexU* pBack = pFront + 4 * uiNumSteps - 2;
        VertexU* pThird = pBack + 1;
        color.a = 0;

        vPos = pTrail->GetPos(0);
        vPosEnd = vPos + pTrail->GetDir(0) * fLength;
        vCap = pTrail->GetDir(0) - vCamZ * DotProduct(pTrail->GetDir(0), vCamZ);
        if (SquaredLength(vCap) > 0.01f)
        {
            vCap = Normalize(vCap);
        }
        else
        {
            vCap *= 10.0f;
        }
        pFront->SetPos(vPos - vCap * fWidth);
        pFront->SetColor(colorEdge);
        ++pFront;
        pFront->SetPos(vPos);
        pFront->SetColor(color);
        ++pFront;
        pBack->SetPos(vPosEnd);
        pBack->SetColor(color);
        --pBack;
        pBack->SetPos(vPos);
        pBack->SetColor(color);
        --pBack;
        pThird->SetPos(vPosEnd + vCap * fWidth);
        pThird->SetColor(colorEdge);
        ++pThird;

        bool bCoreDone = !pTrail->IsTrailOn();
        float fTimePrev = pTrail->GetTime(0);
        float fAgePrev = pTrail->GetTimer() - fTimePrev;
        float fDist = 0.0f;
        for (unsigned int i = 1; i < pTrail->GetNumSamples(); ++i)
        {
            const float fSampleDist = pTrail->GetDist(i);
            const bool bMoved = fSampleDist > 0.0f;
            bool bNextVisible;
            if (i == pTrail->GetNumSamples() - 1)
            {
                bNextVisible = pTrail->IsTrailOn();
            }
            else
            {
                bNextVisible = auiNumSteps[i + 1] != 0;
            }
            const float fSampleTime = pTrail->GetTime(i);
            const int iNumSteps = auiNumSteps[i];
            const float fStepInv = 1.0f / iNumSteps;
            const float fTimeStep = (fSampleTime - fTimePrev) * fStepInv;
            const float fDistStep = fStepInv * fSampleDist;
            for (int j = 0; j < iNumSteps; ++j)
            {
                fTime = fTimePrev + fTimeStep;
                fDist += fDistStep;
                color = colorTrail;
                const float fAge = pTrail->GetTimer() - fTime;
                const PblVector3 vDir = pTrail->GetInterpolatedDir(fTime);
                vCap = vDir - vCamZ * DotProduct(vDir, vCamZ);
                if (SquaredLength(vCap) > 0.01f)
                {
                    vCap = Normalize(vCap);
                }
                else
                {
                    vCap *= 10.0f;
                }

                if (!bCoreDone && fAge <= m_fLIGHTSABER_TRAIL_CORE_MAX_TIME &&
                    fDist > fTotalDist - m_fLIGHTSABER_TRAIL_CORE_MAX_DISTANCE)
                {
                    const PblVector3 vPosPrev = vPos;
                    const PblVector3 vPosEndPrev = vPosEnd;
                    const PblVector3 vPosNew = pTrail->GetInterpolatedPos(fTime);
                    const PblVector3 vPosEndNew = vPosPrev + vDir * fLength;
                    const float fPosMove = Length(vPosNew - vPosPrev);
                    const float fTipMove = Length(vPosEndNew - vPosEndPrev);
                    float fCore;
                    if (fAgePrev > m_fLIGHTSABER_TRAIL_CORE_MAX_TIME)
                    {
                        fCore = (m_fLIGHTSABER_TRAIL_CORE_MAX_TIME - fAge) / (fAgePrev - fAge);
                    }
                    else
                    {
                        fCore = (fDist - (fTotalDist - m_fLIGHTSABER_TRAIL_CORE_MAX_DISTANCE)) /
                                (fPosMove > fTipMove ? fPosMove : fTipMove);
                        if (fCore > 1.0f)
                        {
                            fCore = 1.0f;
                        }
                    }
                    float fPosWidth;
                    if (fPosMove > 0.01f)
                    {
                        fPosWidth = m_fLIGHTSABER_TRAIL_CORE_WIDTH * fWidth * 0.5f / fPosMove;
                    }
                    else
                    {
                        fPosWidth = 0.0f;
                    }
                    float fTipWidth;
                    if (fTipMove > 0.01f)
                    {
                        fTipWidth = m_fLIGHTSABER_TRAIL_CORE_WIDTH * fWidth * 0.5f / fTipMove;
                    }
                    else
                    {
                        fTipWidth = 0.0f;
                    }

                    float fPos = fPosWidth + fCore;
                    PblVector3 vPosNext = Interpolate(vPosNew, vPosPrev, fPos);
                    float fTip = fTipWidth + fCore;
                    PblVector3 vPosEndNext = Interpolate(vPosEndNew, vPosEndPrev, fTip);
                    pFront->SetPos(vPosNext - vCap * fWidth);
                    pFront->SetColor(colorEdge);
                    ++pFront;
                    pFront->SetPos(vPosNext);
                    pFront->SetColor(colorTrail);
                    ++pFront;
                    pBack->SetPos(vPosEndNext);
                    pBack->SetColor(colorTrail);
                    --pBack;
                    pBack->SetPos(vPosNext);
                    pBack->SetColor(colorTrail);
                    --pBack;
                    pThird->SetPos(vPosEndNext);
                    pThird->SetColor(colorTrail);
                    ++pThird;
                    pThird->SetPos(vPosEndNext + vCap * fWidth);
                    pThird->SetColor(colorEdge);
                    ++pThird;

                    if (i == pTrail->GetNumSamples() - 1 && j == iNumSteps - 1)
                    {
                        if (fPosWidth > fCore)
                        {
                            fPosWidth = fCore;
                        }
                        if (fTipWidth > fCore)
                        {
                            fTipWidth = fCore;
                        }
                    }
                    fPos = fCore - fPosWidth;
                    fTip = fCore - fTipWidth;
                    vPos = Interpolate(vPosNew, vPosPrev, fPos);
                    vPosEnd = Interpolate(vPosEndNew, vPosEndPrev, fTip);
                    color = colorHead;
                    pFront->SetPos(vPos - vCap * fWidth);
                    pFront->SetColor(colorEdge);
                    ++pFront;
                    pFront->SetPos(vPos);
                    pFront->SetColor(color);
                    ++pFront;
                    pBack->SetPos(vPosEnd);
                    pBack->SetColor(color);
                    --pBack;
                    pBack->SetPos(vPos);
                    pBack->SetColor(color);
                    --pBack;
                    pThird->SetPos(vPosEnd);
                    pThird->SetColor(color);
                    ++pThird;
                    pThird->SetPos(vPosEnd + vCap * fWidth);
                    pThird->SetColor(colorEdge);
                    ++pThird;

                    if (fPos > 0.0f)
                    {
                        vPos = vPosNew;
                    }
                    if (fTip > 0.0f)
                    {
                        vPosEnd = vPosEndNew;
                    }
                    bCoreDone = true;
                }
                else
                {
                    vPos = pTrail->GetInterpolatedPos(fTime);
                    vPosEnd = vPos + vDir * fLength;
                }

                if (bCoreDone && pTrail->IsTrailOn())
                {
                    color = colorHead;
                }
                else if (fAge >= fPersistTime || !bMoved || (!bNextVisible && j == iNumSteps - 1))
                {
                    color.a = 0;
                }
                else if (fAge > fFadeTime)
                {
                    const float fFade = (fPersistTime - fAge) * fFadeTimeInv * ((fPersistTime - fAge) * fFadeTimeInv);
                    color.a = (unsigned char)(int)(color.a * fFade);
                }

                pFront->SetPos(vPos - vCap * fWidth);
                pFront->SetColor(colorEdge);
                ++pFront;
                pFront->SetPos(vPos);
                pFront->SetColor(color);
                ++pFront;
                pBack->SetPos(vPosEnd);
                pBack->SetColor(color);
                --pBack;
                pBack->SetPos(vPos);
                pBack->SetColor(color);
                --pBack;
                pThird->SetPos(vPosEnd);
                pThird->SetColor(color);
                ++pThird;
                pThird->SetPos(vPosEnd + vCap * fWidth);
                pThird->SetColor(colorEdge);
                ++pThird;

                fAgePrev = fAge;
                fTimePrev = fTime;
            }
            fTimePrev = fSampleTime;
        }
    }
    pPrim->EndVertexData();
    RedRenderer::pcRenderPrimitive(pPrim, m_pShader,
                                   gMatrixIdentity, RedColor::WHITE,
                                   uiFlags, 0, 0, 0);
    pPrim->Release();
}
