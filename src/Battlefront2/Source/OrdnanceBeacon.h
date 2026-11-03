#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrdnanceSticky.h"

class OrdnanceClass;

// VTABLE: SWBF2 0x007ac588
class OrdnanceBeaconClass : public OrdnanceStickyClass
{
public:
    OrdnanceBeaconClass(unsigned int uHash);
    OrdnanceBeaconClass(OrdnanceBeaconClass* pBase, unsigned int uHash);
    virtual ~OrdnanceBeaconClass();

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static PblHash rttiHashOrdnanceBeaconClass;
    static const char rttiNameOrdnanceBeaconClass[];

    float mAimAzimuth;
    float mAimElevation;
    float mAimDistance;
    int mSalvoCount;
    float mSalvoDelay;
    float mScatterDistance;
    OrdnanceClass* mOrdnanceClass;
};
  
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007ac53c Thread
// VTABLE: SWBF2 0x007ac4d4
// VTABLE: SWBF2 0x007ac580 PblHandled
class OrdnanceBeacon : public OrdnanceSticky
{
public:
    OrdnanceBeacon(OrdnanceBeaconClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceBeacon();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool Update(float fDeltaTime);

    int mSalvoCount;

    static PblHash rttiHashOrdnanceBeacon;
    static const char rttiNameOrdnanceBeacon[];
};
 
 

// FUNCTION: SWBF2 0x005f8c70
inline bool OrdnanceBeacon::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceBeacon == uRtti)
    {
        return true;
    }
    return OrdnanceSticky::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005f8cb0
inline unsigned int OrdnanceBeacon::GetDerivedRtti() const
{
    return rttiHashOrdnanceBeacon;
}

// FUNCTION: SWBF2 0x005f8cc0
inline const char* OrdnanceBeacon::GetDerivedRttiName() const
{
    return rttiNameOrdnanceBeacon;
}

// FUNCTION: SWBF2 0x005f8d00
inline bool OrdnanceBeaconClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceBeaconClass == uRtti)
    {
        return true;
    }
    return OrdnanceStickyClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005f8d40
inline unsigned int OrdnanceBeaconClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceBeaconClass;
}

// FUNCTION: SWBF2 0x005f8d50
inline const char* OrdnanceBeaconClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceBeaconClass;
}
