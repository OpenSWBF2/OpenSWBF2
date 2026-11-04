#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "GameSound.h"
#include "Weapon.h"
#include "PebbleFL/Common/PblHashTable.h"

class OrdnanceClass;

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
