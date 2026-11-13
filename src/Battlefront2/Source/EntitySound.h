#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>
#include "Entity.h"
#include "RedEngineFL/Util/RedRegion.h"
#include "SoundFL/source/sndspace.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class SoundSpaceRegion : public Snd::Space::Region
{
public:

    SoundSpaceRegion(const RedRegion& rRegion) : Region(rRegion) {}

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }

    ~SoundSpaceRegion() {}

    // SYNTHETIC: SWBF2 0x004fad10
    // SoundSpaceRegion::operator delete
    static void operator delete(void* p) { sMemoryPool.Free(p); }

    static void DestroyAll();

    static void SetupMemoryPool(unsigned int uCount, unsigned int uSize, unsigned int uGrow) { sMemoryPool.Setup(uCount, uSize, uGrow); }

    static MemoryPool sMemoryPool;
};

// VTABLE: SWBF2 0x0079fbc0
class SoundSpaceRegionFactory : public RedRegionFactory
{
public:
    SoundSpaceRegionFactory() : RedRegionFactory("soundspace") {}

    // SYNTHETIC: SWBF2 0x00539300
    // SoundSpaceRegionFactory::`scalar deleting destructor'

    virtual void* CreateRegion(RedRegionDesc& desc, const char* pcName) const;
};
  

class EntitySoundOverrides
{
public:

    EntitySoundOverrides()
    {
        mMinDistance = 1.0f;
        mMaxDistance = 10.0f;
        mMuteDistance = 10.0f;
        mFlags = 0;
    }

    void SetMinDistance(float fDistance)
    {
        mFlags |= 1;
        mMinDistance = fDistance;
    }
    void SetMaxDistance(float fDistance)
    {
        mFlags |= 2;
        mMaxDistance = fDistance;
    }
    void SetMuteDistance(float fDistance)
    {
        mFlags |= 4;
        mMuteDistance = fDistance;
    }

    float mMinDistance;
    float mMaxDistance;
    float mMuteDistance;
    unsigned char mFlags;
};
 

// VTABLE: SWBF2 0x0079d368
class EntitySoundClass : public EntityClass, public EntitySoundOverrides
{
public:
    EntitySoundClass(unsigned int uHash);

    EntitySoundClass(EntitySoundClass* pBase, unsigned int uHash);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~EntitySoundClass();
    virtual void SetProperty(unsigned int uProperty, const char* pcValue);

    static PblHash rttiHashEntitySoundClass;
    static const char rttiNameEntitySoundClass[];

    static class EntitySound* CreateSoundFromRegion(RedRegionDesc& desc, const char* pcName,
                                                    const char* pcPrefix, EntitySoundClass& soundClass);

    unsigned int mSoundId;
};
 
 
 
 
 
 

// VTABLE: SWBF2 0x0079d330
class EntitySound : public Entity, public EntitySoundOverrides
{
public:
    EntitySound(EntitySoundClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~EntitySound();
    virtual void SetProperty(unsigned int uProperty, const char* pcValue);
    virtual void Play() = 0;

    void SetSoundId(unsigned int uSoundId) { mSoundId = uSoundId; }

    static void PlayAll();
    static void DestroyAll();

    static PblHash rttiHashEntitySound;
    static const char rttiNameEntitySound[];
    static PblListSingle sEntitySoundList;

    PblVector3 mPosition;
    unsigned int mSoundId;
    PblListSingle::Node mNode;
};
 
 
 
 
 

// FUNCTION: SWBF2 0x004fac70
inline bool EntitySound::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntitySound == uRtti)
    {
        return true;
    }
    return Entity::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004faca0
inline unsigned int EntitySound::GetDerivedRtti() const
{
    return rttiHashEntitySound;
}

// FUNCTION: SWBF2 0x004facb0
inline const char* EntitySound::GetDerivedRttiName() const
{
    return rttiNameEntitySound;
}

// FUNCTION: SWBF2 0x004facc0
inline bool EntitySoundClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntitySoundClass == uRtti)
    {
        return true;
    }
    return EntityClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004facf0
inline unsigned int EntitySoundClass::GetDerivedRtti() const
{
    return rttiHashEntitySoundClass;
}

// FUNCTION: SWBF2 0x004fad00
inline const char* EntitySoundClass::GetDerivedRttiName() const
{
    return rttiNameEntitySoundClass;
}
