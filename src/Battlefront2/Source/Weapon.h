#pragma once

class PblFileChunk;

#include <stddef.h>

#include "AmmoCounter.h"
#include "EnergyBar.h"
#include "Factory.h"
#include "GameSound.h"
#include "Thread.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Graphics/RedColor.h"

class FLEffectClass;
class OrdnanceClass;
class RedModel;

struct WeaponAmmoDesc
{
    unsigned int uiAmmo : 24;
    unsigned int uiHeat : 5;
    unsigned int bFlagAmmoInfinite : 1;
    unsigned int bFlagShareHeat : 1;
    unsigned int bFlagShareAmmo : 1;
};

class Aimer;
class Controllable;
class GameObject;
class MemoryPool;
class NetPktGroup;
class Ordnance;
class ParticleEmitterObject;
class RedPose;
class Trigger;

struct OrdnanceDesc;

struct WeaponDesc
{
    Controllable* mOwner;
    Aimer* mAimer;
    Trigger* mTrigger;
    Trigger* mReload;
    int mNumClips;
    AmmoCounter* m_pAmmoCounter;
    EnergyBar* m_pEnergyBar;
};

// VTABLE: SWBF2 0x007b0170
class WeaponClass : public Factory<Weapon, WeaponClass, WeaponDesc>
{
public:

    enum Label
    {
        BLASTER = 0,
        LASER = 1,
        THROWN = 2,
        LAUNCHER = 3,
        MELEE = 4,
        SPECIAL = 5,
        NONE = 6
    };

    static void Read(PblFileChunk& chunk);

    enum FoleyFX
    {
        foleyFX_leftWalk = 0,
        foleyFX_rightWalk = 1,
        foleyFX_leftRun = 2,
        foleyFX_rightRun = 3,
        foleyFX_jump = 4,
        foleyFX_land = 5,
        foleyFX_roll = 6,
        foleyFX_prone = 7,
        foleyFX_squat = 8,
        foleyFX_stand = 9,
        foleyFX_max = 10
    };

    WeaponClass(unsigned int uHash);

    WeaponClass(WeaponClass* pBase, unsigned int uHash);
    virtual ~WeaponClass();
    virtual WeaponClass* Derive(unsigned int uHash);

    virtual bool IsRtti(unsigned int uRtti);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual unsigned int GetDerivedRtti();
    virtual const char* GetDerivedRttiName();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void PostReadInit();
    virtual bool PostLoadInit();

    void Render(const class PblMatrix& mMatrix, const class RedPose* pPose, const RedColor& color,
                unsigned int uiFlags, bool bArg) const;

    void RenderFlash(const PblVector3& vPos, const PblVector3& vDir, float fScale) const;
    virtual float GetHeatPerSalvo();
    virtual float GetShotsPerSalvo();
    virtual int GetBarrageMin();
    virtual int GetBarrageMax();

    void CreateMuzzleFlash(const PblMatrix& mFire) const;

    float GetBarrageDelay() const;

    static PblHash rttiHashWeaponClass;
    static char rttiNameWeaponClass[];

    static GameSound sWeaponChangeUnableSound;

    int mSoldierAnimationWeapon;
    PblVector3 mFirePointOffset;
    char mFilename[32];
    unsigned short* mLabel;
    unsigned int mNameHash;
    unsigned int mHUDNameHash;
    float mStrengthLimit;
    WeaponClass* mNextCharge;
    RedModel* mModel;
    RedModel* mHighResModel;
    unsigned int mIconTexture;
    unsigned int mReticuleTexture;
    unsigned int mScopeTexture;
    OrdnanceClass* mOrdnanceClass;
    float mRoundsPerShot;
    float mHeatPerShot;
    float mHeatRecoverRate;
    AmmoCounterClass mAmmoCounterClass;
    EnergyBarClass mEnergyBarClass;
    float mReloadTime;
    float mZoomMin;
    float mZoomMax;
    float mZoomRate;
    float mZoomTurnDivisorMin;
    float mZoomTurnDivisorMax;
    float mRecoilStrengthHeavy;
    float mRecoilStrengthLight;
    float mRecoilLengthLight;
    float mRecoilLengthHeavy;
    float mRecoilDelayLight;
    float mRecoilDelayHeavy;
    float mRecoilDecayLight;
    float mRecoilDecayHeavy;
    float mChargeRateLight;
    float mChargeRateHeavy;
    float mMaxChargeStrengthHeavy;
    float mMaxChargeStrengthLight;
    float mChargeDelayLight;
    float mChargeDelayHeavy;
    float mTimeAtMaxCharge;
    float mExtremeRange;
    float mLockOnRange;
    float mLockOnAngle;
    float mLockOffAngle;
    float mMinRange;
    float mOptimalRange;
    float mMaxRange;
    int mTargetSides;
    int mTargetTypes;
    int mAITargetTypes;
    GameSound mFireSound;
    GameSound mFireLoopSound;
    GameSound mFireEmptySound;
    GameSound mReloadSound;
    GameSound mChargeSound;
    float mChargeSoundPitch;
    GameSound mChargedSound;
    GameSound mWeaponChangeSound;
    GameSound mOverheatSound;
    float mOverheatSoundPitch;
    unsigned int mFireSoundStop;
    GameSound mFoleyFX[10];
    FLEffectClass* mChargeUpEffect;
    float mFlashLength;
    float mEnergyDrain;
    float mAutoPitchScreenDist;
    float mAutoTurnScreenDist;
    float mTargetLockMaxDistance;
    float mTargetLockMaxDistanceLose;
    short mScoreForMedalsType;
    short mMedalsTypeToUnlock;
    short mMedalsTypeToLock;
    unsigned int mFireAnim : 2;
    unsigned int mTriggerSingle : 1;
    unsigned int mZoomFirstPerson : 1;
    unsigned int mSniperScope : 1;
    unsigned int mMaxRangeDefault : 1;
    unsigned int mInstantPlayFireAnim : 1;
    unsigned int mIsOffhand : 1;

    bool mHashWeaponName : 1;
    bool mDisplayRefire : 1;
    bool mPostLoadInit : 1;
    bool mReticuleInAimingOnly : 1;
    bool mNoFirstPersonFireAnim : 1;
    bool mStopChargeSound : 1;
    bool mAIUseBubbleCircle : 1;
    bool mWarnedAboutLocalization : 1;
    float mAIBubbleSizeMultiplier;
    float mAIBubbleScaleDistDivider;
    float mAIBubbleScaleClamp;
    RedModel* mMuzzleFlashModel;
    FLEffectClass* mMuzzleFlashEffect;
    float mFlashRadius;
    float mFlashWidth;
    RedColor mFlashColor;
    RedColor mFlashLightColor;
    float mFlashLightRadius;
    float mFlashLightDuration;
    int mBarrageMin;
    int mBarrageMax;
    float mBarrageDelay;
    char mName[20];
};

class Weapon : public Thread
{
public:
    enum WeaponState
    {
        IDLE = 0,
        FIRE = 1,
        FIRE2 = 2,
        CHARGE = 3,
        RELOAD = 4,
        OVERHEAT = 5,
        EMPTY = 6
    };

    Weapon(const WeaponClass* pClass, const WeaponDesc& desc);
    virtual ~Weapon();
    virtual bool Update(float fDeltaTime);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void Init();
    virtual float GetOrdnanceVelocity() const;
    virtual float GetOrdnanceGravity() const;
    virtual bool IsLocked() const;
    virtual const GameObject* GetLocked() const;
    virtual int GetLockedTargetBodyId() const;
    virtual void SetKickSpreadFactor(float fFactor);
    virtual void SetYawSpreadFactor(float fFactor);
    virtual float GetYawSpread() const;
    virtual float GetPitchSpread() const;
    virtual bool Deflect(const Ordnance* pOrdnance, const PblVector3& pos,
                         const PblVector3& dir);
    virtual void SignalFire();
    virtual bool ShouldShowReticule() const;
    virtual bool IsMelee() const;
    virtual bool IsMeleeThrow() const;
    virtual void NotifySoldierState(int iState, float fTime, bool b0, bool b1,
                                    const PblVector3& vec);
    virtual bool SoldierCanOperate() const;
    virtual bool OverrideSoldierVelocity(float& f0, float& f1, float& f2,
                                         float& f3) const;
    virtual bool OverrideSoldierControls();
    virtual bool OverrideSoldierEnergyRestore(float& fRestore) const;
    virtual bool OverrideAimer();
    virtual void Select(unsigned int uFlags, bool bOffhand);
    virtual void Deselect(unsigned int uFlags, bool bOffhand);
    virtual bool IsBusy() const;
    virtual bool WillBecomeBusy() const;
    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);
    virtual void Render(const PblMatrix& mat, const RedPose* pPose,
                        const RedColor& color, unsigned int uFlags,
                        bool bArg) const;
    virtual unsigned int GetNameHash() const;

protected:
    virtual void EnterIdle();
    virtual bool UpdateIdle(float fDeltaTime);
    virtual void ExitIdle();
    virtual void EnterFire();
    virtual bool UpdateFire(float fDeltaTime);
    virtual void ExitFire();
    virtual void EnterCharge();
    virtual bool UpdateCharge(float fDeltaTime);
    virtual void ExitCharge();
    virtual void EnterReload();
    virtual bool UpdateReload(float fDeltaTime);
    virtual void ExitReload();
    virtual void EnterEmpty();
    virtual bool UpdateEmpty(float fDeltaTime);
    virtual void ExitEmpty();
    virtual bool CheckCharge() const;
    virtual bool CheckFire() const;
    virtual bool FireDone() const;
    virtual bool CheckEmpty() const;
    virtual bool ForceReload() const;
    virtual bool CheckReload() const;
    virtual bool ReloadDone() const;

    void PlayFireSound(bool bArg);
    void StopFireSound();

public:
    bool CheckOverheat() const;

    void SetSkip(bool bSkip);

    void ResetSkip();

    void UpdateZoom(float fDeltaTime, float fZoomInput);
    bool CycleZoom(bool bCycle);
    bool ZoomFirstPerson() const;
    int GetSoldierAnimationWeapon() const;
    float GetSoundRadius() const;
    void PlayFoleyFX(unsigned int uIndex);
    void ActivatePhysics();
    void DeactivatePhysics();

    Controllable* GetOwner() const;

    float GetHeat() const;
    float GetStateTimer() const { return mStateTimer; }
    const Trigger* GetTrigger() const { return mTrigger; }

    bool IsSelected() const { return mSelectedFlag; }

    // FUNCTION: SWBF2 0x00438ad0
    const WeaponClass* GetClass() const { return mClass; }
    bool GetFiredFlag() const { return mFiredFlag != 0; }
    void ClearFiredFlag() { mFiredFlag = 0; }
    WeaponState GetState() const { return mState; }

    static PblHash rttiHashWeapon;
    static char rttiNameWeapon[];

    static void operator delete(void* p);

    static void* operator new(size_t uSize);
    static void SetupMemoryPool(unsigned int uCount, unsigned int uSize, unsigned int uGrow) { sMemoryPool.Setup(uCount, uSize, uGrow); }
    static void UpdateMemoryPoolSize(unsigned int uSize)
    {
        if (sMemoryPool.GetSize() < uSize)
        {
            sMemoryPool.SetSize(uSize);
        }
    }
    static MemoryPool sMemoryPool;

    PblMatrix mFirePointMatrix;
    WeaponClass* mStart;
    WeaponClass* mClass;
    WeaponClass* mRenderClass;
    Controllable* mOwner;
    Aimer* mAimer;
    Trigger* mTrigger;
    Trigger* mReload;
    PblVector3 mFirePos;
    AmmoCounter* m_pAmmoCounter;
    EnergyBar* m_pEnergyBar;
    float mCurChargeStrengthHeavy;
    float mCurChargeStrengthLight;
    float mCurChargeDelayHeavy;
    float mCurChargeDelayLight;
    float mCurChargeRateLight;
    float mCurChargeRateHeavy;
    float mCurTimeAtMaxCharge;
    unsigned int mHideWeapon : 1;
    unsigned int mFiredFlag : 1;
    unsigned int mSelectedFlag : 1;
    int m_iSoldierState : 6;
    WeaponState mState;
    float mStateTimer;
    float mStateLimit;
    float mZoom;
    float mZoomTurnScale;
    float mMuzzleFlashStartTime;
    int mSoldierAnimationMap;
    GameSoundControllable mSoundControl;
    GameSoundControllable mSoundControlFire;
    GameSoundControllable mFoleyFXControl;
    GameSound mSoundProps;
    GameSound mSoundPropsFire;
    GameSound mFoleyFXProps;
    PblHandle<ParticleEmitterObject> mChargeUpEmitter;
    float mLastFireTime;
    float mSkipTime;
    bool mSkip : 1;
    bool mCharged : 1;
    unsigned char mDeactivateScheduled;
    PblHandle<const GameObject> mTarget;
    int mTargetBodyID;
};
