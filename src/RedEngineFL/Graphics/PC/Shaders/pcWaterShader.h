#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/RedMaterial.h"
#include "RedEngineFL/Graphics/RedTextureTransform.h"
#include "PebbleFL/Common/PblVector.h"

class RedTexture;
class pcRedVertexFormat;

// VTABLE: SWBF2 0x00784f2c
class pcWaterShader : public pcRedShader
{
public:
    pcWaterShader(unsigned int uiFlags,
                  const unsigned int* pParams,
                  unsigned int uiNumParams,
                  unsigned int uiMaterialFlags,
                  const RedColor& color0,
                  const RedColor& color1,
                  float fArg7);

    virtual ~pcWaterShader()
    {
    }

    // SYNTHETIC: SWBF2 0x007087b0
    // pcWaterShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00709670
    // pcWaterShader::GetID
    // FUNCTION: SWBF2 0x007096a0
    // pcWaterShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00709600
    // pcWaterShader::Bind
    // FUNCTION: SWBF2 0x007096b0
    // pcWaterShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcWaterShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

    void SetBumpMap(RedTexture* pTex1, RedTextureTransform* pTransform1,
                    RedTexture* pTex2, RedTextureTransform* pTransform2, float fBlend);
    void SetNormalMap(RedTexture* pTex1, RedTextureTransform* pTransform1,
                      RedTexture* pTex2, RedTextureTransform* pTransform2, float fBlend);
    void SetDiffuseMap(RedTexture* pTex1, RedTextureTransform* pTransform1,
                       RedTexture* pTex2, RedTextureTransform* pTransform2, float fBlend);
    void SetSpecularMask(RedTexture* pTex1, RedTextureTransform* pTransform1,
                         RedTexture* pTex2, RedTextureTransform* pTransform2, float fBlend);

private:
    virtual void _StaticInit() const;

public:
    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    unsigned int GetState_Prog2(const RenderItem*) const;
    void Begin_Fixed(unsigned int uiState, unsigned int uiPass) const;
    void Begin_Prog1(unsigned int uiState, unsigned int uiPass) const;
    void Begin_Prog2(unsigned int uiState, unsigned int uiPass) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void End_Prog2(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem* pItem, unsigned int uiPass) const;
    void Render_Prog1(const RenderItem* pItem, unsigned int uiPass) const;
    void Render_Prog2(const RenderItem* pItem, unsigned int uiPass) const;

    pcRedVertexFormat* m_pVertexFormat;
    RedTexture* m_reflectionMap;
    RedTexture* m_projectedNormalMap;
    RedTexture* m_normalMap1;
    RedTexture* m_normalMap2;
    RedTextureTransform m_normalMapTransform1;
    RedTextureTransform m_normalMapTransform2;
    float m_normalMapBlend;
    RedTexture* m_bumpMap1;
    RedTexture* m_bumpMap2;
    RedTextureTransform m_bumpMapTransform1;
    RedTextureTransform m_bumpMapTransform2;
    float m_bumpMapBlend;
    RedTexture* m_diffuseMap1;
    RedTexture* m_diffuseMap2;
    RedTextureTransform m_diffuseMapTransform1;
    RedTextureTransform m_diffuseMapTransform2;
    float m_diffuseMapBlend;
    RedTexture* m_specularMask1;
    RedTexture* m_specularMask2;
    RedTextureTransform m_specularMaskTransform1;
    RedTextureTransform m_specularMaskTransform2;
    float m_specularMaskBlend;

    // FUNCTION: SWBF2 0x00720500
    static void SetFresnelMinMax(float fMin, float fMax)
    {
        if (fMax < 0.0f)
        {
            fMax = 0.0f;
        }
        else if (fMax > 1.0f)
        {
            fMax = 1.0f;
        }
        float fClampedMin = fMin < 0.0f ? 0.0f : (fMin > 1.0f ? 1.0f : fMin);
        s_fresnelMinMax.r = 0.0f;
        s_fresnelMinMax.g = 0.0f;
        s_fresnelMinMax.b = fClampedMin;
        s_fresnelMinMax.a = fMax;
    }
    static void SetReflectionColor(RedColor color);
    static void SetRefractionColor(RedColor color);
    static void SetSpecularEnable(bool bEnable);

    static void SetSpecularDirection(const PblVector3& vDirection);
    void SetReflectionMap(RedTexture* pTexture);
    void SetProjectedNormalMap(RedTexture* pTexture);

    // GLOBAL: SWBF2 0x007e1ee0
    static bool s_specularEnable;

    // GLOBAL: SWBF2 0x007e1ef4
    static PblVector3 s_vSpecularDirection;

    // GLOBAL: SWBF2 0x007e1f40
    static RedColorValue s_refractionColor;

    // GLOBAL: SWBF2 0x007e1f50
    static RedColorValue s_reflectionColor;

    // GLOBAL: SWBF2 0x007e1f60
    static RedColorValue s_fresnelMinMax;

private:
    pcWaterShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);
};
 
 
 
 
 
 
 
 
 
 
 
 
