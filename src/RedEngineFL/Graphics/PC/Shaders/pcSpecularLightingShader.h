#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x00785660
class pcSpecularLightingShader : public pcRedShader
{
public:

    virtual ~pcSpecularLightingShader()
    {
    }

    // SYNTHETIC: SWBF2 0x007126d0
    // pcSpecularLightingShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00712970
    // pcSpecularLightingShader::GetID
    // FUNCTION: SWBF2 0x007129a0
    // pcSpecularLightingShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00712930
    // pcSpecularLightingShader::Bind
    // FUNCTION: SWBF2 0x007129b0
    // pcSpecularLightingShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcSpecularLightingShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

private:

    // FUNCTION: SWBF2 0x007136e0

    // FUNCTION: SWBF2 0x007129b0



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
    void PerVertexLighting(const RenderItem* pItem, unsigned int uiPass) const;
    void PerPixelLighting(const RenderItem* pItem, unsigned int uiPass) const;
public:

private:

public:

    // FUNCTION: SWBF2 0x00712660
    pcSpecularLightingShader(unsigned int uiFlags,
                             const unsigned int* pParams,
                             unsigned int uiNumParams,
                             unsigned int uiMaterialFlags,
                             const RedColor& color0,
                             const RedColor& color1,
                             float fSpecularPower);

private:
    // FUNCTION: SWBF2 0x007125a0
    pcSpecularLightingShader();

    // FUNCTION: SWBF2 0x00713770
};
 
