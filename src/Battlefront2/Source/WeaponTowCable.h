#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "GameSound.h"
#include "WeaponCannon.h"
#include "WeaponCannon.h"
#include "PebbleFL/Common/PblHashTable.h"

class WeaponTowCableClass;

// VTABLE: SWBF2 0x007b1c58
class WeaponTowCable : public WeaponCannon
{
public:
    WeaponTowCable(const WeaponTowCableClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponTowCable();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);

protected:
    virtual bool CheckFire() const;

public:
    virtual void SetOrdnance(Ordnance* pOrdnance);
    virtual Ordnance* CreateOrdnance(OrdnanceDesc& desc);

    bool HasOrdnance() const;
    void KillMyCable();

    static PblHash rttiHashWeaponTowCable;
    static const char rttiNameWeaponTowCable[];

    Ordnance* mMyCable;
    bool mCableOut;
};
  
 
 

// VTABLE: SWBF2 0x007b1c20
class WeaponTowCableClass : public WeaponCannonClass
{
public:
    WeaponTowCableClass(unsigned int uHash);

    WeaponTowCableClass(WeaponTowCableClass* pBase, unsigned int uHash);
    virtual ~WeaponTowCableClass();

    static PblHash rttiHashWeaponTowCableClass;
    static const char rttiNameWeaponTowCableClass[];

    // FUNCTION: SWBF2 0x005b4240
    static unsigned int GetRtti() { return rttiHashWeaponTowCableClass; }

    virtual WeaponClass* Derive(unsigned int uHash);

    virtual WeaponTowCable* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    GameSound mWhir;
    GameSound mDetatch;
};
  
 
 

// FUNCTION: SWBF2 0x006927b0
inline bool WeaponTowCableClass::IsRtti(unsigned int uRtti) const
{
    if (GetRtti() == uRtti)
    {
        return true;
    }
    return WeaponCannonClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006927e0
inline unsigned int WeaponTowCableClass::GetDerivedRtti() const
{
    return rttiHashWeaponTowCableClass;
}

// FUNCTION: SWBF2 0x006927f0
inline const char* WeaponTowCableClass::GetDerivedRttiName() const
{
    return rttiNameWeaponTowCableClass;
}

// FUNCTION: SWBF2 0x00692730
inline bool WeaponTowCable::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponTowCable == uRtti)
    {
        return true;
    }
    return WeaponCannon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00692760
inline unsigned int WeaponTowCable::GetDerivedRtti() const
{
    return rttiHashWeaponTowCable;
}

// FUNCTION: SWBF2 0x00692770
inline const char* WeaponTowCable::GetDerivedRttiName() const
{
    return rttiNameWeaponTowCable;
}
