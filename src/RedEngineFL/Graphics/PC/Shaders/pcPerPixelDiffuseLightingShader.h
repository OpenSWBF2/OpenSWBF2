#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x00785550
class pcPerPixelDiffuseLightingShader : public pcRedShader
{
public:

    virtual ~pcPerPixelDiffuseLightingShader()
    {
    }

    // SYNTHETIC: SWBF2 0x00711690
    // pcPerPixelDiffuseLightingShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x007118a0
    // pcPerPixelDiffuseLightingShader::GetID
    // FUNCTION: SWBF2 0x007118d0
    // pcPerPixelDiffuseLightingShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00711860
    // pcPerPixelDiffuseLightingShader::Bind
    // FUNCTION: SWBF2 0x007118e0
    // pcPerPixelDiffuseLightingShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcPerPixelDiffuseLightingShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

private:

    // FUNCTION: SWBF2 0x007123d0

    // FUNCTION: SWBF2 0x007118e0



public:

    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem*, unsigned int) const;
    void Render_Prog1(const RenderItem*, unsigned int) const;

private:

public:

    // FUNCTION: SWBF2 0x00711620
    pcPerPixelDiffuseLightingShader(unsigned int uiFlags,
                                    const unsigned int* pParams,
                                    unsigned int uiNumParams,
                                    unsigned int uiMaterialFlags,
                                    const RedColor& color0,
                                    const RedColor& color1,
                                    float fSpecularPower);

private:
    // FUNCTION: SWBF2 0x00711560
    pcPerPixelDiffuseLightingShader();

    // FUNCTION: SWBF2 0x00712460
};
 
