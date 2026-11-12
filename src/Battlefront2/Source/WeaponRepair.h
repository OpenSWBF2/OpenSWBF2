#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Weapon.h"
#include "WeaponCannon.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007b1950
class WeaponRepairClass : public WeaponClass
{
public:
    WeaponRepairClass(unsigned int uHash);
    WeaponRepairClass(WeaponRepairClass* pBase, unsigned int uHash);
    virtual ~WeaponRepairClass();

    static PblHash rttiHashWeaponRepairClass;
    static const char rttiNameWeaponRepairClass[];

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mShotDelay;
    float mAddHealth[8];
    float mAddAmmo[8];
    float mAddBuild;
    float mAddRebuild;
};
  
 
 
 
 
 

// VTABLE: SWBF2 0x007b1988
class WeaponRepair : public Weapon
{
public:
    WeaponRepair(const WeaponRepairClass* pClass, const WeaponDesc& desc);

    virtual ~WeaponRepair();
    bool Update(float fDeltaTime);
    bool IsRtti(unsigned int uRtti) const;
    unsigned int GetDerivedRtti() const;
    const char* GetDerivedRttiName() const;
    bool IsLocked() const;
    void Select(unsigned int uFlags, bool bOffhand);
    void Deselect(unsigned int uFlags, bool bOffhand);
    bool IsBusy() const;

protected:
    void EnterFire();
    bool UpdateFire(float fDeltaTime);
    void ExitFire();
    bool CheckFire() const;
    bool FireDone() const;
    bool Fire(float fDeltaTime);

public:

    static PblHash rttiHashWeaponRepair;
    static const char rttiNameWeaponRepair[];

    bool mFired;

    char mUnmodeledTail[0x120 - 0x111];
};
 
 
 
 
 

// FUNCTION: SWBF2 0x006905f0
inline bool WeaponRepairClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponRepairClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00690620
inline unsigned int WeaponRepairClass::GetDerivedRtti() const
{
    return rttiHashWeaponRepairClass;
}

// FUNCTION: SWBF2 0x00690630
inline const char* WeaponRepairClass::GetDerivedRttiName() const
{
    return rttiNameWeaponRepairClass;
}

// FUNCTION: SWBF2 0x006905a0
inline bool WeaponRepair::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponRepair == uRtti)
    {
        return true;
    }
    return reinterpret_cast<const Weapon*>(this)->Weapon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006905d0
inline unsigned int WeaponRepair::GetDerivedRtti() const
{
    return rttiHashWeaponRepair;
}

// FUNCTION: SWBF2 0x006905e0
inline const char* WeaponRepair::GetDerivedRttiName() const
{
    return rttiNameWeaponRepair;
}
