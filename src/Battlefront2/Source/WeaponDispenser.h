#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Weapon.h"
#include "WeaponCannonInstance.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblHashTable.h"

class Entity;
class EntityClass;
class GameObject;
class NetPktGroup;

// VTABLE: SWBF2 0x007b0be4
class WeaponDispenserClass : public WeaponClass
{
public:
    virtual ~WeaponDispenserClass();
    WeaponDispenserClass(unsigned int uHash);
    WeaponDispenserClass(WeaponDispenserClass* pBase, unsigned int uHash);

    static PblHash rttiHashWeaponDispenserClass;
    static const char rttiNameWeaponDispenserClass[];

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void PostReadInit();
    virtual bool PostLoadInit();

    EntityClass* mDispenseClass;
    float mVelocity;
    float mShotDelay;
    float mInitialSalvoDelay[2];
    float mMaxPressedTime;
    float mMinStrength;
    float mMaxStrength;
    int mMaxItems;
    bool mHideOnFire;
    bool mHideOnEmpty;
    PblVector3 mFirePointExtraOffset;
    PblHandle<GameObject>* mTrackItemPool;
    PblHandle<GameObject>* mTrackItemArray[600];
    int mTrackItemCount[600];
    PblVector3 m_avFireOffset[3];
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007b0c1c
class WeaponDispenser : public Weapon
{
public:
    WeaponDispenser(const WeaponDispenserClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponDispenser();

    virtual bool Update(float fDeltaTime);

    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);

    bool IsRtti(unsigned int uRtti) const;
    unsigned int GetDerivedRtti() const;
    const char* GetDerivedRttiName() const;
    bool IsBusy() const;
    void SignalFire();
    void EnterFire();
    void ExitFire();
    void EnterCharge();
    bool UpdateCharge(float fDeltaTime);
    void ExitCharge();
    void ExitReload();
    void EnterEmpty();
    void ExitEmpty();
    bool CheckCharge() const;
    bool CheckFire() const;
    bool FireDone() const;
    int* GetItemCount() const;
    const GameObject* GetItem(int iIndex) const;

protected:

    virtual bool UpdateFire(float fDeltaTime);
    virtual Entity* Fire();

public:

    static PblHash rttiHashWeaponDispenser;
    static const char rttiNameWeaponDispenser[];

    bool mNetFiredFlag;
    bool mFired;
    float mStrength;
    PblHandle<GameObject>** mItemArray;
    int* mItemCount;
};
 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00682430
inline bool WeaponDispenserClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponDispenserClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006836d0
inline unsigned int WeaponDispenserClass::GetDerivedRtti() const
{
    return rttiHashWeaponDispenserClass;
}

// FUNCTION: SWBF2 0x006836e0
inline const char* WeaponDispenserClass::GetDerivedRttiName() const
{
    return rttiNameWeaponDispenserClass;
}

// FUNCTION: SWBF2 0x00682400
inline bool WeaponDispenser::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponDispenser == uRtti)
    {
        return true;
    }
    return reinterpret_cast<const Weapon*>(this)->Weapon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006836b0
inline unsigned int WeaponDispenser::GetDerivedRtti() const
{
    return rttiHashWeaponDispenser;
}

// FUNCTION: SWBF2 0x006836c0
inline const char* WeaponDispenser::GetDerivedRttiName() const
{
    return rttiNameWeaponDispenser;
}
