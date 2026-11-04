#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "FoleyFXOrdnance.h"

// VTABLE: SWBF2 0x0079f280
class FoleyFXSoldier : public FoleyFXImpact
{
public:
    static void* operator new(unsigned int) { return smFreeList.Alloc(); }
    static void operator delete(void* p) { smFreeList.Free(p); }

    FoleyFXSoldier();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~FoleyFXSoldier();
    virtual bool Read(PblFileChunk& chunk, unsigned int uChunkID);

    GameSound mFootstepLeftWalk;
    GameSound mFootstepRightWalk;
    GameSound mFootstepLeftRun;
    GameSound mFootstepRightRun;
    GameSound mFootstepLeftStop;
    GameSound mFootstepRightStop;
    GameSound mJump;
    GameSound mLand;
    GameSound mRoll;
    GameSound mProne;
    GameSound mSquat;
    GameSound mStand;
    GameSound mBodyFall;

    static PblHash rttiHashFoleyFXSoldier;
    static const char rttiNameFoleyFXSoldier[];
    static Snd::FreeList smFreeList;
};
 
 
 

// FUNCTION: SWBF2 0x0052cc40
inline bool FoleyFXSoldier::IsRtti(unsigned int uRtti) const
{
    if (rttiHashFoleyFXSoldier == uRtti)
    {
        return true;
    }
    return FoleyFX::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0052cc70
inline unsigned int FoleyFXSoldier::GetDerivedRtti() const
{
    return rttiHashFoleyFXSoldier;
}

// FUNCTION: SWBF2 0x0052cc80
inline const char* FoleyFXSoldier::GetDerivedRttiName() const
{
    return rttiNameFoleyFXSoldier;
}
