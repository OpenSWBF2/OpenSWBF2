#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class RedTexture;
class pcRedVertexFormat;

// VTABLE: SWBF2 0x00784948
class pcSkyFogShader : public pcRedShader
{
public:

    pcSkyFogShader(unsigned int uiFlags,
                   const unsigned int* pParams,
                   unsigned int uiNumParams,
                   unsigned int uiMaterialFlags,
                   const RedColor& color0,
                   const RedColor& color1,
                   float fArg7);

    virtual ~pcSkyFogShader()
    {
    }

    // SYNTHETIC: SWBF2 0x00703e00
    // pcSkyFogShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00704080
    // pcSkyFogShader::GetID
    // FUNCTION: SWBF2 0x007040b0
    // pcSkyFogShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00704030
    // pcSkyFogShader::Bind
    // FUNCTION: SWBF2 0x007040c0
    // pcSkyFogShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcSkyFogShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;
    virtual RedShadingState* CreateShadingState() const;

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

    void SetTexture(const RedTexture* pTexture)
    {
        m_pSkyTexture = const_cast<RedTexture*>(pTexture);
    }

    RedTexture* m_pSkyTexture;
    pcRedVertexFormat* m_pVertexInputFormat;

private:

    pcSkyFogShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

};
 
 
 
