#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x00784e88
class pcShadowQuadShader : public pcRedShader
{
public:

    pcShadowQuadShader(unsigned int uiFlags,
                       const unsigned int* pParams,
                       unsigned int uiNumParams,
                       unsigned int uiMaterialFlags,
                       const RedColor& color0,
                       const RedColor& color1,
                       float fArg7);

    virtual ~pcShadowQuadShader()
    {
    }

    // SYNTHETIC: SWBF2 0x00708070
    // pcShadowQuadShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00708250
    // pcShadowQuadShader::GetID
    // FUNCTION: SWBF2 0x00708280
    // pcShadowQuadShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00708200
    // pcShadowQuadShader::Bind
    // FUNCTION: SWBF2 0x00708290
    // pcShadowQuadShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcShadowQuadShader)

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

    pcRedVertexFormat* m_pVertexInputFormat;

private:

    pcShadowQuadShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

public:
    static unsigned int s_uiStencilMask;
};
 
 
