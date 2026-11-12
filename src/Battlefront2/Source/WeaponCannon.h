#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "GameSound.h"
#include "Weapon.h"
#include "PebbleFL/Common/PblHashTable.h"

class OrdnanceClass;
class WeaponCannonClass;

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
    static const char rttiNameWeaponCannon[];

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

// FUNCTION: SWBF2 0x0067e3c0
inline bool WeaponCannon::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponCannon == uRtti)
    {
        return true;
    }
    return Weapon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0067e3f0
inline unsigned int WeaponCannon::GetDerivedRtti() const
{
    return rttiHashWeaponCannon;
}

// FUNCTION: SWBF2 0x0067e400
inline const char* WeaponCannon::GetDerivedRttiName() const
{
    return rttiNameWeaponCannon;
}

// VTABLE: SWBF2 0x007b0674
class WeaponCannonClass : public WeaponClass
{
public:
    virtual ~WeaponCannonClass();
    WeaponCannonClass(unsigned int uHash);

    WeaponCannonClass(WeaponCannonClass* pBase, unsigned int uHash);

    struct ShotPattern
    {
        float mPitch;
        float mYaw;
    };

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual float GetHeatPerSalvo() const;
    virtual float GetShotsPerSalvo() const;
    virtual int GetBarrageMin() const;
    virtual int GetBarrageMax() const;

    static PblHash rttiHashWeaponCannonClass;
    static const char rttiNameWeaponCannonClass[];

    float mMaxPressedTime;
    float mShotDelayMin;
    float mShotDelayMax;
    float mPitchSpread;
    float mYawSpread;
    float mStanceSpread[3][3];
    float mSpreadPerShot;
    float mSpreadRecoverRate;
    float mSpreadThreshold;
    float mSpreadLimit;
    float mKickStrength;
    float mKickBuildup;
    int mSalvoCount;
    int mShotsPerSalvo;
    float mSalvoDelayMin;
    float mSalvoDelayMax;
    float mInitialSalvoDelay;
    ShotPattern* mPatternArray;
    int mPatternCount;
    float mWarmUpTime;
    float mCoolDownTime;
    GameSound mWarmUpSound;
    float mWarmUpSoundStartPitch;
    float mWarmUpSoundFadeInTime;
    float mWarmUpSoundFadeOutTime;
    GameSound mBarrelSound;
    float mBarrelSoundFadeInTime;
    float mBarrelSoundFadeOutTime;
    GameSound mCoolDownSound;
    float mCoolDownSoundEndPitch;
    float mCoolDownSoundFadeInTime;
    float mCoolDownSoundFadeOutTime;
    OrdnanceClass* mSecondaryOrdnanceClass;
    int mSecondaryOrdnancePeriod;
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x0067e440
inline bool WeaponCannonClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponCannonClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0067e470
inline unsigned int WeaponCannonClass::GetDerivedRtti() const
{
    return rttiHashWeaponCannonClass;
}

// FUNCTION: SWBF2 0x0067e480
inline const char* WeaponCannonClass::GetDerivedRttiName() const
{
    return rttiNameWeaponCannonClass;
}
