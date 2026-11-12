#pragma once

#include <stddef.h>
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class PblMatrix;
class RedTexture;

// VTABLE: SWBF2 0x007842dc
class pcTerrainShader : public pcRedShader
{
public:
    pcTerrainShader(unsigned int uiFlags,
                    const unsigned int* pParams,
                    unsigned int uiNumParams,
                    unsigned int uiMaterialFlags,
                    const RedColor& color0,
                    const RedColor& color1,
                    float fSpecularPower);

    virtual ~pcTerrainShader()
    {
    }

    // SYNTHETIC: SWBF2 0x006f8a60
    // pcTerrainShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x006fac90
    // pcTerrainShader::GetID
    // FUNCTION: SWBF2 0x006face0
    // pcTerrainShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x006fac20
    // pcTerrainShader::Bind
    // FUNCTION: SWBF2 0x006fad10
    // pcTerrainShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcTerrainShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;
    virtual RedShadingState* CreateShadingState() const;
    virtual unsigned int GetZPrepassState() const;
    virtual void RenderZPrepass(const RenderItem* pItem) const;
    virtual unsigned short GetShaderChain(pcRedShader** const ppOut,
                                          unsigned short uiMax,
                                          const RenderItem* pItem);

    void EnableDetail(bool bEnable)
    {
        if (bEnable)
        {
            m_uiOptions |= 1;
        }
        else
        {
            m_uiOptions &= ~1;
        }
    }

    void EnableFarRendering(bool bEnable, bool bFarDetail)
    {
        if (bEnable)
        {
            m_uiOptions |= 8;
        }
        else
        {
            m_uiOptions &= ~8;
        }

        if (bFarDetail)
        {
            m_uiOptions |= 1;
        }
        else
        {
            m_uiOptions &= ~1;
        }
    }

    void EnablePrePixelLightingPass(bool bEnable)
    {
        if (bEnable)
        {
            m_uiOptions |= 4;
        }
        else
        {
            m_uiOptions &= ~4;
        }
    }

    void SetMultiLayer(int iLayer) { m_uiOptions = (m_uiOptions & ~0x30) | ((iLayer & 3) << 4); }

    unsigned int GetState_Fixed(const RenderItem* pItem) const;
    unsigned int GetState_Prog1(const RenderItem* pItem) const;
    void Begin_Fixed(unsigned int uiState, unsigned int uiPass) const;
    void Begin_Prog1(unsigned int uiState, unsigned int uiPass) const;
    void End_Fixed(unsigned int uiState, unsigned int uiPass) const;
    void End_Prog1(unsigned int uiState, unsigned int uiPass) const;
    void Render_Fixed(const RenderItem* pItem, unsigned int uiPass) const;
    void Render_Prog1(const RenderItem* pItem, unsigned int uiPass) const;

    static void __cdecl _InitBumpLuminosityPixelLighting(unsigned int uiHash,
                                                         unsigned int uiBumpLuminosityHash,
                                                         const PblVector4& vScale0,
                                                         const PblVector4& vScale1,
                                                         float fStaticBumpFactor);
    static void __cdecl _InitDetail(unsigned int uiHash, const PblVector4& vScale0,
                                    const PblVector4& vScale1, float fFadeDistance);
    static void __cdecl _InitFarDetail(unsigned int uiHash, float fX, float fY,
                                       float fZ, float fW);
    static void __cdecl _InitFarRendering(unsigned int uiHash, float fX, float fY,
                                          float fZ, float fW);
    static void __cdecl _InitLayer(int iIndex, unsigned int uiHash,
                                   const PblVector4& vScale0,
                                   const PblVector4& vScale1);
    static void __cdecl _InitPixelLighting(unsigned int uiLightVolumeHash,
                                           unsigned int uiNormalizationCubeHash);
    static void __cdecl _InitShadowMap(RedTexture* pTexture, unsigned int uiWrapMode,
                                       const PblVector4& vU, const PblVector4& vV,
                                       const PblVector4& vW);

    static void _SetLightningValue(float fValue) { s_fLightningValue = fValue; }

private:
    unsigned char m_uiLayerIndex[4];
    unsigned char m_uiOptions;
    bool m_lightingInSeparatePass;

public:
    // GLOBAL: SWBF2 0x009c8548
    static pcRedVertexFormat* s_pVertexInputFormat;
    // GLOBAL: SWBF2 0x009c854c
    static pcRedVertexFormat* s_pPosVertexInputFormat;
    // GLOBAL: SWBF2 0x009c8550
    static pcRedVertexFormat* s_pPosNormalVertexInputFormat;
    // GLOBAL: SWBF2 0x009c8554
    static pcRedVertexFormat* s_pFarVertexInputFormat;
    // GLOBAL: SWBF2 0x009c8558
    static pcRedVertexFormat* s_pFFLayer0VertexInputFormat;
    // GLOBAL: SWBF2 0x009c855c
    static pcRedVertexFormat* s_pFFLayerNVertexInputFormat;
    // GLOBAL: SWBF2 0x009c8560
    static RedTexture* s_apTextures[16];
    // GLOBAL: SWBF2 0x009c85a0
    static RedTexture* s_pDetailTexture;
    // GLOBAL: SWBF2 0x009c85a4
    static RedTexture* s_pBumpSpecularTexture;
    // GLOBAL: SWBF2 0x009c85a8
    static RedTexture* s_pShadowMapTexture;
    // GLOBAL: SWBF2 0x009c85ac
    static RedTexture* s_pNormalizationCubeMap;
    // GLOBAL: SWBF2 0x009c85b0
    static RedTexture* s_pLightVolumeMap;
    // GLOBAL: SWBF2 0x009c85b4
    static RedTexture* s_pBumpLuminosityTexture;
    // GLOBAL: SWBF2 0x009c85b8
    static RedTexture* s_pFarTexture;
    // GLOBAL: SWBF2 0x009c85bc
    static RedTexture* s_pFarDetailTexture;
    // GLOBAL: SWBF2 0x009c85c0
    static float s_fDetailFadeDistance;
    // GLOBAL: SWBF2 0x009c85c4
    static float s_fStaticBumpFactor;
    // GLOBAL: SWBF2 0x009c85c8
    static unsigned int s_uiOptions;
    // GLOBAL: SWBF2 0x009c85cc
    static float s_fLightningValue;
    // GLOBAL: SWBF2 0x009c8600
    static PblVector4 s_BumpSpecularTextureTransform[2];
    // GLOBAL: SWBF2 0x009c8644
    static PblVector4 s_BumpLuminosityTextureTransform[2];
    // GLOBAL: SWBF2 0x009c8664
    static PblVector4 s_FarTextureTransform;

    // GLOBAL: SWBF2 0x009c8678
    static PblVector4 s_TextureTransform[16][2];
    // GLOBAL: SWBF2 0x009c8880
    static PblVector4 s_FarDetailTextureTransform;
    // GLOBAL: SWBF2 0x009c8890
    static PblMatrix s_mProjectionRestore;
    // GLOBAL: SWBF2 0x009c88d4
    static PblVector4 s_DetailTextureTransform[2];

    // GLOBAL: SWBF2 0x007e1798
    static unsigned int s_uiShadowMapWrapMode;

private:

    pcTerrainShader();

    virtual void _StaticInit() const;

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fSpecularPower);
};
 
 
 
 
