#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityBuildingAnimated.h"

class MountedTurretClass;

// VTABLE: SWBF2 0x00799b3c
class EntityBuildingAnimatedArmedClass : public EntityBuildingAnimatedClass
{
public:
    static const int kMaxTurrets = 8;
    EntityBuildingAnimatedArmedClass(unsigned int uHash);

    EntityBuildingAnimatedArmedClass(EntityBuildingAnimatedArmedClass* pBase, unsigned int uHash);

    virtual ~EntityBuildingAnimatedArmedClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static PblHash rttiHashEntityBuildingAnimatedArmedClass;
    static const char rttiNameEntityBuildingAnimatedArmedClass[];

    MountedTurretClass* mTurretClass[kMaxTurrets];
};
  
 

class Controllable;
class MountedTurret;
class Trackable;

// VTABLE: SWBF2 0x00799900 RedSceneObject
// VTABLE: SWBF2 0x00799968 Damageable
// VTABLE: SWBF2 0x00799994 PblHandled
// VTABLE: SWBF2 0x0079999c EntityEx
// VTABLE: SWBF2 0x00799ad0 CollisionObject
// VTABLE: SWBF2 0x00799b24
class EntityBuildingAnimatedArmed : public EntityBuildingAnimated
{
public:
    static const int kMaxTurrets = EntityBuildingAnimatedArmedClass::kMaxTurrets;
    EntityBuildingAnimatedArmed(EntityBuildingAnimatedArmedClass* pClass, const EntityDesc& desc);
    virtual ~EntityBuildingAnimatedArmed();
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual void Init();
    virtual void ActivatePhysics();
    virtual void DeactivatePhysics();
    virtual bool Update(float fDeltaTime);
    virtual void Kill();
    virtual void Respawn();
    virtual void Render(unsigned char uFlags, float fDist, unsigned int uMask);
    unsigned int GetDerivedRtti() const;
    const char* GetDerivedRttiName() const;

    GameObjectType GetGameObjectType();
    EntityType GetEntityType() const;
    virtual Trackable* GetTrackable();
    virtual int GetTurretCount() const;
    virtual void AddSends(void (*pfnSend)(GameObject*, int), int iArg);
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();
    virtual const Controllable* GetTurretPtr(int iIndex) const;

    static unsigned int GetRtti();

    static PblHash rttiHashEntityBuildingAnimatedArmed;
    static const char rttiNameEntityBuildingAnimatedArmed[];

    MountedTurret* mTurret[kMaxTurrets];
    int mNumTurrets;

    char mTailPadding[0xc];
};
 
 
 

// FUNCTION: SWBF2 0x00476780
inline bool EntityBuildingAnimatedArmedClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingAnimatedArmedClass == uRtti)
    {
        return true;
    }
    return EntityBuildingAnimatedClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00495040
inline unsigned int EntityBuildingAnimatedArmedClass::GetDerivedRtti() const
{
    return rttiHashEntityBuildingAnimatedArmedClass;
}

// FUNCTION: SWBF2 0x00495050
inline const char* EntityBuildingAnimatedArmedClass::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingAnimatedArmedClass;
}

// FUNCTION: SWBF2 0x004766e0
inline bool EntityBuildingAnimatedArmed::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingAnimatedArmed == uRtti)
    {
        return true;
    }
    return EntityBuildingAnimated::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00494fe0
inline unsigned int EntityBuildingAnimatedArmed::GetDerivedRtti() const
{
    return rttiHashEntityBuildingAnimatedArmed;
}

// FUNCTION: SWBF2 0x00494ff0
inline const char* EntityBuildingAnimatedArmed::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingAnimatedArmed;
}
