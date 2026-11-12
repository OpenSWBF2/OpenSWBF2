#pragma once

#ifndef SWBF2_GRAPHICS_PC_STATEMANAGER_DX_H
#define SWBF2_GRAPHICS_PC_STATEMANAGER_DX_H

#include <d3d9.h>
#include <d3dx9math.h>
#include <new>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Util/ext/Globals.h"
#include "RedEngineFL/Util/ext/basevector.h"
#include "RedEngineFL/Util/ext/types.h"

namespace util
{

template <class ELEM, class KEY>
struct TKeyPair_
{
    TKeyPair_() : key_(0) {}
    TKeyPair_(ELEM const& elem, KEY key) : elem_(elem), key_(key) {}

    bool operator>=(TKeyPair_ const& rOther) const { return key_ >= rOther.key_; }

    ELEM elem_;
    KEY key_;
};

}

namespace dx
{

class NVStateID
{
public:
    NVStateID() : m_Type(0), m_dwValue0(0), m_dwValue1(0) {}

    NVStateID(unsigned long type, unsigned long value0, unsigned long value1)
        : m_Type(type), m_dwValue0(value0), m_dwValue1(value1) {}

    unsigned long m_Type;
    union
    {
        unsigned long m_FVF;
        unsigned long m_dwValue0;
        unsigned long m_dwStage;
        _D3DRENDERSTATETYPE m_RenderStateType;
        _D3DTRANSFORMSTATETYPE m_TransformStateType;
    };
    union
    {
        unsigned long m_dwValue1;
        _D3DTEXTURESTAGESTATETYPE m_TextureStageStateType;
        _D3DSAMPLERSTATETYPE m_SamplerStateType;
    };
};

class NVStreamData
{
public:
    NVStreamData() : m_pStreamData(0), m_Offset(0), m_Stride(0) {}

    IDirect3DVertexBuffer9* m_pStreamData;
    unsigned int m_Offset;
    unsigned int m_Stride;
};

class NVIndexData
{
public:
    NVIndexData() : m_pIndexData(0), m_BaseVertexIndex(0) {}

    void* m_pIndexData;
    unsigned int m_BaseVertexIndex;
};

class StateManagerState
{
public:

    StateManagerState();

    void ResetState();

    void CaptureDeviceState(IDirect3DDevice9* pD3DDev);

    bool m_ValidRenderStates[256];
    unsigned long m_RenderStates[256];
    bool m_ValidTextureStates[128];

    unsigned long m_TextureStates[8][128];
    bool m_ValidSamplerStates[128];
    unsigned long m_SamplerStates[16][128];
    D3DXVECTOR4 m_VertexShaderConstants[1024];
    D3DXVECTOR4 m_PixelShaderConstants[1024];
    IDirect3DBaseTexture9* m_Textures[8];
    unsigned long m_FVF;
    IDirect3DPixelShader9* m_PixelShader;
    IDirect3DVertexShader9* m_VertexShader;
    IDirect3DVertexDeclaration9* m_VertexShaderDecl;
    D3DXMATRIX m_Matrices[300];
    _D3DMATERIAL9 m_D3DMaterial;
    NVStreamData m_StreamData[16];
    NVIndexData m_IndexData;
};

// VTABLE: SWBF2 0x007857d0
class StateManager_DX
{
public:

    StateManager_DX(IDirect3DDevice9* pD3DDev);

    virtual ~StateManager_DX();

    // SYNTHETIC: SWBF2 0x00715e10
    // dx::StateManager_DX::`scalar deleting destructor'

    static void ClearInstance();

    static void CreateInstance(IDirect3DDevice9* pD3DDev);

    static StateManager_DX& GetInstance();

    void ResetAndCaptureStates();

    void SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);

    void SetIndices(IDirect3DIndexBuffer9* pIndexData);
    void SetPixelShader(IDirect3DPixelShader9* pShader);
    void SetSamplerState(unsigned long ulSampler, D3DSAMPLERSTATETYPE eType,
                         unsigned long ulValue);
    void SetStreamSource(unsigned int uiStream, IDirect3DVertexBuffer9* pStreamData,
                         unsigned int uiOffset, unsigned int uiStride);
    bool SetTexture(unsigned long ulStage, IDirect3DBaseTexture9* pTexture);
    void SetTextureStageState(unsigned long ulStage, D3DTEXTURESTAGESTATETYPE eType,
                              unsigned long ulValue);
    void SetVertexShader(IDirect3DVertexShader9* pShader);

    void FlushState();
    void FlushStateEx_();

    long DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, unsigned int StartVertex,
                       unsigned int PrimitiveCount)
    {
        FlushState();
        return m_pD3DDev->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
    }
    long DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, unsigned int BaseVertexIndex,
                              unsigned int MinVertexIndex, unsigned int NumVertices,
                              unsigned int startIndex, unsigned int primCount)
    {
        FlushState();
        return m_pD3DDev->DrawIndexedPrimitive(Type, BaseVertexIndex, MinVertexIndex, NumVertices,
                                               startIndex, primCount);
    }

    StateManagerState m_BackupState;
    StateManagerState m_ChipState;
    StateManagerState m_CurrentState;
    StateManagerState m_CopyState;

    util::Vector<NVStateID> m_DirtyStates;
    util::Vector<NVStateID> renderStates_;
    util::Vector<NVStateID> textureStageStates_;
    util::Vector<NVStateID> samplerStates_;
    util::Vector<NVStateID> textureStates_;
    bool m_bForce;
    bool caching_;
    char pad_40932[2];
    IDirect3DDevice9* m_pD3DDev;
    D3DCAPS9 capabilities_;
};

extern StateManager_DX* localInstance_;

}

// FUNCTION: SWBF2 0x006b3dc0
inline bool dx::StateManager_DX::SetTexture(unsigned long ulStage, IDirect3DBaseTexture9* pTexture)
{
    if (!m_bForce && m_CurrentState.m_Textures[ulStage] == pTexture)
    {
        return false;
    }
    textureStates_.Add(NVStateID(2, ulStage, 0));
    m_CurrentState.m_Textures[ulStage] = pTexture;
    return true;
}

// FUNCTION: SWBF2 0x006b3e30
inline void dx::StateManager_DX::SetTextureStageState(unsigned long ulStage, D3DTEXTURESTAGESTATETYPE eType,
                                               unsigned long ulValue)
{
    if (m_bForce || m_CurrentState.m_TextureStates[ulStage][eType] != ulValue)
    {
        textureStageStates_.Add(NVStateID(3, ulStage, eType));
        m_CurrentState.m_TextureStates[ulStage][eType] = ulValue;
    }
}

// FUNCTION: SWBF2 0x006b3be0
inline void dx::StateManager_DX::SetSamplerState(unsigned long ulSampler, D3DSAMPLERSTATETYPE eType,
                                          unsigned long ulValue)
{
    if (m_bForce || m_CurrentState.m_SamplerStates[ulSampler][eType] != ulValue)
    {
        samplerStates_.Add(NVStateID(4, ulSampler, eType));
        m_CurrentState.m_SamplerStates[ulSampler][eType] = ulValue;
    }
}

// FUNCTION: SWBF2 0x006b3ec0
inline void dx::StateManager_DX::SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix)
{
    if (pMatrix)
    {
        m_DirtyStates.Add(NVStateID(9, State, 0));
        const D3DMATRIX matrix = *pMatrix;
        memcpy(&m_CurrentState.m_Matrices[State], &matrix, sizeof(D3DMATRIX));
    }
}

#define COUNT_PERFRAME_VALUE(COUNTER, NAME)                                                              \
    {                                                                                            \
        static util::VariableSet<util::SingletonAllocator<util::DefaultStaticAllocator> >&       \
            vset = util::Globals::GetInstance().GetSet("perFrame");                        \
        static int& COUNTER = vset.GetInt(NAME);                                           \
        ++COUNTER;                                                                               \
    }

// FUNCTION: SWBF2 0x006ae270
inline void dx::StateManager_DX::FlushStateEx_()
{
    if (m_bForce)
    {
        for (int i = 0; i < renderStates_.GetSize(); ++i)
        {
            const NVStateID& rId = renderStates_[i];
            m_pD3DDev->SetRenderState(rId.m_RenderStateType,
                                      m_CurrentState.m_RenderStates[rId.m_RenderStateType]);
            m_ChipState.m_RenderStates[rId.m_RenderStateType] =
                m_CurrentState.m_RenderStates[rId.m_RenderStateType];
        }
        for (int i = 0; i < textureStageStates_.GetSize(); ++i)
        {
            const NVStateID& rId = textureStageStates_[i];
            m_pD3DDev->SetTextureStageState(rId.m_dwStage, rId.m_TextureStageStateType,
                                            m_CurrentState.m_TextureStates[rId.m_dwStage][rId.m_TextureStageStateType]);
            m_ChipState.m_TextureStates[rId.m_dwStage][rId.m_TextureStageStateType] =
                m_CurrentState.m_TextureStates[rId.m_dwStage][rId.m_TextureStageStateType];
        }
        for (int i = 0; i < samplerStates_.GetSize(); ++i)
        {
            const NVStateID& rId = samplerStates_[i];
            m_pD3DDev->SetSamplerState(rId.m_dwStage, rId.m_SamplerStateType,
                                       m_CurrentState.m_SamplerStates[rId.m_dwStage][rId.m_SamplerStateType]);
            m_ChipState.m_SamplerStates[rId.m_dwStage][rId.m_SamplerStateType] =
                m_CurrentState.m_SamplerStates[rId.m_dwStage][rId.m_SamplerStateType];
        }
        for (int i = 0; i < textureStates_.GetSize(); ++i)
        {
            COUNT_PERFRAME_VALUE(numSetTex, "numSetTex");
            const NVStateID& rId = textureStates_[i];
            m_pD3DDev->SetTexture(rId.m_dwStage, m_CurrentState.m_Textures[rId.m_dwStage]);
            m_ChipState.m_Textures[rId.m_dwStage] = m_CurrentState.m_Textures[rId.m_dwStage];
        }
    }
    else
    {
        for (int i = 0; i < renderStates_.GetSize(); ++i)
        {
            const NVStateID& rId = renderStates_[i];
            if (m_ChipState.m_RenderStates[rId.m_RenderStateType] !=
                m_CurrentState.m_RenderStates[rId.m_RenderStateType])
            {
                m_pD3DDev->SetRenderState(rId.m_RenderStateType,
                                          m_CurrentState.m_RenderStates[rId.m_RenderStateType]);
                m_ChipState.m_RenderStates[rId.m_RenderStateType] =
                    m_CurrentState.m_RenderStates[rId.m_RenderStateType];
            }
        }
        for (int i = 0; i < textureStageStates_.GetSize(); ++i)
        {
            const NVStateID& rId = textureStageStates_[i];
            const unsigned long ulValue =
                m_CurrentState.m_TextureStates[rId.m_dwStage][rId.m_TextureStageStateType];
            if (m_ChipState.m_TextureStates[rId.m_dwStage][rId.m_TextureStageStateType] != ulValue)
            {
                m_pD3DDev->SetTextureStageState(rId.m_dwStage, rId.m_TextureStageStateType, ulValue);
                m_ChipState.m_TextureStates[rId.m_dwStage][rId.m_TextureStageStateType] =
                    m_CurrentState.m_TextureStates[rId.m_dwStage][rId.m_TextureStageStateType];
            }
        }
        for (int i = 0; i < samplerStates_.GetSize(); ++i)
        {
            const NVStateID& rId = samplerStates_[i];
            const unsigned long ulValue =
                m_CurrentState.m_SamplerStates[rId.m_dwStage][rId.m_SamplerStateType];
            if (m_ChipState.m_SamplerStates[rId.m_dwStage][rId.m_SamplerStateType] != ulValue)
            {
                m_pD3DDev->SetSamplerState(rId.m_dwStage, rId.m_SamplerStateType, ulValue);
                m_ChipState.m_SamplerStates[rId.m_dwStage][rId.m_SamplerStateType] =
                    m_CurrentState.m_SamplerStates[rId.m_dwStage][rId.m_SamplerStateType];
            }
        }
        for (int i = 0; i < textureStates_.GetSize(); ++i)
        {
            const NVStateID& rId = textureStates_[i];
            if (m_ChipState.m_Textures[rId.m_dwStage] != m_CurrentState.m_Textures[rId.m_dwStage])
            {
                COUNT_PERFRAME_VALUE(numSetTex, "numSetTex");
                m_pD3DDev->SetTexture(rId.m_dwStage, m_CurrentState.m_Textures[rId.m_dwStage]);
                m_ChipState.m_Textures[rId.m_dwStage] = m_CurrentState.m_Textures[rId.m_dwStage];
            }
        }
    }
}

// FUNCTION: SWBF2 0x006adb60
inline void dx::StateManager_DX::FlushState()
{
    FlushStateEx_();

    for (NVStateID* pState = m_DirtyStates.Begin(); pState != m_DirtyStates.End(); ++pState)
    {
        switch (pState->m_Type)
        {
        case 0:
            m_pD3DDev->SetMaterial(&m_CurrentState.m_D3DMaterial);
            m_ChipState.m_D3DMaterial = m_CurrentState.m_D3DMaterial;
            break;
        case 7:
            m_pD3DDev->SetFVF(m_CurrentState.m_FVF);
            m_ChipState.m_FVF = m_CurrentState.m_FVF;
            break;
        case 5:
            if (m_bForce || m_ChipState.m_VertexShader != m_CurrentState.m_VertexShader)
            {
                COUNT_PERFRAME_VALUE(numcalls, "numSetVShader");
                m_pD3DDev->SetVertexShader(m_CurrentState.m_VertexShader);
                m_ChipState.m_VertexShader = m_CurrentState.m_VertexShader;
            }
            break;
        case 6:
            m_pD3DDev->SetVertexDeclaration(m_CurrentState.m_VertexShaderDecl);
            m_ChipState.m_VertexShaderDecl = m_CurrentState.m_VertexShaderDecl;
            break;
        case 8:
            if (m_bForce || m_ChipState.m_PixelShader != m_CurrentState.m_PixelShader)
            {
                COUNT_PERFRAME_VALUE(numcalls, "numSetPShader");
                m_pD3DDev->SetPixelShader(m_CurrentState.m_PixelShader);
                m_ChipState.m_PixelShader = m_CurrentState.m_PixelShader;
            }
            break;
        case 9:
        {
            m_pD3DDev->SetTransform((D3DTRANSFORMSTATETYPE)pState->m_dwValue0,
                                    &m_CurrentState.m_Matrices[pState->m_dwValue0]);
            const D3DMATRIX matrix = m_CurrentState.m_Matrices[pState->m_dwValue0];
            memcpy(&m_ChipState.m_Matrices[pState->m_dwValue0], &matrix, sizeof(D3DMATRIX));
            break;
        }
        case 10:
            if (!m_bForce)
            {
                if (m_ChipState.m_VertexShaderConstants[pState->m_dwValue0] == m_CurrentState.m_VertexShaderConstants[pState->m_dwValue0])
                {
                    break;
                }
            }
            m_pD3DDev->SetVertexShaderConstantF(pState->m_dwValue0, m_CurrentState.m_VertexShaderConstants[pState->m_dwValue0], 1);
            m_ChipState.m_VertexShaderConstants[pState->m_dwValue0] = m_CurrentState.m_VertexShaderConstants[pState->m_dwValue0];
            break;
        case 11:
            if (!m_bForce)
            {
                if (m_ChipState.m_PixelShaderConstants[pState->m_dwValue0] == m_CurrentState.m_PixelShaderConstants[pState->m_dwValue0])
                {
                    break;
                }
            }
            m_pD3DDev->SetPixelShaderConstantF(pState->m_dwValue0, m_CurrentState.m_PixelShaderConstants[pState->m_dwValue0], 1);
            m_ChipState.m_PixelShaderConstants[pState->m_dwValue0] = m_CurrentState.m_PixelShaderConstants[pState->m_dwValue0];
            break;
        case 12:
            if (m_bForce
                || m_ChipState.m_StreamData[pState->m_dwStage].m_pStreamData
                       != m_CurrentState.m_StreamData[pState->m_dwStage].m_pStreamData
                || m_ChipState.m_StreamData[pState->m_dwStage].m_Stride
                       != m_CurrentState.m_StreamData[pState->m_dwStage].m_Stride)
            {
                COUNT_PERFRAME_VALUE(numSetStream, "numSetStream");
                m_pD3DDev->SetStreamSource(pState->m_dwStage,
                                           m_CurrentState.m_StreamData[pState->m_dwStage].m_pStreamData,
                                           m_CurrentState.m_StreamData[pState->m_dwStage].m_Offset,
                                           m_CurrentState.m_StreamData[pState->m_dwStage].m_Stride);
                m_ChipState.m_StreamData[pState->m_dwStage] = m_CurrentState.m_StreamData[pState->m_dwStage];
            }
            break;
        case 13:
            if (m_bForce
                || m_ChipState.m_IndexData.m_pIndexData != m_CurrentState.m_IndexData.m_pIndexData
                || m_ChipState.m_IndexData.m_BaseVertexIndex != m_CurrentState.m_IndexData.m_BaseVertexIndex)
            {
                COUNT_PERFRAME_VALUE(numSetIndices, "numSetIndices");
                m_pD3DDev->SetIndices((IDirect3DIndexBuffer9*)m_CurrentState.m_IndexData.m_pIndexData);
                m_ChipState.m_IndexData = m_CurrentState.m_IndexData;
            }
            break;
        default:
            break;
        }
    }

    m_DirtyStates.SetSize(0);
    renderStates_.SetSize(0);
    textureStageStates_.SetSize(0);
    samplerStates_.SetSize(0);
    textureStates_.SetSize(0);
}

#undef COUNT_PERFRAME_VALUE

#endif
