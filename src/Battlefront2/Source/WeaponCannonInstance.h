#pragma once

#include "GameSound.h"
#include "WeaponCannon.h"

// VTABLE: SWBF2 0x007b057c
class WeaponCannon : public Weapon
{
public:
    WeaponCannon(const WeaponCannonClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponCannon();
    virtual bool Update(float fDeltaTime);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetKickSpreadFactor(float fFactor);
    virtual void SetYawSpreadFactor(float fFactor);
    virtual float GetYawSpread() const;
    virtual float GetPitchSpread() const;
    virtual void NotifySoldierState(int iState, float fTime, bool b0, bool b1,
                                    const PblVector3& vec);
    virtual bool IsBusy() const;
    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);
    virtual void EnterFire();
    virtual bool UpdateFire(float fDeltaTime);
    virtual void ExitFire();
    virtual void EnterCharge();
    virtual bool UpdateCharge(float fDeltaTime);
    virtual void ExitCharge();
    virtual void EnterEmpty();
    virtual bool CheckCharge() const;
    virtual bool CheckFire() const;
    virtual bool FireDone() const;
    virtual bool CheckEmpty() const;
    virtual bool ReloadDone() const;
    virtual void SetOrdnance(Ordnance* pOrdnance);
    virtual Ordnance* CreateOrdnance(OrdnanceDesc& desc);

    bool Fire();
    void PlayWarmUpSound();
    void StopWarmUpSound();
    void UpdateWarmUpSound(float fDeltaTime);
    void PlayBarrelSound();
    void StopBarrelSound();
    void UpdateBarrelSound(float fDeltaTime);
    void PlayCoolDownSound();
    void StopCoolDownSound();
    void UpdateCoolDownSound(float fDeltaTime);

    static PblHash rttiHashWeaponCannon;
    static char rttiNameWeaponCannon[];

    float mStrength;
    int mSalvoCount;
    float mSalvoDelay;
    int mPatternIndex;
    float mKickSpread;
    float mKickSpreadFactor;
    float mYawSpreadFactor;
    float mStanceSpread;
    float mWarmCoolTimer;
    bool mWarmUp : 1;
    GameSoundControllable mSoundControlWarmUp;
    GameSound mSoundPropsWarmUp;
    GameSoundControllable mSoundControlBarrel;
    GameSound mSoundPropsBarrel;
    GameSoundControllable mSoundControlCoolDown;
    GameSound mSoundPropsCoolDown;
    int mOrdCounter;
};
