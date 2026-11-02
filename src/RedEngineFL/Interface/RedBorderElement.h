#pragma once

#include <stddef.h>

#include "RedInterfaceElement.h"

class pcInterfaceShader;
class RedTexture;

// VTABLE: SWBF2 0x0078417c
class RedBorderElement : public RedInterfaceElement
{
public:

    RedBorderElement();

    RedBorderElement(const char* pcName);

    virtual ~RedBorderElement();

    virtual void SetBlendMode(BlendModeT eMode);
    virtual BlendModeT GetBlendMode() const;

    virtual bool IsBorderElement() const;

    virtual void CreateHotSpot(float fRadius);
    virtual void Cleanup();

    virtual void RenderUsingContext(const PblMatrix& mContext, const RedColor& color);

    virtual void Init();

    virtual void SetAlignedBorderRect(float fWidth, float fHeight, int eHorizontal,
                                 int eVertical, float fBorderX, float fBorderY);

    virtual void SetBorderRect(float fLeft, float fTop, float fRight, float fBottom,
                                 float fBorderX, float fBorderY);

    virtual void SetTextureHash(unsigned int uiTextureHash);

    virtual void BeforeRender();

    const RedTexture* GetTexture() const;

    void SetRoundedVertexPositions(float fLeft, float fTop, float fRight, float fBottom, float fBorderX,
                         float fBorderY);

    void GetBorderRect(float& fLeft, float& fTop, float& fRight, float& fBottom,
                         float& fBorderX, float& fBorderY);

    void GetBorderTexCoords(float& fU0, float& fV0, float& fU1, float& fV1, float& fBorderU,
                    float& fBorderV);

    void SetBorderTexCoords(float fLeft, float fTop, float fRight, float fBottom,
                         float fBorderX, float fBorderY, int iUnused);

    struct Vertex
    {
        float x;
        float y;
        float z;
        float u;
        float v;
        float u2;
        float v2;
    };

    unsigned int mShadowMode;
    BlendModeT m_eBlendMode;

    float mBorderRect[6];
    Vertex m_vertices[16];

    pcInterfaceShader* m_pInterfaceShader;
};
 
