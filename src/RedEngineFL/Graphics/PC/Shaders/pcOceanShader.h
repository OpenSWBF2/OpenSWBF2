#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/RedMaterial.h"
#include "RedEngineFL/Graphics/RedTextureTransform.h"

class RedTexture;
class pcRedVertexFormat;

// VTABLE: SWBF2 0x0078517c
class pcOceanShader : public pcRedShader
{
public:
    pcOceanShader(unsigned int uiFlags,
                  const unsigned int* pParams,
                  unsigned int uiNumParams,
                  unsigned int uiMaterialFlags,
                  const RedColor& color0,
                  const RedColor& color1,
                  float fArg7);

    virtual ~pcOceanShader()
    {
    }

    // SYNTHETIC: SWBF2 0x0070be70
    // pcOceanShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x0070c290
    // pcOceanShader::GetID
    // FUNCTION: SWBF2 0x0070c2c0
    // pcOceanShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x0070c250
    // pcOceanShader::Bind
    // FUNCTION: SWBF2 0x0070c2d0
    // pcOceanShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcOceanShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;
    virtual unsigned int GetZPrepassState() const;
    virtual void RenderZPrepass(const RenderItem* pItem) const;

private:
    virtual void _StaticInit() const;

public:
    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Fixed(unsigned int uiState, unsigned int uiPass) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem* pItem, unsigned int) const;
    void Render_Prog1(const RenderItem* pItem, unsigned int) const;

    void SetBaseMap(RedTexture* pTexture, RedTextureTransform* pTransform);
    void SetFoamMap(RedTexture* pTexture, RedTextureTransform* pTransform);

    pcRedVertexFormat* m_pVertexFormat;
    pcRedVertexFormat* m_pZPrepassVertexInputFormat;
    RedTexture* m_reflectionMap;
    RedTexture* m_foamMap;
    RedTexture* m_baseMap;
    RedTextureTransform m_textureTransform;
    RedTextureTransform m_foamMapTransform;
    RedColorValue m_transmissiveColor;
    RedColorValue m_reflectionColor;

private:
    pcOceanShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);
};
 
 
 
 
 
 
 
