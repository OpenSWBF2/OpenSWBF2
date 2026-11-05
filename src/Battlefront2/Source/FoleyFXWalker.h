#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "FoleyFXOrdnance.h"

// VTABLE: SWBF2 0x0079f2b4
class FoleyFXWalker : public FoleyFXImpact
{
public:
    static void* operator new(size_t) { return smFreeList.Alloc(); }
    // SYNTHETIC: SWBF2 0x0052c080
    // FoleyFXWalker::operator delete
    static void operator delete(void* p) { smFreeList.Free(p); }

    FoleyFXWalker();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~FoleyFXWalker();
    virtual bool Read(PblFileChunk& chunk, unsigned int uChunkID);

    GameSound mFootstep[6];

    static PblHash rttiHashFoleyFXWalker;
    static const char rttiNameFoleyFXWalker[];
    static Snd::FreeList smFreeList;
};
 
 

// FUNCTION: SWBF2 0x0052d000
inline bool FoleyFXWalker::IsRtti(unsigned int uRtti) const
{
    if (rttiHashFoleyFXWalker == uRtti)
    {
        return true;
    }
    return FoleyFX::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0052d030
inline unsigned int FoleyFXWalker::GetDerivedRtti() const
{
    return rttiHashFoleyFXWalker;
}

// FUNCTION: SWBF2 0x0052d040
inline const char* FoleyFXWalker::GetDerivedRttiName() const
{
    return rttiNameFoleyFXWalker;
}


