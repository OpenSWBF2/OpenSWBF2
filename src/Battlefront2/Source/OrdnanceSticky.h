#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrdnanceShell.h"

// VTABLE: SWBF2 0x007ad138
class OrdnanceStickyClass : public OrdnanceShellClass
{
public:
    OrdnanceStickyClass(unsigned int uHash);
    OrdnanceStickyClass(OrdnanceStickyClass* pBase, unsigned int uHash);
    virtual ~OrdnanceStickyClass();

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    GameSound mSoundCollisionWater;
    GameSound mSoundCollisionLeaf;
    GameSound mSoundCollisionOther;
    bool mStartTimerOnContact;
    bool mAlignVertical;
    int mStickTypes;

    static PblHash rttiHashOrdnanceStickyClass;
    static const char rttiNameOrdnanceStickyClass[];
};
  
 
 
 
 
 
 

class StickInfo;

// VTABLE: SWBF2 0x007ad08c
// VTABLE: SWBF2 0x007ad0f4 Thread
// VTABLE: SWBF2 0x007ad168 PblHandled
class OrdnanceSticky : public OrdnanceShell
{
public:
    OrdnanceSticky(OrdnanceStickyClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceSticky();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool CheckCollision();
    virtual bool Collide(CollisionObject* pObject, const PblVector3& vPos,
                         const PblVector3& vNormal, int iBodyId);
    virtual void SoundEvent();

    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);

    virtual bool Update(float fDeltaTime);

public:
    StickInfo* mStickInfo;
    unsigned char mNumBounces;
    float mLastEventTime;
    float mBeginTime;

    static PblHash rttiHashOrdnanceSticky;
    static const char rttiNameOrdnanceSticky[];
};
 
 
 
 
 

// FUNCTION: SWBF2 0x005f8c30
inline bool OrdnanceStickyClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceStickyClass == uRtti)
    {
        return true;
    }
    return OrdnanceShellClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00605fb0
inline unsigned int OrdnanceStickyClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceStickyClass;
}

// FUNCTION: SWBF2 0x00605fc0
inline const char* OrdnanceStickyClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceStickyClass;
}

// FUNCTION: SWBF2 0x005f8bf0
inline bool OrdnanceSticky::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceSticky == uRtti)
    {
        return true;
    }
    return OrdnanceShell::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00605f60
inline unsigned int OrdnanceSticky::GetDerivedRtti() const
{
    return rttiHashOrdnanceSticky;
}

// FUNCTION: SWBF2 0x00605f70
inline const char* OrdnanceSticky::GetDerivedRttiName() const
{
    return rttiNameOrdnanceSticky;
}
