#ifndef SWBF2_REDENGINEFL_GRAPHICS_REDTEXTURETRANSFORM_H
#define SWBF2_REDENGINEFL_GRAPHICS_REDTEXTURETRANSFORM_H

#include <string.h>

class RedTextureTransform
{
public:

    RedTextureTransform() : m_bTransform(0) {}

    const RedTextureTransform& operator=(const RedTextureTransform& rhs)
    {
        m_bTransform = rhs.m_bTransform;
        if (m_bTransform)
        {
            memcpy(m_Transform, rhs.m_Transform, sizeof(m_Transform));
        }
        return *this;
    }

    float& operator()(int iRow, int iCol)
    {
        if (!m_bTransform)
        {
            m_bTransform = 1;
            m_Transform[0][0] = 1.0f;
            m_Transform[0][1] = 0.0f;
            m_Transform[1][0] = 0.0f;
            m_Transform[1][1] = 1.0f;
            m_Transform[2][0] = 0.0f;
            m_Transform[2][1] = 0.0f;
        }
        return m_Transform[iRow][iCol];
    }

    // FUNCTION: SWBF2 0x006fe970
    float operator()(int iRow, int iCol) const
    {
        if (!m_bTransform)
        {
            return (iRow == iCol) ? 1.0f : 0.0f;
        }
        return m_Transform[iRow][iCol];
    }

    void SetOffset(float fU, float fV)
    {
        if (!m_bTransform)
        {
            m_bTransform = 1;
            m_Transform[0][0] = 1.0f;
            m_Transform[1][0] = 0.0f;
            m_Transform[0][1] = 0.0f;
            m_Transform[1][1] = 1.0f;
        }
        m_Transform[2][0] = fU;
        m_Transform[2][1] = fV;
    }

    void SetScale(float fScaleX, float fScaleY)
    {
        if (!m_bTransform)
        {
            m_bTransform = 1;
            m_Transform[2][0] = m_Transform[2][1] = 0.0f;
        }
        m_Transform[0][0] = fScaleX;
        m_Transform[0][1] = m_Transform[1][0] = 0.0f;
        m_Transform[1][1] = fScaleY;
    }

    void SetOriginScale(float fOriginU, float fOriginV, float fScaleU, float fScaleV)
    {
        m_bTransform = 1;
        m_Transform[0][0] = fScaleU;
        m_Transform[0][1] = 0.0f;
        m_Transform[1][0] = 0.0f;
        m_Transform[1][1] = fScaleV;
        m_Transform[2][0] = fScaleU * fOriginU;
        m_Transform[2][1] = fOriginV * fScaleV;
    }

    unsigned int m_bTransform;

    float m_Transform[3][2];
};
 

#endif
