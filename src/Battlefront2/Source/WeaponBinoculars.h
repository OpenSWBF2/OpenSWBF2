#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include "Weapon.h"
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
  

#include "WeaponCannonInstance.h"

// VTABLE: SWBF2 0x007b0468
class WeaponBinoculars : public Weapon
{
public:
    WeaponBinoculars(const WeaponBinocularsClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponBinoculars();
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static PblHash rttiHashWeaponBinoculars;
    static const char rttiNameWeaponBinoculars[];
};
  

// FUNCTION: SWBF2 0x0067e120
inline bool WeaponBinoculars::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponBinoculars == uRtti)
    {
        return true;
    }
    return Weapon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0067e150
inline unsigned int WeaponBinoculars::GetDerivedRtti() const
{
    return rttiHashWeaponBinoculars;
}

// FUNCTION: SWBF2 0x0067e160
inline const char* WeaponBinoculars::GetDerivedRttiName() const
{
    return rttiNameWeaponBinoculars;
}

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
