#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityProp.h"
#include "Thread.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Graphics/RedPose.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrSkeleton.h"

class RedAnimation;
class ZephyrAnim;

// VTABLE: SWBF2 0x0079a7e8
class EntityDoorClass : public EntityPropClass
{
public:
    struct AnimationTrigger
    {
        unsigned int mTriggerPoint;
        float mTriggerRadius;
    };

    EntityDoorClass(unsigned int uHash);
    EntityDoorClass(EntityDoorClass* pBase, unsigned int uHash);

    virtual ~EntityDoorClass();
    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    bool InitAnimations(const char* pszName);

    static PblHash rttiHashEntityDoorClass;
    static const char rttiNameEntityDoorClass[];

    const ZephyrAnim* mAnimation;
    RedAnimation* mAnimObj;
    AnimationTrigger mAnimationTriggers[3];
    GameSound mOpenSound;
    GameSound mCloseSound;
    GameSound mLockedSound;
};
  
 
 
 
 
 
 
 

class EntityDoorClass;
class EntityClass;

// VTABLE: SWBF2 0x0079a810
// VTABLE: SWBF2 0x0079a828 RedSceneObject
// VTABLE: SWBF2 0x0079a890 CollisionObject
// VTABLE: SWBF2 0x0079a8e4 EntityEx
class EntityDoor : public EntityProp, public Thread
{
public:

    EntityDoor(EntityDoorClass* pClass, const EntityDesc& desc);

    static void ResetDoorList();

    static void EnterClient();
    static void LeaveClient();

    void SetIsLocked(bool locked) { mIsLocked = locked; }

    static EntityDoor* Find(unsigned int uHash);
    static void BeginIter();
    static EntityDoor* GetCurrentIter();
    static void IncrementIter();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~EntityDoor();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual EntityClass* GetEntityClass();
    virtual const EntityClass* GetEntityClass() const;
    static unsigned int GetRtti();

    virtual const RedPose* GetPose() const;

    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);
    virtual bool Update(float fDeltaTime);

    static PblHash rttiHashEntityDoor;
    static const char rttiNameEntityDoor[];
    static PblHashTable<EntityDoor, 32> msHostDoors;
    static PblHashTable<EntityDoor, 32> msClientDoors;
    static PblHashTable<EntityDoor, 32>* mspDoors;
    static PblHashTable<EntityDoor, 32>::Itor msDoorIter;

    void* portal_;
    bool hasPortal_;
    float mRenderDt;
    float mLastAnimTime;
    EntityDoorClass* mAnimatedClass;
    RedAnimation* mAnimObj;
    ZephyrSkeleton<32> mZephyrSkeleton;
    ZephyrPoseDyn<32> mZephyrPoseDyn;
    RedPose mPose;
    bool mOpenTrigger : 1;
    bool mIsOpen : 1;
    bool mIsLocked : 1;
    bool mJustRendered : 1;
    bool mTriggerSkipLastResult : 1;
    unsigned char mTriggerSkipCnt : 2;
    float mTimeSinceLastTriggered;
};
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00497f70
inline unsigned int EntityDoor::GetDerivedRtti() const
{
    return rttiHashEntityDoor;
}

// FUNCTION: SWBF2 0x00497f80
inline const char* EntityDoor::GetDerivedRttiName() const
{
    return rttiNameEntityDoor;
}

// FUNCTION: SWBF2 0x00497f20
inline bool EntityDoor::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityDoor == uRtti)
    {
        return true;
    }
    return reinterpret_cast<const EntityProp*>(this)->EntityProp::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00497ff0
inline bool EntityDoorClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityDoorClass == uRtti)
    {
        return true;
    }
    return EntityPropClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00498020
inline unsigned int EntityDoorClass::GetDerivedRtti() const
{
    return rttiHashEntityDoorClass;
}

// FUNCTION: SWBF2 0x00498030
inline const char* EntityDoorClass::GetDerivedRttiName() const
{
    return rttiNameEntityDoorClass;
}
