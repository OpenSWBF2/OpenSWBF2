#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/RedMaterial.h"

class RedTexture;
class pcRedVertexFormat;

// VTABLE: SWBF2 0x00784b24
class pcSampleShader : public pcRedShader
{
public:

    pcSampleShader(unsigned int uiFlags,
                   const unsigned int* pParams,
                   unsigned int uiNumParams,
                   unsigned int uiMaterialFlags,
                   const RedColor& color0,
                   const RedColor& color1,
                   float fArg7);

    virtual ~pcSampleShader()
    {
    }

    // SYNTHETIC: SWBF2 0x00705660
    // pcSampleShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00705850
    // pcSampleShader::GetID
    // FUNCTION: SWBF2 0x00705880
    // pcSampleShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00705810
    // pcSampleShader::Bind
    // FUNCTION: SWBF2 0x00705890
    // pcSampleShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcSampleShader)

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

    void SetInputTexture(RedTexture* pTexture) { m_inputTexture = pTexture; }
    void SetOutputPosition(float fX, float fY)
    {
        m_outputX = fX;
        m_outputY = fY;
    }

    void SetInputOffset(float fX, float fY)
    {
        m_inputOffsetX = fX;
        m_inputOffsetY = fY;
    }
    void SetScale(float fScale)
    {
        m_scale = RedColorValue(fScale, fScale, fScale, fScale);
    }

    RedTexture* m_inputTexture;
    float m_outputX;
    float m_outputY;
    float m_inputOffsetX;
    float m_inputOffsetY;
    RedColorValue m_scale;
    unsigned int m_uiMaterialFlags;
    pcRedVertexFormat* m_pVertexFormat;

private:

    pcSampleShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

};
 
 
 
 
 
