#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrdnanceBullet.h"

// VTABLE: SWBF2 0x007ac8d4
class OrdnanceFatRayClass : public OrdnanceBulletClass
{
public:
    OrdnanceFatRayClass(unsigned int uHash);
    virtual ~OrdnanceFatRayClass();
    OrdnanceFatRayClass(OrdnanceFatRayClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mMaxDamage;
    float mMinDamage;
    float mDamageDeduction;
    float mMaxPush;
    float mMinPush;
    float mPushDeduction;
    float mRayRadius;

    static PblHash rttiHashOrdnanceFatRayClass;
    static const char rttiNameOrdnanceFatRayClass[];
};
  
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007ac82c
// VTABLE: SWBF2 0x007ac894 Thread
// VTABLE: SWBF2 0x007ac904 PblHandled
class OrdnanceFatRay : public OrdnanceBullet
{
public:
    OrdnanceFatRay(OrdnanceFatRayClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceFatRay();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    virtual bool CheckCollision();

    float mDamage;
    float mPush;

    static PblHash rttiHashOrdnanceFatRay;
    static const char rttiNameOrdnanceFatRay[];
};
 
 
 

// FUNCTION: SWBF2 0x005fe730
inline bool OrdnanceFatRay::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceFatRay == uRtti)
    {
        return true;
    }
    return OrdnanceBullet::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005fe760
inline unsigned int OrdnanceFatRay::GetDerivedRtti() const
{
    return rttiHashOrdnanceFatRay;
}

// FUNCTION: SWBF2 0x005fe770
inline const char* OrdnanceFatRay::GetDerivedRttiName() const
{
    return rttiNameOrdnanceFatRay;
}

// FUNCTION: SWBF2 0x005fe7b0
inline bool OrdnanceFatRayClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceFatRayClass == uRtti)
    {
        return true;
    }
    return OrdnanceBulletClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005fe7e0
inline unsigned int OrdnanceFatRayClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceFatRayClass;
}

// FUNCTION: SWBF2 0x005fe7f0
inline const char* OrdnanceFatRayClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceFatRayClass;
}
