#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class RedTexture;
class pcRedVertexFormat;

// VTABLE: SWBF2 0x00784c4c
class pcClearShader : public pcRedShader
{
public:

    enum BlendModeT
    {
        BLENDMODE_NONE = 0,
        BLENDMODE_BLEND = 1,
        BLENDMODE_ADD = 2,
        BLENDMODE_MODULATE = 3,
        BLENDMODE_CONSTANT_BLEND = 4,
    };

    pcClearShader(unsigned int uiFlags,
                  const unsigned int* pParams,
                  unsigned int uiNumParams,
                  unsigned int uiMaterialFlags,
                  const RedColor& color0,
                  const RedColor& color1,
                  float fArg7);

    virtual ~pcClearShader()
    {
    }

    // SYNTHETIC: SWBF2 0x00706990
    // pcClearShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00706ad0
    // pcClearShader::GetID
    // FUNCTION: SWBF2 0x00706b00
    // pcClearShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00706aa0
    // pcClearShader::Bind
    // FUNCTION: SWBF2 0x00706b10
    // pcClearShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcClearShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

private:

    virtual void _StaticInit() const;




public:

    unsigned int GetState_Fixed(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem*, unsigned int) const;

    void SetBlendMode(BlendModeT mode) { m_blendMode = mode; }

    pcRedVertexFormat* m_pVertexFormat;
    RedTexture* m_pTex;
    RedTexture* m_pAlphaMaskTex;
    unsigned int m_uiMaterialFlags;
    BlendModeT m_blendMode;

private:

    pcClearShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

};
 
 
 
 
