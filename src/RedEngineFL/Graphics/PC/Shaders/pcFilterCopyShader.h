#pragma once

#include <stddef.h>
#include <string.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class RedTexture;
class pcRedVertexFormat;

struct FilterCopyShaderSample
{
    float fValue;
    float fOffsetX;
    float fOffsetY;
};

// VTABLE: SWBF2 0x0078539c
class pcFilterCopyShader : public pcRedShader
{
public:

    pcFilterCopyShader(unsigned int uiFlags,
                       const unsigned int* pParams,
                       unsigned int uiNumParams,
                       unsigned int uiMaterialFlags,
                       const RedColor& color0,
                       const RedColor& color1,
                       float fArg7);

    virtual ~pcFilterCopyShader()
    {
    }

    // SYNTHETIC: SWBF2 0x0070f960
    // pcFilterCopyShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x0070fd40
    // pcFilterCopyShader::GetID
    // FUNCTION: SWBF2 0x0070fd70
    // pcFilterCopyShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x0070fd00
    // pcFilterCopyShader::Bind
    // FUNCTION: SWBF2 0x0070fd80
    // pcFilterCopyShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcFilterCopyShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

private:

    virtual void _StaticInit() const;




public:
    // FUNCTION: SWBF2 0x007102e0
    virtual void SetTextures(RedTexture* pSrc, RedTexture* pDest)
    {
        m_srcTex = pSrc;
        m_destTex = pDest;
    }

    // FUNCTION: SWBF2 0x007102b0
    virtual void SetSamples(unsigned int uiNumSamples,
                            FilterCopyShaderSample* const pSamples)
    {
        memcpy(m_samples, pSamples, uiNumSamples * sizeof(FilterCopyShaderSample));
        m_numSamples = uiNumSamples;
    }

    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem*, unsigned int) const;
    void Render_Prog1(const RenderItem*, unsigned int) const;

    pcRedVertexFormat* m_pVertexFormat;
    RedTexture* m_srcTex;
    RedTexture* m_destTex;
    FilterCopyShaderSample m_samples[16];
    unsigned int m_numSamples;

    mutable unsigned int m_curSample;

private:

    pcFilterCopyShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

};
 
 
 
 
 
