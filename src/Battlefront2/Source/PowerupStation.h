#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityBuilding.h"
#include "GameSound.h"
#include "Thread.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "RedEngineFL/Graphics/RedPose.h"

// VTABLE: SWBF2 0x007adb6c
class PowerupStationClass : public EntityBuildingClass
{
public:
    PowerupStationClass(unsigned int uHash);
    PowerupStationClass(PowerupStationClass* pBase, unsigned int uHash);
    virtual ~PowerupStationClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static PblHash rttiHashPowerupStationClass;
    static const char rttiNamePowerupStationClass[];

    float mPowerupDelay;
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
    float mIdleRotateSpeed;
    float mIdleWaitTime;
    PblMatrix mBaseWobbleMatrix;
    unsigned int mIdleWobbleHash;
    unsigned int mIdleWobbleLeftFootHash;
    unsigned int mIdleWobbleRightFootHash;
    float mIdleWobbleFactor;
    unsigned int mActiveRotateHash[4];
    unsigned int mActiveSpinHash;
    GameSound mPowerupSound;
    GameSound mAmbientSound;
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

class RedRegion;

// VTABLE: SWBF2 0x007adbb8 CollisionObject
// VTABLE: SWBF2 0x007adc0c EntityEx
// VTABLE: SWBF2 0x007add34 RedSceneObject
// VTABLE: SWBF2 0x007add9c
// VTABLE: SWBF2 0x007addb4 PblHandled
// VTABLE: SWBF2 0x007addbc Damageable
class PowerupStation : public EntityBuilding, public Thread
{
public:
    enum PowerupStationState
    {
        PS_IDLE = 0,
        PS_WAIT = 1,
        PS_FACE = 2,
        PS_APPLY = 3,
    };

    PowerupStation(PowerupStationClass* pClass, const EntityDesc& desc);
    virtual ~PowerupStation();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual bool Update(float fDeltaTime);

    bool IsRtti(unsigned int uRtti) const;
    unsigned int GetDerivedRtti() const;
    const char* GetDerivedRttiName() const;
    EntityType GetEntityType() const;
    bool AddToTeamManager() const;
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();
    const PblMatrix* GetRenderMatrix() const;
    const RedPose* GetRenderPose() const;

    int GetObjectsInRegion(RedRegion* pRegion, GameObject** const ppObjects, int iMaxObjects) const;
    bool IsInsideRegion(RedRegion* pRegion, const GameObject* pObject) const;

    static PblHash rttiHashPowerupStation;
    static const char rttiNamePowerupStation[];

    PowerupStationClass* mClass;
    RedRegion* mEffectRegion;
    float mPowerupTimer;
    PowerupStationState mPowerupState;
    RedPose mPose;
    float mRotateWaitTime;
    char mRotateDir;
    float mIdleWobblePeriodicity;
    PblMatrix mRenderMatrix;
    PblMatrix mProceduralMatrix[5];
    GameSoundControllable mAmbientSound;
};
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x006245a0
inline bool PowerupStationClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashPowerupStationClass == uRtti)
    {
        return true;
    }
    return EntityBuildingClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x006245e0
inline unsigned int PowerupStationClass::GetDerivedRtti() const
{
    return rttiHashPowerupStationClass;
}

// FUNCTION: SWBF2 0x006245f0
inline const char* PowerupStationClass::GetDerivedRttiName() const
{
    return rttiNamePowerupStationClass;
}

// FUNCTION: SWBF2 0x006244f0
inline bool PowerupStation::IsRtti(unsigned int uRtti) const
{
    if (rttiHashPowerupStation == uRtti)
    {
        return true;
    }
    return EntityBuilding::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00624540
inline unsigned int PowerupStation::GetDerivedRtti() const
{
    return rttiHashPowerupStation;
}

// FUNCTION: SWBF2 0x00624550
inline const char* PowerupStation::GetDerivedRttiName() const
{
    return rttiNamePowerupStation;
}
