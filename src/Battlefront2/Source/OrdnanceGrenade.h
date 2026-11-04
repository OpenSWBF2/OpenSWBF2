#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrdnanceShell.h"

// VTABLE: SWBF2 0x007aca48
class OrdnanceGrenadeClass : public OrdnanceShellClass
{
public:
    virtual ~OrdnanceGrenadeClass();
    OrdnanceGrenadeClass(unsigned int uHash);
    OrdnanceGrenadeClass(OrdnanceGrenadeClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    float mRebound;
    float mFriction;
    GameSound mSoundCollisionLeaf;
    GameSound mSoundCollisionOther;
    unsigned int mImpactEffectWater;
    bool mStartTimerOnContact;
    bool mAlignVertical;

    static PblHash rttiHashOrdnanceGrenadeClass;
    static const char rttiNameOrdnanceGrenadeClass[];
};
  
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007aca78 PblHandled
// VTABLE: SWBF2 0x007aca80
// VTABLE: SWBF2 0x007acae8 Thread
class OrdnanceGrenade : public OrdnanceShell
{
public:
    OrdnanceGrenade(OrdnanceGrenadeClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceGrenade();

    virtual bool Update(float fDeltaTime);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool CheckCollision();
    virtual bool Collide(CollisionObject* pObject, const PblVector3& vPos,
                         const PblVector3& vNormal, int iBodyId);
    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);
    virtual void SoundEvent();

    int mNumBounces;
    float mBeginTime;
    float mLastEventTime;

    static PblHash rttiHashOrdnanceGrenade;
    static const char rttiNameOrdnanceGrenade[];
};
 
 
 
 

// FUNCTION: SWBF2 0x006000c0
inline bool OrdnanceGrenadeClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceGrenadeClass == uRtti)
    {
        return true;
    }
    return OrdnanceShellClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00600100
inline unsigned int OrdnanceGrenadeClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceGrenadeClass;
}

// FUNCTION: SWBF2 0x00600110
inline const char* OrdnanceGrenadeClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceGrenadeClass;
}

// FUNCTION: SWBF2 0x00600030
inline bool OrdnanceGrenade::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceGrenade == uRtti)
    {
        return true;
    }
    return OrdnanceShell::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00600070
inline unsigned int OrdnanceGrenade::GetDerivedRtti() const
{
    return rttiHashOrdnanceGrenade;
}

// FUNCTION: SWBF2 0x00600080
inline const char* OrdnanceGrenade::GetDerivedRttiName() const
{
    return rttiNameOrdnanceGrenade;
}
