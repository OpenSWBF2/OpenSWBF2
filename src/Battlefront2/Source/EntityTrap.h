#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityBuilding.h"
#include "Thread.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "RedEngineFL/Graphics/RedPose.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrSkeleton.h"

class CollisionObject;
class CollisionResult;
class Entity;
class GameObject;
class RedAnimation;
class Restrictor;
class ZephyrAnim;

// VTABLE: SWBF2 0x0079d55c
class EntityTrapClass : public EntityBuildingClass
{
public:
    EntityTrapClass(unsigned int uHash);

    EntityTrapClass(EntityTrapClass* pBase, unsigned int uHash);

    virtual ~EntityTrapClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    bool InitAnimations(const char* pszName);
    bool SetupAnimation(ZephyrPoseDyn<64>& kPose, const ZephyrAnim* pAnim, float fTime);

    static PblHash rttiHashEntityTrapClass;
    static const char rttiNameEntityTrapClass[];

    ZephyrAnim* mAnimBuildTrap;
    PblMatrix mBuildOffset;
    EntityClass* mBuildBoxOdf;
    ZephyrAnim* mAnimTriggerTrap;
    RedAnimation* mAnimObj;
    int mTriggerSide;
    int mResetSide;
    unsigned int mTriggerNode;
    PblVector3 mTriggerOffset;
    float mTriggerRadius;
    float mMinEnemyRadius;
    float mDeathTime;
    unsigned int mDeathEffect;
    PblVector3 mDeathEffectOffset;
    GameSound mTriggerSound;
    bool mCollisionOnTrigger;
    bool mCollisionOnBuilt;
    bool mDeathOnCollision;
    CollisionModel mTrapCollisionModel;
    CollisionModel mBuildBoxCollisionModel;
    float mRayTriggerWidth;
    float mRayTriggerMinSpeed;
    bool mHideUnbuiltModel;
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x0079d5a8 PblHandled
// VTABLE: SWBF2 0x0079d5b0 EntityEx
// VTABLE: SWBF2 0x0079d6d8
// VTABLE: SWBF2 0x0079d6f0 CollisionObject
// VTABLE: SWBF2 0x0079d744 Damageable
// VTABLE: SWBF2 0x0079d770 RedSceneObject
class EntityTrap : public EntityBuilding, public Thread
{
public:

    EntityTrap(EntityTrapClass* pClass, const EntityDesc& desc);

    virtual ~EntityTrap();

    bool IsRtti(unsigned int uRtti) const;
    unsigned int GetDerivedRtti() const;
    const char* GetDerivedRttiName() const;
    const RedPose* GetPose() const;
    EntityType GetEntityType() const;
    bool IsVisibleToMe(const GameObject* pObject) const;
    int GetTargetBodyID(const PblVector3& vFrom, const PblVector3& vTo) const;
    PblVector3 GetTargetPoint(const PblVector3& vFrom, const PblVector3& vTo, int iBody) const;
    void Render(unsigned char uFlags, float fDist, unsigned int uMask);
    bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                           Restrictor* pRestrictor);

    void HealthChangeCallback(bool bArg, float fDelta);
    void SetAnimation(const ZephyrAnim* pAnim);
    bool Update(float fDeltaTime);

    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();

    static PblHash rttiHashEntityTrap;
    static const char rttiNameEntityTrap[];

    float mAnimationTime;
    EntityTrapClass* mTrapClass;
    RedAnimation* mAnimObj;
    ZephyrSkeleton<64> mZephyrSkeleton;
    ZephyrPoseDyn<64> mZephyrPoseDyn;
    RedPose mPose;
    ZephyrAnim* mCurrAnim;
    bool mBuildingTrap;
    bool mTriggered;
    float mUnbuiltTime;
    PblSphere mTrapCollisionSphere;
    PblSphere mBuildBoxCollisionSphere;
    Entity* mBuildBox;
};
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x004fcc70
inline bool EntityTrap::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityTrap == uRtti)
    {
        return true;
    }
    return EntityBuilding::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004fccc0
inline unsigned int EntityTrap::GetDerivedRtti() const
{
    return rttiHashEntityTrap;
}

// FUNCTION: SWBF2 0x004fccd0
inline const char* EntityTrap::GetDerivedRttiName() const
{
    return rttiNameEntityTrap;
}

// FUNCTION: SWBF2 0x004fcd30
inline bool EntityTrapClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityTrapClass == uRtti)
    {
        return true;
    }
    return EntityBuildingClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004fcd70
inline unsigned int EntityTrapClass::GetDerivedRtti() const
{
    return rttiHashEntityTrapClass;
}

// FUNCTION: SWBF2 0x004fcd80
inline const char* EntityTrapClass::GetDerivedRttiName() const
{
    return rttiNameEntityTrapClass;
}
