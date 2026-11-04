#ifndef SWBF2_GRAPHICS_PC_PCREDPRIMITIVE_H
#define SWBF2_GRAPHICS_PC_PCREDPRIMITIVE_H

#include <stddef.h>
#include <d3d9.h>

#include "PebbleFL/Common/PblRef.h"
#include "PebbleFL/Common/PblVector.h"

#pragma pack(push, 1)
struct CompressedPos
{
    short x;
    short y;
    short z;
    short w;

    // FUNCTION: SWBF2 0x0040cbe0
    static void SetBoundingBox(const PblBox& box)
    {
        s_vMin = box.GetMin();
        s_vScale = (box.GetMax() - box.GetMin()) / 65535.0f;
        s_vScaleInv = PblVector3(1.0f / s_vScale.x, 1.0f / s_vScale.y, 1.0f / s_vScale.z);
    }

    // FUNCTION: SWBF2 0x0040cc80
    void SetPos(float fX, float fY, float fZ)
    {
        x = (short)((fX - s_vMin.x) * s_vScaleInv.x - 32767.5f);
        y = (short)((fY - s_vMin.y) * s_vScaleInv.y - 32767.5f);
        z = (short)((fZ - s_vMin.z) * s_vScaleInv.z - 32767.5f);
    }

    void SetPos(const PblVector3& v) { SetPos(v.x, v.y, v.z); }

    static PblVector3 s_vMin;
    static PblVector3 s_vScale;
    static PblVector3 s_vScaleInv;
};
#pragma pack(pop)

class pcRedIndexBuffer;
class pcRedVertexBuffer;
class pcRedVertexFormat;

#pragma pack(push, 1)
struct PosXYZ
{
    void SetPos(float fX, float fY, float fZ) { x = fX; y = fY; z = fZ; }
    void SetPos(const PblVector3& v) { x = v.x; y = v.y; z = v.z; }
    void GetPos(PblVector3& v) const;

    float x, y, z;
};

struct TexCoord
{
    void SetUV(float fU, float fV) { u = fU; v = fV; }
    void GetUV(float& fU, float& fV) const;

    float u, v;
};

struct CompressedTexCoord
{
    void SetUV(float fU, float fV) { u = (short)(fU * 2048.0f); v = (short)(fV * 2048.0f); }
    void GetUV(float& fU, float& fV) const;
    void SetUVHex(short sU, short sV) { u = sU; v = sV; }

    short u, v;
};
#pragma pack(pop)

// VTABLE: SWBF2 0x00771c94
class pcRedPrimitive : public PblRef
{
public:

    virtual ~pcRedPrimitive();

    // SYNTHETIC: SWBF2 0x006ad730
    // pcRedPrimitive::`scalar deleting destructor'

    pcRedPrimitive(D3DPRIMITIVETYPE primType,
                   unsigned int uiVertexFormat,
                   unsigned int uiVertexSize,
                   unsigned int uiNumVertices,
                   unsigned int uiNumIndices,
                   bool bStatic);

    pcRedPrimitive(D3DPRIMITIVETYPE primType,
                   unsigned int uiPrimCount,
                   unsigned int uiVertexFormat,
                   unsigned int uiNumVertices,
                   unsigned int uiStartVertex,
                   pcRedVertexBuffer* pVertexBuffer,
                   unsigned int uiStartIndex,
                   pcRedIndexBuffer* pIndexBuffer);

    static pcRedPrimitive* Create(D3DPRIMITIVETYPE primType,
                                  unsigned int uiVertexFormat,
                                  unsigned int uiVertexSize,
                                  unsigned int uiNumVertices,
                                  unsigned int uiNumIndices,
                                  bool bStatic);

    static pcRedPrimitive* Create(D3DPRIMITIVETYPE primType,
                                  unsigned int uiPrimCount,
                                  unsigned int uiVertexFormat,
                                  unsigned int uiNumVertices,
                                  unsigned int uiStartVertex,
                                  pcRedVertexBuffer* pVertexBuffer,
                                  unsigned int uiStartIndex,
                                  pcRedIndexBuffer* pIndexBuffer);

    void* BeginVertexData();

    void EndVertexData();

    unsigned short* BeginIndexData();
    void EndIndexData();

    void Draw() const;

    static unsigned int GetNumD3DPrimitives(D3DPRIMITIVETYPE primType,
                                            unsigned int uiNumVertices);

    pcRedIndexBuffer* m_indexBuffer;
    pcRedVertexBuffer* m_vertexBuffer;
    D3DPRIMITIVETYPE m_primType;
    unsigned int m_startVertex;
    unsigned int m_startIndex;
    unsigned int m_primCount;
    unsigned int m_numVertices;
    unsigned int m_numIndices;
    pcRedVertexFormat* m_vertexFormat;
    PblBox m_bbox;
};

 
 
 

 #endif
