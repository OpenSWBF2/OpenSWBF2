#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityBuildingArmed.h"
#include "PebbleFL/Common/PblHashTable.h"

class MemoryPool;

// VTABLE: SWBF2 0x0079a03c
class EntityBuildingArmedDynamicClass : public EntityBuildingArmedClass
{
public:
    EntityBuildingArmedDynamicClass(unsigned int uHash);

    EntityBuildingArmedDynamicClass(EntityBuildingArmedDynamicClass* pBase, unsigned int uHash);

    virtual ~EntityBuildingArmedDynamicClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static PblHash rttiHashEntityBuildingArmedDynamicClass;
    static const char rttiNameEntityBuildingArmedDynamicClass[];
};
  
 

// VTABLE: SWBF2 0x00799ebc
// VTABLE: SWBF2 0x00799ed4 EntityEx
// VTABLE: SWBF2 0x0079a008 PblHandled
// VTABLE: SWBF2 0x0079a010 Damageable
// VTABLE: SWBF2 0x0079a088 RedSceneObject
// VTABLE: SWBF2 0x0079a0f0 CollisionObject
class EntityBuildingArmedDynamic : public EntityBuildingArmed
{
public:

    EntityBuildingArmedDynamic(EntityBuildingArmedDynamicClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    virtual EntityType GetEntityType() const;
    virtual void AddSends(void (*pfnSend)(GameObject*, int), int iArg);
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);

    static void* operator new(unsigned int uSize);
    static void operator delete(void* pBlock);

    static PblHash rttiHashEntityBuildingArmedDynamic;
    static MemoryPool sMemoryPool;
    static const char rttiNameEntityBuildingArmedDynamic[];
};
 

// FUNCTION: SWBF2 0x00496b90
inline bool EntityBuildingArmedDynamicClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingArmedDynamicClass == uRtti)
    {
        return true;
    }
    return EntityBuildingArmedClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00496be0
inline unsigned int EntityBuildingArmedDynamicClass::GetDerivedRtti() const
{
    return rttiHashEntityBuildingArmedDynamicClass;
}

// FUNCTION: SWBF2 0x00496bf0
inline const char* EntityBuildingArmedDynamicClass::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingArmedDynamicClass;
}

// FUNCTION: SWBF2 0x00496b10
inline unsigned int EntityBuildingArmedDynamic::GetDerivedRtti() const
{
    return rttiHashEntityBuildingArmedDynamic;
}

// FUNCTION: SWBF2 0x00496b20
inline const char* EntityBuildingArmedDynamic::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingArmedDynamic;
}

// FUNCTION: SWBF2 0x00496ac0
inline bool EntityBuildingArmedDynamic::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingArmedDynamic == uRtti)
    {
        return true;
    }
    return EntityBuildingArmed::IsRtti(uRtti);
}
