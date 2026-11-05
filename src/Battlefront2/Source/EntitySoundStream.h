#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntitySound.h"
#include "Thread.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "SoundFL/source/sndsoundstream.h"

#include "RedEngineFL/Memory/RedMemoryPool.h"

// VTABLE: SWBF2 0x0079d458
class EntitySoundStreamClass : public EntitySoundClass
{
public:
    EntitySoundStreamClass(unsigned int uHash);
    EntitySoundStreamClass(EntitySoundStreamClass* pBase, unsigned int uHash);
    virtual ~EntitySoundStreamClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static PblHash rttiHashEntitySoundStreamClass;
    static const char rttiNameEntitySoundStreamClass[];
};
  

// VTABLE: SWBF2 0x0079d47c EntitySound
// VTABLE: SWBF2 0x0079d4b4 Thread
class EntitySoundStream : public EntitySound, public Thread
{
public:
    EntitySoundStream(EntitySoundStreamClass* pClass, const EntityDesc& desc);
    virtual ~EntitySoundStream();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void Play();
    virtual bool Update(float fDeltaTime);

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x004fba00
    // EntitySoundStream::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    static PblHash rttiHashEntitySoundStream;
    static MemoryPool sMemoryPool;
    static const char rttiNameEntitySoundStream[];

    Snd::SoundStream mSoundStream;
};
 
 

// FUNCTION: SWBF2 0x004fba10
inline bool EntitySoundStreamClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntitySoundStreamClass == uRtti)
    {
        return true;
    }
    return EntitySoundClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004fba40
inline unsigned int EntitySoundStreamClass::GetDerivedRtti() const
{
    return rttiHashEntitySoundStreamClass;
}

// FUNCTION: SWBF2 0x004fba50
inline const char* EntitySoundStreamClass::GetDerivedRttiName() const
{
    return rttiNameEntitySoundStreamClass;
}

// FUNCTION: SWBF2 0x004fb9b0
inline bool EntitySoundStream::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntitySoundStream == uRtti)
    {
        return true;
    }
    return Entity::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004fb9e0
inline unsigned int EntitySoundStream::GetDerivedRtti() const
{
    return rttiHashEntitySoundStream;
}

// FUNCTION: SWBF2 0x004fb9f0
inline const char* EntitySoundStream::GetDerivedRttiName() const
{
    return rttiNameEntitySoundStream;
}


