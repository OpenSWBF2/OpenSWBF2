#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntitySound.h"
#include "GameSound.h"
#include "Thread.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x0079d408
class EntitySoundStaticClass : public EntitySoundClass
{
public:
    EntitySoundStaticClass(unsigned int uHash);
    EntitySoundStaticClass(EntitySoundStaticClass* pBase, unsigned int uHash);
    virtual ~EntitySoundStaticClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static PblHash rttiHashEntitySoundStaticClass;
    static const char rttiNameEntitySoundStaticClass[];
};
  

// VTABLE: SWBF2 0x0079d3d0 EntitySound
// VTABLE: SWBF2 0x0079d3b8 Thread
class EntitySoundStatic : public EntitySound, public Thread
{
public:
    EntitySoundStatic(EntitySoundStaticClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~EntitySoundStatic();
    virtual void Play();
    virtual bool Update(float fDeltaTime);

    static void* operator new(size_t uSize);

    // SYNTHETIC: SWBF2 0x004fb470
    // EntitySoundStatic::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    static PblHash rttiHashEntitySoundStatic;
    static const char rttiNameEntitySoundStatic[];
    static MemoryPool sMemoryPool;

    GameSoundControllable mSound;
    GameSound mSoundProperties;
};
 
 
 

// FUNCTION: SWBF2 0x004fb480
inline bool EntitySoundStaticClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntitySoundStaticClass == uRtti)
    {
        return true;
    }
    return EntitySoundClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004fb4b0
inline unsigned int EntitySoundStaticClass::GetDerivedRtti() const
{
    return rttiHashEntitySoundStaticClass;
}

// FUNCTION: SWBF2 0x004fb4c0
inline const char* EntitySoundStaticClass::GetDerivedRttiName() const
{
    return rttiNameEntitySoundStaticClass;
}

// FUNCTION: SWBF2 0x004fb420
inline bool EntitySoundStatic::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntitySoundStatic == uRtti)
    {
        return true;
    }
    return Entity::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004fb450
inline unsigned int EntitySoundStatic::GetDerivedRtti() const
{
    return rttiHashEntitySoundStatic;
}

// FUNCTION: SWBF2 0x004fb460
inline const char* EntitySoundStatic::GetDerivedRttiName() const
{
    return rttiNameEntitySoundStatic;
}


