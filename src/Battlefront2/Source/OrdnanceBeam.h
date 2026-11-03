#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Ordnance.h"
#include "Battlefront2Lod.h"
#include "RedEngineFL/Util/RedSceneObject.h"

class Aimer;
class FLEffectClass;

// VTABLE: SWBF2 0x007ac684
class OrdnanceBeamClass : public OrdnanceClass
{
public:
    virtual ~OrdnanceBeamClass();
    OrdnanceBeamClass(unsigned int uHash);

    OrdnanceBeamClass(OrdnanceBeamClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    static const char rttiNameOrdnanceBeamClass[];
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual float GetRange();

    float mRange;
    float mBeamWidth;
    unsigned int mBeamTextureId;
    RedColor mBeamColor;
    float mGlowWidth;
    unsigned int mGlowTextureId;
    RedColor mGlowColor;
    bool mPassThrough : 1;
    FLEffectClass* mImpactEffect;

    static PblHash rttiHashOrdnanceBeamClass;
};
  
 
 
 
 
 
 
 
 

class BeamLodData : public RedLodData
{
public:
    BeamLodData() { mLowRezFlags = 5; }
};
 

// VTABLE: SWBF2 0x007ac5dc
// VTABLE: SWBF2 0x007ac644 Thread
// VTABLE: SWBF2 0x007ac6b4 PblHandled
class OrdnanceBeam : public Ordnance, public RedSceneObject
{
public:
    OrdnanceBeam(OrdnanceBeamClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceBeam();

    virtual bool Update(float fDeltaTime);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void CreateRicochet(const PblVector3& vPos, const PblVector3& vNormal,
                                GameObject* pObject) const;
    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);

    void SetDead();

    PblVector3 mDirection;
    float mLength;
    Aimer* mAttachAimer;
    OrdnanceBeam* m_pRicochet;
    bool mDeadFlag;

    static PblHash rttiHashOrdnanceBeam;
    static const char rttiNameOrdnanceBeam[];
};
 
 
 
 
 
 

// FUNCTION: SWBF2 0x005f9640
inline bool OrdnanceBeamClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceBeamClass == uRtti)
    {
        return true;
    }
    return OrdnanceClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005f95f0
inline bool OrdnanceBeam::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceBeam == uRtti)
    {
        return true;
    }
    return Ordnance::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x005f9620
inline unsigned int OrdnanceBeam::GetDerivedRtti() const
{
    return rttiHashOrdnanceBeam;
}

// FUNCTION: SWBF2 0x005f9630
inline const char* OrdnanceBeam::GetDerivedRttiName() const
{
    return rttiNameOrdnanceBeam;
}
