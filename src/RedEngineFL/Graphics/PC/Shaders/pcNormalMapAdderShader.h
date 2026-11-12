#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/RedTextureTransform.h"

class RedTexture;
class pcRedVertexFormat;

// VTABLE: SWBF2 0x007850bc
class pcNormalMapAdderShader : public pcRedShader
{
public:

    pcNormalMapAdderShader(unsigned int uiFlags,
                           const unsigned int* pParams,
                           unsigned int uiNumParams,
                           unsigned int uiMaterialFlags,
                           const RedColor& color0,
                           const RedColor& color1,
                           float fArg7);

    void SetNormalMap(RedTexture* pTex1, RedTexture* pTex2, float fBlend);
    void SetTextureTransform(RedTextureTransform* pTransform1,
                             RedTextureTransform* pTransform2);

    virtual ~pcNormalMapAdderShader()
    {
    }

    // SYNTHETIC: SWBF2 0x0070b650
    // pcNormalMapAdderShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x0070b7b0
    // pcNormalMapAdderShader::GetID
    // FUNCTION: SWBF2 0x0070b7e0
    // pcNormalMapAdderShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x0070b770
    // pcNormalMapAdderShader::Bind
    // FUNCTION: SWBF2 0x0070b7f0
    // pcNormalMapAdderShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcNormalMapAdderShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

private:
    virtual void _StaticInit() const;

public:

    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem*, unsigned int) const;
    void Render_Prog1(const RenderItem*, unsigned int) const;

    pcRedVertexFormat* m_pVertexFormat[2];
    RedTexture* m_normalMap1;
    RedTexture* m_normalMap2;
    RedTextureTransform m_textureTransform1;
    RedTextureTransform m_textureTransform2;
    float m_blend;
    int m_iShaderState;

private:

    pcNormalMapAdderShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

};
 
 
 
 
 
 
 
