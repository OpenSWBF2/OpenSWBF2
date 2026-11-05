#include <string.h>

#include "StateManager_DX.h"

#include "RedEngineFL/Memory/RedMemory.h"

namespace dx
{

// GLOBAL: SWBF2 0x009caafc
StateManager_DX* localInstance_;

// FUNCTION: SWBF2 0x00715ab0
StateManagerState::StateManagerState()
{
}

// FUNCTION: SWBF2 0x006b29a0
void StateManagerState::CaptureDeviceState(IDirect3DDevice9* pD3DDev)
{
    pD3DDev->GetFVF(&m_FVF);

    for (unsigned int i = 0; i < 256; ++i)
    {
        m_ValidRenderStates[i] = false;
    }
    for (unsigned int i = 0; i < 128; ++i)
    {
        m_ValidTextureStates[i] = false;
    }
    for (unsigned int i = 0; i < 128; ++i)
    {
        m_ValidSamplerStates[i] = false;
    }

    m_ValidRenderStates[D3DRS_ZENABLE] = true;
    m_ValidRenderStates[D3DRS_FILLMODE] = true;
    m_ValidRenderStates[D3DRS_SHADEMODE] = true;
    m_ValidRenderStates[D3DRS_ZWRITEENABLE] = true;
    m_ValidRenderStates[D3DRS_ALPHATESTENABLE] = true;
    m_ValidRenderStates[D3DRS_LASTPIXEL] = true;
    m_ValidRenderStates[D3DRS_SRCBLEND] = true;
    m_ValidRenderStates[D3DRS_DESTBLEND] = true;
    m_ValidRenderStates[D3DRS_CULLMODE] = true;
    m_ValidRenderStates[D3DRS_ZFUNC] = true;
    m_ValidRenderStates[D3DRS_ALPHAREF] = true;
    m_ValidRenderStates[D3DRS_ALPHAFUNC] = true;
    m_ValidRenderStates[D3DRS_DITHERENABLE] = true;
    m_ValidRenderStates[D3DRS_ALPHABLENDENABLE] = true;
    m_ValidRenderStates[D3DRS_FOGENABLE] = true;
    m_ValidRenderStates[D3DRS_SPECULARENABLE] = true;
    m_ValidRenderStates[D3DRS_FOGCOLOR] = true;
    m_ValidRenderStates[D3DRS_FOGTABLEMODE] = true;
    m_ValidRenderStates[D3DRS_FOGSTART] = true;
    m_ValidRenderStates[D3DRS_FOGEND] = true;
    m_ValidRenderStates[D3DRS_FOGDENSITY] = true;
    m_ValidRenderStates[D3DRS_RANGEFOGENABLE] = true;
    m_ValidRenderStates[D3DRS_STENCILENABLE] = true;
    m_ValidRenderStates[D3DRS_STENCILFAIL] = true;
    m_ValidRenderStates[D3DRS_STENCILZFAIL] = true;
    m_ValidRenderStates[D3DRS_STENCILPASS] = true;
    m_ValidRenderStates[D3DRS_STENCILFUNC] = true;
    m_ValidRenderStates[D3DRS_STENCILREF] = true;
    m_ValidRenderStates[D3DRS_STENCILMASK] = true;
    m_ValidRenderStates[D3DRS_STENCILWRITEMASK] = true;
    m_ValidRenderStates[D3DRS_TEXTUREFACTOR] = true;
    m_ValidRenderStates[D3DRS_WRAP0] = true;
    m_ValidRenderStates[D3DRS_WRAP1] = true;
    m_ValidRenderStates[D3DRS_WRAP2] = true;
    m_ValidRenderStates[D3DRS_WRAP3] = true;
    m_ValidRenderStates[D3DRS_WRAP4] = true;
    m_ValidRenderStates[D3DRS_WRAP5] = true;
    m_ValidRenderStates[D3DRS_WRAP6] = true;
    m_ValidRenderStates[D3DRS_WRAP7] = true;
    m_ValidRenderStates[D3DRS_CLIPPING] = true;
    m_ValidRenderStates[D3DRS_LIGHTING] = true;
    m_ValidRenderStates[D3DRS_AMBIENT] = true;
    m_ValidRenderStates[D3DRS_FOGVERTEXMODE] = true;
    m_ValidRenderStates[D3DRS_COLORVERTEX] = true;
    m_ValidRenderStates[D3DRS_LOCALVIEWER] = true;
    m_ValidRenderStates[D3DRS_NORMALIZENORMALS] = true;
    m_ValidRenderStates[D3DRS_DIFFUSEMATERIALSOURCE] = true;
    m_ValidRenderStates[D3DRS_SPECULARMATERIALSOURCE] = true;
    m_ValidRenderStates[D3DRS_AMBIENTMATERIALSOURCE] = true;
    m_ValidRenderStates[D3DRS_EMISSIVEMATERIALSOURCE] = true;
    m_ValidRenderStates[D3DRS_VERTEXBLEND] = true;
    m_ValidRenderStates[D3DRS_CLIPPLANEENABLE] = true;
    m_ValidRenderStates[D3DRS_POINTSIZE] = true;
    m_ValidRenderStates[D3DRS_POINTSIZE_MIN] = true;
    m_ValidRenderStates[D3DRS_POINTSPRITEENABLE] = true;
    m_ValidRenderStates[D3DRS_POINTSCALEENABLE] = true;
    m_ValidRenderStates[D3DRS_POINTSCALE_A] = true;
    m_ValidRenderStates[D3DRS_POINTSCALE_B] = true;
    m_ValidRenderStates[D3DRS_POINTSCALE_C] = true;
    m_ValidRenderStates[D3DRS_MULTISAMPLEANTIALIAS] = true;
    m_ValidRenderStates[D3DRS_MULTISAMPLEMASK] = true;
    m_ValidRenderStates[D3DRS_PATCHEDGESTYLE] = true;
    m_ValidRenderStates[D3DRS_DEBUGMONITORTOKEN] = true;
    m_ValidRenderStates[D3DRS_POINTSIZE_MAX] = true;
    m_ValidRenderStates[D3DRS_INDEXEDVERTEXBLENDENABLE] = true;
    m_ValidRenderStates[D3DRS_COLORWRITEENABLE] = true;
    m_ValidRenderStates[D3DRS_TWEENFACTOR] = true;
    m_ValidRenderStates[D3DRS_BLENDOP] = true;
    m_ValidRenderStates[D3DRS_POSITIONDEGREE] = true;
    m_ValidRenderStates[D3DRS_NORMALDEGREE] = true;
    m_ValidRenderStates[D3DRS_SCISSORTESTENABLE] = true;
    m_ValidRenderStates[D3DRS_SLOPESCALEDEPTHBIAS] = true;
    m_ValidRenderStates[D3DRS_ANTIALIASEDLINEENABLE] = true;
    m_ValidRenderStates[D3DRS_MINTESSELLATIONLEVEL] = true;
    m_ValidRenderStates[D3DRS_MAXTESSELLATIONLEVEL] = true;
    m_ValidRenderStates[D3DRS_ADAPTIVETESS_X] = true;
    m_ValidRenderStates[D3DRS_ADAPTIVETESS_Y] = true;
    m_ValidRenderStates[D3DRS_ADAPTIVETESS_Z] = true;
    m_ValidRenderStates[D3DRS_ADAPTIVETESS_W] = true;
    m_ValidRenderStates[D3DRS_ENABLEADAPTIVETESSELLATION] = true;
    m_ValidRenderStates[D3DRS_TWOSIDEDSTENCILMODE] = true;
    m_ValidRenderStates[D3DRS_CCW_STENCILFAIL] = true;
    m_ValidRenderStates[D3DRS_CCW_STENCILZFAIL] = true;
    m_ValidRenderStates[D3DRS_CCW_STENCILPASS] = true;
    m_ValidRenderStates[D3DRS_CCW_STENCILFUNC] = true;
    m_ValidRenderStates[D3DRS_COLORWRITEENABLE1] = true;
    m_ValidRenderStates[D3DRS_COLORWRITEENABLE2] = true;
    m_ValidRenderStates[D3DRS_COLORWRITEENABLE3] = true;
    m_ValidRenderStates[D3DRS_BLENDFACTOR] = true;
    m_ValidRenderStates[D3DRS_SRGBWRITEENABLE] = true;
    m_ValidRenderStates[D3DRS_DEPTHBIAS] = true;
    m_ValidRenderStates[D3DRS_WRAP8] = true;
    m_ValidRenderStates[D3DRS_WRAP9] = true;
    m_ValidRenderStates[D3DRS_WRAP10] = true;
    m_ValidRenderStates[D3DRS_WRAP11] = true;
    m_ValidRenderStates[D3DRS_WRAP12] = true;
    m_ValidRenderStates[D3DRS_WRAP13] = true;
    m_ValidRenderStates[D3DRS_WRAP14] = true;
    m_ValidRenderStates[D3DRS_WRAP15] = true;
    m_ValidRenderStates[D3DRS_SEPARATEALPHABLENDENABLE] = true;
    m_ValidRenderStates[D3DRS_SRCBLENDALPHA] = true;
    m_ValidRenderStates[D3DRS_DESTBLENDALPHA] = true;
    m_ValidRenderStates[D3DRS_BLENDOPALPHA] = true;

    m_ValidTextureStates[D3DTSS_COLOROP] = true;
    m_ValidTextureStates[D3DTSS_COLORARG1] = true;
    m_ValidTextureStates[D3DTSS_COLORARG2] = true;
    m_ValidTextureStates[D3DTSS_ALPHAOP] = true;
    m_ValidTextureStates[D3DTSS_ALPHAARG1] = true;
    m_ValidTextureStates[D3DTSS_ALPHAARG2] = true;
    m_ValidTextureStates[D3DTSS_BUMPENVMAT00] = true;
    m_ValidTextureStates[D3DTSS_BUMPENVMAT01] = true;
    m_ValidTextureStates[D3DTSS_BUMPENVMAT10] = true;
    m_ValidTextureStates[D3DTSS_BUMPENVMAT11] = true;
    m_ValidTextureStates[D3DTSS_TEXCOORDINDEX] = true;
    m_ValidTextureStates[D3DTSS_BUMPENVLSCALE] = true;
    m_ValidTextureStates[D3DTSS_BUMPENVLOFFSET] = true;
    m_ValidTextureStates[D3DTSS_TEXTURETRANSFORMFLAGS] = true;
    m_ValidTextureStates[D3DTSS_COLORARG0] = true;
    m_ValidTextureStates[D3DTSS_ALPHAARG0] = true;
    m_ValidTextureStates[D3DTSS_RESULTARG] = true;
    m_ValidTextureStates[D3DTSS_CONSTANT] = true;

    m_ValidSamplerStates[D3DSAMP_ADDRESSU] = true;
    m_ValidSamplerStates[D3DSAMP_ADDRESSV] = true;
    m_ValidSamplerStates[D3DSAMP_ADDRESSW] = true;
    m_ValidSamplerStates[D3DSAMP_BORDERCOLOR] = true;
    m_ValidSamplerStates[D3DSAMP_MAGFILTER] = true;
    m_ValidSamplerStates[D3DSAMP_MINFILTER] = true;
    m_ValidSamplerStates[D3DSAMP_MIPFILTER] = true;
    m_ValidSamplerStates[D3DSAMP_MIPMAPLODBIAS] = true;
    m_ValidSamplerStates[D3DSAMP_MAXMIPLEVEL] = true;
    m_ValidSamplerStates[D3DSAMP_MAXANISOTROPY] = true;
    m_ValidSamplerStates[D3DSAMP_SRGBTEXTURE] = true;
    m_ValidSamplerStates[D3DSAMP_ELEMENTINDEX] = true;
    m_ValidSamplerStates[D3DSAMP_DMAPOFFSET] = true;

    for (unsigned long i = 1; i < 256; i++)
    {
        if (!m_ValidRenderStates[i])
        {
            continue;
        }
        if (pD3DDev->GetRenderState((D3DRENDERSTATETYPE)i, &m_RenderStates[i]) != D3D_OK)
        {
            m_RenderStates[i] = 0;
            m_ValidRenderStates[i] = false;
        }
    }

    for (unsigned long i = 0; i < 8; i++)
    {
        pD3DDev->GetTexture(i, &m_Textures[i]);
        if (m_Textures[i])
        {
            m_Textures[i]->Release();
        }
    }

    for (unsigned long uiStage = 0; uiStage < 8; uiStage++)
    {
        for (unsigned long uiType = 0; uiType < 128; uiType++)
        {
            if (m_ValidTextureStates[uiType])
            {
                pD3DDev->GetTextureStageState(uiStage, (D3DTEXTURESTAGESTATETYPE)uiType,
                                              &m_TextureStates[uiStage][uiType]);
            }
        }
    }

    for (unsigned long uiSampler = 0; uiSampler < 16; uiSampler++)
    {
        for (unsigned long uiType = 0; uiType < 128; uiType++)
        {
            if (m_ValidSamplerStates[uiType])
            {
                pD3DDev->GetSamplerState(uiSampler, (D3DSAMPLERSTATETYPE)uiType,
                                         &m_SamplerStates[uiSampler][uiType]);
            }
        }
    }
}

// FUNCTION: SWBF2 0x006b3240
void StateManagerState::ResetState()
{
    for (int i = 0; i < 256; i++)
    {
        m_RenderStates[i] = 0;
    }
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 128; j++)
        {
            m_TextureStates[i][j] = 0;
        }
    }

    for (int i = 0; i < 16; i++)
    {
        m_StreamData[i] = NVStreamData();
    }
    m_IndexData.m_pIndexData = 0;
    m_IndexData.m_BaseVertexIndex = 0;

    for (int i = 0; i < 8; i++)
    {
        m_Textures[i] = 0;
    }

    for (int i = 0; i < 300; i++)
    {
        D3DXMatrixIdentity(&m_Matrices[i]);
    }

    for (int i = 0; i < 1024; i++)
    {
        m_VertexShaderConstants[i] = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    for (int i = 0; i < 1024; i++)
    {
        m_PixelShaderConstants[i] = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    m_FVF = 0;
    m_PixelShader = 0;
    m_VertexShader = 0;
    m_VertexShaderDecl = 0;

    memset(m_Matrices, 0, sizeof(m_Matrices));
}

// FUNCTION: SWBF2 0x006b4130
void StateManager_DX::ResetAndCaptureStates()
{
    m_CurrentState.ResetState();
    m_CopyState.ResetState();
    m_ChipState.ResetState();

    m_CurrentState.CaptureDeviceState(m_pD3DDev);
    m_CopyState.CaptureDeviceState(m_pD3DDev);
    m_ChipState.CaptureDeviceState(m_pD3DDev);
}

// FUNCTION: SWBF2 0x00715b00
StateManager_DX::StateManager_DX(IDirect3DDevice9* pD3DDev)
{
    caching_ = true;

    m_pD3DDev = pD3DDev;
    m_pD3DDev->AddRef();

    m_CurrentState.ResetState();
    m_CopyState.ResetState();
    m_ChipState.ResetState();

    m_bForce = false;
}

// FUNCTION: SWBF2 0x00715c90
StateManager_DX::~StateManager_DX()
{
    if (m_pD3DDev)
    {
        m_pD3DDev->Release();
    }
    m_pD3DDev = 0;
}

// FUNCTION: SWBF2 0x00715e40
void StateManager_DX::ClearInstance()
{
    delete localInstance_;
    localInstance_ = 0;
}

// FUNCTION: SWBF2 0x00715e60
void StateManager_DX::CreateInstance(IDirect3DDevice9* pD3DDev)
{
    if (localInstance_)
    {
        delete localInstance_;
        localInstance_ = 0;
    }

    localInstance_ = new StateManager_DX(pD3DDev);
}

// FUNCTION: SWBF2 0x00715ef0
StateManager_DX& StateManager_DX::GetInstance()
{
    return *localInstance_;
}

}
