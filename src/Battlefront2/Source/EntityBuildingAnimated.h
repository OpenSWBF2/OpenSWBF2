#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityBuilding.h"
#include "Thread.h"
#include "RedEngineFL/Graphics/RedPose.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrSkeleton.h"

class RedAnimation;
class ZephyrAnim;

// VTABLE: SWBF2 0x007996d0
class EntityBuildingAnimatedClass : public EntityBuildingClass
{
public:
    EntityBuildingAnimatedClass(unsigned int uHash);

    EntityBuildingAnimatedClass(EntityBuildingAnimatedClass* pBase, unsigned int uHash);

    virtual ~EntityBuildingAnimatedClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    bool InitAnimations(const char* pszName);
    bool SetupAnimation(ZephyrPoseDyn<32>& kPose, int iAnim, float fTime, bool bLoop);

    static PblHash rttiHashEntityBuildingAnimatedClass;
    static const char rttiNameEntityBuildingAnimatedClass[];

    ZephyrAnim* mAnimations[2];
    RedAnimation* mAnimObj;
    bool mDeathExplosions;
    unsigned int mDeathEffectHash[5];
    unsigned int mDeathEffectNodeHash[5];
};
  
 
 
 
 
 

class EntityBuildingAnimatedClass;
struct EntityDesc;

// VTABLE: SWBF2 0x00799684 Damageable
// VTABLE: SWBF2 0x007996b0
// VTABLE: SWBF2 0x007996c8 PblHandled
// VTABLE: SWBF2 0x0079971c CollisionObject
// VTABLE: SWBF2 0x00799770 EntityEx
// VTABLE: SWBF2 0x00799898 RedSceneObject
class EntityBuildingAnimated : public EntityBuilding, public Thread
{
public:

    EntityBuildingAnimated(EntityBuildingAnimatedClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~EntityBuildingAnimated();
    virtual void Init();
    virtual EntityClass* GetEntityClass();
    virtual const EntityClass* GetEntityClass() const;

    virtual const RedPose* GetPose() const;
    virtual void ActivatePhysics();
    virtual void DeactivatePhysics();
    virtual void Render(unsigned char uFlags, float fDist, unsigned int uMask);
    virtual void Kill();
    virtual bool Update(float fDeltaTime);

    void SetAnimation(unsigned int uAnimationId, bool bLoop);

    static PblHash rttiHashEntityBuildingAnimated;
    static const char rttiNameEntityBuildingAnimated[];

    EntityBuildingAnimatedClass* mAnimatedClass;
    float mAnimationTime;
    float mDeathExplosionTime;
    unsigned int mAnimationId;
    RedAnimation* mAnimObj;
    ZephyrSkeleton<32> mZephyrSkeleton;
    ZephyrPoseDyn<32> mZephyrPoseDyn;
    RedPose mPose;
};
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00475fb0
inline bool EntityBuildingAnimatedClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingAnimatedClass == uRtti)
    {
        return true;
    }
    return EntityBuildingClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00494140
inline unsigned int EntityBuildingAnimatedClass::GetDerivedRtti() const
{
    return rttiHashEntityBuildingAnimatedClass;
}

// FUNCTION: SWBF2 0x00494150
inline const char* EntityBuildingAnimatedClass::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingAnimatedClass;
}

// FUNCTION: SWBF2 0x00475f30
inline bool EntityBuildingAnimated::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingAnimated == uRtti)
    {
        return true;
    }
    return EntityBuilding::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004940f0
inline unsigned int EntityBuildingAnimated::GetDerivedRtti() const
{
    return rttiHashEntityBuildingAnimated;
}

// FUNCTION: SWBF2 0x00494100
inline const char* EntityBuildingAnimated::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingAnimated;
}
