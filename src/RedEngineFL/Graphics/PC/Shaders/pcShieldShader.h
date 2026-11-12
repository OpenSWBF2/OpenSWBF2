#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "PebbleFL/Common/PblVector.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x00772e7c
class pcShieldShader : public pcRedShader
{
public:

    pcShieldShader(unsigned int uiFlags,
                   const unsigned int* pParams,
                   unsigned int uiNumParams,
                   unsigned int uiMaterialFlags,
                   const RedColor& color0,
                   const RedColor& color1,
                   float fArg7);

    virtual ~pcShieldShader()
    {
    }

    // SYNTHETIC: SWBF2 0x006cce30
    // pcShieldShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x006cd4b0
    // pcShieldShader::GetID
    // FUNCTION: SWBF2 0x006cd4e0
    // pcShieldShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x006cd460
    // pcShieldShader::Bind
    // FUNCTION: SWBF2 0x006cd4f0
    // pcShieldShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcShieldShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;
    virtual RedShadingState* CreateShadingState() const;

    void SetSpecularPower(float fPower);

    void SetTexOffset(float fOffsetU, float fOffsetV)
    {
        m_fOffsetU = fOffsetU;
        m_fOffsetV = fOffsetV;
    }

    void SetViewAngleFactor(float fFactor, float fMinDistSqr)
    {
        m_fViewAngleFactor = fFactor;
        m_fVertexMinDistSqr = fMinDistSqr;
    }

    static void SetNearSceneFade(float fFadeStart, float fFadeEnd)
    {
        s_fFadeStart = fFadeStart;
        s_fFadeEnd = fFadeEnd;
    }

private:

    static PblMatrix _GetNearSceneFadeTextureTransform();

public:
    unsigned int GetState_Fixed(const RenderItem* pItem) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem* pItem, unsigned int) const;
    void Render_Prog1(const RenderItem* pItem, unsigned int) const;

    pcRedVertexFormat* m_pVertexInputFormat;
    unsigned int m_uiTextureHash;
    unsigned int m_uiMaterialFlags;
    RedColor m_colorDiffuse;
    RedColor m_colorSpecular;
    float m_specPower;
    float m_specSizeInv;
    float m_fOffsetU;
    float m_fOffsetV;
    float m_fViewAngleFactor;
    float m_fVertexMinDistSqr;

    // GLOBAL: SWBF2 0x00964fb4
    static float s_fFadeStart;

    // GLOBAL: SWBF2 0x00964fb8
    static float s_fFadeEnd;

    // GLOBAL: SWBF2 0x0096504c
    static PblVector3 s_vLightDir;

    // GLOBAL: SWBF2 0x00965058
    static PblVector3 s_vLightX;

    // GLOBAL: SWBF2 0x00965064
    static PblVector3 s_vLightY;

private:

    pcShieldShader();


    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);
};
 
 
 
 
 
