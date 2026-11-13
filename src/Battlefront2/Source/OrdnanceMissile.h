#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrdnanceBullet.h"

// VTABLE: SWBF2 0x007acecc
class OrdnanceMissileClass : public OrdnanceBulletClass
{
public:
    OrdnanceMissileClass(unsigned int uHash);
    virtual ~OrdnanceMissileClass();
    OrdnanceMissileClass(OrdnanceMissileClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    // FUNCTION: SWBF2 0x006049a0
    virtual float GetVelocity() const
    {
        return 3.0e8f;
    }

    static PblHash rttiHashOrdnanceMissileClass;
    static const char rttiNameOrdnanceMissileClass[];

    float mScaleRate;
    float mTurnRate;
    float mWaverRate;
    float mWaverTurn;
    GameSound mLockedOnSound;
    float mMinSpeed;
    float mAcceleration;
};
  
 
 
 
 
 
 
 

class GameObject;

// VTABLE: SWBF2 0x007ace84 Thread
// VTABLE: SWBF2 0x007acec4 PblHandled
// VTABLE: SWBF2 0x007acefc
class OrdnanceMissile : public OrdnanceBullet
{
public:
    OrdnanceMissile(OrdnanceMissileClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceMissile();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

protected:
    virtual bool Update(float fDeltaTime);

public:
    PblVector3 mTargetDir;
    PblHandle<GameObject const> mTarget;
    int mBodyID;
    float mScale;
    float mWaverPitch;
    float mWaverYaw;
    GameSoundControllable mLockedOnSoundCtrl;
    PblVector3 mVelocityDir;
    float mGoalSpeed;
    float mSpeed;

    static PblHash rttiHashOrdnanceMissile;
    static const char rttiNameOrdnanceMissile[];
};
 
 
 
 
 

// FUNCTION: SWBF2 0x006048d0
inline bool OrdnanceMissile::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceMissile == uRtti)
    {
        return true;
    }
    return OrdnanceBullet::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00604900
inline unsigned int OrdnanceMissile::GetDerivedRtti() const
{
    return rttiHashOrdnanceMissile;
}

// FUNCTION: SWBF2 0x00604910
inline const char* OrdnanceMissile::GetDerivedRttiName() const
{
    return rttiNameOrdnanceMissile;
}

// FUNCTION: SWBF2 0x00604950
inline bool OrdnanceMissileClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceMissileClass == uRtti)
    {
        return true;
    }
    return OrdnanceBulletClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00604980
inline unsigned int OrdnanceMissileClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceMissileClass;
}

// FUNCTION: SWBF2 0x00604990
inline const char* OrdnanceMissileClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceMissileClass;
}
