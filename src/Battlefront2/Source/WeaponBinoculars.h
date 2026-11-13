#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "Weapon.h"
#include "PebbleFL/Common/PblRtti.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007b0430
class WeaponBinocularsClass : public WeaponClass
{
public:
    WeaponBinocularsClass(unsigned int uHash);
    WeaponBinocularsClass(WeaponBinocularsClass* pBase, unsigned int uHash);
    virtual ~WeaponBinocularsClass();

    static PblHash rttiHashWeaponBinocularsClass;
    static const char rttiNameWeaponBinocularsClass[];

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
};
  

#include "WeaponCannon.h"

// VTABLE: SWBF2 0x007b0468
class WeaponBinoculars : public Weapon
{
public:
    WeaponBinoculars(const WeaponBinocularsClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponBinoculars();
    // FUNCTION: SWBF2 0x0067e120
    // WeaponBinoculars::IsRtti
    // FUNCTION: SWBF2 0x0067e150
    // WeaponBinoculars::GetDerivedRtti
    // FUNCTION: SWBF2 0x0067e160
    // WeaponBinoculars::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(WeaponBinoculars, Weapon)

};
  

// FUNCTION: SWBF2 0x0067e170
inline bool WeaponBinocularsClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponBinocularsClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0067e1a0
inline unsigned int WeaponBinocularsClass::GetDerivedRtti() const
{
    return rttiHashWeaponBinocularsClass;
}

// FUNCTION: SWBF2 0x0067e1b0
inline const char* WeaponBinocularsClass::GetDerivedRttiName() const
{
    return rttiNameWeaponBinocularsClass;
}
