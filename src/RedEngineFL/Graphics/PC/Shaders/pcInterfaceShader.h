#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

// VTABLE: SWBF2 0x007846f8
class pcInterfaceShader : public pcRedShader
{
public:

    enum ModeT
    {
        INTERFACE_MODE_MASKEDBITMAP = 0,
        INTERFACE_MODE_VECTOR = 1,
        INTERFACE_MODE_BITMAP = 2
    };

    // FUNCTION: SWBF2 0x00701840
    pcInterfaceShader(unsigned int uiFlags,
                      const unsigned int* pParams,
                      unsigned int uiNumParams,
                      unsigned int uiMaterialFlags,
                      const RedColor& color0,
                      const RedColor& color1,
                      float fArg7);

    virtual ~pcInterfaceShader()
    {
    }

    // SYNTHETIC: SWBF2 0x007018d0
    // pcInterfaceShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00701aa0
    // pcInterfaceShader::GetID
    // FUNCTION: SWBF2 0x00701ad0
    // pcInterfaceShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x00701a60
    // pcInterfaceShader::Bind
    // FUNCTION: SWBF2 0x00701ae0
    // pcInterfaceShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcInterfaceShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

private:

    // FUNCTION: SWBF2 0x00702330
    virtual void _StaticInit() const;

    // FUNCTION: SWBF2 0x00701b30

    // FUNCTION: SWBF2 0x00702120

    // FUNCTION: SWBF2 0x00701ae0

public:

    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem*, unsigned int) const;
    void Render_Prog1(const RenderItem*, unsigned int) const;

    unsigned int GetTexture() const { return m_textureHash; }

    void SetTexture(unsigned int uiTextureHash) { m_textureHash = uiTextureHash; }

    void SetBitmapMask(unsigned int uiMaskHash) { m_bitmapMaskHash = uiMaskHash; }

    unsigned int m_textureHash;
    unsigned int m_bitmapMaskHash;

    bool m_shadowEnable;
    ModeT m_mode;
    unsigned int m_materialFlags;

    // GLOBAL: SWBF2 0x009c9528
    static pcRedVertexFormat* s_pBitmapTexVertexFormat;

    // GLOBAL: SWBF2 0x009c952c
    static pcRedVertexFormat* s_pBitmapPosVertexFormat;

    // GLOBAL: SWBF2 0x009c9530
    static pcRedVertexFormat* s_pMaskedVertexFormat;

    // GLOBAL: SWBF2 0x009c9534
    static pcRedVertexFormat* s_pVectorVertexFormat;

private:

    // FUNCTION: SWBF2 0x00701780
    pcInterfaceShader();

    // FUNCTION: SWBF2 0x007021b0
    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

    // FUNCTION: SWBF2 0x007021f0
};
 
 
 
 
 
 
