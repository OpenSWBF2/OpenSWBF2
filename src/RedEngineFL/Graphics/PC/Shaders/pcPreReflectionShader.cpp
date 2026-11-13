#include <string.h>
#include "pcPreReflectionShader.h"

#include "PebbleFL/Common/PblChunk.h"
#include "RedEngineFL/Memory/engineAlloc.h"
#include "RedEngineFL/Graphics/PC/pcRedPrimitive.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedVertexFormat.h"

#pragma init_seg(lib)

void __cdecl PblTraceP(const char* Format, ...);

// GLOBAL: SWBF2 0x009ca0dc
// pcPreReflectionShader::s_uiID
unsigned int pcPreReflectionShader::s_uiID;
// GLOBAL: SWBF2 0x009ca0e0
// pcPreReflectionShader::GetStateFuncArray
pcRedShader::GetStateFuncT pcPreReflectionShader::GetStateFuncArray[MAX_PIPELINES];
// GLOBAL: SWBF2 0x009ca0ec
// pcPreReflectionShader::BeginFuncArray
pcRedShader::BeginFuncT pcPreReflectionShader::BeginFuncArray[MAX_PIPELINES];
// GLOBAL: SWBF2 0x009ca0f8
// pcPreReflectionShader::EndFuncArray
pcRedShader::EndFuncT pcPreReflectionShader::EndFuncArray[MAX_PIPELINES];
// GLOBAL: SWBF2 0x009ca104
// pcPreReflectionShader::RenderFuncArray
pcRedShader::RenderFuncT pcPreReflectionShader::RenderFuncArray[MAX_PIPELINES];

// FUNCTION: SWBF2 0x0070afc0
pcPreReflectionShader::pcPreReflectionShader()
{
    memset(GetStateFuncArray, 0, sizeof(GetStateFuncArray));
    memset(BeginFuncArray, 0, sizeof(BeginFuncArray));
    memset(EndFuncArray, 0, sizeof(EndFuncArray));
    memset(RenderFuncArray, 0, sizeof(RenderFuncArray));

    s_uiID = _Register(s_uiRenderTypeHash, this);

    _InitPipelineFuncArray();
}

// FUNCTION: SWBF2 0x0070b080
pcPreReflectionShader::pcPreReflectionShader(unsigned int uiFlags,
                                             const unsigned int* pParams,
                                             unsigned int uiNumParams,
                                             unsigned int uiMaterialFlags,
                                             const RedColor& color0,
                                             const RedColor& color1,
                                             float fArg7)
{
    _BindPipeline(GetStateFuncArray, BeginFuncArray, EndFuncArray, RenderFuncArray);
    _Init(uiFlags, pParams, uiNumParams, uiMaterialFlags, color0, color1, fArg7);
}

// FUNCTION: SWBF2 0x0070b140
void pcPreReflectionShader::Begin_Fixed(unsigned int, unsigned int) const
{
    RedRenderer::pcSetCullMode(PCREDCULL_NONE);
    RedRenderer::pcSetRenderState(D3DRS_FOGENABLE, FALSE);
    RedRenderer::pcSetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    RedRenderer::pcSetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    RedRenderer::pcSetRenderState(D3DRS_COLORWRITEENABLE, 0);
}

// FUNCTION: SWBF2 0x0070b180
void pcPreReflectionShader::Begin_Prog1(unsigned int, unsigned int) const
{
    RedRenderer::pcSetCullMode(PCREDCULL_NONE);
    RedRenderer::pcSetRenderState(D3DRS_FOGENABLE, FALSE);
    RedRenderer::pcSetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    RedRenderer::pcSetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    RedRenderer::pcSetRenderState(D3DRS_COLORWRITEENABLE, 0);
}

// FUNCTION: SWBF2 0x0070b1e0
void pcPreReflectionShader::End_Fixed(unsigned int, unsigned int) const
{
}

// FUNCTION: SWBF2 0x0070b1f0
void pcPreReflectionShader::End_Prog1(unsigned int, unsigned int) const
{
}

// FUNCTION: SWBF2 0x0070b220
RenderPhaseT pcPreReflectionShader::GetRenderPhase(const RenderItem*, unsigned int) const
{
    return RENDERPHASE_OPAQUE;
}

// FUNCTION: SWBF2 0x0070b250
unsigned int pcPreReflectionShader::GetState_Fixed(const RenderItem*) const
{
    return 1;
}

// FUNCTION: SWBF2 0x0070b260
unsigned int pcPreReflectionShader::GetState_Prog1(const RenderItem*) const
{
    return 1;
}

// FUNCTION: SWBF2 0x0070b270
void pcPreReflectionShader::InitFixedFunctionShaderDef() const
{
    // GLOBAL: SWBF2 0x00785090
    static const pcRedShaderDef::FixedFunctionStateT aFFStates[1] =
    {
        { 1, { 0x11, 0x00, 0x00, 0x00 } },
    };

    GetShaderDef()->InitFixedFunction(0, 1, aFFStates);
}

// FUNCTION: SWBF2 0x0070b290
void pcPreReflectionShader::Render_Fixed(const RenderItem* pItem, unsigned int) const
{
    RedRenderer::pcLoadFormat(m_pVertexFormat);
    pItem->pPrim->Draw();
}

// FUNCTION: SWBF2 0x0070b2b0
void pcPreReflectionShader::Render_Prog1(const RenderItem* pItem, unsigned int) const
{
    RedRenderer::pcLoadFormat(m_pVertexFormat);
    pItem->pPrim->Draw();
}

// FUNCTION: SWBF2 0x0070b2d0
pcRedShader* pcPreReflectionShader::_Create(unsigned int uiFlags,
                                            const unsigned int* pParams,
                                            unsigned int uiNumParams,
                                            unsigned int uiMaterialFlags,
                                            const RedColor& color0,
                                            const RedColor& color1,
                                            float fArg7) const
{
    return new pcPreReflectionShader(uiFlags, pParams, uiNumParams, uiMaterialFlags,
                                     color0, color1, fArg7);
}

// FUNCTION: SWBF2 0x0070b360
void pcPreReflectionShader::_Init(unsigned int uiFlags,
                                  const unsigned int*,
                                  unsigned int,
                                  unsigned int uiMaterialFlags,
                                  const RedColor&,
                                  const RedColor&,
                                  float)
{
    m_uiMaterialFlags = uiMaterialFlags;
    m_pVertexFormat = pcRedVertexFormat::Create(uiFlags, 2);
}

// FUNCTION: SWBF2 0x0070b390
void pcPreReflectionShader::_InitPipelineFuncArray()
{
    GetStateFuncArray[PIPELINE_FIXED] =
        (GetStateFuncT)(&pcPreReflectionShader::GetState_Fixed);
    BeginFuncArray[PIPELINE_FIXED] =
        (BeginFuncT)(&pcPreReflectionShader::Begin_Fixed);
    EndFuncArray[PIPELINE_FIXED] =
        (EndFuncT)(&pcPreReflectionShader::End_Fixed);
    RenderFuncArray[PIPELINE_FIXED] =
        (RenderFuncT)(&pcPreReflectionShader::Render_Fixed);

    GetStateFuncArray[PIPELINE_PROG1] =
        (GetStateFuncT)(&pcPreReflectionShader::GetState_Prog1);
    BeginFuncArray[PIPELINE_PROG1] =
        (BeginFuncT)(&pcPreReflectionShader::Begin_Prog1);
    EndFuncArray[PIPELINE_PROG1] =
        (EndFuncT)(&pcPreReflectionShader::End_Prog1);
    RenderFuncArray[PIPELINE_PROG1] =
        (RenderFuncT)(&pcPreReflectionShader::Render_Prog1);
}

// FUNCTION: SWBF2 0x0070b3f0
void pcPreReflectionShader::_Read(PblFileChunk& chunk) const
{
    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        if (child._iId != CHUNK_ID('N', 'A', 'M', 'E'))
        {
            continue;
        }

        char acName[256];
        child.ReadString(acName, 256);
        const unsigned int uiHash = PblHash(acName);
        PblTraceP("pcRedShaderDef [%s]\tHash 0x%08x\n", acName, uiHash);

        pcRedShaderDef* pDef = GetShaderDef();
        if (pDef)
        {
            pDef->Read(chunk);
        }
        else
        {
            PblTraceP("Error: pcRedShaderDef 0x%08x not found\n", uiHash);
        }
    }
}

// FUNCTION: SWBF2 0x0070b4d0
void pcPreReflectionShader::_StaticInit() const
{
}

// SYNTHETIC: SWBF2 0x0040a0d0
// `dynamic initializer for 'pcPreReflectionShader::s_uiRenderTypeHash''

// GLOBAL: SWBF2 0x009ca170
// pcPreReflectionShader::s_uiRenderTypeHash
unsigned int pcPreReflectionShader::s_uiRenderTypeHash = PblHash("PreReflection");

// SYNTHETIC: SWBF2 0x0040a0f0
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x0040a110
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

// SYNTHETIC: SWBF2 0x0040a090
// `dynamic initializer for 'pcPreReflectionShader::s_Factory''
// SYNTHETIC: SWBF2 0x00769ea0
// `dynamic atexit destructor for 'pcPreReflectionShader::s_Factory''

// GLOBAL: SWBF2 0x009ca148
// pcPreReflectionShader::s_Factory
pcPreReflectionShader pcPreReflectionShader::s_Factory;

// SYNTHETIC: SWBF2 0x0040a0b0
// `dynamic initializer for 'pcPreReflectionShader::s_shaderDef''
// SYNTHETIC: SWBF2 0x00769ec0
// `dynamic atexit destructor for 'pcPreReflectionShader::s_shaderDef''

// GLOBAL: SWBF2 0x009ca11c
// pcPreReflectionShader::s_shaderDef
pcRedShaderDef pcPreReflectionShader::s_shaderDef("prereflection_shader.xml");

// GLOBAL: SWBF2 0x009ca110
unsigned int g_pcPreReflectionShader;
