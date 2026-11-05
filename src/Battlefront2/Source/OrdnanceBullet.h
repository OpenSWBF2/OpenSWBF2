#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Ordnance.h"
#include "Battlefront2Lod.h"
#include "RedEngineFL/Util/RedSceneObject.h"
#include "PebbleFL/Common/PblHashTable.h"

class RedModel;

// VTABLE: SWBF2 0x007ac6e0
class OrdnanceBulletClass : public OrdnanceClass
{
public:
    virtual ~OrdnanceBulletClass();
    OrdnanceBulletClass(unsigned int uHash);
    OrdnanceBulletClass(OrdnanceBulletClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual float GetVelocity();
    virtual float GetRange();

    const RedModel* mModel;
    bool mStraightBullet;
    bool mFadeColorLight;
    RedLodData mLodData;
    float mVelocity;
    unsigned int mImpactEffectSoft;
    unsigned int mImpactEffectRigid;
    unsigned int mImpactEffectStatic;
    unsigned int mImpactEffectTerrain;
    unsigned int mImpactEffectWater;
    unsigned int mImpactEffectShieldSmall;
    unsigned int mImpactEffectShieldLarge;
    unsigned int mImpactDecalStatic;
    unsigned int mImpactDecalTerrain;
    unsigned int m_uiLightOdfNameHash;
    RedColor m_colorLight;
    RedColor m_endColorLight;
    float m_fRadiusLight;

    static PblHash rttiHashOrdnanceBulletClass;
    static const char rttiNameOrdnanceBulletClass[];
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

class EntityLight;

// VTABLE: SWBF2 0x007ac710 Thread
// VTABLE: SWBF2 0x007ac750 PblHandled
// VTABLE: SWBF2 0x007ac758
class OrdnanceBullet : public Ordnance, public RedSceneObject
{
public:
    OrdnanceBullet(OrdnanceBulletClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceBullet();

protected:
    virtual bool Update(float fDeltaTime);

public:
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual const PblVector3& GetOldPosition() const;
    virtual const PblVector3& GetVelocity() const;
    virtual bool Collide(CollisionObject* pObject, const PblVector3& vPos,
                         const PblVector3& vNormal, int iBodyId);
    virtual void CreateRicochet(const PblVector3& vPos, const PblVector3& vNormal,
                                GameObject* pObject) const;
    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);

    PblVector3 mOldPosition;
    PblVector3 mVelocity;
    PblVector3 mFinalCollisionNormal;
    CollisionObject* mFinalCollision;
    PblHandle<GameObject> mFinalCollisionHandle;
    bool mFinalCollisionOnNextUpdate;
    EntityLight* m_pLight;
    float mOffscreenTime;

    static PblHash rttiHashOrdnanceBullet;
    static const char rttiNameOrdnanceBullet[];
};
 
 
 

// FUNCTION: SWBF2 0x005f8b30
inline bool OrdnanceBulletClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceBulletClass == uRtti)
    {
        return true;
    }
    return OrdnanceClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005fa870
inline unsigned int OrdnanceBulletClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceBulletClass;
}

// FUNCTION: SWBF2 0x005fa880
inline const char* OrdnanceBulletClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceBulletClass;
}

// FUNCTION: SWBF2 0x005f8ae0
inline bool OrdnanceBullet::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceBullet == uRtti)
    {
        return true;
    }
    return Ordnance::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005fa820
inline unsigned int OrdnanceBullet::GetDerivedRtti() const
{
    return rttiHashOrdnanceBullet;
}

// FUNCTION: SWBF2 0x005fa830
inline const char* OrdnanceBullet::GetDerivedRttiName() const
{
    return rttiNameOrdnanceBullet;
}
