#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityItem.h"

// VTABLE: SWBF2 0x007adae4
class PowerupItemClass : public EntityItemClass
{
public:
    PowerupItemClass(unsigned int uHash);
    PowerupItemClass(PowerupItemClass* pBase, unsigned int uHash);
    virtual ~PowerupItemClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mAddHealth[8];
    float mAddAmmo[8];
    float mAddEnergy[8];
    float mBuffOffenseTimer;
    float mBuffOffenseMult;
    float mBuffDefenseTimer;
    float mBuffDefenseMult;
    float mBuffHealthTimer;
    float mBuffHealthRate;
    float mDebuffDamageTimer;
    float mDebuffDamageRate;
    GameSound mPowerupSound;
    GameSound mAmbientSound;

    static PblHash rttiHashPowerupItemClass;
    static const char rttiNamePowerupItemClass[];
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 

#include "GameSound.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

// VTABLE: SWBF2 0x007ad9bc EntityEx
// VTABLE: SWBF2 0x007ad8d4 CollisionObject
// VTABLE: SWBF2 0x007ad928 RedSceneObject
// VTABLE: SWBF2 0x007ad990 Damageable
// VTABLE: SWBF2 0x007adadc PblHandled
// VTABLE: SWBF2 0x007adb30
class PowerupItem : public EntityItem
{
public:
    PowerupItem(PowerupItemClass* pClass, const EntityDesc& desc);
    virtual ~PowerupItem();

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x00623860
    // PowerupItem::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual EntityType GetEntityType() const;
    virtual bool AddToTeamManager() const;
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);

    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);

    virtual bool Update(float fDeltaTime);

    GameSoundControllable mAmbientSound;

    static unsigned int GetRtti() { return rttiHashPowerupItem; }

    static MemoryPool sMemoryPool;
    static PblHash rttiHashPowerupItem;
    static const char rttiNamePowerupItem[];
};
 
 

// FUNCTION: SWBF2 0x00623890
inline bool PowerupItemClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashPowerupItemClass == uRtti)
    {
        return true;
    }
    return EntityItemClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006238d0
inline unsigned int PowerupItemClass::GetDerivedRtti() const
{
    return rttiHashPowerupItemClass;
}

// FUNCTION: SWBF2 0x006238e0
inline const char* PowerupItemClass::GetDerivedRttiName() const
{
    return rttiNamePowerupItemClass;
}

// FUNCTION: SWBF2 0x006237f0
inline bool PowerupItem::IsRtti(unsigned int uRtti) const
{
    if (rttiHashPowerupItem == uRtti)
    {
        return true;
    }
    return EntityItem::rttiHashEntityItem == uRtti || GameObject::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00623840
inline unsigned int PowerupItem::GetDerivedRtti() const
{
    return rttiHashPowerupItem;
}

// FUNCTION: SWBF2 0x00623850
inline const char* PowerupItem::GetDerivedRttiName() const
{
    return rttiNamePowerupItem;
}


