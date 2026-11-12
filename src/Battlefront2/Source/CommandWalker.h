#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CommandPost.h"
#include "EntityWalker.h"

// VTABLE: SWBF2 0x00798854 EntityWalkerClass
// VTABLE: SWBF2 0x007987d4 CommandPostClass
class CommandWalkerClass : public EntityWalkerClass, public CommandPostClass
{
public:
    CommandWalkerClass(unsigned int uHash);

    CommandWalkerClass(CommandWalkerClass* pBase, unsigned int uHash);

    virtual ~CommandWalkerClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static unsigned int GetRtti();

    static PblHash rttiHashCommandWalkerClass;
    static const char rttiNameCommandWalkerClass[];
};

   
 

#include "RedEngineFL/Memory/RedMemoryPool.h"

// VTABLE: SWBF2 0x00798690 EntityEx
// VTABLE: SWBF2 0x007989a0 CollisionObject
// VTABLE: SWBF2 0x00798628 RedSceneObject
// VTABLE: SWBF2 0x00798974 Damageable
// VTABLE: SWBF2 0x0079884c PblHandled
// VTABLE: SWBF2 0x007988a4 Thread
// VTABLE: SWBF2 0x007987e0 Trackable
class CommandWalker : public EntityWalker
{
public:
    CommandWalker(CommandWalkerClass* pClass, const EntityDesc& desc);
    virtual ~CommandWalker();

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x0047fdb0
    // CommandWalker::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    virtual bool IsRtti(unsigned int uRtti) const;
    static unsigned int GetRtti() { return rttiHashCommandWalker; }
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void Init();
    virtual void SetTeam(int iTeam);
    virtual EntityType GetEntityType() const;
    virtual void WriteCreate(NetPktGroup& group);
    virtual void ReadCreate(NetPktGroup& group);
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual void SkipReadObject(NetObjState* pState);
    virtual GameObject* AddToClient();
    virtual void Kill();
    virtual void Respawn();

    CommandPost* mPost;
    char _tailPad[0xc];

    static MemoryPool sMemoryPool;
    static PblHash rttiHashCommandWalker;
};
 
 

// FUNCTION: SWBF2 0x0047fe00
inline bool CommandWalkerClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandWalkerClass == uRtti)
    {
        return true;
    }
    return EntityWalkerClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0047fe50
inline unsigned int CommandWalkerClass::GetDerivedRtti() const
{
    return rttiHashCommandWalkerClass;
}

// FUNCTION: SWBF2 0x0047fe60
inline const char* CommandWalkerClass::GetDerivedRttiName() const
{
    return rttiNameCommandWalkerClass;
}

// FUNCTION: SWBF2 0x0047fd40
inline bool CommandWalker::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandWalker == uRtti)
    {
        return true;
    }
    return EntityWalker::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0047fd90
inline unsigned int CommandWalker::GetDerivedRtti() const
{
    return rttiHashCommandWalker;
}

// FUNCTION: SWBF2 0x0047fda0
inline const char* CommandWalker::GetDerivedRttiName() const
{
    return "CommandWalker";
}


