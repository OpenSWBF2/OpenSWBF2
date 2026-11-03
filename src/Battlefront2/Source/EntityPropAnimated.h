#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityProp.h"
#include "Thread.h"
#include "PebbleFL/Common/PblHandle.h"
#include "RedEngineFL/Graphics/RedPose.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrPoseStatic.h"
#include "Zephyr/Source/ZephyrSkeleton.h"

class RedAnimation;
class ZephyrAnim;
class GameObject;
class EntityPropAnimated;

// VTABLE: SWBF2 0x0079ca40
class EntityPropAnimatedClass : public EntityPropClass
{
public:
    struct AnimationTrigger
    {
        unsigned int mTriggerPoint;
        float mTriggerRadius;
        unsigned int mAnimationID;
    };

    struct AttachTrigger
    {
        unsigned int mAttachPoint;
        float mAttachRadius;
        unsigned int mAnimationID;
        int mSoldierAnimation;
        float mAnimationTime;
        int mPostAttachEvent;
    };

    EntityPropAnimatedClass(unsigned int uHash);

    EntityPropAnimatedClass(EntityPropAnimatedClass* pBase, unsigned int uHash);
    virtual ~EntityPropAnimatedClass();
    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    bool InitAnimations(const char* pszName);
    bool SetupAnimation(ZephyrPoseDyn<32>& kPose, int iAnim, float fTime, bool bLoop);

    static PblHash rttiHashEntityPropAnimatedClass;
    static const char rttiNameEntityPropAnimatedClass[];

    ZephyrAnim* mAnimations[5];
    RedAnimation* mAnimObj;
    GameSound mAnimationSound[5];
    AnimationTrigger mAnimationTriggers[3];
    AttachTrigger mAttachTriggers[3];
    GameSound mKillSoldierSound;
    bool mDisableForCloneWars;
    float mIdleDelay;
    bool mTrackDeathOnAttach;
};
  
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x0079c9e0 EntityEx
// VTABLE: SWBF2 0x0079c90c RedSceneObject
// VTABLE: SWBF2 0x0079c974 CollisionObject
// VTABLE: SWBF2 0x0079c9c8
class EntityPropAnimated : public EntityProp, public Thread
{
public:
    EntityPropAnimated(EntityPropAnimatedClass* pClass, const EntityDesc& desc);
    virtual ~EntityPropAnimated();

    virtual bool IsRtti(unsigned int uRtti) const;
    static unsigned int GetRtti();
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual EntityClass* GetEntityClass();
    virtual const EntityClass* GetEntityClass() const;

    virtual const RedPose* GetPose() const;
    virtual void Render(unsigned char ucLod, float fAlpha, unsigned int uFlags);
    virtual bool Update(float fDeltaTime);

    void SetAnimation(unsigned int uAnimationId, bool bLoop);

    static PblHash rttiHashEntityPropAnimated;
    static const char rttiNameEntityPropAnimated[];

    EntityPropAnimatedClass* mAnimatedClass;
    float mAnimationTime;
    float mBlendFactor;
    unsigned int mAnimationId;
    bool mLooping;
    RedAnimation* mAnimObj;
    ZephyrSkeleton<32> mZephyrSkeleton;
    ZephyrPoseStatic<32> mZephyrPoseStatic;
    ZephyrPoseDyn<32> mZephyrPoseDyn;
    RedPose mPose;
    PblHandle<GameObject> mAttachedSoldier;
    int mAttachedSoldierAnimation;
    int mAttachEvent;
    PblMatrix* mAttachMatrix;
    PblMatrix mCameraMatrix;
    float mIdleDelayTimer;
};
 
 
 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x004db370
inline bool EntityPropAnimatedClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityPropAnimatedClass == uRtti)
    {
        return true;
    }
    return EntityPropClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004db2a0
inline bool EntityPropAnimated::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityPropAnimated == uRtti)
    {
        return true;
    }
    return EntityProp::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004db3a0
inline unsigned int EntityPropAnimatedClass::GetDerivedRtti() const
{
    return rttiHashEntityPropAnimatedClass;
}

// FUNCTION: SWBF2 0x004db3b0
inline const char* EntityPropAnimatedClass::GetDerivedRttiName() const
{
    return rttiNameEntityPropAnimatedClass;
}

// FUNCTION: SWBF2 0x004db2f0
inline unsigned int EntityPropAnimated::GetDerivedRtti() const
{
    return rttiHashEntityPropAnimated;
}

// FUNCTION: SWBF2 0x004db300
inline const char* EntityPropAnimated::GetDerivedRttiName() const
{
    return rttiNameEntityPropAnimated;
}
