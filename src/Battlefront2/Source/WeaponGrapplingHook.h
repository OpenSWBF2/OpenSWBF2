#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "WeaponCannon.h"
#include "WeaponCannonInstance.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007b0d68
class WeaponGrapplingHookClass : public WeaponCannonClass
{
public:
    WeaponGrapplingHookClass(unsigned int uHash);
    WeaponGrapplingHookClass(WeaponGrapplingHookClass* pBase, unsigned int uHash);
    virtual ~WeaponGrapplingHookClass();

    static PblHash rttiHashWeaponGrapplingHookClass;
    static const char rttiNameWeaponGrapplingHookClass[];

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
};
 class Ordnance;
class NetPktGroup;

// VTABLE: SWBF2 0x007b0da0
class WeaponGrapplingHook : public WeaponCannon
{
public:
    WeaponGrapplingHook(const WeaponGrapplingHookClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponGrapplingHook();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);

protected:
    virtual bool CheckFire() const;

public:
    virtual void SetOrdnance(Ordnance* pOrdnance);

    static PblHash rttiHashWeaponGrapplingHook;
    static const char rttiNameWeaponGrapplingHook[];

    bool mCableOut;
};
  

 

// FUNCTION: SWBF2 0x00684d10
inline bool WeaponGrapplingHookClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponGrapplingHookClass == uRtti)
    {
        return true;
    }
    return WeaponCannonClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00684d40
inline unsigned int WeaponGrapplingHookClass::GetDerivedRtti() const
{
    return rttiHashWeaponGrapplingHookClass;
}

// FUNCTION: SWBF2 0x00684d50
inline const char* WeaponGrapplingHookClass::GetDerivedRttiName() const
{
    return rttiNameWeaponGrapplingHookClass;
}

// FUNCTION: SWBF2 0x00684c90
inline bool WeaponGrapplingHook::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponGrapplingHook == uRtti)
    {
        return true;
    }
    return WeaponCannon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00684cc0
inline unsigned int WeaponGrapplingHook::GetDerivedRtti() const
{
    return rttiHashWeaponGrapplingHook;
}

// FUNCTION: SWBF2 0x00684cd0
inline const char* WeaponGrapplingHook::GetDerivedRttiName() const
{
    return rttiNameWeaponGrapplingHook;
}
