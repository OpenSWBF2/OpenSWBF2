#pragma once

#include <stddef.h>
#include "RedEngineFL/Graphics/PC/pcRedShader.h"

class PblVector4;
class RedTexture;

// VTABLE: SWBF2 0x0078447c
class pcTerrainShader2 : public pcRedShader
{
public:

    pcTerrainShader2(unsigned int uiFlags,
                     const unsigned int* pParams,
                     unsigned int uiNumParams,
                     unsigned int uiMaterialFlags,
                     const RedColor& color0,
                     const RedColor& color1,
                     float fSpecularPower);

    virtual ~pcTerrainShader2()
    {
    }

    // SYNTHETIC: SWBF2 0x006fccb0
    // pcTerrainShader2::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x006fd060
    // pcTerrainShader2::GetID
    // FUNCTION: SWBF2 0x006fd090
    // pcTerrainShader2::GetRenderTypeHash
    // FUNCTION: SWBF2 0x006fd010
    // pcTerrainShader2::Bind
    // FUNCTION: SWBF2 0x006fd150
    // pcTerrainShader2::GetShaderDef
    DECLARE_PC_RED_SHADER_METHODS(pcTerrainShader2)

    virtual RenderPhaseT GetRenderPhase(const RenderItem*, unsigned int) const;
    virtual RedShadingState* CreateShadingState() const;
    virtual unsigned int GetZPrepassState() const;
    virtual void RenderZPrepass(const RenderItem* pItem) const;
    virtual unsigned short GetShaderChain(pcRedShader** const ppOut,
                                          unsigned short uiMax,
                                          const RenderItem* pItem);

    unsigned int GetState_Fixed(const RenderItem*) const;
    unsigned int GetState_Prog1(const RenderItem* pItem) const;
    void Begin_Fixed(unsigned int, unsigned int) const;
    void Begin_Prog1(unsigned int uiState, unsigned int uiPass) const;
    void End_Fixed(unsigned int, unsigned int) const;
    void End_Prog1(unsigned int, unsigned int) const;
    void Render_Fixed(const RenderItem*, unsigned int) const;
    void Render_Prog1(const RenderItem* pItem, unsigned int uiPass) const;

    static bool __cdecl IsEnabled();
    static void __cdecl SetEnabled(bool bEnabled);

    static void __cdecl _InitMaps(float fScale);
    static void __cdecl _InitNoiseMap(int iIndex, unsigned int uiHash,
                                      const PblVector4& vScale, const PblVector4& vOffset);
    static void __cdecl _InitDiffuseMap(int iIndex, unsigned int uiHash,
                                        const PblVector4& vScale);
    static void __cdecl _InitLayer(int iIndex, unsigned int uiHash,
                                   const PblVector4& vScale0, const PblVector4& vScale1);
    static void __cdecl _InitDetail(unsigned int uiHash, const PblVector4& vScale0,
                                    const PblVector4& vScale1, float fFadeDistance);
    static void __cdecl _InitBumpLuminosityPixelLighting(unsigned int uiHash,
                                                         unsigned int uiBumpLuminosityHash,
                                                         const PblVector4& vScale0,
                                                         const PblVector4& vScale1,
                                                         float fStaticBumpFactor);
    static void __cdecl _InitPixelLighting(unsigned int uiHashA, unsigned int uiHashB);
    static void __cdecl _InitShadowMap(RedTexture* pTexture, unsigned int uiWrapMode,
                                       const PblVector4& vU, const PblVector4& vV,
                                       const PblVector4& vW);

    unsigned char m_uiLayerIndex[4];
    unsigned char m_uiOptions;
    bool m_lightingInSeparatePass;

private:

    pcTerrainShader2();

    virtual void _StaticInit() const;

    void _Init(unsigned int uiFlags,
               const unsigned int* pParams,
               unsigned int uiNumParams,
               unsigned int uiMaterialFlags,
               const RedColor& color0,
               const RedColor& color1,
               float fSpecularPower);
};
 
 
 
 
