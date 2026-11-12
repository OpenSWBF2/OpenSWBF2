#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x00785434
class pcPerVertexDiffuseLightingShader : public pcRedShader
{
public:

    virtual ~pcPerVertexDiffuseLightingShader()
    {
    }

    // SYNTHETIC: SWBF2 0x007106b0
    // pcPerVertexDiffuseLightingShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x007108c0
    // pcPerVertexDiffuseLightingShader::GetID
    // FUNCTION: SWBF2 0x007108f0
    // pcPerVertexDiffuseLightingShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00710880
    // pcPerVertexDiffuseLightingShader::Bind
    // FUNCTION: SWBF2 0x00710900
    // pcPerVertexDiffuseLightingShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcPerVertexDiffuseLightingShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

private:

    // FUNCTION: SWBF2 0x00711380

    // FUNCTION: SWBF2 0x00710900



public:

    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem*, unsigned int) const;
    void Render_Prog1(const RenderItem*, unsigned int) const;

    mutable void* m_pLightingStatePrev;

private:

public:

    // FUNCTION: SWBF2 0x00710620
    pcPerVertexDiffuseLightingShader(unsigned int uiFlags,
                                     const unsigned int* pParams,
                                     unsigned int uiNumParams,
                                     unsigned int uiMaterialFlags,
                                     const RedColor& color0,
                                     const RedColor& color1,
                                     float fSpecularPower);

private:
    // FUNCTION: SWBF2 0x00710560
    pcPerVertexDiffuseLightingShader();

    // FUNCTION: SWBF2 0x00711410
    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fSpecularPower);

    // FUNCTION: SWBF2 0x00711420
};
 
 
