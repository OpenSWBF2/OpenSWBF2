#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Entity.h"
#include "PebbleFL/Common/PblHashTable.h"

class FLEffectClass;

// VTABLE: SWBF2 0x0079b1a0
class EntityEffectClass : public EntityClass
{
public:
    EntityEffectClass(unsigned int uHash);

    virtual ~EntityEffectClass();
    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static PblHash rttiHashEntityEffectClass;
    static const char rttiNameEntityEffectClass[];

    FLEffectClass* mEffectClass;
};
  
 

// FUNCTION: SWBF2 0x004a99b0
inline bool EntityEffectClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityEffectClass == uRtti)
    {
        return true;
    }
    return EntityClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004a99e0
inline unsigned int EntityEffectClass::GetDerivedRtti() const
{
    return rttiHashEntityEffectClass;
}

// FUNCTION: SWBF2 0x004a99f0
inline const char* EntityEffectClass::GetDerivedRttiName() const
{
    return rttiNameEntityEffectClass;
}
