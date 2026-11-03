#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrbitalStrike.h"
#include "Weapon.h"
#include "WeaponCannonInstance.h"
#include "PebbleFL/Common/PblHashTable.h"

class ExplosionClass;

// VTABLE: SWBF2 0x007b081c
class WeaponDestructClass : public WeaponClass, public OrbitalStrikeClass
{
public:
    WeaponDestructClass(unsigned int uHash);

    WeaponDestructClass(WeaponDestructClass* pBase, unsigned int uHash);
    virtual ~WeaponDestructClass();

    static PblHash rttiHashWeaponDestructClass;
    static const char rttiNameWeaponDestructClass[];

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mMaxPressedTime;
    ExplosionClass* mExplosionClass;
};
  
 
 
 
 

// VTABLE: SWBF2 0x007b0854
class WeaponDestruct : public Weapon
{
public:
    WeaponDestruct(const WeaponDestructClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponDestruct();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool IsBusy() const;

protected:
    virtual void EnterFire();
    virtual void ExitFire();
    virtual void EnterCharge();
    virtual bool UpdateCharge(float fDeltaTime);
    virtual bool CheckCharge() const;
    virtual bool CheckFire() const;
    virtual bool FireDone() const;

public:
    static PblHash rttiHashWeaponDestruct;
    static const char rttiNameWeaponDestruct[];

    PblHandle<OrbitalStrike> mStrike;
};
 
 

// FUNCTION: SWBF2 0x00681a10
inline bool WeaponDestruct::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponDestruct == uRtti)
    {
        return true;
    }
    return Weapon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00681a40
inline unsigned int WeaponDestruct::GetDerivedRtti() const
{
    return rttiHashWeaponDestruct;
}

// FUNCTION: SWBF2 0x00681a50
inline const char* WeaponDestruct::GetDerivedRttiName() const
{
    return rttiNameWeaponDestruct;
}

// FUNCTION: SWBF2 0x00681a60
inline bool WeaponDestructClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponDestructClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00681a90
inline unsigned int WeaponDestructClass::GetDerivedRtti() const
{
    return rttiHashWeaponDestructClass;
}

// FUNCTION: SWBF2 0x00681aa0
inline const char* WeaponDestructClass::GetDerivedRttiName() const
{
    return rttiNameWeaponDestructClass;
}
