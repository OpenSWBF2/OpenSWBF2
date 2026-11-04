#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CommandPost.h"
#include "EntityFlyer.h"
#include "EntityCarrier.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

// VTABLE: SWBF2 0x00797ff8 EntityFlyerClass
// VTABLE: SWBF2 0x00797fc8 CommandPostClass
class CommandFlyerClass : public EntityFlyerClass, public CommandPostClass
{
public:
    CommandFlyerClass(unsigned int uHash);
    virtual ~CommandFlyerClass();

    CommandFlyerClass(CommandFlyerClass* pBase, unsigned int uHash);

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static unsigned int GetRtti();

    static PblHash rttiHashCommandFlyerClass;
    static const char rttiNameCommandFlyerClass[];
};

   
 

// VTABLE: SWBF2 0x00797e08 Trackable
// VTABLE: SWBF2 0x00797e50 CollisionObject
// VTABLE: SWBF2 0x00797ea4 EntityEx
// VTABLE: SWBF2 0x00797fd4
// VTABLE: SWBF2 0x00798044 PblHandled
// VTABLE: SWBF2 0x0079804c Thread
// VTABLE: SWBF2 0x0079811c Damageable
// VTABLE: SWBF2 0x00798148 RedSceneObject
class CommandFlyer : public EntityFlyer
{
public:

    CommandFlyer(CommandFlyerClass* pClass, const EntityDesc& desc);

    virtual ~CommandFlyer();

    static void* operator new(unsigned int uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x00477be0
    // CommandFlyer::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    static void SetupMemoryPool(unsigned int uCount, unsigned int uSize, unsigned int uGrow) { sMemoryPool.Setup(uCount, uSize, uGrow); }

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void Init();
    virtual void SetTeam(int iTeam);
    virtual EntityType GetEntityType() const;
    virtual void Kill();
    virtual void Respawn();
    virtual bool Update(float fDeltaTime);

    bool IsLanded();

    Controllable* GetPassengerSlot(int);

    void EnablePost(bool bEnable);

    CommandFlyerClass* mClass;
    CommandPost* mPost;
    bool mbHasTakenOffBefore;
    char _tailPad[0x7];

    static MemoryPool sMemoryPool;
    static PblHash rttiHashCommandFlyer;
    static const char rttiNameCommandFlyer[];
};
 
 
 
 

// FUNCTION: SWBF2 0x00477c30
inline bool CommandFlyerClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandFlyerClass == uRtti)
    {
        return true;
    }
    return EntityFlyerClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00477c80
inline unsigned int CommandFlyerClass::GetDerivedRtti() const
{
    return rttiHashCommandFlyerClass;
}

// FUNCTION: SWBF2 0x00477c90
inline const char* CommandFlyerClass::GetDerivedRttiName() const
{
    return rttiNameCommandFlyerClass;
}

// FUNCTION: SWBF2 0x00477b70
inline bool CommandFlyer::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandFlyer == uRtti)
    {
        return true;
    }
    return EntityFlyer::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00477bc0
inline unsigned int CommandFlyer::GetDerivedRtti() const
{
    return rttiHashCommandFlyer;
}

// FUNCTION: SWBF2 0x00477bd0
inline const char* CommandFlyer::GetDerivedRttiName() const
{
    return "CommandFlyer";
}


