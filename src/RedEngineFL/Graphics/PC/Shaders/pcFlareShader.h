#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class RedTexture;
class pcRedVertexFormat;

// VTABLE: SWBF2 0x00784bb8
class pcFlareShader : public pcRedShader
{
public:

    pcFlareShader(unsigned int uiFlags,
                  const unsigned int* pParams,
                  unsigned int uiNumParams,
                  unsigned int uiMaterialFlags,
                  const RedColor& color0,
                  const RedColor& color1,
                  float fArg7);

    virtual ~pcFlareShader()
    {
    }

    // SYNTHETIC: SWBF2 0x00706010
    // pcFlareShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00706210
    // pcFlareShader::GetID
    // FUNCTION: SWBF2 0x00706240
    // pcFlareShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x007061d0
    // pcFlareShader::Bind
    // FUNCTION: SWBF2 0x00706250
    // pcFlareShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcFlareShader)

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

    void SetTexture(RedTexture* pTexture) { m_texture = pTexture; }

    RedTexture* m_texture;
    unsigned int m_uiMaterialFlags;
    RedColor m_colorDiffuse;
    pcRedVertexFormat* m_pVertexFormat;

private:

    pcFlareShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

};
 
 
 
 
 
