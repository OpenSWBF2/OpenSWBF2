#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Aimer.h"
#include "Character.h"
#include "CollisionModel.h"
#include "Controllable.h"
#include "EnergyBar.h"
#include "EntityPathFollower.h"
#include "GameObject.h"
#include "GameSound.h"
#include "Trackable.h"
#include "VehicleEngine.h"
#include "VehicleMusic.h"
#include "Weapon.h"
#include "GameLoop.h"
#include "RedEngineFL/Graphics/RedPose.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrSkeleton.h"

class ControllableDestination;
class MountedTurret;
class Obstacle;
class PassengerSlot;
class ExplosionClass;
class MountedTurretClass;
class PassengerSlotClass;
class RedAnimation;
class ZephyrAnim;

// VTABLE: SWBF2 0x0079b278
class EntityFlyerClass : public GameObjectClass,
                        public ControllableClass,
                        public VehicleMusicClass,
                        public VehicleEngineClass,
                        public EntityPathFollowerClass
{
public:
    static const int kMaxTurrets = 8;
    static const int kMaxContrailEffects = 12;
    static const int kMaxThrustEffects = 8;
    EntityFlyerClass(unsigned int uHash);
    // SYNTHETIC: SWBF2 0x004b6f10
    // EntityFlyerClass::`scalar deleting destructor'

    virtual ~EntityFlyerClass();

    EntityFlyerClass(EntityFlyerClass* pSrc, unsigned int uHash);

private:

    friend void test_sweep_entityflyer_animation();

    bool InitAnimations(const char* pszName);

public:

    float GetTurnRate() const { return mTurnRate; }

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    // FUNCTION: SWBF2 0x00477b00
    virtual GameObjectClass::VehicleType GetVehicleType() const
    {
        return mVehicleType;
    }
    // FUNCTION: SWBF2 0x00477b60
    virtual const ControllableClass* GetControllableClass() const
    {
        return this ? (const ControllableClass*)this : 0;
    }
    // FUNCTION: SWBF2 0x00477b10
    virtual int GetWeaponCount() const
    {
        return mWeaponCount;
    }
    // FUNCTION: SWBF2 0x00477b20
    virtual const WeaponClass* GetWeaponClass(int iIndex) const
    {
        return mWeaponClass[iIndex];
    }
    // FUNCTION: SWBF2 0x00477b40
    virtual float GetTimeRequiredToEject() const
    {
        return mTimeRequiredToEject;
    }
    // FUNCTION: SWBF2 0x00477b50
    virtual float GetReboardTime() const
    {
        return mTimeTilReboard;
    }

    static PblHash rttiHashEntityFlyerClass;
    static const char rttiNameEntityFlyerClass[];

    CollisionModel mFlyingCollisionModel;
    bool mFlyingCollisionInited;
    RedAnimation* mAnimObj;
    ZephyrAnim* mAnimTakeoff;
    ZephyrAnim* mAnimFins;
    float mAcceleration;
    float mBoostAcceleration;
    float mMinFlyerSpeed;
    float mMidFlyerSpeed;
    float mMaxFlyerSpeed;
    float mBoostFlyerSpeed;
    float mStrafeSpeed;
    float mPitchRate;
    float mTurnRate;
    float mPitchFilter;
    float mPitchFilterDecel;
    float mTurnFilter;
    float mTurnFilterDecel;
    float mPitchBuildupMultiplier;
    float mTurnBuildupMultiplier;
    float mThrustPitchAngle;
    float mStrafeRollAngle;
    float mBankAngle;
    float mBankFilter;
    float mLevelFilter;
    float mLevelFilterLanding;
    float mRollRate;
    float mRollRateAccel;
    float mTakeoffHeight;
    float mTakeoffTime;
    float mTakeoffSpeed;
    float mLandingTime;
    float mLandingSpeed;
    float mLandedHeight;
    float mCollisionThreshold[3];
    float mCollisionScale[3];
    float mCollisionInflict;
    float mCrashShakeStart;
    float mCrashShakeEnd;
    float mCrashShakeLength;
    float mMinCrashRollRate;
    float mMaxCrashRollRate;
    AimerClass mAimerClass[4];
    WeaponClass* mWeaponClass[4];
    WeaponAmmoDesc mWeaponAmmo[kMaxWeapons];
    int mWeaponChannel[kMaxWeapons];
    int mWeaponCount;
    GameObjectClass::VehicleType mVehicleType;
    float mBlurEffect;
    float mBlurStartV;
    float mFOVEffect3rd;
    PblVector3 mFOVEFFectMinCamOffset3rd;
    PblVector3 mFOVEFFectMaxCamOffset3rd;
    float mFOVEffect1st;
    EnergyBarClass mEnergyBarClass;
    float mEnergyAutoRestore;
    float mEnergyBoostDrain;
    float mEnergyTrickDrainSingleTap;
    float mEnergyTrickDrainDoubleTap;
    float mTrickRollSpeed;
    float mTrickFlipSpeed;
    float mTrickFlipCameraDetach;
    float mTrickSideRollCameraDetach;
    float mTrickSideRollStrafeSpeed;
    float mMomentumFilter;
    float mNoiseFactor;
    float mBarrierX;
    float mBarrierZ;
    float mBarrierOX;
    float mBarrierOZ;
    CameraTrackSetting mCameraSettings;
    PassengerSlotClass* mPassengerSlotClass;
    unsigned char mNumPassengerSlots;
    MountedTurretClass* mTurretClass[kMaxTurrets];
    ExplosionClass* mExplosionCritical;
    ExplosionClass* mExplosionDestruct;
    unsigned int mThrustEffectName[kMaxThrustEffects];
    unsigned int mThrustEffectHardPoint[kMaxThrustEffects];
    PblVector3 mThrustEffectAttachOffset[kMaxThrustEffects];
    float mThrustEffectMinScale[kMaxThrustEffects];
    float mThrustEffectMaxScale[kMaxThrustEffects];
    float mThrustEffectScaleStart[kMaxThrustEffects];
    unsigned int mContrailEffectName[kMaxContrailEffects];
    unsigned int mContrailEffectHardPoint[kMaxContrailEffects];
    PblVector3 mContrailEffectAttachOffset[kMaxContrailEffects];
    float mContrailEffectMinScale[kMaxContrailEffects];
    float mContrailEffectMaxScale[kMaxContrailEffects];
    float mContrailEffectMinSpeed[kMaxContrailEffects];
    float mContrailEffectMaxSpeed[12];
    int mCockpitModelHandle;
    float mFreeFlyTime;
    float mTrickTimeMid;
    float mTrickTimeRange;
    float mCircleATATFrequency;
    float mSquadronMaxAngle;
    float mSquadronSquaredDist;
    float mTimeRequiredToEject;
    float mEjectResist;
    float mTimeTilReboard;

    bool IsGuidedMissile() const { return mGuidedMissile; }
    bool mNeverCrashWhenUnpiloted : 1;
    bool mGuidedMissile : 1;
    float mGuidedMissileDamage;
    float mGuidedMissileDamageBoost;
    float mGuidedMissileHealthScale[8];
    float mGuidedMissileShieldScale;
    float mGuidedMissileDeathCamPause;
    float mGuidedMissileDeathCamPullbackSpeed;
    float mGuidedMissileDeathCamMinDistance;
    float mGuidedMissileBoostExplosionSpeedThreshold;
    GameSound mSoundTrick;
    GameSound mSoundFlip;
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

 
 
 
 

// FUNCTION: SWBF2 0x00477ac0
inline bool EntityFlyerClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityFlyerClass == uRtti)
    {
        return true;
    }
    return GameObjectClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004a9d90
inline unsigned int EntityFlyerClass::GetDerivedRtti() const
{
    return rttiHashEntityFlyerClass;
}

// FUNCTION: SWBF2 0x004a9da0
inline const char* EntityFlyerClass::GetDerivedRttiName() const
{
    return rttiNameEntityFlyerClass;
}

// VTABLE: SWBF2 0x0079b20c CollisionObject
// VTABLE: SWBF2 0x0079b2c4 PblHandled
// VTABLE: SWBF2 0x0079b2cc
// VTABLE: SWBF2 0x0079b2f0 Damageable
// VTABLE: SWBF2 0x0079b31c Trackable
// VTABLE: SWBF2 0x0079b364 RedSceneObject
// VTABLE: SWBF2 0x0079b3cc Thread
// VTABLE: SWBF2 0x0079b49c EntityEx
class EntityFlyer : public GameObject, public Controllable, public VehicleMusic,
                    public VehicleEngine, public EntityPathFollower
{
public:
    static const int kMaxTurrets = EntityFlyerClass::kMaxTurrets;
    static const int kMaxThrustEffects = EntityFlyerClass::kMaxThrustEffects;
    EntityFlyer(EntityFlyerClass* pClass, const EntityDesc& desc);
    virtual ~EntityFlyer();

    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static void* operator new(unsigned int uSize);
    static void operator delete(void* pBlock);

    virtual void Init();
    virtual GameObject* GetGameObject();

    virtual const GameObject* GetGameObject() const;
    virtual Entity* GetEntity();
    virtual const Entity* GetEntity() const;

    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);

    virtual float GetSoundRadius() const;
    virtual void ActivatePhysics();
    void InitAsLanded();
    bool ResetSquadronTime();

protected:
    void SendLandedEvent();

public:

    const ControllableClass* GetControllableClass() const;
    ControllableClass* GetControllableClass();
    int GetWeaponCount() const;

    float GetFlyerMaxSpeed() const;
    float GetFlyerMidSpeed() const;
    float GetFlyerMinSpeed() const;
    float GetMaxSpeed() const;
    int GetReservableSlots(ReserveType eType) const;

    // FUNCTION: SWBF2 0x00477710
    const PblVector3& GetVelocity() const
    {
        return mVelocity;
    }

    // FUNCTION: SWBF2 0x004778b0
    void SetVelocity(const PblVector3& v)
    {
        mVelocity = v;
    }

    // FUNCTION: SWBF2 0x004778f0
    const Controllable* GetControllable() const
    {
        return this ? (const Controllable*)this : 0;
    }

    // FUNCTION: SWBF2 0x00477900
    Controllable* GetControllable()
    {
        return this ? (Controllable*)this : 0;
    }

    // FUNCTION: SWBF2 0x00477980
    virtual GameObjectType GetGameObjectType() { return (GameObjectType)(GO_VEHICLE | GO_FLYER); }

    // FUNCTION: SWBF2 0x00496e10
    virtual EntityType GetEntityType() const { return ENT_FLYER; }
    virtual void AddSends(void (*pfnSend)(GameObject*, int), int iArg);

    // FUNCTION: SWBF2 0x00477a10
    void ReserveForPlayer(float fSeconds)
    {
        mReserveForPlayer = GameLoop::GetMissionTime() + fSeconds;
    }

    // FUNCTION: SWBF2 0x00477a30
    bool IsReservedForPlayer() const
    {
        return mReserveForPlayer > GameLoop::GetMissionTime();
    }

    // FUNCTION: SWBF2 0x00477a50
    void SetEjectTimer(float fSeconds)
    {
        if (!mProcedurallyAnimated)
        {
            mAIEng.ai.mEjectTimer = fSeconds;
        }
    }

    // FUNCTION: SWBF2 0x00477a70
    float GetEjectTimer() const
    {
        return mProcedurallyAnimated ? 0.0f : mAIEng.ai.mEjectTimer;
    }

    // FUNCTION: SWBF2 0x00477aa0
    void ResetTimeSinceLastHack()
    {
        if (!mProcedurallyAnimated)
        {
            mAIEng.ai.mTimeSinceLastHack = 0.0f;
        }
    }

    bool IsRtti(unsigned int uRtti) const;

    void InitAsFlying();

    void TakeOff();

    enum State { LANDED, TAKEOFF, FLYING, LANDING, CRASHING, CRASHED };
    enum Trick { FLIP_UP, FLIP_DOWN, FLIP_UP_RIGHT, FLIP_DOWN_RIGHT,
                 FLIP_UP_LEFT, FLIP_DOWN_LEFT, SIDE_ROLL, TRICK_MAX };
    State GetState() const { return mState; }
    const EntityFlyerClass* GetClass() const { return mClass; }

    void SetInLandingRegion(float fFactor)
    {
        mInLandingRegionFlag = true;
        mInLandingRegionFactor = fFactor;
    }

    // FUNCTION: SWBF2 0x00666770
    const PblHandle<GameObject> GetTowCableWalker() const { return mTowCableWalker; }

    void SetTowCableWalker(GameObject* pWalker);

    static PblHash rttiHashEntityFlyer;

    // FUNCTION: SWBF2 0x00438bc0
    static unsigned int GetRtti() { return rttiHashEntityFlyer; }
    static MemoryPool sMemoryPool;

    // VTABLE: SWBF2 0x0079b260
    class PostCollision : public Thread
    {
    public:

        PostCollision(EntityFlyer* pOwner) : mOwner(pOwner) {}

        virtual bool Update(float fDeltaTime);

        EntityFlyer* mOwner;
    };

    // FUNCTION: SWBF2 0x00477910
    int GetWeaponIndex(int iChannel) const
    {
        return mWeaponIndex[iChannel];
    }

    // FUNCTION: SWBF2 0x00477930
    Weapon* GetWeaponPtr(int iIndex) const
    {
        return mWeapon[iIndex];
    }

    // FUNCTION: SWBF2 0x00477950
    EnergyBar* GetEnergyBar()
    {
        return &mEnergyBar;
    }

    // FUNCTION: SWBF2 0x00477960
    const Aimer* GetAimer(int iIndex) const
    {
        return mAimer[iIndex];
    }

    // FUNCTION: SWBF2 0x00477990
    const RedPose* GetPose() const
    {
        return &mPose;
    }

    void SetPosition(const PblVector3& vPos);
    void SetMatrix(const PblMatrix& mMatrix);
    // FUNCTION: SWBF2 0x00477870
    bool GetFollowingPath() const
    {
        return mFollowingPath && mState == FLYING && IsAlive() &&
               mAIEng.ai.mFreeFlyTime <= 0.0f && !mInRoll && !mInFlip;
    }
    Trackable* GetTrackable();
    // FUNCTION: SWBF2 0x004779a0
    unsigned int GetAllegiance()
    {
        const Character* pCharacter = mCharacter;
        return pCharacter ? pCharacter->mTeamNumber : 0;
    }
    // FUNCTION: SWBF2 0x004779c0
    bool CanJoinSquadron() const
    {
        return !EntitySquadron::InSquadron() && GetFollowingPath();
    }

    virtual float GetMaxStrafe() const;
    virtual ExplosionClass* GetExplosionClass();
    virtual bool ShouldShowReticule() const;
    virtual void ClientKill();
    virtual void DeactivatePhysics();
    virtual PblVector3 GetTargetPointMissVector() const;
    virtual const PblMatrix& GetMatrix() const;
    virtual bool UpdateIndirect(float fDeltaTime);

    virtual bool Update(float fDeltaTime);
    virtual const EntitySquadronClass* GetEntitySquadronClass() const;
    virtual EntityPathFollower::BranchParams GetBranchParams();
    virtual bool PostCollisionUpdate(float fDeltaTime);
    virtual void Render(unsigned char renderMode, float lod, unsigned int flags);

    virtual const PblMatrix& GetSmoothedMatrix() const;
    bool Smooth(float fDeltaTime) const;

    void Move(const PblMatrix& matrix);

    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual void WriteLateState(NetPktGroup& group);
    virtual void ReadLateState(NetPktGroup& group);

    void DoTrick(Trick eTrick);

    void SideRoll();

    virtual void Kill();
    virtual void HealthChangeCallback(bool bArg, float fDelta);
    virtual bool RenderTrackable(unsigned int uView) const;
    virtual CameraTrackSetting* GetCameraTrackSettings();
    virtual void SetupCameraZoomFactor(float& rZoom);
    virtual void SetupCameraFOV(float& rFOV);

    virtual void SetupCameraTrackMatrix(PblMatrix& rMatrix, float fDeltaTime);
    virtual void SetupInterpolatedCameraTrackMatrix(PblMatrix& rMatrix);
    void RecalculateSpeed();
    void TurnOffContrailEffects();
    void UpdatePathFollower(float fDeltaTime, PblMatrix& rMatrix);
    void SideRollStop();
    void SideRollUpdate(float fDeltaTime);
    void RollUpdate(float fDeltaTime);
    void RollStop();

    void RollAdd(float fRoll);

    void FlipAdd(float fFlip);

    void FlipUpdate(float fDeltaTime, PblMatrix& rMatrix);
    void FlipStop();
    int GetFirstPersonIndex();
    int GetJoystickIndex();

    virtual bool ApplyDamage(const DamageDesc& damage, const PblVector3& vDir, const PblVector3& vPos,
                             int iBodyPart, unsigned int uFlags);

    void Land();

protected:
    void PlayDustEffect();
    void TurnOnThrustEffects();

    void TurnOffThrustEffects();
    void TargetReact(Controllable* pAttacker);
    void UpdateContrailEffects();

    bool TurnOnContrailEffect(int iIndex);

    virtual void SetupController();

    bool CalculateDest(float fDeltaTime, ControllableDestination* pDest, PblVector3* pPos,
                       float* pfSpeedScale, PblVector3* pDir);

    void RaiseHeight(const PblVector3& vPos, bool bTaper, PblVector3* pDest, float* pfSpeedScale,
                     float fTaperFactor);
    bool AtSlideDest() const;

    virtual float GetAimTurnRate() const;
    virtual EnergyMode GetEnergyMode() const;
    virtual void SetWeaponIndex(int iWeaponIndex, int iChannel);
    virtual bool IsForcedThirdPerson() const;

public:

    PblHandle<GameObject> mTowCableWalker;
    PblVector3 mVelocity;
    PblVector3 mOmega;
    float mSetSpeed, mSetStrafe, mSetStrafeVertical;
    State mState;
    float mFlightRatio, mBankPitchAngle, mBankPitchAngleDelta;
    float mBankRollAngle, mBankRollAngleDelta, mRoll, mRollTarget;
    float mFlip, mFlipTarget, mTrickCameraInterpDetach;
    float mTrickCameraInterpDir, mTrickCameraDetach;
    PblVector3 mSideRoll;
    Trick mLastTrickPerformed;
    float mPitchControllerPrevUpdate, mTurnControllerPrevUpdate;
    float mReserveForPlayer;
    bool mInRoll : 1, mInFlip : 1, mBoost : 1, mFollowingPath : 1;
    bool mSliding : 1, mInLandingRegionFlag : 1;
    bool mInLandingRegionFlag_ToolTip : 1;
    bool mFlyerAutoCorrectingToStayInBattlefield : 1;
    unsigned char _flightFlagPadding[3];
    float mGetSpeedSpeed, mInLandingRegionFactor, mLandedHeight;
    float mAutoLevelingAllowed, mWorldRoll, mCrashTimer;
    float mTrick, mTrickDoubleclick;
    EnergyBar mEnergyBar;
    PblVector3 mAmbientShift, mAmbientShiftTarget;
    float mAmbientShiftSpeedMax;
    PblVector3 mAmbientShiftSpeed;
    RedAnimation* mAnimObj;
    float mNextBarrierCheck;
    PblVector3 mSlidePos, mSlideBeginPos;
    EntityFlyerClass* mClass;
    PassengerSlot* mPassengerSlots[4];
    MountedTurret* mTurret[kMaxTurrets];
    char mNumTurrets;
    unsigned char _turretPadding[3];
    float mAnimationTime;
    Aimer* mAimer[4];
    Weapon* mWeapon[kMaxWeapons];
    int mWeaponIndex[kNumWeaponChannels];
    float mLastAltitude;
    ZephyrSkeleton<32> mZephyrSkeleton;
    ZephyrPoseDyn<32> mZephyrPoseDyn;
    RedPose mPose;
    Snd::SoundParameterized mSoundEngine;
    float mSpeedPrev, mTurbulance, mLastDistanceToTarget;
    struct AITimers {
        float mFreeFlyTime, mTrickTime, mTimeToNextTrick, mSquadronFormationTime;
        float mReactionThinkTime, mReactionTime, mEjectTimer, mTimeSinceLastHack;
    };
    struct ProceduralEngineParams {
        unsigned char velocity[12];
        struct PreviousPosition { float mPosition[3]; float mValid; } prevPos;
         
    };
    union AIEng { AITimers ai; ProceduralEngineParams eng; } mAIEng;
    Obstacle* mObstacle;
    float mTotalUpdateDt;
    bool mJustRendered;
    unsigned char _renderPadding[3];
    float mTrickEngineValue;
    GameSoundControllable mSoundTrick;
    PostCollision mPostCollision;
    PblHandle<ParticleEmitterObject> mThrustEffect[kMaxThrustEffects];
    PblHandle<ParticleEmitterObject> mContrailEffect[12];
};
