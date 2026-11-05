#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityBuildingArmedDynamic.h"
#include "GameSound.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class Character;

// VTABLE: SWBF2 0x0079c66c
class EntityPortableTurretClass : public EntityBuildingArmedDynamicClass
{
public:
    EntityPortableTurretClass(unsigned int uHash);

    EntityPortableTurretClass(EntityPortableTurretClass* pBase, unsigned int uHash);

    virtual ~EntityPortableTurretClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    GameSound mCollisionSound;
    GameSound mCollisionFoliageSound;
    GameSound mCollisionWaterSound;
    float mLifeTime;
    float mGravity;
    float mRebound;
    float mFriction;

    static PblHash rttiHashEntityPortableTurretClass;
    static const char rttiNameEntityPortableTurretClass[];
};
  
 
 
 
 
 
 
 

class CollisionResult;
class Restrictor;

// VTABLE: SWBF2 0x0079c4b8
// VTABLE: SWBF2 0x0079c4d0 RedSceneObject
// VTABLE: SWBF2 0x0079c538 EntityEx
// VTABLE: SWBF2 0x0079c6b8 PblHandled
// VTABLE: SWBF2 0x0079c6c0 Damageable
// VTABLE: SWBF2 0x0079c6ec CollisionObject
class EntityPortableTurret : public EntityBuildingArmedDynamic
{
public:

    EntityPortableTurret(EntityPortableTurretClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual const PblVector3& GetVelocity() const;
    virtual void SetVelocity(const PblVector3& vVelocity);
    virtual void RecalculateAABB();
    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);
    virtual bool Update(float fDeltaTime);

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x004d7c10
    // EntityPortableTurret::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    static PblHash rttiHashEntityPortableTurret;
    static const char rttiNameEntityPortableTurret[];
    static MemoryPool sMemoryPool;

    EntityPortableTurretClass* mClass;
    float mLifeTimer;
    PblVector3 mVelocity;
    Character* mOwnerChar;
    bool mStationary;
    bool mBecomeStationaryNow;

    char _tailPad[6];
};

 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x004d7b90
inline bool EntityPortableTurret::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityPortableTurret == uRtti)
    {
        return true;
    }
    return EntityBuildingArmedDynamic::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004d7bf0
inline unsigned int EntityPortableTurret::GetDerivedRtti() const
{
    return rttiHashEntityPortableTurret;
}

// FUNCTION: SWBF2 0x004d7c00
inline const char* EntityPortableTurret::GetDerivedRttiName() const
{
    return rttiNameEntityPortableTurret;
}

// FUNCTION: SWBF2 0x004d7c80
inline bool EntityPortableTurretClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityPortableTurretClass == uRtti)
    {
        return true;
    }
    return EntityBuildingArmedDynamicClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004d7cd0
inline unsigned int EntityPortableTurretClass::GetDerivedRtti() const
{
    return rttiHashEntityPortableTurretClass;
}

// FUNCTION: SWBF2 0x004d7ce0
inline const char* EntityPortableTurretClass::GetDerivedRttiName() const
{
    return rttiNameEntityPortableTurretClass;
}


