#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x00784da4
class pcStencilShadowShader : public pcRedShader
{
public:

    enum MODE
    {
        MODE_CPU_EXTRUSION = 0,
        MODE_GPU_EXTRUSION_VERTEXNORMAL_UNSKINNED = 1,
        MODE_GPU_EXTRUSION_FACENORMAL_SKINNEDHARD = 2,
        MODE_GPU_EXTRUSION_GENERATENORMAL_SKINNEDHARD = 3,
        MODE_GPU_EXTRUSION_FACENORMAL_SKINNEDSOFT = 4,
        MODE_GPU_EXTRUSION_GENERATENORMAL_SKINNEDSOFT = 5,
        NUM_MODES = 6,
        MODE_FORCE_DWORD = 0x7fffffff
    };

    pcStencilShadowShader(unsigned int uiFlags,
                          const unsigned int* pParams,
                          unsigned int uiNumParams,
                          unsigned int uiMaterialFlags,
                          const RedColor& color0,
                          const RedColor& color1,
                          float fArg7);

    virtual ~pcStencilShadowShader()
    {
    }

    // SYNTHETIC: SWBF2 0x007070b0
    // pcStencilShadowShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00707350
    // pcStencilShadowShader::GetID
    // FUNCTION: SWBF2 0x00707390
    // pcStencilShadowShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00707310
    // pcStencilShadowShader::Bind
    // FUNCTION: SWBF2 0x007073a0
    // pcStencilShadowShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcStencilShadowShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

    void SetMode(MODE eMode);

private:
    virtual void _StaticInit() const;

    void RenderVolume(const RenderItem* pItem, unsigned int uiPass) const;

public:
    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem* pItem) const;
    void Begin_Fixed(unsigned int, unsigned int uiPass) const;
    void Begin_Prog1(unsigned int, unsigned int uiPass) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem* pItem, unsigned int uiPass) const;
    void Render_Prog1(const RenderItem* pItem, unsigned int uiPass) const;

    pcRedVertexFormat* m_pVertexFormat;
    MODE m_mode;

    // GLOBAL: SWBF2 0x009c9cc0
    static pcRedVertexFormat* s_pSkinnedInputFormat[4];

    // GLOBAL: SWBF2 0x007e1d80
    static unsigned int s_uiStencilMask;

private:

    pcStencilShadowShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

public:
    static const unsigned int s_stencilRef;
};
 
 
 
