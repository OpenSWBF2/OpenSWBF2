#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

#include "RedEngineFL/Graphics/RedMaterial.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x007845f8
class pcNormalShader : public pcRedShader
{
public:

    virtual ~pcNormalShader()
    {
    }

    // SYNTHETIC: SWBF2 0x006fe9e0
    // pcNormalShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x006ff650
    // pcNormalShader::GetID
    // FUNCTION: SWBF2 0x006ff6a0
    // pcNormalShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x006ff540
    // pcNormalShader::Bind
    // FUNCTION: SWBF2 0x006ff7c0
    // pcNormalShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcNormalShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;
    virtual RedShadingState* CreateShadingState() const;
    virtual void SetData(const RedShaderData&);
    virtual unsigned int GetZPrepassState() const;
    virtual void RenderZPrepass(const RenderItem*) const;
    virtual bool SortAsZWritingTransparent() const;
    virtual bool IsTransparent() const;
    virtual unsigned short GetShaderChain(pcRedShader** const, unsigned short,
                                          const RenderItem*);
    virtual int GetDiffuseMapHash(int, int);

private:

    // FUNCTION: SWBF2 0x007011d0

    // FUNCTION: SWBF2 0x007015a0
    virtual void _StaticInit() const;

    // FUNCTION: SWBF2 0x006ff7c0

    // FUNCTION: SWBF2 0x006ff990

public:

    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem*, unsigned int) const;
    void Render_Prog1(const RenderItem*, unsigned int) const;

    pcRedVertexFormat* m_pVertexInputFormat;
    pcRedVertexFormat* m_pZPrepassVertexInputFormat;

    RedMaterial m_material;
    bool m_lightingInSeparatePass;

private:

public:

    // FUNCTION: SWBF2 0x006fe8b0
    pcNormalShader(unsigned int uiFlags,
                   const unsigned int* pParams,
                   unsigned int uiNumParams,
                   unsigned int uiMaterialFlags,
                   const RedColor& color0,
                   const RedColor& color1,
                   float fSpecularPower);

private:
    // FUNCTION: SWBF2 0x006fe7e0
    pcNormalShader();

    // FUNCTION: SWBF2 0x00701260
    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fSpecularPower);

    // FUNCTION: SWBF2 0x00701460

    void UpdateAnimation();
};
 
 
