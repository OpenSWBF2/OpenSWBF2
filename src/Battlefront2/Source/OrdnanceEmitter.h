#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Ordnance.h"

class ExplosionClass;
class FLEffectClass;

// VTABLE: SWBF2 0x007ac7d8
class OrdnanceEmitterClass : public OrdnanceClass
{
public:
    OrdnanceEmitterClass(unsigned int uHash);
    virtual ~OrdnanceEmitterClass();

    OrdnanceEmitterClass(OrdnanceEmitterClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual float GetRange();
    virtual DamageDesc GetDamage(const DamageOwner& owner, float fUnused, float fRatio,
                                 float fElapsed) const;

    float GetDamage(float fStrength) const;

    bool DoBuff(GameObject* pObject, const OrdnanceDesc& desc, bool bExplode);

    FLEffectClass* mFireEffect;
    FLEffectClass* mLightningEffectClass;
    float mLightningEffectScale;
    int mNoTargetLightningEffectCount;
    float mNoTargetLightningEffectRandomSpread;
    unsigned int mStunAnimation;
    float mStunTime;
    float mMinDamage;
    float mMaxDamage;
    float mDamageThreshold;
    float mMaxJumpDistance;
    float mJumpDeduction;
    float mPush;
    float mConeLength;
    float mConeWidth;
    float mFirstRadius;
    int mMaxTargets;
    bool mNoChaining;
    float mRadius;
    float mBuffOffenseTimer;
    float mBuffOffenseMult;
    float mBuffDefenseTimer;
    float mBuffDefenseMult;
    float mBuffHealthTimer;
    float mBuffHealthRate;
    float mDebuffDamageTimer;
    float mDebuffDamageRate;
    unsigned int mPlayEffectOnOwner;
    unsigned int mPlayEffectOnOwnerAimer;
    float mSmolderEffectTimer;
    bool mSmolderVanishDeath : 1;
    unsigned int mSmolderEffectBone[5];
    unsigned int mSmolderEffect[5];
    float mSmolderDamageRate;
    bool mAffectFriends : 1;
    bool mAffectEnemies : 1;
    bool mApplyOnOwnerIfOnOthers : 1;
    ExplosionClass* mExplosion;

    static PblHash rttiHashOrdnanceEmitterClass;
    static const char rttiNameOrdnanceEmitterClass[];
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x005fc3b0
inline bool OrdnanceEmitterClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceEmitterClass == uRtti)
    {
        return true;
    }
    return OrdnanceClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005fc3e0
inline unsigned int OrdnanceEmitterClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceEmitterClass;
}

// FUNCTION: SWBF2 0x005fc3f0
inline const char* OrdnanceEmitterClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceEmitterClass;
}
