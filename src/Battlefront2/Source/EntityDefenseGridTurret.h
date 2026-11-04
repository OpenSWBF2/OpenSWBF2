#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityBuildingArmedDynamic.h"

// VTABLE: SWBF2 0x0079a778
class EntityDefenseGridTurretClass : public EntityBuildingArmedDynamicClass
{
public:
    EntityDefenseGridTurretClass(unsigned int uHash);

    EntityDefenseGridTurretClass(EntityDefenseGridTurretClass* pBase, unsigned int uHash);

    virtual ~EntityDefenseGridTurretClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mRespawnTime;

    static PblHash rttiHashEntityDefenseGridTurretClass;

    static unsigned int GetRtti() { return rttiHashEntityDefenseGridTurretClass; }
    static const char rttiNameEntityDefenseGridTurretClass[];
};
  
 

class MemoryPool;

// VTABLE: SWBF2 0x0079a544 EntityEx
// VTABLE: SWBF2 0x0079a53c PblHandled
// VTABLE: SWBF2 0x0079a678 Damageable
// VTABLE: SWBF2 0x0079a6a4
// VTABLE: SWBF2 0x0079a6bc RedSceneObject
// VTABLE: SWBF2 0x0079a724 CollisionObject
class EntityDefenseGridTurret : public EntityBuildingArmedDynamic
{
public:

    EntityDefenseGridTurret(EntityDefenseGridTurretClass* pClass, const EntityDesc& desc);
    // SYNTHETIC: SWBF2 0x00497a30
    // EntityDefenseGridTurret::`scalar deleting destructor'
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool Update(float fDeltaTime);

    static void* operator new(unsigned int uSize);
    static void operator delete(void* pBlock);

    static MemoryPool sMemoryPool;
    static PblHash rttiHashEntityDefenseGridTurret;
    static const char rttiNameEntityDefenseGridTurret[];

    EntityDefenseGridTurretClass* mClass;
    float mRespawnTimer;
};
 
 
 

// FUNCTION: SWBF2 0x004979a0
inline bool EntityDefenseGridTurret::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityDefenseGridTurret == uRtti)
    {
        return true;
    }
    return EntityBuildingArmedDynamic::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00497a00
inline unsigned int EntityDefenseGridTurret::GetDerivedRtti() const
{
    return rttiHashEntityDefenseGridTurret;
}

// FUNCTION: SWBF2 0x00497a10
inline const char* EntityDefenseGridTurret::GetDerivedRttiName() const
{
    return rttiNameEntityDefenseGridTurret;
}

// FUNCTION: SWBF2 0x00497a60
inline bool EntityDefenseGridTurretClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityDefenseGridTurretClass == uRtti)
    {
        return true;
    }
    return EntityBuildingArmedDynamicClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00497ab0
inline unsigned int EntityDefenseGridTurretClass::GetDerivedRtti() const
{
    return rttiHashEntityDefenseGridTurretClass;
}

// FUNCTION: SWBF2 0x00497ac0
inline const char* EntityDefenseGridTurretClass::GetDerivedRttiName() const
{
    return rttiNameEntityDefenseGridTurretClass;
}
