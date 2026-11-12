#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Weapon.h"
#include "PebbleFL/Common/PblRtti.h"
#include "WeaponCannon.h"
#include "WeaponMelee.h"

// VTABLE: SWBF2 0x007b16ac
class WeaponMeleeThrowClass : public WeaponClass
{
public:
    WeaponMeleeThrowClass(unsigned int uHash);
    WeaponMeleeThrowClass(WeaponMeleeThrowClass* pBase, unsigned int uHash);
    virtual ~WeaponMeleeThrowClass();

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    int m_iPrimaryWeaponIndex;
    float m_fInitialSalvoDelay;

    static PblHash rttiHashWeaponMeleeThrowClass;

    // FUNCTION: SWBF2 0x0046dbf0
    static unsigned int GetRtti() { return rttiHashWeaponMeleeThrowClass; }
    static const char rttiNameWeaponMeleeThrowClass[];
};
  
 
 

class Ordnance;
class WeaponMelee;

// VTABLE: SWBF2 0x007b16e4
class WeaponMeleeThrow : public Weapon
{
public:
    WeaponMeleeThrow(const WeaponMeleeThrowClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponMeleeThrow();
    // FUNCTION: SWBF2 0x0068f520
    // WeaponMeleeThrow::IsRtti
    // FUNCTION: SWBF2 0x0068f550
    // WeaponMeleeThrow::GetDerivedRtti
    // FUNCTION: SWBF2 0x0068f560
    // WeaponMeleeThrow::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(WeaponMeleeThrow, Weapon)
    virtual bool IsMeleeThrow() const;
    virtual void SetOrdnance(Ordnance* pOrdnance);
    virtual Ordnance* CreateOrdnance(OrdnanceDesc& desc);
    void NotifyThrow();
    void NotifyReturned();
    bool IsCurrentlyThrown() const { return m_bThrow; }
    void AcknowledgeReturn() { m_bReturnedThisFrame = 0; }
    bool ReturnedThisFrame() { return m_bReturnedThisFrame != 0; }
    const PblMatrix& GetFireMatrixLocal() const { return m_mThrow0Local; }
    virtual bool IsBusy() const;
    virtual bool Update(float fDeltaTime);
    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);
    virtual void Render(const PblMatrix& matrix, const RedPose* pPose,
                        const RedColor& color, unsigned int uiFlags, bool bRenderModel) const;

protected:
    virtual void EnterFire();
    virtual bool UpdateFire(float fDeltaTime);
    virtual void ExitFire();
    virtual void EnterEmpty();
    virtual bool CheckFire() const;

    void Fire();
    virtual bool FireDone() const;
    virtual bool CheckEmpty() const;
    virtual bool ReloadDone() const;
    const WeaponMeleeClass* _GetPrimaryClass() const
    {
        return (const WeaponMeleeClass*)m_pPrimaryWeapon->mClass;
    }

public:

    WeaponMelee* m_pPrimaryWeapon;
    unsigned int m_bFired : 1;
    unsigned int m_bThrow : 1;
    unsigned int m_bReturnedThisFrame : 1;
    unsigned int z_uiReserved0 : 29;
    PblMatrix m_mThrow0Local;

};
  
 
 

// FUNCTION: SWBF2 0x0068f5c0
inline bool WeaponMeleeThrowClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponMeleeThrowClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0068f5f0
inline unsigned int WeaponMeleeThrowClass::GetDerivedRtti() const
{
    return rttiHashWeaponMeleeThrowClass;
}

// FUNCTION: SWBF2 0x0068f600
inline const char* WeaponMeleeThrowClass::GetDerivedRttiName() const
{
    return rttiNameWeaponMeleeThrowClass;
}
