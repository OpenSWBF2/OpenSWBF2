#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CommandPost.h"
#include "EntityHover.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class MemoryPool;

// VTABLE: SWBF2 0x007983a0 EntityHoverClass
// VTABLE: SWBF2 0x00798580 CommandPostClass
class CommandHoverClass : public EntityHoverClass, public CommandPostClass
{
public:
    CommandHoverClass(unsigned int uHash);

    CommandHoverClass(CommandHoverClass* pBase, unsigned int uHash);

    virtual ~CommandHoverClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static unsigned int GetRtti();

    static PblHash rttiHashCommandHoverClass;
    static const char rttiNameCommandHoverClass[];
};

   
 

// VTABLE: SWBF2 0x00798214 Thread
// VTABLE: SWBF2 0x007982e4 RedSceneObject
// VTABLE: SWBF2 0x0079834c CollisionObject
// VTABLE: SWBF2 0x007983ec Damageable
// VTABLE: SWBF2 0x00798418 Trackable
// VTABLE: SWBF2 0x00798460 EntityEx
// VTABLE: SWBF2 0x0079858c PblHandled
class CommandHover : public EntityHover
{
public:

    CommandHover(CommandHoverClass* pClass, const EntityDesc& desc);

    virtual ~CommandHover();

    static void* operator new(unsigned int uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x00478ee0
    // CommandHover::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    virtual bool IsRtti(unsigned int uRtti) const;
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
    virtual void Kill();
    virtual void Respawn();
    virtual GameObject* AddToClient();

    CommandHoverClass* mClass;
    CommandPost* mPost;
    char _tailPad[0x8];

    static MemoryPool sMemoryPool;
    static PblHash rttiHashCommandHover;
    static const char rttiNameCommandHover[];
};
 
 
 

// FUNCTION: SWBF2 0x00478e70
inline bool CommandHover::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandHover == uRtti)
    {
        return true;
    }
    return EntityHover::rttiHashEntityHover == uRtti ||
           GameObject::rttiHashGameObject == uRtti ||
           EntityGeometry::rttiHashEntityGeometry == uRtti ||
           EntityEx::rttiHashEntityEx == uRtti ||
           Entity::rttiHashEntity == uRtti;
}

// FUNCTION: SWBF2 0x00478ec0
inline unsigned int CommandHover::GetDerivedRtti() const
{
    return rttiHashCommandHover;
}

// FUNCTION: SWBF2 0x00478ed0
inline const char* CommandHover::GetDerivedRttiName() const
{
    return rttiNameCommandHover;
}

// FUNCTION: SWBF2 0x00478f30
inline bool CommandHoverClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandHoverClass == uRtti)
    {
        return true;
    }
    return EntityHoverClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00478f80
inline unsigned int CommandHoverClass::GetDerivedRtti() const
{
    return rttiHashCommandHoverClass;
}

// FUNCTION: SWBF2 0x00478f90
inline const char* CommandHoverClass::GetDerivedRttiName() const
{
    return rttiNameCommandHoverClass;
}


