#ifndef SWBF2_GRAPHICS_REDSEGMENT_H
#define SWBF2_GRAPHICS_REDSEGMENT_H

#include "RedEngineFL/Memory/engineAlloc.h"
#include "PebbleFL/Common/PblVector.h"

class PblMatrix;
class RedColor;
class RedShadingState;
class PblFileChunk;
class RedModel;

struct RedShaderData
{
    unsigned char uiData0;
    unsigned char uiData1;
    unsigned int attachedLightName;
};

// VTABLE: SWBF2 0x007858b4
class RedSegment
{
public:

    // SYNTHETIC: SWBF2 0x007179e0
    // RedSegment::~RedSegment
    virtual ~RedSegment()
    {
        if (m_puiBoneIndexMap)
        {
            delete[] m_puiBoneIndexMap;
        }
    }

    // SYNTHETIC: SWBF2 0x00717a00
    // RedSegment::`scalar deleting destructor'

    virtual void Render(const PblMatrix* pWorld, const PblMatrix* pBoneMatrices,
                        int iNumBones, unsigned int uiFlags, const RedColor& color,
                        RedShadingState* pState) = 0;

    virtual unsigned int GetRenderTypeHash() const = 0;

    // FUNCTION: SWBF2 0x00717b70
    virtual void Free()
    {
    }

    // FUNCTION: SWBF2 0x00717b40
    virtual RedShadingState* CreateShadingState() const
    {
        return 0;
    }

    // FUNCTION: SWBF2 0x00718950
    virtual void SetData(const RedShaderData&)
    {
    }

    // FUNCTION: SWBF2 0x00717cf0
    virtual bool IsTransparent() const
    {
        return false;
    }

    // FUNCTION: SWBF2 0x00717cc0
    virtual bool IsLit() const
    {
        return true;
    }

    // FUNCTION: SWBF2 0x00717d40
    virtual bool IsVertexLit() const
    {
        return false;
    }

    static RedSegment* __cdecl Read(PblFileChunk& chunk, const RedModel* pModel);

    unsigned int m_uiMaterialNameHash;
    unsigned int m_uiBoneIndex;

    unsigned int m_uiNumBones;

    unsigned char* m_puiBoneIndexMap;

    PblBox mBox;
};

 #endif
