#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class pcRedVertexFormat;

// VTABLE: SWBF2 0x00785018
class pcPreReflectionShader : public pcRedShader
{
public:

    pcPreReflectionShader(unsigned int uiFlags,
                          const unsigned int* pParams,
                          unsigned int uiNumParams,
                          unsigned int uiMaterialFlags,
                          const RedColor& color0,
                          const RedColor& color1,
                          float fArg7);

    virtual ~pcPreReflectionShader()
    {
    }

    // SYNTHETIC: SWBF2 0x0070b110
    // pcPreReflectionShader::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x0070b200
    // pcPreReflectionShader::GetID
    // FUNCTION: SWBF2 0x0070b230
    // pcPreReflectionShader::GetRenderTypeHash
    // FUNCTION: SWBF2 0x0070b1c0
    // pcPreReflectionShader::Bind
    // FUNCTION: SWBF2 0x0070b240
    // pcPreReflectionShader::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcPreReflectionShader)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;

    void SetReflectionID(char id) { m_reflectionID = id; }

private:
    virtual void _StaticInit() const;

public:
    unsigned int GetState_Fixed(const RenderItem*) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem* pItem, unsigned int) const;

    unsigned int GetState_Prog1(const RenderItem*) const;
    void Begin_Prog1(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Prog1(const RenderItem* pItem, unsigned int) const;

    unsigned int m_uiMaterialFlags;
    char m_reflectionID;
    pcRedVertexFormat* m_pVertexFormat;

private:

    pcPreReflectionShader();

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fArg7);

};
 
 
 
 
