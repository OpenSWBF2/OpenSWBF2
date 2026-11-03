#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrdnanceBullet.h"

// VTABLE: SWBF2 0x007acd0c
class OrdnanceLaserClass : public OrdnanceBulletClass
{
public:
    OrdnanceLaserClass(unsigned int uHash);
    virtual ~OrdnanceLaserClass();
    OrdnanceLaserClass(OrdnanceLaserClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mLaserWidth;
    float mLaserLength;
    unsigned int mTextureId;
    RedColor mGlowColor;
    RedColor mEndGlowColor;
    float mGlowLength;
    float mBlurLength;
    float mFadeOutSpeed;

    static PblHash rttiHashOrdnanceLaserClass;
    static const char rttiNameOrdnanceLaserClass[];
};
  
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007acc38 PblHandled
// VTABLE: SWBF2 0x007acc40
// VTABLE: SWBF2 0x007acccc Thread
class OrdnanceLaser : public OrdnanceBullet
{
public:
    OrdnanceLaser(OrdnanceLaserClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceLaser();

protected:
    virtual bool Update(float fDeltaTime);

public:
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void KillOrdnance();
    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);

    PblVector3 mStartPos;
    float mGrowLength;
    float mFadeOutTime;
    bool mDeadFlag;
    float mMaxLength;

    static PblHash rttiHashOrdnanceLaser;
    static const char rttiNameOrdnanceLaser[];
};
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00601180
inline bool OrdnanceLaserClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceLaserClass == uRtti)
    {
        return true;
    }
    return OrdnanceBulletClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006011b0
inline unsigned int OrdnanceLaserClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceLaserClass;
}

// FUNCTION: SWBF2 0x006011c0
inline const char* OrdnanceLaserClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceLaserClass;
}

// FUNCTION: SWBF2 0x00601100
inline bool OrdnanceLaser::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceLaser == uRtti)
    {
        return true;
    }
    return OrdnanceBullet::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00601130
inline unsigned int OrdnanceLaser::GetDerivedRtti() const
{
    return rttiHashOrdnanceLaser;
}

// FUNCTION: SWBF2 0x00601140
inline const char* OrdnanceLaser::GetDerivedRttiName() const
{
    return rttiNameOrdnanceLaser;
}
