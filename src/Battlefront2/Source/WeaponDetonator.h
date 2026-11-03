#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "WeaponDispenser.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007b0a60
class WeaponDetonatorClass : public WeaponDispenserClass
{
public:
    virtual ~WeaponDetonatorClass();
    WeaponDetonatorClass(unsigned int uHash);
    WeaponDetonatorClass(WeaponDetonatorClass* pBase, unsigned int uHash);

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual WeaponDispenser* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    unsigned int mPlantedHash;

    static PblHash rttiHashWeaponDetonatorClass;
    static const char rttiNameWeaponDetonatorClass[];
};
  
 

// VTABLE: SWBF2 0x007b096c
class WeaponDetonator : public WeaponDispenser
{
public:
    virtual ~WeaponDetonator();
    WeaponDetonator(const WeaponDetonatorClass* pClass, const WeaponDesc& desc);

    virtual bool Update(float fDeltaTime);
    virtual bool IsRtti(unsigned int uRtti) const;

    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);
    virtual unsigned int GetNameHash() const;

    bool HasItemsDispensed() const;

    void NetAttachItem(Entity* pAttach);

    virtual bool CheckCharge() const;
    virtual bool CheckFire() const;

    // FUNCTION: SWBF2 0x005b4250
    static unsigned int GetRtti() { return rttiHashWeaponDetonator; }

    static PblHash rttiHashWeaponDetonator;
    static const char rttiNameWeaponDetonator[];
};
 

// FUNCTION: SWBF2 0x00682490
inline unsigned int WeaponDetonator::GetDerivedRtti() const
{
    return rttiHashWeaponDetonator;
}

// FUNCTION: SWBF2 0x006824a0
inline const char* WeaponDetonator::GetDerivedRttiName() const
{
    return rttiNameWeaponDetonator;
}

// FUNCTION: SWBF2 0x006824b0
inline bool WeaponDetonatorClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponDetonatorClass == uRtti)
    {
        return true;
    }
    return WeaponDispenserClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006824e0
inline unsigned int WeaponDetonatorClass::GetDerivedRtti() const
{
    return rttiHashWeaponDetonatorClass;
}

// FUNCTION: SWBF2 0x006824f0
inline const char* WeaponDetonatorClass::GetDerivedRttiName() const
{
    return rttiNameWeaponDetonatorClass;
}

// FUNCTION: SWBF2 0x00682460
inline bool WeaponDetonator::IsRtti(unsigned int uRtti) const
{
    if (GetRtti() == uRtti)
    {
        return true;
    }
    return WeaponDispenser::IsRtti(uRtti);
}
