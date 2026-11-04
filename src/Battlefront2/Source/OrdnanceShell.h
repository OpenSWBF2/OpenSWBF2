#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "OrdnanceBullet.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007acf88
class OrdnanceShellClass : public OrdnanceBulletClass
{
public:
    virtual ~OrdnanceShellClass();
    OrdnanceShellClass(unsigned int uHash);
    OrdnanceShellClass(OrdnanceShellClass* pBase, unsigned int uHash);

    virtual OrdnanceClass* Derive(unsigned int uHash);
    virtual Ordnance* Build(const OrdnanceDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual float GetGravity();

    float mGravity;

    static PblHash rttiHashOrdnanceShellClass;
    static const char rttiNameOrdnanceShellClass[];
};
  
 

// VTABLE: SWBF2 0x007acfb8 PblHandled
// VTABLE: SWBF2 0x007acfc0
// VTABLE: SWBF2 0x007ad028 Thread
class OrdnanceShell : public OrdnanceBullet
{
public:
    OrdnanceShell(OrdnanceShellClass* pClass, const OrdnanceDesc& desc);
    virtual ~OrdnanceShell();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

protected:
    virtual bool Update(float fDeltaTime);

public:
    float mGravity;

    static PblHash rttiHashOrdnanceShell;
    static const char rttiNameOrdnanceShell[];
};
 
 

// FUNCTION: SWBF2 0x005f8bb0
inline bool OrdnanceShellClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceShellClass == uRtti)
    {
        return true;
    }
    return OrdnanceBulletClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00605b90
inline unsigned int OrdnanceShellClass::GetDerivedRtti() const
{
    return rttiHashOrdnanceShellClass;
}

// FUNCTION: SWBF2 0x00605ba0
inline const char* OrdnanceShellClass::GetDerivedRttiName() const
{
    return rttiNameOrdnanceShellClass;
}

// FUNCTION: SWBF2 0x005f8b80
inline bool OrdnanceShell::IsRtti(unsigned int uRtti) const
{
    if (rttiHashOrdnanceShell == uRtti)
    {
        return true;
    }
    return OrdnanceBullet::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00605b40
inline unsigned int OrdnanceShell::GetDerivedRtti() const
{
    return rttiHashOrdnanceShell;
}

// FUNCTION: SWBF2 0x00605b50
inline const char* OrdnanceShell::GetDerivedRttiName() const
{
    return rttiNameOrdnanceShell;
}
