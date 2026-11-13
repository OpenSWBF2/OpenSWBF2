#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Weapon.h"
#include "WeaponCannon.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007b0ebc
class WeaponGrenadeClass : public WeaponClass
{
public:
    WeaponGrenadeClass(unsigned int uHash);

    WeaponGrenadeClass(WeaponGrenadeClass* pBase, unsigned int uHash);

    virtual ~WeaponGrenadeClass();

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual bool PostLoadInit();

    float mShotDelay;
    float mInitialSalvoDelay[3];
    float mShotElevate;
    float mMaxPressedTime;
    bool mHideOnFire;
    char mForcedFireAnim;
    PblVector3 m_avFireOffset[3];

    static PblHash rttiHashWeaponGrenadeClass;
    static const char rttiNameWeaponGrenadeClass[];
};
  
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007b0ef4
class WeaponGrenade : public Weapon
{
public:
    WeaponGrenade(const WeaponGrenadeClass* pClass, const WeaponDesc& desc);

    virtual ~WeaponGrenade();
    bool IsRtti(unsigned int uRtti) const;
    unsigned int GetDerivedRtti() const;
    const char* GetDerivedRttiName() const;
    void SignalFire();
    bool IsBusy() const;
    void Write(NetPktGroup& group);
    void Read(NetPktGroup& group);
    void EnterFire();
    bool CheckCharge() const;
    bool CheckFire() const;

protected:
    bool UpdateFire(float fDeltaTime);
    void ExitFire();
    void EnterCharge();
    bool UpdateCharge(float fDeltaTime);
    void ExitCharge();
    void ExitReload();
    void EnterEmpty();
    bool FireDone() const;

    bool Fire();

public:

    static PblHash rttiHashWeaponGrenade;
    static const char rttiNameWeaponGrenade[];

    bool mNetFiredFlag;
    bool mFired;
    char m_iFirePos;
    float mStrength;

    char mUnmodeledTail[0x120 - 0x118];
};
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00684f90
inline bool WeaponGrenadeClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponGrenadeClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00684fc0
inline unsigned int WeaponGrenadeClass::GetDerivedRtti() const
{
    return rttiHashWeaponGrenadeClass;
}

// FUNCTION: SWBF2 0x00684fd0
inline const char* WeaponGrenadeClass::GetDerivedRttiName() const
{
    return rttiNameWeaponGrenadeClass;
}

// FUNCTION: SWBF2 0x00684f40
inline bool WeaponGrenade::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponGrenade == uRtti)
    {
        return true;
    }
    return reinterpret_cast<const Weapon*>(this)->Weapon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00684f70
inline unsigned int WeaponGrenade::GetDerivedRtti() const
{
    return rttiHashWeaponGrenade;
}

// FUNCTION: SWBF2 0x00684f80
inline const char* WeaponGrenade::GetDerivedRttiName() const
{
    return rttiNameWeaponGrenade;
}
