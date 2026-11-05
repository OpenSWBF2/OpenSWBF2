#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>
#include "WalkerAnimation.h"

#include "Aimer.h"
#include "Controllable.h"
#include "EnergyBar.h"
#include "GameObject.h"
#include "GameModel.h"
#include "RedEngineFL/Graphics/RedModel.h"
#include "GameLoop.h"
#include "GameSound.h"
#include "Trackable.h"
#include "VehicleEngine.h"
#include "VehicleMusic.h"
#include "Weapon.h"

class MemoryPool;
class ExplosionClass;
class FLEffectClass;
class MountedTurretClass;
class PassengerSlotClass;
class RedAnimation;
struct WalkerLegPairInfo;
class ZephyrSkeletonShared;

#include "PebbleFL/Common/PblAngle.h"
#include "RedEngineFL/Graphics/RedPose.h"
#include "Zephyr/Source/ZephyrPoseStatic.h"
#include "Zephyr/Source/ZephyrSkeleton.h"

class CameraTrackSetting;
class CollisionResult;
class EnergyBar;
class Entity;
class FoleyFX;
class MountedTurret;
class NetObjState;
class NetPktGroup;
class PassengerSlot;
class RedStencilShadow;
class Restrictor;
class WalkerLegPair;
class Weapon;
class DamageDesc;
class EntityWalkerClass;
class PblAngle;

// VTABLE: SWBF2 0x0079db50 EntityEx
// VTABLE: SWBF2 0x0079ddf4 CollisionObject
// VTABLE: SWBF2 0x0079dae8 RedSceneObject
// VTABLE: SWBF2 0x0079dce4 Damageable
// VTABLE: SWBF2 0x0079dd1c PblHandled
// VTABLE: SWBF2 0x0079dd24 Thread
// VTABLE: SWBF2 0x0079daa0 Trackable
class EntityWalker : public GameObject,
                     public Controllable,
                     public VehicleMusic,
                     public VehicleEngine
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
        StateCount = 11
    };

    enum eDirection
    {
        None = 0,
        Forward = 1,
        Backward = 2
    };

    enum eTransition
    {
        ToIdle = 0,
        ToIdleBackwards = 1,
        ToForward = 2,
        ToBackward = 3,
        ToLeft = 4,
        ToRight = 5,
        ToDying = 6,
        TransitionCount = 7
    };

    struct StateTableEntry
    {
        eState next[TransitionCount];
    };

    // VTABLE: SWBF2 0x0079da88
    class PostCollision : public Thread
    {
    public:

        PostCollision(EntityWalker* pOwner) : mOwner(pOwner) {}
        // SYNTHETIC: SWBF2 0x004ff650
        // EntityWalker::PostCollision::~PostCollision
        virtual bool Update(float fDeltaTime);

        EntityWalker* mOwner;
    };

    EntityWalker(EntityWalkerClass* pClass, const EntityDesc& desc);
    virtual ~EntityWalker();

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x004ff500
    // EntityWalker::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void Init();
    virtual const Entity* GetEntity() const;
    virtual Entity* GetEntity();
    virtual const GameObject* GetGameObject() const;
    virtual GameObject* GetGameObject();
    // FUNCTION: SWBF2 0x0047f9a0
    virtual const PblVector3& GetVelocity() const
    {
        return mVelocity;
    }
    // FUNCTION: SWBF2 0x0047f9b0
    virtual void SetVelocity(const PblVector3& vVelocity)
    {
        mVelocity = vVelocity;
    }
    virtual float GetMaxSpeed() const;
    // FUNCTION: SWBF2 0x0047fb20
    virtual float GetMaxStrafe() const
    {
        return 0.0f;
    }
    virtual const Controllable* GetControllable() const;
    virtual Controllable* GetControllable();
    virtual bool ShouldShowReticule() const;
    virtual int GetReservableSlots(ReserveType eType) const;
    virtual const PblVector3& GetAttachOffset() const;
    virtual void ActivatePhysics();
    virtual void DeactivatePhysics();
    // FUNCTION: SWBF2 0x0047fbd0
    virtual GameObjectType GetGameObjectType()
    {
        return (GameObjectType)(GO_VEHICLE | GO_WALKER);
    }
    // FUNCTION: SWBF2 0x0047fbe0
    virtual void ReserveForPlayer(float fTime)
    {
        mReserveForPlayer = GameLoop::GetMissionTime() + fTime;
    }
    // FUNCTION: SWBF2 0x0047fc00
    virtual bool IsReservedForPlayer() const
    {
        return mReserveForPlayer > GameLoop::GetMissionTime();
    }
    virtual EntityType GetEntityType() const;
    virtual ExplosionClass* GetExplosionClass();
    // FUNCTION: SWBF2 0x0047fc40
    virtual float GetEjectTimer() const
    {
        return mEjectTimer;
    }
    // FUNCTION: SWBF2 0x0047fc20
    virtual void SetEjectTimer(float fTime)
    {
        mEjectTimer = fTime;
    }
    // FUNCTION: SWBF2 0x0047fc50
    virtual void ResetTimeSinceLastHack()
    {
        mTimeSinceLastHack = 0.0f;
    }
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual void AddSends(void (*pfnSend)(GameObject*, int), int iArg);
    virtual const PblMatrix& GetSmoothedMatrix() const;
    virtual void WriteLateState(NetPktGroup& group);
    virtual void ReadLateState(NetPktGroup& group);
    virtual bool PostCollisionUpdate(float fDeltaTime);

protected:
    virtual void UpdatePilot();
    virtual void UpdatePlayer();
    virtual void UpdateState(float fDeltaTime, float fMove, float fStrafe, float& rTurnSpeed,
                             float fTurn);
    virtual void ApplyBlend();
    virtual void CorrectJoints();
    virtual void BuildWorldMatrices();
    virtual void RestoreJoints();
    virtual void UpdateDone();

public:

    // FUNCTION: SWBF2 0x0047f9d0
    virtual void SetPosition(const PblVector3& vPosition)
    {
        mMatrix.SetTranslation(vPosition);
        PblVector3 center;
        D3DXVec3TransformCoord(&center, &mModel->_mModel->_BoundingSphere.GetCenter(),
                               reinterpret_cast<const D3DXMATRIX*>(&mMatrix));
        SetCollisionCenter(center);
        RecalculateAABB();
    }

    // FUNCTION: SWBF2 0x0047fa70
    virtual void SetMatrix(const PblMatrix& matrix)
    {
        mMatrix = matrix;
        PblVector3 center;
        D3DXVec3TransformCoord(&center, &mModel->_mModel->_BoundingSphere.GetCenter(),
                               reinterpret_cast<const D3DXMATRIX*>(&mMatrix));
        SetCollisionCenter(center);
        RecalculateAABB();
    }
    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);
    virtual void SetTerrainHeight(int iIndex, float fHeight);

    // FUNCTION: SWBF2 0x0047f990
    virtual const PblMatrix& GetMatrix() const
    {
        return mMatrix;
    }
    // FUNCTION: SWBF2 0x0047f940
    virtual const RedPose* GetPose() const
    {
        return &mPose;
    }

    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);

    virtual void Kill();
    virtual bool ApplyDamage(const DamageDesc& damage, const PblVector3& vDir,
                             const PblVector3& vPos, int iBodyPart,
                             unsigned int uFlags);

    virtual bool Update(float fDeltaTime);
    // FUNCTION: SWBF2 0x0047fb30
    virtual Trackable* GetTrackable()
    {
        return static_cast<Trackable*>(this);
    }
    virtual float GetSoundRadius() const;
    virtual float GetAimTurnRate() const;
    virtual int GetWeaponCount() const;
    // FUNCTION: SWBF2 0x0047fb70
    virtual int GetWeaponIndex(int iChannel) const
    {
        return mWeaponIndex[iChannel];
    }
    // FUNCTION: SWBF2 0x0047fb90
    virtual Weapon* GetWeaponPtr(int iIndex) const
    {
        return mWeapon[iIndex];
    }
    // FUNCTION: SWBF2 0x0047fc60
    virtual EnergyBar* GetEnergyBar()
    {
        return &mEnergyBar;
    }
    virtual void SetWeaponIndex(int iChannel, int iIndex);
    // FUNCTION: SWBF2 0x0047fbb0
    virtual const Aimer* GetAimer(int iIndex) const
    {
        return mAimer[iIndex];
    }
    virtual ControllableClass* GetControllableClass();
    virtual const ControllableClass* GetControllableClass() const;
    virtual void SetupController();
    virtual PilotDeathType GetPilotDeathType();

protected:
    virtual bool Jump();

public:
    virtual bool UpdateIndirect(float fDeltaTime);

    virtual void SetupCameraTrackMatrix(PblMatrix& rMatrix, float fDeltaTime);
    virtual void SetupCameraZoomFactor(float& rZoom);
    virtual void SetupCameraFOV(float& rFOV);
    virtual void SetupInterpolatedCameraTrackMatrix(PblMatrix& rMatrix);
    virtual CameraTrackSetting* GetCameraTrackSettings();
    virtual bool RenderTrackable(unsigned int uView) const;

protected:
    // FUNCTION: SWBF2 0x0047fc70
    virtual bool IsForcedThirdPerson() const
    {
        return true;
    }

public:

    void RotateY(const PblAngle& rAngle);
    void DoFootImpactEffects();
    int GetFirstPersonIndex();
    int GetJoystickIndex();
    void MoveForward(float fDistance);

    int IsTowCableCollisionValid(int iBodyId, int iLastBodyId, int iPrevBodyId);

    const PblVector3& GetTowCableCollisionOffset(int iBodyID);

protected:
    FoleyFX* GetFoleyFX() const;
    void Die();
    void NextState(int iInput);
    void UpdateOrientation(float fDeltaTime);

    void SetState(int iState);
    void ResetRotation();
    void AdjustRotation(PblAngle angle);
    void CheckTurn(float fMove, float fStrafe);

    void ApplyAttackDamage(const PblVector3& vPosition, float fRadius, float fDamage,
                           float fPush) const;

public:

    bool Smooth(float fDeltaTime) const;

    void Move(const PblMatrix& matrix);

public:
    static PblHash rttiHashEntityWalker;
    static MemoryPool sMemoryPool;
    static const char rttiNameEntityWalker[];
    static StateTableEntry sStateTable[];

    EntityWalkerClass* mClass;
    RedAnimation* mAnimObj;
    PblVector3 mVelocity;
    PassengerSlot* mPassengerSlots[4];
    MountedTurret* mTurret[8];
    int mNumTurrets;
    RedStencilShadow* mStencilShadow;
    float mLegAngle;
    float mYawValue;
    float mPitchValue;
    float mBodyPitch;
    float mBodyRoll;
    float mStopTime;
    Aimer* mAimer[4];
    Weapon* mWeapon[4];
    int mWeaponIndex[2];
    ZephyrSkeleton<64> mZephyrSkeleton;
    ZephyrPoseStatic<64> mZephyrPoseStatic;
    WalkerBlend* mBlend;
    PblBitVector<64> mMainMask;
    RedPose mPose;
    float mTotalTurn;
    float mReserveForPlayer;
    unsigned int mFootprintLeft : 1;
    unsigned int mIsDead : 1;
    unsigned int mIsStopped : 1;
    unsigned int mExplodedDeath : 1;
    unsigned int mExplodedFinal : 1;
    unsigned int mDoneDustCloud : 1;
    unsigned int m_bIsGrounded : 1;
    unsigned int m_bIsJumping : 1;
    unsigned int m_uiAttackControls : 2;
    unsigned int m_uiAttackAnim : 3;
    unsigned int z_uiReserved0 : 18;
    int mLegPairCount;
    WalkerLegPair* mLegPairs[3];
    float mLastFootHeight[6];
    float mMinFootHeight[6];
    unsigned int mFootState;
    unsigned int mFootWaterState;
    float mDeathExplosionTime;
    float mDeathTime;
    float m_fGroundedTimer;
    int mState;
    PblVector3 mPrevAimerPos;
    eDirection mDirection;
    EnergyBar mEnergyBar;
    PblAngle mTurnOffset;
    PblAngle mPitchOffset;
    PblAngle mRollOffset;
    PblVector3 mPivotOffset;
    PblVector3 mVelocityNoGrav;
    PostCollision mPostCollision;
    float mWaterDamageTimer;
    float mSpeedPrev;
    bool mBoost : 1;
    float mEjectTimer;
    float mTimeSinceLastHack;
    float mAttackControlTimer;
};
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x0079dc94
class EntityWalkerClass : public GameObjectClass,
                          public ControllableClass,
                          public VehicleMusicClass,
                          public VehicleEngineClass
{
public:
    virtual ~EntityWalkerClass();


    struct WalkerAttackAnim
    {
        unsigned int uiAnimName;
        unsigned int uiControls;
        PblVector3 vPosition;
        float fTime;
        float fRadius;
        float fDamage;
        float fPush;
    };

    EntityWalkerClass(unsigned int uHash);

    EntityWalkerClass(EntityWalkerClass* pBase, unsigned int uHash);

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void PostReadSetup();
    // FUNCTION: SWBF2 0x0047fcc0
    virtual int GetVehicleType() const
    {
        return mVehicleType;
    }
    // FUNCTION: SWBF2 0x0047fd30
    virtual const ControllableClass* GetControllableClass() const
    {
        return static_cast<const ControllableClass*>(this);
    }
    // FUNCTION: SWBF2 0x0047fcd0
    virtual int GetWeaponCount() const
    {
        return mWeaponCount;
    }
    // FUNCTION: SWBF2 0x0047fce0
    virtual const WeaponClass* GetWeaponClass(int iIndex) const
    {
        return mWeaponClass[iIndex];
    }
    // FUNCTION: SWBF2 0x0047fd00
    virtual float GetCollisionInflict() const
    {
        return mCollisionInflict;
    }
    // FUNCTION: SWBF2 0x0047fd10
    virtual float GetTimeRequiredToEject() const
    {
        return mTimeRequiredToEject;
    }
    // FUNCTION: SWBF2 0x0047fd20
    virtual float GetReboardTime() const
    {
        return mTimeTilReboard;
    }
    WalkerAnimation* GetAnimation(int i) { return mAnimations[i]; }

    virtual bool InitAnimations(const char* pszName);

    PblBitVector<64> HierarchyJointMask(const ZephyrSkeletonShared* pSkeleton, int iJoint);

protected:

    friend class EntityWalker;
    float GetAnimationBlendDuration(EntityWalker::eState eFrom, EntityWalker::eState eTo) const;

    int IsTowCableCollisionValid(int iBodyId, int iLastBodyId, int iPrevBodyId);
    const PblVector3& GetTowCableCollisionOffset(int iBodyID);

public:

    static PblHash rttiHashEntityWalkerClass;
    static const char rttiNameEntityWalkerClass[];

    RedAnimation* mAnimObj[3];
    int mNumAnimObjs;
    ZephyrSkeletonShared* m_pZephyrSkeletonShared;
    float mSkeletonRootScale;
    int mAnimationCount;
    WalkerAnimation** mAnimations;
    bool m_bAnimationBlending;
    unsigned char m_uiNumAttackAnims;
    WalkerAttackAnim m_aAttackAnims[4];
    int mVehicleType;
    float mAcceleration;
    float mMaxSpeed;
    float mBoostSpeed;
    float mJumpVerticalSpeed;
    float mJumpForwardSpeedBoost;
    float mJumpMinSpeed;
    float mMaxTurnSpeed;
    float mMaxYawSpeed;
    float mMinYaw;
    float mMaxYaw;
    float mMaxPitchSpeed;
    float mMinPitch;
    float mMaxPitch;
    float mStoppedTurnSpeed;
    float mForwardTurnSpeed;
    float mBoostTurnSpeed;
    float mTurnThreshold;
    float mMaxTerrainAngle;
    bool mOrientRoll;
    float mWidth;
    EnergyBarClass mEnergyBarClass;
    float mEnergyAutoRestore;
    float mEnergyBoostDrain;
    float mEnergyCostJump;
    float mCollisionThreshold[3];
    float mCollisionScale[3];
    float mCollisionInflict;
    AimerClass mAimerClass[4];
    WeaponClass* mWeaponClass[4];
    WeaponAmmoDesc mWeaponAmmo[4];
    int mWeaponChannel[4];
    int mWeaponCount;
    float mStompRumbleLight;
    float mStompRumbleHeavy;
    float mStompRumbleLightDuration;
    float mStompRumbleHeavyDuration;
    unsigned int mStompEffect;
    unsigned int mStompDecal;
    unsigned int mFootWaterSplashEffect;
    float mStompDecalSize;
    float mNoiseFactor;
    CameraTrackSetting mCameraSettings;
    PassengerSlotClass* mPassengerSlotClass;
    unsigned char mNumPassengerSlots;
    MountedTurretClass* mTurretClass[8];
    ExplosionClass* mExplosion;
    PblVector3 mExplosionOffset;
    ExplosionClass* mFinalExplosion;
    PblVector3 mFinalExplosionOffset;
    int mDeathAnimationExplosionCount;
    unsigned int mDeathAnimationExplosions[4];
    float mDeathAnimationExplosionTime;
    unsigned int mDeathDustClassHash;
    FLEffectClass* mDeathDustEffect;
    float mDeathDustDelay;
    PblVector3 mDeathDustOffset;
    float mDeathShakeDelay;
    float mDeathShakeForce;
    float mDeathShakeDuration;
    float mDeathShakeRadius;
    float mDeathFadeTimeStart;
    float mDeathFadeTimeEnd;
    float m_fThrustAngleToStrafe_Tan;
    float m_fThrustAngleToTurn_Tan;
    float m_fThrustAngleToStop_Tan;
    float m_fSteerTowardAimRate;
    bool mAIUseHoverPhysics : 1;
    bool mSteerAtYawLimit : 1;
    bool mSteerAtStrafeLimit : 1;
    bool mCorrectAimForSteering : 1;
    bool mReverseBackwardSteering : 1;
    bool mRenderPilotFlag : 1;
    bool mSmashParkedFlyers : 1;
    unsigned char mStompDetectionType;
    unsigned char mNumFeet;
    int mFootPrimitives[6];
    float mStompThreshold;
    float mLegRayHitLength;
    int mLegPairCount;
    WalkerLegPairInfo* mLegPairInfo;
    unsigned int mAttachHash;
    PblVector3 mAttachOffset;
    float mWaterDamageInterval;
    float mWaterDamageAmount;
    GameSound mHydraulicSound;
    float mHydraulicSoundHeight;
    GameSound mHydraulicLowerSound;

    float mHydrualicLowerHeight;
    GameSound mFootstepSounds[6];
    float mFPWalkScale;
    unsigned int mFPPoseHash;
    int mTowCableCollisionBodyID[12];
    PblVector3 mTowCableCollisionOffset[12];
    unsigned char mNumTowCableCollisions;
    int mFirstPersonModelHandle;
    int mIgnorableCollisions;
    float mDropShadowSize;
    float mTimeRequiredToEject;
    float mEjectResist;
    float mTimeTilReboard;
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

 
 
 

// FUNCTION: SWBF2 0x0047fc80
inline bool EntityWalkerClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityWalkerClass == uRtti)
    {
        return true;
    }
    return GameObjectClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004ff4e0
inline unsigned int EntityWalker::GetDerivedRtti() const
{
    return rttiHashEntityWalker;
}

// FUNCTION: SWBF2 0x004ff4f0
inline const char* EntityWalker::GetDerivedRttiName() const
{
    return rttiNameEntityWalker;
}

// FUNCTION: SWBF2 0x004ff660
inline unsigned int EntityWalkerClass::GetDerivedRtti() const
{
    return rttiHashEntityWalkerClass;
}

// FUNCTION: SWBF2 0x004ff670
inline const char* EntityWalkerClass::GetDerivedRttiName() const
{
    return rttiNameEntityWalkerClass;
}

// FUNCTION: SWBF2 0x0047f900
inline bool EntityWalker::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityWalker == uRtti)
    {
        return true;
    }
    return GameObject::IsRtti(uRtti);
}


