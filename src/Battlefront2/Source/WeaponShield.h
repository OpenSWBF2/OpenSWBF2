#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CollisionModel.h"
#include "CollisionObject.h"
#include "GameSound.h"
#include "Weapon.h"
#include "WeaponCannon.h"
#include "PebbleFL/Common/PblHashTable.h"

class CollisionBodyPrimitive;
class FLEffectClass;

// VTABLE: SWBF2 0x007b1be8
class WeaponShieldClass : public WeaponClass
{
public:
    WeaponShieldClass(unsigned int uHash);
    WeaponShieldClass(WeaponShieldClass* pBase, unsigned int uHash);
    virtual ~WeaponShieldClass();

    static PblHash rttiHashWeaponShieldClass;
    static const char rttiNameWeaponShieldClass[];

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void PostReadInit();

    float mMaxShield;
    float mAddShield;
    float mAddShieldOff;
    PblVector3 m_vShieldOffset;
    PblVector3 m_vShieldSize;
    CollisionBodyPrimitive* mCollisionBody;
    CollisionModel mCollisionModel;
    FLEffectClass* mShieldEffect;
    GameSound mSoundShield;
    GameSound mSoundShieldOff;
};
  
 
 
 
 
 
 
 
 
 
 

class FLEffectObject;
class CollisionResult;
class Restrictor;

// VTABLE: SWBF2 0x007b1a9c Weapon
// VTABLE: SWBF2 0x007b1b94 CollisionObject
class WeaponShield : public Weapon, public CollisionObject
{
public:
    WeaponShield(const WeaponShieldClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponShield();
    virtual bool Update(float fDeltaTime);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool IsBusy() const;

protected:
    virtual void EnterFire();
    virtual bool UpdateFire(float fDeltaTime);
    virtual void ExitFire();
    virtual bool CheckFire() const;
    virtual bool FireDone() const;

public:

    virtual const CollisionObject* GetCollisionObject() const;
    virtual CollisionObject* GetCollisionObject();

    virtual const PblVector3& GetPosition() const;
    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);
    virtual const PblMatrix& GetMatrix() const;

    static PblHash rttiHashWeaponShield;

    static unsigned int GetRtti() { return rttiHashWeaponShield; }
    static const char rttiNameWeaponShield[];

    FLEffectObject* mShieldEffect;
};
 
 

// FUNCTION: SWBF2 0x00691770
inline bool WeaponShieldClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponShieldClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006917a0
inline unsigned int WeaponShieldClass::GetDerivedRtti() const
{
    return rttiHashWeaponShieldClass;
}

// FUNCTION: SWBF2 0x006917b0
inline const char* WeaponShieldClass::GetDerivedRttiName() const
{
    return rttiNameWeaponShieldClass;
}

// FUNCTION: SWBF2 0x00691700
inline bool WeaponShield::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponShield == uRtti)
    {
        return true;
    }
    return Weapon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00691730
inline unsigned int WeaponShield::GetDerivedRtti() const
{
    return rttiHashWeaponShield;
}

// FUNCTION: SWBF2 0x00691740
inline const char* WeaponShield::GetDerivedRttiName() const
{
    return "WeaponShield";
}
