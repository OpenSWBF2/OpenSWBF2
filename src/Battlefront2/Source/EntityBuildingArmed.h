#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityBuilding.h"
#include "Thread.h"

class MountedTurretClass;

// VTABLE: SWBF2 0x00799c8c
class EntityBuildingArmedClass : public EntityBuildingClass
{
public:
    EntityBuildingArmedClass(unsigned int uHash);

    EntityBuildingArmedClass(EntityBuildingArmedClass* pBase, unsigned int uHash);

    virtual ~EntityBuildingArmedClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    const MountedTurretClass* GetTurretClass(int iIndex) const;

    MountedTurretClass* mTurretClass[8];

    static PblHash rttiHashEntityBuildingArmedClass;

    // FUNCTION: SWBF2 0x00476ed0
    static unsigned int GetRtti() { return rttiHashEntityBuildingArmedClass; }
    static const char rttiNameEntityBuildingArmedClass[];

};
  
 

#include "PebbleFL/Common/PblHandle.h"
#include "RedEngineFL/Graphics/RedPose.h"

class Controllable;
class MountedTurret;
class Trackable;
class FLEffectObject;

// VTABLE: SWBF2 0x00799bf8 Damageable
// VTABLE: SWBF2 0x00799c24 RedSceneObject
// VTABLE: SWBF2 0x00799cd8
// VTABLE: SWBF2 0x00799cf0 CollisionObject
// VTABLE: SWBF2 0x00799d44 EntityEx
// VTABLE: SWBF2 0x00799e78 PblHandled
class EntityBuildingArmed : public EntityBuilding, public Thread
{
public:

    EntityBuildingArmed(EntityBuildingArmedClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~EntityBuildingArmed();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void Init();
    virtual const Controllable* GetControllable() const;
    virtual Controllable* GetControllable();
    virtual void ActivatePhysics();
    virtual void DeactivatePhysics();
    virtual GameObjectType GetGameObjectType();
    virtual EntityType GetEntityType() const;
    virtual void AddSends(void (*pfnSend)(GameObject*, int), int iArg);
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();
    virtual Trackable* GetTrackable();
    virtual int GetTurretCount() const;
    virtual const Controllable* GetTurretPtr(int iIndex) const;
    virtual const RedPose* GetPose() const;
    virtual void Render(unsigned char uFlags, float fDist, unsigned int uMask);
    virtual void Kill();
    virtual void Respawn();
    virtual bool Update(float fDeltaTime);

    RedPose mPose;
    MountedTurret* mTurret[8];
    int mNumTurrets;
    PblHandle<FLEffectObject> mSmokePlume;
    bool mNoAIBoard : 1;

    static PblHash rttiHashEntityBuildingArmed;

    // FUNCTION: SWBF2 0x0043ef20
    static unsigned int GetRtti() { return rttiHashEntityBuildingArmed; }
    static const char rttiNameEntityBuildingArmed[];
};
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00476e90
inline bool EntityBuildingArmedClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingArmedClass == uRtti)
    {
        return true;
    }
    return EntityBuildingClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00495a20
inline unsigned int EntityBuildingArmedClass::GetDerivedRtti() const
{
    return rttiHashEntityBuildingArmedClass;
}

// FUNCTION: SWBF2 0x00495a30
inline const char* EntityBuildingArmedClass::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingArmedClass;
}

// FUNCTION: SWBF2 0x00476db0
inline bool EntityBuildingArmed::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingArmed == uRtti)
    {
        return true;
    }
    return EntityBuilding::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004959c0
inline unsigned int EntityBuildingArmed::GetDerivedRtti() const
{
    return rttiHashEntityBuildingArmed;
}

// FUNCTION: SWBF2 0x004959d0
inline const char* EntityBuildingArmed::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingArmed;
}
