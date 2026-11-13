#include <string.h>
#include "pcPerVertexDiffuseLightingShader.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedVertexFormat.h"
#include "RedEngineFL/Graphics/PC/pcRedTexture.h"
#include "RedEngineFL/Graphics/RedMaterial.h"
#include "RedEngineFL/Graphics/RedLight.h"
#include "RedEngineFL/Graphics/RedLight.h"
#include "RedEngineFL/Graphics/RedDirectionalLight.h"
#include "RedEngineFL/Graphics/RedOmniLight.h"
#include "RedEngineFL/Graphics/RedSpotLight.h"

#include "PebbleFL/Common/PblChunk.h"

#pragma init_seg(lib)

void __cdecl PblTraceP(const char* Format, ...);

// GLOBAL: SWBF2 0x009ca840
// pcPerVertexDiffuseLightingShader::s_uiID
unsigned int pcPerVertexDiffuseLightingShader::s_uiID;
// GLOBAL: SWBF2 0x009ca844
// pcPerVertexDiffuseLightingShader::GetStateFuncArray
pcRedShader::GetStateFuncT pcPerVertexDiffuseLightingShader::GetStateFuncArray[MAX_PIPELINES];
// GLOBAL: SWBF2 0x009ca850
// pcPerVertexDiffuseLightingShader::BeginFuncArray
pcRedShader::BeginFuncT pcPerVertexDiffuseLightingShader::BeginFuncArray[MAX_PIPELINES];
// GLOBAL: SWBF2 0x009ca85c
// pcPerVertexDiffuseLightingShader::EndFuncArray
pcRedShader::EndFuncT pcPerVertexDiffuseLightingShader::EndFuncArray[MAX_PIPELINES];
// GLOBAL: SWBF2 0x009ca868
// pcPerVertexDiffuseLightingShader::RenderFuncArray
pcRedShader::RenderFuncT pcPerVertexDiffuseLightingShader::RenderFuncArray[MAX_PIPELINES];

// FUNCTION: SWBF2 0x00710560
pcPerVertexDiffuseLightingShader::pcPerVertexDiffuseLightingShader()
{
    memset(GetStateFuncArray, 0, sizeof(GetStateFuncArray));
    memset(BeginFuncArray, 0, sizeof(BeginFuncArray));
    memset(EndFuncArray, 0, sizeof(EndFuncArray));
    memset(RenderFuncArray, 0, sizeof(RenderFuncArray));

    s_uiID = _Register(s_uiRenderTypeHash, this);

    _InitPipelineFuncArray();
}

// FUNCTION: SWBF2 0x00710620
pcPerVertexDiffuseLightingShader::pcPerVertexDiffuseLightingShader(unsigned int uiFlags,
                                                                   const unsigned int* pParams,
                                                                   unsigned int uiNumParams,
                                                                   unsigned int uiMaterialFlags,
                                                                   const RedColor& color0,
                                                                   const RedColor& color1,
                                                                   float fSpecularPower)
{
    _BindPipeline(GetStateFuncArray, BeginFuncArray, EndFuncArray, RenderFuncArray);
    _Init(uiFlags, pParams, uiNumParams, uiMaterialFlags,
          color0, color1, fSpecularPower);
}

// FUNCTION: SWBF2 0x007106e0
void pcPerVertexDiffuseLightingShader::Begin_Fixed(unsigned int uiState, unsigned int) const
{
    RedRenderer::pcSetRenderState(D3DRS_COLORWRITEENABLE, 0);
    RedRenderer::pcSetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    RedRenderer::pcSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    if (uiState == 1)
    {
        RedRenderer::pcSetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        RedRenderer::pcSetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
        RedRenderer::pcSetRenderState(D3DRS_ALPHAREF, 0x7f);
        RedRenderer::pcSetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        RedRenderer::pcSetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        RedRenderer::pcSetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        RedRenderer::pcSetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        RedRenderer::pcSetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
        RedRenderer::pcSetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        RedRenderer::pcSetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }
    else
    {
        RedRenderer::pcSetTexture(0, 0);
        RedRenderer::pcSetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        RedRenderer::pcSetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
        RedRenderer::pcSetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        RedRenderer::pcSetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }
}

// FUNCTION: SWBF2 0x00710830
void pcPerVertexDiffuseLightingShader::Begin_Prog1(unsigned int, unsigned int) const
{
    RedRenderer::pcSetRenderState(D3DRS_COLORWRITEENABLE, 7);
    RedRenderer::pcSetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    RedRenderer::pcSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    RedRenderer::pcSetTexture(0, 0);
    RedRenderer::pcSetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    m_pLightingStatePrev = 0;
}

// FUNCTION: SWBF2 0x007108a0
void pcPerVertexDiffuseLightingShader::End_Fixed(unsigned int, unsigned int) const
{
}

// FUNCTION: SWBF2 0x007108b0
void pcPerVertexDiffuseLightingShader::End_Prog1(unsigned int, unsigned int) const
{
}

// FUNCTION: SWBF2 0x007108e0
RenderPhaseT pcPerVertexDiffuseLightingShader::GetRenderPhase(const RenderItem*, unsigned int) const
{
    return RENDERPHASE_OPAQUE;
}

// FUNCTION: SWBF2 0x00710910
unsigned int pcPerVertexDiffuseLightingShader::GetState_Fixed(const RenderItem*) const
{
    return 0;
}

// FUNCTION: SWBF2 0x00710920
unsigned int pcPerVertexDiffuseLightingShader::GetState_Prog1(const RenderItem* pItem) const
{
    const unsigned char uiInverted = ~pItem->flags;
    return (uiInverted >> 2) & 4;
}

// FUNCTION: SWBF2 0x00710940
void pcPerVertexDiffuseLightingShader::InitFixedFunctionShaderDef() const
{
    // GLOBAL: SWBF2 0x007854a4
    static const pcRedShaderDef::FixedFunctionStateT aFFStates[2] =
    {
        { 1, { 0x21, 0x00, 0x00, 0x00 } },
        { 1, { 0x21, 0x00, 0x00, 0x00 } },
    };

    GetShaderDef()->InitFixedFunction(0x28, 2, aFFStates);
}

// FUNCTION: SWBF2 0x00710960
void pcPerVertexDiffuseLightingShader::Render_Fixed(const RenderItem*, unsigned int) const
{
}

// FUNCTION: SWBF2 0x00710970
void pcPerVertexDiffuseLightingShader::Render_Prog1(const RenderItem* pItem, unsigned int) const
{
    RedRenderer::pcLoadFormat(pItem->pPrim->m_vertexFormat);

    if (!pItem->uiShaderState)
    {
        pItem->pPrim->Draw();
        return;
    }

    const RedLightingState* pLighting = pItem->pLightingState;
    const bool bVertexLit = (pItem->pPrim->m_vertexFormat->m_uiVertexBufferRVF & 0x100) != 0;
    if (pLighting != m_pLightingStatePrev)
    {
        m_pLightingStatePrev = (void*)pLighting;
        const RedColorValue& black = RedColorValue::BLACK;

        pcRedVertexShaderConstantRegister* pRegs = RedRenderer::pcLockVertexShaderConstants(19, 6);
        if (!bVertexLit)
        {
            const RedAmbientLight* pAmbient = &pLighting->ambientLight;
            pRegs[19].SetColor(pAmbient->m_topColor);
            pRegs[20].SetColor(pAmbient->m_bottomColor);
        }
        else
        {
            pRegs[19].SetColor(black);
            pRegs[20].SetColor(black);
        }

        pRegs[21].SetColorAlpha(pLighting->pDirectionalLights[0]->m_color, pLighting->pDirectionalFactor[0]);
        pRegs[22].SetVectorXYZ(pLighting->pDirectionalLights[0]->m_vDirection);
        if (pLighting->pDirectionalLights[1])
        {
            pRegs[23].SetColorAlpha(pLighting->pDirectionalLights[1]->m_color, pLighting->pDirectionalFactor[1]);
            pRegs[24].SetVectorXYZ(pLighting->pDirectionalLights[1]->m_vDirection);
        }
        else
        {
            pRegs[23].SetColorAlpha(black, 0.0f);
            pRegs[24].SetFloats(0.0f, 0.0f, 0.0f, 1.0f);
        }
        RedRenderer::pcUnlockVertexShaderConstants(pRegs);

        if (pLighting->pSpotLights[0])
        {
            pRegs = RedRenderer::pcLockVertexShaderConstants(29, 5);
            const RedSpotLight* pSpot = pLighting->pSpotLights[0];
            pRegs[29].SetColorAlpha(pSpot->m_color, 1.0f);
            pRegs[30].SetVectorW(pSpot->m_vPos, pSpot->m_fRadiusInvSq);
            pRegs[31].SetVectorW(pSpot->m_vDirection, pSpot->m_bBidirectional ? 2.0f : 0.0f);
            pRegs[32].SetFloats(
                pSpot->m_fCosHalfOuterConeAngle, pSpot->m_fCosHalfInnerConeAngle,
                1.0f / (pSpot->m_fCosHalfInnerConeAngle - pSpot->m_fCosHalfOuterConeAngle),
                pSpot->m_fFalloff);
            RedRenderer::pcUnlockVertexShaderConstants(pRegs);

            pRegs = RedRenderer::pcLockVertexShaderConstants(35, 3);
            pRegs[35].SetFloats(pSpot->GetProjectionMatrix()(0, 0),
                                pSpot->GetProjectionMatrix()(1, 0),
                                pSpot->GetProjectionMatrix()(2, 0),
                                pSpot->GetProjectionMatrix()(3, 0));
            pRegs[36].SetFloats(pSpot->GetProjectionMatrix()(0, 1),
                                pSpot->GetProjectionMatrix()(1, 1),
                                pSpot->GetProjectionMatrix()(2, 1),
                                pSpot->GetProjectionMatrix()(3, 1));
            pRegs[37].SetFloats(pSpot->GetProjectionMatrix()(0, 3),
                                pSpot->GetProjectionMatrix()(1, 3),
                                pSpot->GetProjectionMatrix()(2, 3),
                                pSpot->GetProjectionMatrix()(3, 3));
            RedRenderer::pcUnlockVertexShaderConstants(pRegs);

            RedRenderer::pcSetTexture(0, RedTexture::Find(PblHash("flames1")));
            RedRenderer::pcSetTexture(1, RedTexture::Find(PblHash("white")));
            RedRenderer::pcGetDevice()->SetPixelShaderConstantF(0, &RedColorValue::WHITE.r, 1);
            RedRenderer::pcGetDevice()->SetPixelShaderConstantF(1, &RedColorValue::BLACK.r, 1);
            RedRenderer::pcSetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            RedRenderer::pcSetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            RedRenderer::pcSetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_PROJECTED);
            RedRenderer::pcSetTexture(1, RedShadowMap::GetShadowMapTexture());
            RedRenderer::pcSetSamplerState(1, D3DSAMP_ADDRESSU, RedShadowMap::GetShadowMapWrapMode());
            RedRenderer::pcSetSamplerState(1, D3DSAMP_ADDRESSV, RedShadowMap::GetShadowMapWrapMode());
            RedRenderer::pcSetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_PROJECTED);
            RedRenderer::pcSetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            RedRenderer::pcSetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            RedRenderer::pcSetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
            RedRenderer::pcGetDevice()->SetPixelShaderConstantF(
                2, &pLighting->pDirectionalLights[0]->m_color.r, 1);
        }

        if (pLighting->pOmniLights[0])
        {
            pRegs = RedRenderer::pcLockVertexShaderConstants(25, 8);
            if (pLighting->pOmniLights[3])
            {
                {
                    const RedOmniLight* pLight = pLighting->pOmniLights[0];
                    pRegs[25].SetColorAlpha(pLight->m_color, 1.0f);
                    pRegs[26].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                }
                {
                    const RedOmniLight* pLight = pLighting->pOmniLights[1];
                    pRegs[27].SetColorAlpha(pLight->m_color, 1.0f);
                    pRegs[28].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                }
                {
                    const RedOmniLight* pLight = pLighting->pOmniLights[2];
                    pRegs[29].SetColorAlpha(pLight->m_color, 1.0f);
                    pRegs[30].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                }
                {
                    const RedOmniLight* pLight = pLighting->pOmniLights[3];
                    pRegs[31].SetColorAlpha(pLight->m_color, 1.0f);
                    pRegs[32].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                }
            }
            else if (pLighting->pOmniLights[2])
            {
                {
                    const RedOmniLight* pLight = pLighting->pOmniLights[0];
                    pRegs[25].SetColorAlpha(pLight->m_color, 1.0f);
                    pRegs[26].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                }
                {
                    const RedOmniLight* pLight = pLighting->pOmniLights[1];
                    pRegs[27].SetColorAlpha(pLight->m_color, 1.0f);
                    pRegs[28].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                }
                {
                    const RedOmniLight* pLight = pLighting->pOmniLights[2];
                    pRegs[29].SetColorAlpha(pLight->m_color, 1.0f);
                    pRegs[30].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                }
                pRegs[31].SetColorAlpha(black, 1.0f);
                pRegs[32].SetFloats(0.0f, 0.0f, 0.0f, 1.0f);
            }
            else
            {
                {
                    const RedOmniLight* pLight = pLighting->pOmniLights[0];
                    pRegs[25].SetColorAlpha(pLight->m_color, 1.0f);
                    pRegs[26].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                }
                if (pLighting->pOmniLights[1])
                {
                    {
                        const RedOmniLight* pLight = pLighting->pOmniLights[1];
                        pRegs[27].SetColorAlpha(pLight->m_color, 1.0f);
                        pRegs[28].SetVectorW(pLight->m_pos, pLight->m_fRadiusInvSq);
                    }
                }
                else
                {
                    pRegs[27].SetColorAlpha(black, 1.0f);
                    pRegs[28].SetFloats(0.0f, 0.0f, 0.0f, 1.0f);
                }
                pRegs[29].SetColorAlpha(black, 1.0f);
                pRegs[30].SetFloats(0.0f, 0.0f, 0.0f, 1.0f);
                pRegs[31].SetColorAlpha(black, 1.0f);
                pRegs[32].SetFloats(0.0f, 0.0f, 0.0f, 1.0f);
            }
            RedRenderer::pcUnlockVertexShaderConstants(pRegs);
        }
    }

    pItem->pPrim->Draw();
}

// FUNCTION: SWBF2 0x00711380
pcRedShader* pcPerVertexDiffuseLightingShader::_Create(unsigned int uiFlags,
                                                       const unsigned int* pParams,
                                                       unsigned int uiNumParams,
                                                       unsigned int uiMaterialFlags,
                                                       const RedColor& color0,
                                                       const RedColor& color1,
                                                       float fSpecularPower) const
{
    return new pcPerVertexDiffuseLightingShader(uiFlags, pParams, uiNumParams, uiMaterialFlags,
                                                color0, color1, fSpecularPower);
}

// FUNCTION: SWBF2 0x00711410
void pcPerVertexDiffuseLightingShader::_Init(unsigned int,
                                             const unsigned int*,
                                             unsigned int,
                                             unsigned int,
                                             const RedColor&,
                                             const RedColor&,
                                             float)
{
    m_pLightingStatePrev = 0;
}

// FUNCTION: SWBF2 0x00711420
void pcPerVertexDiffuseLightingShader::_InitPipelineFuncArray()
{
    GetStateFuncArray[PIPELINE_FIXED] =
        (GetStateFuncT)(&pcPerVertexDiffuseLightingShader::GetState_Fixed);
    BeginFuncArray[PIPELINE_FIXED] =
        (BeginFuncT)(&pcPerVertexDiffuseLightingShader::Begin_Fixed);
    EndFuncArray[PIPELINE_FIXED] =
        (EndFuncT)(&pcPerVertexDiffuseLightingShader::End_Fixed);
    RenderFuncArray[PIPELINE_FIXED] =
        (RenderFuncT)(&pcPerVertexDiffuseLightingShader::Render_Fixed);

    GetStateFuncArray[PIPELINE_PROG1] =
        (GetStateFuncT)(&pcPerVertexDiffuseLightingShader::GetState_Prog1);
    BeginFuncArray[PIPELINE_PROG1] =
        (BeginFuncT)(&pcPerVertexDiffuseLightingShader::Begin_Prog1);
    EndFuncArray[PIPELINE_PROG1] =
        (EndFuncT)(&pcPerVertexDiffuseLightingShader::End_Prog1);
    RenderFuncArray[PIPELINE_PROG1] =
        (RenderFuncT)(&pcPerVertexDiffuseLightingShader::Render_Prog1);
}

// FUNCTION: SWBF2 0x00711480
void pcPerVertexDiffuseLightingShader::_Read(PblFileChunk& chunk) const
{
    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        if (child._iId != CHUNK_ID('N', 'A', 'M', 'E'))
        {
            continue;
        }

        char acName[256];
        child.ReadString(acName, 256);
        const unsigned int uiHash = PblHash(acName);
        PblTraceP("pcRedShaderDef [%s]\tHash 0x%08x\n", acName, uiHash);

        pcRedShaderDef* pDef = GetShaderDef();
        if (pDef)
        {
            pDef->Read(chunk);
        }
        else
        {
            PblTraceP("Error: pcRedShaderDef 0x%08x not found\n", uiHash);
        }
    }
}

// SYNTHETIC: SWBF2 0x0040a550
// `dynamic initializer for 'pcPerVertexDiffuseLightingShader::s_uiRenderTypeHash''
// GLOBAL: SWBF2 0x009ca87c
// pcPerVertexDiffuseLightingShader::s_uiRenderTypeHash
unsigned int pcPerVertexDiffuseLightingShader::s_uiRenderTypeHash = PblHash("pervertexdiffuselighting");

// SYNTHETIC: SWBF2 0x0040a510
// `dynamic initializer for 'pcPerVertexDiffuseLightingShader::s_Factory''
// SYNTHETIC: SWBF2 0x00769fc0
// `dynamic atexit destructor for 'pcPerVertexDiffuseLightingShader::s_Factory''
// GLOBAL: SWBF2 0x009ca888
// pcPerVertexDiffuseLightingShader::s_Factory
pcPerVertexDiffuseLightingShader pcPerVertexDiffuseLightingShader::s_Factory;

// SYNTHETIC: SWBF2 0x0040a530
// `dynamic initializer for 'pcPerVertexDiffuseLightingShader::s_shaderDef''
// SYNTHETIC: SWBF2 0x00769fe0
// `dynamic atexit destructor for 'pcPerVertexDiffuseLightingShader::s_shaderDef''
// GLOBAL: SWBF2 0x009ca8a8
// pcPerVertexDiffuseLightingShader::s_shaderDef
pcRedShaderDef pcPerVertexDiffuseLightingShader::s_shaderDef("pervertexdiffuselighting_shader.xml");

// SYNTHETIC: SWBF2 0x0040a590
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040a570
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// GLOBAL: SWBF2 0x009ca874
// g_pcPerVertexDiffuseLightingShader
unsigned int g_pcPerVertexDiffuseLightingShader;
