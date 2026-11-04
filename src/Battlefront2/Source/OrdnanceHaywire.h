#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrdnanceSticky.h"

// VTABLE: SWBF2 0x007acba0
class OrdnanceHaywireClass : public OrdnanceStickyClass
{
public:
    virtual ~OrdnanceHaywireClass();
    OrdnanceHaywireClass(unsigned int uHash);
    OrdnanceHaywireClass(OrdnanceHaywireClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mDisableTime;

    static PblHash rttiHashOrdnanceHaywireClass;
    static const char rttiNameOrdnanceHaywireClass[];
};
  
 

// VTABLE: SWBF2 0x007acb5c Thread
// VTABLE: SWBF2 0x007acb54 PblHandled
// VTABLE: SWBF2 0x007acbd0
class OrdnanceHaywire : public OrdnanceSticky
{
public:
    OrdnanceHaywire(OrdnanceHaywireClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceHaywire();
    virtual bool Update(float fDeltaTime);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static PblHash rttiHashOrdnanceHaywire;
    static const char rttiNameOrdnanceHaywire[];
};
 

// FUNCTION: SWBF2 0x00600d50
inline bool OrdnanceHaywire::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceHaywire == uRtti)
    {
        return true;
    }
    return OrdnanceSticky::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00600d90
inline unsigned int OrdnanceHaywire::GetDerivedRtti() const
{
    return rttiHashOrdnanceHaywire;
}

// FUNCTION: SWBF2 0x00600da0
inline const char* OrdnanceHaywire::GetDerivedRttiName() const
{
    return rttiNameOrdnanceHaywire;
}

// FUNCTION: SWBF2 0x00600de0
inline bool OrdnanceHaywireClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceHaywireClass == uRtti)
    {
        return true;
    }
    return OrdnanceStickyClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00600e20
inline unsigned int OrdnanceHaywireClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceHaywireClass;
}

// FUNCTION: SWBF2 0x00600e30
inline const char* OrdnanceHaywireClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceHaywireClass;
}
