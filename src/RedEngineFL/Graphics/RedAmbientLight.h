#pragma once

#include <stddef.h>

// VTABLE: SWBF2 0x00772170
class RedAmbientLight : public RedLight
{
public:

    // FUNCTION: SWBF2 0x006afca0
    RedAmbientLight(const RedColorValue& color = RedColorValue::WHITE,
                    const RedColorValue& topColor = RedColorValue::WHITE,
                    const RedColorValue& bottomColor = RedColorValue::WHITE)
        : RedLight(TYPE_AMBIENT), m_topColor(topColor), m_bottomColor(bottomColor)
    {
        m_color = color;
    }

    // FUNCTION: SWBF2 0x006afed0
    virtual ~RedAmbientLight() {}

    // FUNCTION: SWBF2 0x006aff00
    RedAmbientLight& operator=(const RedAmbientLight& rOther)
    {
        RedLight::operator=(rOther);
        m_topColor = rOther.m_topColor;
        m_bottomColor = rOther.m_bottomColor;
        return *this;
    }

    // FUNCTION: SWBF2 0x006b2570
    virtual bool IsVisible(const RedCamera* pCamera) const
    {
        return true;
    }

    // FUNCTION: SWBF2 0x006b0c20
    virtual void DebugDraw() const
    {
    }

    // FUNCTION: SWBF2 0x006b4020
    virtual bool SphereIntersect(const PblVector3& vCenter, float fRadius) const
    {
        return true;
    }

    // FUNCTION: SWBF2 0x006b00c0
    virtual bool AABBIntersect(const PblVector3& vMin, const PblVector3& vMax) const
    {
        return true;
    }

    void SetColor(const RedColorValue& color) { m_color = color; }
    void SetTopColor(const RedColorValue& color) { m_topColor = color; }
    void SetBottomColor(const RedColorValue& color) { m_bottomColor = color; }

    const RedColorValue& GetTopColor() const { return m_topColor; }
    const RedColorValue& GetBottomColor() const { return m_bottomColor; }

    RedColorValue m_topColor;
    RedColorValue m_bottomColor;

    // GLOBAL: SWBF2 0x007e23a0
    static RedAmbientLight WHITE;
    // GLOBAL: SWBF2 0x007e2410
    static RedAmbientLight BLACK;
};
 
 
 


 
 
 
 
 
 
 
