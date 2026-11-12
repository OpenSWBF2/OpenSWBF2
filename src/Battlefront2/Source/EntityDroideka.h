#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Aimer.h"
#include "Battlefront2Lod.h"
#include "CollisionModel.h"
#include "Controllable.h"
#include "EnergyBar.h"
#include "GameObject.h"
#include "GameModel.h"
#include "GameSound.h"
#include "PlayableEntitySounds.h"
#include "Trackable.h"
#include "VehicleEngine.h"
#include "Weapon.h"
#include "PebbleFL/Common/PblAngle.h"
#include "RedEngineFL/Graphics/RedPose.h"
#include "RedEngineFL/Graphics/RedModel.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrSkeleton.h"

class MemoryPool;
class Aimer;
class Entity;
class GameObject;
class PlayableEntitySounds;
class ExplosionClass;
class FLEffectClass;
class GameModel;
class RedAnimation;
class ZephyrAnim;

// VTABLE: SWBF2 0x0079aeb4
class EntityDroidekaClass : public GameObjectClass,
                            public ControllableClass,
                            public VehicleEngineClass,
                            public PlayableEntitySoundsClass
{
public:

    static PblHash rttiHashEntityDroidekaClass;
    static const char rttiNameEntityDroidekaClass[];

    // FUNCTION: SWBF2 0x004294b0
    static unsigned int GetRtti() { return rttiHashEntityDroidekaClass; }

    EntityDroidekaClass(unsigned int uHash);

    EntityDroidekaClass(EntityDroidekaClass* pSrc, unsigned int uHash);
    virtual ~EntityDroidekaClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void PostReadSetup();
    virtual GameObjectClass::VehicleType GetVehicleType() const;
    virtual const ControllableClass* GetControllableClass() const;
    // FUNCTION: SWBF2 0x0049ea70
    virtual int GetWeaponCount() const
    {
        if (!mWeaponClass[0])
        {
            return 0;
        }
        return mWeaponClass[1] ? 2 : 1;
    }
    // FUNCTION: SWBF2 0x0049ea90
    virtual const WeaponClass* GetWeaponClass(int iIndex) const
    {
        return mWeaponClass[iIndex];
    }
    virtual bool InitAnimations(const char* pszBank);

protected:

    void InitAnimation(int iIndex, const char* pszName);

public:

    RedLodData mLodData;
    GameModel* mUprightLowRes;
    GameModel* mBallLowRes;
    RedAnimation* mAnimObj;
    int mAnimationCount;
    ZephyrAnim** mAnimations;
    float* mTimeTotals;
    float* mAnimSpeeds;
    float* mAnimRates;
    float mAcceleration;
    float mMaxSpeed;
    float mMaxTurnSpeed;
    float mMaxYawSpeed;
    float mMaxPitchSpeed;
    float mUnrollTime;
    float mRollTime;
    float mMouseMaxTurnSpeed;
    float mMouseMaxYawSpeed;
    float mMouseMaxPitchSpeed;
    float mMouseTurnDecay;
    float mMinYaw;
    float mMaxYaw;
    float mMinPitch;
    float mMaxPitch;
    float mCollisionThreshold[3];
    float mCollisionScale[3];
    float mCollisionInflict;
    AimerClass mAimerClass[4];
    WeaponClass* mWeaponClass[4];
    WeaponAmmoDesc mWeaponAmmo[kMaxWeapons];
    int mWeaponChannel[kMaxWeapons];
    int mWeaponCount;
    unsigned int mStompEffect;
    GameObjectClass::VehicleType mVehicleType;
    float mNoiseFactor;
    float mAnimRateMove;
    float mAnimRateTurn;
    float mTurnThreshold;
    float mMaxTerrainAngle;
    CameraTrackSetting mCameraSettings[3];
    ExplosionClass* mExplosion;
    int mNoDeathExplosions;
    unsigned int mDeathDustClassHash;
    FLEffectClass* mDeathDustEffect;
    float mDeathDustDelay;
    PblVector3 mDeathDustOffset;
    float mDeathShakeDelay;
    float mDeathShakeForce;
    float mDeathShakeDuration;
    float mDeathShakeRadius;
    bool mRenderPilotFlag;
    unsigned char mNumFeet;
    int mFootPrimitives[3];
    GameSound mFootstepSounds[3];
    GameSound mHydraulicSound;
    float mHydraulicSoundHeight;
    unsigned int mAttachHash;
    PblVector3 mAttachOffset;
    float mWaterDamageInterval;
    float mWaterDamageAmount;
    float mBallRadius;
    float mBallMoveSpeed;
    float mBallStoppedTurnSpeed;
    float mBallTurnSpeed;
    float mBallSprintTurnSpeed;
    float mBallSlippage;
    float mBallMaxLean;
    float mBallAcceleration;
    float mBallRollingFriction;
    float mMaxBallAngle;
    float mSprintTimeForBowling;
    float mBowlingPush;
    float mBallSprintSpeedFactor;
    float mBallJumpVerticalSpeed;
    float mBallJumpForwardSpeedFactor;
    EnergyBarClass mEnergyBarClass;
    float mEnergyRestore;
    float mEnergyRestoreIdle;
    float mEnergyDrainSprint;
    float mEnergyCostJump;
    float mEnergyCostBowling;
    int mSpineIdx;
    int mJointIdx[3];
    int mArmJointIdx[2];
    CollisionModel mCollisionBall;
    unsigned int mWakeEffect;
    GameSound mSoundShield;
    GameSound mSoundShieldWearOff;
    GameSound mSoundShieldOff;
    GameSound mSoundJump;
    GameSound mSoundLand;
    int mFirstPersonModelHandle;
    float mUprightWaterDamageHeight;
    float mBallWaterDamageHeight;
    unsigned int mHealthGainEffect;
    unsigned int mAmmoGainEffect;
    unsigned int mEnergyGainEffect;
    unsigned int mBuffHealthEffect;
    unsigned int mBuffOffenseEffect;
    unsigned int mBuffDefenseEffect;
    unsigned int mDebuffEffect;
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

 
 
 

class CameraTrackSetting;
class CollisionResult;
class FoleyFX;
class EnergyBar;
class FLEffectObject;
class NetObjState;
class NetPktGroup;
class Restrictor;

// VTABLE: SWBF2 0x0079add4 CollisionObject
// VTABLE: SWBF2 0x0079ae40 Damageable
// VTABLE: SWBF2 0x0079ae6c Trackable
// VTABLE: SWBF2 0x0079af04 PblHandled
// VTABLE: SWBF2 0x0079af0c EntityEx
// VTABLE: SWBF2 0x0079b044 RedSceneObject
// VTABLE: SWBF2 0x0079b0ac Thread
// VTABLE: SWBF2 0x0079b17c
class EntityDroideka : public GameObject,
                       public Controllable,
                       public VehicleEngine,
                       public PlayableEntitySounds
{
public:
    enum eState
    {
        Idle = 0,
        TurnLeft = 1,
        TurnRight = 2,
        Dying = 3,
        Dead = 4,
        IdleToLeft = 5,
        IdleToRight = 6,
        LeftToRight = 7,
        RightToLeft = 8,
        LeftToIdle = 9,
        RightToIdle = 10,
        IdleToRoll = 11,
        Rolling = 12,
        RollToIdle = 13,
        StateCount = 14
    };

    enum eDirection
    {
        None = 0,
        Forward = 1,
        Backward = 2
    };

    struct StateTableEntry
    {
        eState next[8];
    };

    // VTABLE: SWBF2 0x0079ae28
    class PostCollision : public Thread
    {
    public:

        PostCollision(EntityDroideka* pOwner) : mOwner(pOwner) {}
        // SYNTHETIC: SWBF2 0x0049e9f0
        // EntityDroideka::PostCollision::~PostCollision
        virtual bool Update(float fDeltaTime);

        EntityDroideka* mOwner;
    };

    EntityDroideka(EntityDroidekaClass* pClass, const EntityDesc& desc);

    virtual ~EntityDroideka();

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x0049e3d0
    // EntityDroideka::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void Init();
    virtual const Entity* GetEntity() const;
    virtual Entity* GetEntity();
    virtual const GameObject* GetGameObject() const;
    virtual GameObject* GetGameObject();
    // FUNCTION: SWBF2 0x0049e6f0
    virtual const PblVector3& GetVelocity() const
    {
        return mVelocity;
    }
    // FUNCTION: SWBF2 0x0049e700
    virtual void SetVelocity(const PblVector3& vVelocity)
    {
        mVelocity = vVelocity;
    }
    virtual float GetMaxSpeed() const;
    virtual float GetMaxStrafe() const;
    virtual const Controllable* GetControllable() const;
    virtual Controllable* GetControllable();
    virtual const PlayableEntitySounds* GetPlayableEntitySounds() const;
    virtual PlayableEntitySounds* GetPlayableEntitySounds();
    virtual const PblVector3& GetAttachOffset() const;
    virtual void ActivatePhysics();
    virtual void DeactivatePhysics();
    // FUNCTION: SWBF2 0x0049e800
    virtual GameObjectType GetGameObjectType()
    {
        return (GameObjectType)(GO_VEHICLE | GO_WALKER);
    }
    virtual EntityType GetEntityType() const;
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual void AddSends(void (*pfnSend)(GameObject*, int), int iArg);
    virtual const PblMatrix& GetSmoothedMatrix() const;
    virtual void WriteLateState(NetPktGroup& group);
    virtual void ReadLateState(NetPktGroup& group);

    virtual bool PostCollisionUpdate(float fDeltaTime);

protected:

    virtual void UpdatePilot();
    virtual void UpdatePlayer(float fDeltaTime);
    virtual void UpdateState(float f0, float f1, float f2, float f3);
    virtual void UpdateStateRolling(float f0, float f1, float f2);
    virtual void NextState(int iState);

public:

    // FUNCTION: SWBF2 0x0049e820
    virtual void SetPosition(const PblVector3& vPos)
    {
        mMatrix.SetTranslation(vPos);
        PblVector3 center;
        D3DXVec3TransformCoord(&center, &mModel->_mModel->_BoundingSphere.GetCenter(),
                               reinterpret_cast<const D3DXMATRIX*>(&mMatrix));
        SetCollisionCenter(center);
        RecalculateAABB();
    }

    // FUNCTION: SWBF2 0x0049e8c0
    virtual void SetMatrix(const PblMatrix& matrix)
    {
        mMatrix = matrix;
        PblVector3 center;
        D3DXVec3TransformCoord(&center, &mModel->_mModel->_BoundingSphere.GetCenter(),
                               reinterpret_cast<const D3DXMATRIX*>(&mMatrix));
        SetCollisionCenter(center);
        RecalculateAABB();
    }
    virtual float GetSmallestCollisionDim() const;
    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);
    virtual void SetTerrainHeight(int iArg, float fHeight);
    virtual bool CollideTrisWithAllPrimitives() const;

    virtual const PblMatrix& GetMatrix() const;
    // FUNCTION: SWBF2 0x0049e690
    virtual const RedPose* GetPose() const
    {
        return &mPose;
    }

    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);

    virtual void Kill();
    virtual bool ApplyDamage(const DamageDesc& damage, const PblVector3& vDir,
                             const PblVector3& vPos, int iBodyPart, unsigned int uFlags);
    virtual float GetDamageMultiplier(int iBodyId) const;
    virtual void HealthChangeCallback(bool bArg, float fArg);

    virtual bool Update(float fDeltaTime);
    virtual Trackable* GetTrackable();
    virtual float GetSoundRadius() const;
    virtual float GetAimTurnRate() const;
    virtual int GetWeaponCount() const;
    // FUNCTION: SWBF2 0x0049e790
    virtual int GetWeaponIndex(int iChannel) const
    {
        return mWeaponIndex[iChannel];
    }
    // FUNCTION: SWBF2 0x0049e7b0
    virtual Weapon* GetWeaponPtr(int iIndex) const
    {
        return mWeapon[iIndex];
    }
    // FUNCTION: SWBF2 0x0049e990
    virtual EnergyBar* GetEnergyBar()
    {
        return &mEnergyBar;
    }
    virtual void SetWeaponIndex(int iChannel, int iIndex);
    // FUNCTION: SWBF2 0x0049e7d0
    virtual const Aimer* GetAimer(int iIndex) const
    {
        if (mState == IdleToRoll || mState == Rolling || mState == RollToIdle)
        {
            return 0;
        }
        return mAimer[iIndex];
    }
    virtual ControllableClass* GetControllableClass();
    virtual const ControllableClass* GetControllableClass() const;
    virtual void SetupController();
    virtual PilotDeathType GetPilotDeathType();
    virtual float GetOffenseMultiplier() const;
    virtual bool UpdateIndirect(float fDeltaTime);

    virtual void SetupCameraTrackMatrixHUDTargeting(PblMatrix& rMatrix);
    virtual void SetupCameraTrackMatrix(PblMatrix& rMatrix, float fDeltaTime);
    virtual void SetupCameraZoomFactor(float& rZoom);
    virtual void SetupCameraFOV(float& rFOV);
    virtual void SetupInterpolatedCameraTrackMatrix(PblMatrix& rMatrix);
    virtual CameraTrackSetting* GetCameraTrackSettings();
    virtual bool RenderTrackable(unsigned int uView) const;
    virtual bool IsFirstPersonAvailablePS2() const;
    virtual bool IsForcedThirdPerson() const;

protected:

    virtual bool Jump();
    virtual bool Sprint();
    virtual bool EndSprint();

    bool CanUnroll();

    void AdjustRotation(float fAngle);
    void BuildWorldMatrices();
    void SetState(int iState);

    void Die();
    float GetTurnState();

    float GetStateLimit();
    void SetFrameState(float fState);
    void ResetTurnState();

public:

    void DoFootImpactEffects();
    void UpdateBuffTimers(float fDeltaTime);

    void TurnOffEffects();
    FoleyFX* GetFoleyFX() const;

    // FUNCTION: SWBF2 0x00524070
    bool IsRollingOrTransition() const
    {
        return mState == Rolling || mState == IdleToRoll || mState == RollToIdle;
    }

    bool Smooth(float fDeltaTime) const;

    void Move(const PblMatrix& matrix);

    int GetJoystickIndex();

    void TriggerBuffHealthEffect();
    void TriggerBuffOffenseEffect();
    void TriggerBuffDefenseEffect();
    void TriggerDebuffEffect();

    void BuffOffense(float fTimer, float fMult);
    void BuffDefense(float fTimer, float fMult);
    void BuffHealth(float fTimer, float fRate);
    void DebuffDamage(float fTimer, float fRate, const DamageOwner& owner);
    void TriggerAmmoGainEffect();

    void TriggerHealthGainEffect();
    void TriggerEnergyGainEffect();

    // FUNCTION: SWBF2 0x0049e3e0
    void MoveForward(float fDistance)
    {
        PblVector3 vPos = mMatrix.GetTranslation();
        if (mTurnOffset.mCos < 1.0f)
        {
            float fCos = mTurnOffset.mCos * mTurnOffset.mCos - mTurnOffset.mSin * mTurnOffset.mSin;
            float fSin = mTurnOffset.mCos * 2.0f * mTurnOffset.mSin;
            if (mState != Rolling)
            {
                float f4Cos2 = fCos * fCos * 4.0f;
                fCos = fCos * (f4Cos2 - 3.0f);
                fSin = (f4Cos2 - 1.0f) * fSin;
            }
    
            PblMatrix kTurn(PblVector3(fCos, 0.0f, -fSin), PblVector3(0.0f, 1.0f, 0.0f),
                            PblVector3(fSin, 0.0f, fCos), PblVector3(0.0f, 0.0f, 0.0f));
            kTurn *= mMatrix;
    
            PblVector3 vMove(kTurn.m[2][0] * fDistance, kTurn.m[2][1] * fDistance,
                             kTurn.m[2][2] * fDistance);
            vPos.x += vMove.x;
            vPos.y += vMove.y;
            vPos.z += vMove.z;
        }
        else
        {
            PblVector3 vMove(mMatrix.m[2][0] * fDistance, mMatrix.m[2][1] * fDistance,
                             mMatrix.m[2][2] * fDistance);
            vPos.x += vMove.x;
            vPos.y += vMove.y;
            vPos.z += vMove.z;
        }
        SetPosition(vPos);
    }
    // FUNCTION: SWBF2 0x0049e5d0
    void RotateY(const PblAngle& rAngle)
    {
        PblMatrix kRot;
        kRot.m[1][0] = 0.0f;
        kRot.m[1][1] = 1.0f;
        kRot.m[1][2] = 0.0f;
        kRot.m[1][3] = 0.0f;
        kRot.m[0][0] = rAngle.mCos;
        kRot.m[0][1] = 0.0f;
        kRot.m[0][2] = -rAngle.mSin;
        kRot.m[0][3] = 0.0f;
        kRot.m[2][0] = rAngle.mSin;
        kRot.m[2][1] = 0.0f;
        kRot.m[2][2] = rAngle.mCos;
        kRot.m[2][3] = 0.0f;
        kRot.m[3][0] = 0.0f;
        kRot.m[3][1] = 0.0f;
        kRot.m[3][2] = 0.0f;
        kRot.m[3][3] = 1.0f;
    
        PblMatrix kOut;
        kOut = kRot * mMatrix;
        mMatrix = kOut;
    }
    float UpdateMouseDecay(float fDeltaTime, float fInput);
    void UpdateWalkingGroundNormal();
    int GetFirstPersonIndex();

    static PblHash rttiHashEntityDroideka;
    static MemoryPool sMemoryPool;
    static const char rttiNameEntityDroideka[];
    static StateTableEntry sStateTable[];

    bool mCallUpdateWalkingGroundNormal;
    PblVector3 mVelocity;
    RedAnimation* mAnimObj;
    EntityDroidekaClass* mClass;
    GameModel* mUprightLowRes;
    GameModel* mBallLowRes;
    float mPitchValue;
    Aimer* mAimer[4];
    Weapon* mWeapon[kMaxWeapons];
    int mWeaponIndex[kNumWeaponChannels];
    ZephyrSkeleton<32> mZephyrSkeleton;
    ZephyrPoseDyn<32> mZephyrPoseDyn;
    float mTime;
    RedPose mPose;
    float mLastFootHeight[3];
    unsigned int mFootState;
    float mDeathExplosionTime;
    float mDeathTime;
    int mState;
    eDirection mDirection;
    PblAngle mTurnOffset;
    PblAngle mPitchOffset;
    PblVector3 mPivotOffset;
    PblVector3 mAILastCollisionNormal;
    char mAILastCollisionDirection : 2;
    bool mFootprintLeft : 1;
    bool mIsDead : 1;
    bool mDoneDustCloud : 1;
    bool m_bSprint : 1;
    bool m_bIsGrounded : 1;
    PostCollision mPostCollision;
    float mWaterDamageTimer;
    float mSpeedPrev;
    PblAngle mBallOrientation;
    float mBallAngularVelocity;
    PblAngle mBallLean;
    PblVector3 mTranslationOffset;
    PblVector3 mGroundNormal;
    float mStompTime;
    float mFallTimer;

    EnergyBar mEnergyBar;
    PblHandle<FLEffectObject> mWakeEmitter;
    float mTurnState;
    float mBuffOffenseTimer;
    float mBuffOffenseMult;
    float mBuffDefenseTimer;
    float mBuffDefenseMult;
    float mBuffHealthTimer;
    float mBuffHealthRate;
    float mDebuffDamageTimer;
    float mDebuffDamageRate;
    DamageOwner mDebuffDamageOwner;
    unsigned char mCanChangeClass : 1;
    unsigned char mDoNotPlayCollisionEffect : 4;
    PblHandle<FLEffectObject> mHealthGainEffectHandle;
    PblHandle<FLEffectObject> mAmmoGainEffectHandle;
    PblHandle<FLEffectObject> mEnergyGainEffectHandle;
    PblHandle<FLEffectObject> mBuffHealthEffectHandle;
    PblHandle<FLEffectObject> mBuffOffenseEffectHandle;
    PblHandle<FLEffectObject> mBuffDefenseEffectHandle;
    PblHandle<FLEffectObject> mDebuffEffectHandle;
};
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x0049e370
inline bool EntityDroideka::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityDroideka == uRtti)
    {
        return true;
    }
    return GameObject::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0049e3b0
inline unsigned int EntityDroideka::GetDerivedRtti() const
{
    return rttiHashEntityDroideka;
}

// FUNCTION: SWBF2 0x0049e3c0
inline const char* EntityDroideka::GetDerivedRttiName() const
{
    return rttiNameEntityDroideka;
}

// FUNCTION: SWBF2 0x0049ea00
inline bool EntityDroidekaClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityDroidekaClass == uRtti)
    {
        return true;
    }
    return GameObjectClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0049ea40
inline unsigned int EntityDroidekaClass::GetDerivedRtti() const
{
    return rttiHashEntityDroidekaClass;
}

// FUNCTION: SWBF2 0x0049ea50
inline const char* EntityDroidekaClass::GetDerivedRttiName() const
{
    return rttiNameEntityDroidekaClass;
}

// FUNCTION: SWBF2 0x0049ea60
inline GameObjectClass::VehicleType EntityDroidekaClass::GetVehicleType() const
{
    return mVehicleType;
}



