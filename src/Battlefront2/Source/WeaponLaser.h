#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Weapon.h"
#include "PebbleFL/Common/PblRtti.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007b1158
class WeaponLaserClass : public WeaponClass
{
public:
    WeaponLaserClass(unsigned int uHash);
    WeaponLaserClass(WeaponLaserClass* pBase, unsigned int uHash);
    virtual ~WeaponLaserClass();

    static PblHash rttiHashWeaponLaserClass;
    static const char rttiNameWeaponLaserClass[];

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mShotDelay;
    float mSalvoTime;
};
  
 
 

#include "WeaponCannon.h"

// VTABLE: SWBF2 0x007b1190
class WeaponLaser : public Weapon
{
public:
    WeaponLaser(const WeaponLaserClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponLaser();

    // FUNCTION: SWBF2 0x00686a10
    // WeaponLaser::IsRtti
    // FUNCTION: SWBF2 0x00686a40
    // WeaponLaser::GetDerivedRtti
    // FUNCTION: SWBF2 0x00686a50
    // WeaponLaser::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(WeaponLaser, Weapon)
    virtual bool IsBusy() const;
    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);

protected:
    virtual void EnterFire();
    virtual bool UpdateFire(float fDeltaTime);
    virtual void ExitFire();
    virtual void EnterEmpty();
    virtual bool CheckFire() const;
    virtual bool FireDone() const;

    bool Fire(float fTime);

public:

    float mSalvoTimer;
};
  
 

// FUNCTION: SWBF2 0x00686a90
inline bool WeaponLaserClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponLaserClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00686ac0
inline unsigned int WeaponLaserClass::GetDerivedRtti() const
{
    return rttiHashWeaponLaserClass;
}

// FUNCTION: SWBF2 0x00686ad0
inline const char* WeaponLaserClass::GetDerivedRttiName() const
{
    return rttiNameWeaponLaserClass;
}
