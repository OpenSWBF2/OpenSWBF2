#pragma once

#include <stddef.h>

#include "FirstPerson.h"
#include "SoldierAnimationBank.h"
#include "Thread.h"
#include "Trackable.h"
#include "Trigger.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblList.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblVector.h"

class Aimer;
class Character;
class CollisionModel;
class CommandPost;
class Controller;
class EnergyBar;
class Entity;
class GameObject;
class OrdnanceMissile;
class RedModel;
class RedPose;
class Weapon;
enum EnergyMode;

enum ControllableHeight
{
    HEIGHT_STAND = 0,
    HEIGHT_CROUCH = 1,
    HEIGHT_PRONE = 2,
    HEIGHT_SPRINT = 3,
    HEIGHT_WALK = 4,
    HEIGHT_COUNT = 5
};

enum PilotDeathType
{
    PILOTDEATH_FLY = 0,
    PILOTDEATH_FALL = 1,
    NUM_PILOTDEATH_TYPES = 2
};

enum PilotType
{
    PILOT_NONE = 0,
    PILOT_SELF = 1,
    PILOT_VEHICLE = 2,
    PILOT_REMOTE = 3,
    PILOT_VEHICLESELF = 4
};

 
 

class ControllableClass
{
public:
    static const int kMaxWeapons = 4;
    ControllableClass();

    ControllableClass(ControllableClass* pBase);

    bool SetProperty(unsigned int uProperty, const char* pszValue, const RedModel* pModel);

    void UpdatePilotAnimation(Controllable* pPilot, const RedPose* pPose, const PblMatrix& mMatrix,
                              const PblVector3& vOffset, float fDeltaTime, float f1, float f2, float f3);

    bool DoesCombatInterrupt() const { return mCombatInterrupts; }

    bool mCombatInterrupts;
    bool mIgnoreHintNodes;
    float mShakeScale;
    CamPathInfo mBobPath;
    float mCockpitTension;
    unsigned int mLocalNameID;
    unsigned int mForceMode;
    float mFirstPersonFOV;
    float mThirdPersonFOV;
    unsigned int mPilotParentNodeHash;
    PblMatrix mPilotOffsetMatrix;
    SoldierAnimationBank::ANIMATION mPilotAnimation;
    bool mAnimatedPilotNodeFlag;
    PilotDeathType mPilotDeathType;
    PilotType mPilotType;
    bool mIsPilotExposed;
    bool mIsPilotAnimation9Pose;
    bool mCanCapturePosts;
    bool mCanEnterVehicles;
    bool mControlsUnit;
    bool mAnimatesUnitAsTurret;
};
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

float __cdecl GetDefaultFOV();

class TargetInfo
{
public:
    TargetInfo();

    void SetAimPoint(const PblVector3& aimPoint);

    static float GetTargetPrio(const PblVector3& vStart, const PblVector3& vDir, const PblVector3& vTarget,
                               float fRadius, float fAngle, float fRange, bool bFlag);

    PblVector3 mAimStart;
    PblVector3 mAimPoint;
    bool mIsAiming;
};
 

// VTABLE: SWBF2 0x00798b80 Thread
// VTABLE: SWBF2 0x00798cb0 Trackable
class Controllable : public Thread, public Trackable
{
public:
    static const int kNumWeaponChannels = 2;
    static const int kMaxWeapons = ControllableClass::kMaxWeapons;

    // VTABLE: SWBF2 0x00798c98
    class AIThread : public Thread
    {
    public:

        AIThread(Controllable* pOwner) : mOwner(pOwner) {}

        // FUNCTION: SWBF2 0x00483460
        virtual bool Update(float fDeltaTime)
        {
            mOwner->UpdateIndirect(fDeltaTime);
            return true;
        }
        // FUNCTION: SWBF2 0x00483480
        virtual void DeactivateThread()
        {
            mOwner->DeactivateIndirect();
            Thread::DeactivateThread();
        }

        Controllable* mOwner;
    };

    Controllable(ControllableClass* pClass, int iPlayerId);
    virtual ~Controllable();

protected:
    virtual bool Update(float fDeltaTime);

public:
    virtual void Kill();
    virtual GameObject* GetGameObject();
    virtual const GameObject* GetGameObject() const;
    virtual Entity* GetEntity();
    virtual const Entity* GetEntity() const;
    // FUNCTION: SWBF2 0x004833a0
    virtual Trackable* GetTrackable()
    {
        return 0;
    }
    // FUNCTION: SWBF2 0x004774d0
    virtual CollisionModel* GetDefaultPilotCollision()
    {
        return 0;

    }
    virtual const PblMatrix& GetMatrix() const = 0;
    // FUNCTION: SWBF2 0x004774e0
    virtual float GetTargetBubbleStanceModifier() const
    {
        return 1.0f;
    }
    virtual float GetSoundRadius() const;
    virtual float GetAimTurnRate() const;
    virtual int GetWeaponCount() const;
    virtual int GetWeaponIndex(int iChannel) const;
    virtual Weapon* GetWeaponPtr(int iIndex) const;
    virtual EnergyBar* GetEnergyBar();
    const EnergyBar* GetEnergyBar() const
    {
        return const_cast<Controllable*>(this)->GetEnergyBar();
    }

    float GetEnergy() const;
    // FUNCTION: SWBF2 0x00478a40
    virtual EnergyMode GetEnergyMode() const
    {
        return (EnergyMode)0;

    }
    // FUNCTION: SWBF2 0x004774f0
    virtual SoldierAnimationBank::BANK GetSoldierAnimationBank() const
    {
        return SoldierAnimationBank::INVALID_BANK;
    }
    virtual void SetWeaponIndex(int iChannel, int iIndex);
    virtual const Aimer* GetAimer(int iIndex) const;
    // FUNCTION: SWBF2 0x00477500
    virtual bool CanEnterRemote(const GameObject* pObject) const
    {
        return false;

    }
    // FUNCTION: SWBF2 0x00477510
    virtual bool CanEnterVehicle(const GameObject* pObject) const
    {
        return false;

    }
    // FUNCTION: SWBF2 0x00477520
    virtual bool EnterVehicle(GameObject* pObject)
    {
        return false;

    }
    // FUNCTION: SWBF2 0x00477530
    virtual bool CyclePosition(bool bForward)
    {
        return false;

    }
    // FUNCTION: SWBF2 0x00477540
    virtual bool ExitVehicle(bool bForce, bool bEject)
    {
        return false;

    }

    // FUNCTION: SWBF2 0x00477550
    virtual bool EnterRemote(GameObject* pObject) { return false; }
    // FUNCTION: SWBF2 0x00477560
    virtual bool ExitRemote() { return false; }
    virtual const ControllableClass* GetControllableClass() const;
    virtual ControllableClass* GetControllableClass();
    // FUNCTION: SWBF2 0x00483440
    virtual void SetupController()
    {
    }
    virtual void SetPilot(Controllable* pPilot);
    // FUNCTION: SWBF2 0x00477570
    virtual void SetPilotInfo(const PblMatrix& mMatrix, const PblVector3& vOffset,
                              SoldierAnimationBank::ANIMATION eAnimation, bool bFlag,
                              float f0, float f1, float f2, float f3)
    {
    }
    virtual void GetPilotMatrix(const PblMatrix& mIn, PblMatrix& mOut);
    // FUNCTION: SWBF2 0x00477580
    virtual PilotDeathType GetPilotDeathType()
    {
        return PILOTDEATH_FALL;

    }
    // FUNCTION: SWBF2 0x00477590
    virtual void ApplyWeaponKick(float fPitch, float fYaw)
    {
    }
    // FUNCTION: SWBF2 0x004775a0
    virtual void GetPitchYawLimits(float& fPitchMin, float& fPitchMax,
                                   float& fYawMin, float& fYawMax) const
    {

    }
    // FUNCTION: SWBF2 0x004775b0
    virtual bool Stand()
    {
        return false;

    }
    // FUNCTION: SWBF2 0x004775c0
    virtual bool Crouch()
    {
        return false;

    }
    // FUNCTION: SWBF2 0x004775d0
    virtual bool Prone()
    {
        return false;

    }
    // FUNCTION: SWBF2 0x004775e0
    virtual bool Jump()
    {
        return false;

    }
    // FUNCTION: SWBF2 0x004775f0
    virtual bool Roll()
    {
        return false;

    }
    // FUNCTION: SWBF2 0x00477600
    virtual bool Sprint()
    {
        return false;

    }
    // FUNCTION: SWBF2 0x00477610
    virtual bool EndSprint()
    {
        return false;
    }
    // FUNCTION: SWBF2 0x00477620
    virtual bool JetJump()
    {
        return false;

    }
    // FUNCTION: SWBF2 0x00477630
    virtual bool EndJetJump()
    {
        return false;

    }
    // FUNCTION: SWBF2 0x00477640
    virtual float GetOffenseMultiplier() const
    {
        return 1.0f;
    }
    // FUNCTION: SWBF2 0x00477650
    virtual int GetActiveWeaponChannel()
    {
        return mWpnChannel;

    }
    virtual bool UpdateIndirect(float fDeltaTime) = 0;

protected:
    // FUNCTION: SWBF2 0x00477660
    virtual void DeactivateIndirect()
    {
    }

public:
    void Initialize();

    void SetThrust(float value) { mControlMove = value; }
    void SetStrafe(float value) { mControlStrafe = value; }
    void SetTurn(float value) { mControlTurn = value; }
    void SetPitch(float value) { mControlPitch = value; }
    void ClearControllable();
    void ResetControls();
    void SetSwitch(float fValue, int iIndex);

    float GetTurnBuildup() const { return mTurnBuildup; }
    void SetTurnBuildup(float fBuildup) { mTurnBuildup = fBuildup; }
    float GetPitchBuildup() const { return mPitchBuildup; }
    void SetPitchBuildup(float fBuildup) { mPitchBuildup = fBuildup; }

    float GetThrust() const { return mControlMove; }

    // FUNCTION: SWBF2 0x0049e350
    bool WasPlayer() const { return mInitialPlayerId >= 0; }

    // FUNCTION: SWBF2 0x0049e360
    int GetInitialPlayerId() const { return mInitialPlayerId; }

    // FUNCTION: SWBF2 0x0051ecf0
    void SetInitialPlayerId(int iPlayerId) { mInitialPlayerId = iPlayerId; }

    // FUNCTION: SWBF2 0x004959b0
    PilotType GetPilotType() const { return mPilotType; }

    // FUNCTION: SWBF2 0x0043edd0
    bool IsPilotable() const { return mPilotType == PILOT_VEHICLE || mPilotType == PILOT_VEHICLESELF; }

    // FUNCTION: SWBF2 0x00485b10
    bool IsSelfPilotable() const { return GetPilotType() == PILOT_SELF || GetPilotType() == PILOT_VEHICLESELF; }

    // FUNCTION: SWBF2 0x00483450
    bool IsRemoteVehicle() const { return mPilotType == PILOT_REMOTE; }

    // FUNCTION: SWBF2 0x0047a700
    int GetPlayerId() const { return mPlayerId; }

    bool IsPlayer() const { return mPlayerId >= 0; }
    Controllable* GetEnteredRemoteOrVehicleIfNotMe();

    Controllable* GetPilot();

    const Controllable* GetPilot() const;
    void SetExitVehicle(const CommandPost* pPost);
    void AddController(Controller* pController, bool bIndirect);
    void ReleaseController();
    void SetCharacter(Character* pCharacter);
    void SetCurrentCollision(GameObject* pObject);
    GameObject* GetCurrentCollision() const;
    const Aimer* GetCurAimer() const;

    const PblVector3& GetEyePoint() const { return mEyePoint; }
    const Weapon* GetCurWpn() const;
    Weapon* GetCurWpn();
    int GetWpnChannel() const;
    float GetWpnDistFactor() const;
    void SetWpnDistFactor(float fFactor);

    float GetEnemyLockedOnMeState() const { return mEnemyLockedOnMeState; }
    void SetEnemyLockedOnMeState(float fState, OrdnanceMissile* pMissile, float fDistance);
    float GetTurnAdjusted();
    float GetPitchAdjusted();
    GameObject* GetTargetLockedObj();
    void SetTargetLockedObj(GameObject* pObject);

    static GameObject* GetVehicleIfAttached(GameObject* pObject);
    void TargetTrackingForceClear();

protected:
    void SetIndirectControl(bool bIndirect);

public:

    // FUNCTION: SWBF2 0x00437540
    Character* GetCharacter() const { return mCharacter; }
    Trigger mControlFire[kNumWeaponChannels];
    Trigger mControlReload;
    Trigger mControlJump;
    Trigger mControlCrouch;
    Trigger mControlSprint;
    Trigger mControlUse;
    Trigger mControlZoom;
    Trigger mControlView;
    Trigger mControlLockTarget;
    Trigger mControlSquadCommand;

    Trigger mControlSpecialMove;
    Trigger mControlThrustFwd;
    Trigger mControlThrustBack;
    Trigger mControlStrafeLeft;
    Trigger mControlStrafeRight;
    Trigger mTriggerControlSwitch[2];
    float mControlMove;
    float mControlStrafe;
    float mControlTurn;
    float mControlPitch;
    float mControlSwitch[kNumWeaponChannels];
    int mWpnChannel;
    AIThread mAIThread;
    PblList<Controllable>::Node mNode;
    Controller* mCtrl;
    Character* mCharacter;
    Controllable* mPilot;
    int mPlayerId;
    int mInitialPlayerId;
    PblVector3 mEyePoint;
    PblVector3 mEyeDir;
    float mTurnBuildup;
    float mPitchBuildup;
    bool mIndirectControlFlag;
    bool mUsingTurnAdjusted;
    bool mPlayAnim;
    unsigned int mAnimNameId;
    float mWpnDistFactor;
    PblHandle<GameObject> mCollisionObj;
    float mEnemyLockedOnMeState;
    float mEnemyLockedOnMeTimestamp;
    OrdnanceMissile* mEnemyLockedOnMeMissile;
    float mEnemyLockedOnMeDistance;
    float mTurnAdjusted;
    float mPitchAdjusted;
    float mTurnAuto;
    float mPitchAuto;
    float mLockBreakTimer;
    float mInCaptureRegionTimestamp;
    PblHandle<GameObject> mTargetLockedObj;
    int mAllowedLockTypes;
    PilotType mPilotType;
    TargetInfo mTargetInfo;
    PblHandle<const GameObject> mReticuleTarget[2];
    PblHandle<const GameObject> mAIReticuleTarget[2];
    int mReticuleAffiliation[2];
};
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x0043aad0
inline Controllable* Controllable::GetPilot()
{
    return mPilotType == PILOT_SELF || (mPilotType == PILOT_VEHICLESELF && mPilot == this) ? 0 : mPilot;
}

// FUNCTION: SWBF2 0x0043cf70
inline const Controllable* Controllable::GetPilot() const
{
    return mPilotType == PILOT_SELF || (mPilotType == PILOT_VEHICLESELF && mPilot == this) ? 0 : mPilot;
}
