#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x00784a94
class pcRainShader : public pcRedShader
{
public:

    pcRainShader(unsigned int uiFlags,
                 const unsigned int* pParams,
                 unsigned int uiNumParams,
                 unsigned int uiMaterialFlags,
                 const RedColor& color0,
                 const RedColor& color1,
                 float fArg7);

    virtual ~pcRainShader()
    {
    }

    // SYNTHETIC: SWBF2 0x00704db0
    // pcRainShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00704f40
    // pcRainShader::GetID
    // FUNCTION: SWBF2 0x00704f70
    // pcRainShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00704f00
    // pcRainShader::Bind
    // FUNCTION: SWBF2 0x00704f80
    // pcRainShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcRainShader)

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

    void SetLineWidth(float fWidth) { m_fLineWidth = fWidth; }
    void SetMaxPixelWidth(float fWidth) { m_fMaxPixelWidth = fWidth; }

    pcRedVertexFormat* m_pVertexFormat;
    unsigned int m_uiMaterialFlags;
    float m_fLineWidth;
    float m_fMaxPixelWidth;

private:

    pcRainShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

};
 
 
 
