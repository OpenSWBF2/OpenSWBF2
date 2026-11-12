#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityItem.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "StickInfo.h"

class Character;
class EntityLight;
class ExplosionClass;
class FLEffectObject;
class StickInfo;

// VTABLE: SWBF2 0x0079c394
class EntityMineClass : public EntityItemClass
{
public:
    EntityMineClass(unsigned int uHash);
    EntityMineClass(EntityMineClass* pBase, unsigned int uHash);
    virtual ~EntityMineClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    GameSound mTick;
    float mTickPitch;
    ExplosionClass* mExplosionTrigger;
    ExplosionClass* mExplosionExpire;
    ExplosionClass* mExplosionDeath;
    bool mTriggerContact;
    float mTriggerRadius;
    float mSuppressRadius;

    static PblHash rttiHashEntityMineClass;
    static const char rttiNameEntityMineClass[];
};
  
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x0079c274 EntityEx
// VTABLE: SWBF2 0x0079c1b8 CollisionObject
// VTABLE: SWBF2 0x0079c20c RedSceneObject
// VTABLE: SWBF2 0x0079c3e0 Damageable
// VTABLE: SWBF2 0x0079c40c PblHandled
// VTABLE: SWBF2 0x0079c414
class EntityMine : public EntityItem
{
public:
    static void SetupMemoryPool(unsigned int uCount, unsigned int uSize, unsigned int uGrow) { sMemoryPool.Setup(uCount, uSize, uGrow); }
    EntityMine(EntityMineClass* pClass, const EntityDesc& desc);
    virtual ~EntityMine();

    virtual bool IsRtti(unsigned int uRtti) const;
    static unsigned int GetRtti() { return rttiHashEntityMine; }
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void ClientKill();
    virtual EntityType GetEntityType() const;
    virtual void Kill();
    virtual bool Update(float fDeltaTime);
    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);

    static bool KeepsParentLink(const CollisionObject* pChild);

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x004cf530
    // EntityMine::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    static PblHash rttiHashEntityMine;
    static const char rttiNameEntityMine[];
    static MemoryPool sMemoryPool;

    bool mNetDead;
    int mPrevTimerVal;
    StickInfo* mStickInfo;
    PblVector3 mOldPosition;
    int mHitSurface;
    bool mTrigger;
};
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x004cf580
inline bool EntityMineClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityMineClass == uRtti)
    {
        return true;
    }
    return EntityItemClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004cf5c0
inline unsigned int EntityMineClass::GetDerivedRtti() const
{
    return rttiHashEntityMineClass;
}

// FUNCTION: SWBF2 0x004cf5d0
inline const char* EntityMineClass::GetDerivedRttiName() const
{
    return rttiNameEntityMineClass;
}

// FUNCTION: SWBF2 0x004cf4c0
inline bool EntityMine::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityMine == uRtti)
    {
        return true;
    }
    return EntityItem::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004cf510
inline unsigned int EntityMine::GetDerivedRtti() const
{
    return rttiHashEntityMine;
}

// FUNCTION: SWBF2 0x004cf520
inline const char* EntityMine::GetDerivedRttiName() const
{
    return rttiNameEntityMine;
}

inline bool EntityMine::KeepsParentLink(const CollisionObject* pChild)
{
    const GameObject* pObject = pChild->GetGameObject();
    if (!pObject || !pObject->IsRtti(rttiHashEntityMine))
    {
        return true;
    }
    const StickInfo* pStick = static_cast<const EntityMine*>(pObject)->mStickInfo;
    return !pStick || pStick->IsStuckTo(pChild->mParent);
}


