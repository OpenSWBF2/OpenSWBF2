#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "FoleyFX.h"
#include "GameSound.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "SoundFL/source/sndfreelist.h"

// VTABLE: SWBF2 0x0079f240
class FoleyFXImpact : public FoleyFX
{
public:
    static void* operator new(size_t) { return smFreeList.Alloc(); }
    static void operator delete(void* p) { smFreeList.Free(p); }

    FoleyFXImpact();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~FoleyFXImpact();
    virtual bool Read(PblFileChunk& chunk, unsigned int uChunkID);

    GameSound mImpact;

    static PblHash rttiHashFoleyFXImpact;
    static const char rttiNameFoleyFXImpact[];
    static Snd::FreeList smFreeList;
};
 
 

// FUNCTION: SWBF2 0x0052c9e0
inline bool FoleyFXImpact::IsRtti(unsigned int uRtti) const
{
    if (rttiHashFoleyFXImpact == uRtti)
    {
        return true;
    }
    return FoleyFX::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0052ca10
inline unsigned int FoleyFXImpact::GetDerivedRtti() const
{
    return rttiHashFoleyFXImpact;
}

// FUNCTION: SWBF2 0x0052ca20
inline const char* FoleyFXImpact::GetDerivedRttiName() const
{
    return rttiNameFoleyFXImpact;
}
