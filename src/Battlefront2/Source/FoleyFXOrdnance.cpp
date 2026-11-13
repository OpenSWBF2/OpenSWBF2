#include "FoleyFXOrdnance.h"
#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblFile.h"

// SYNTHETIC: SWBF2 0x00403ad0
// `dynamic initializer for 'FoleyFXImpact::rttiHashFoleyFXImpact''
// FoleyFXImpact::rttiHashFoleyFXImpact

// GLOBAL: SWBF2 0x01ebd01c
// FoleyFXImpact::rttiHashFoleyFXImpact
PblHash FoleyFXImpact::rttiHashFoleyFXImpact("FoleyFXImpact");
// GLOBAL: SWBF2 0x007898b8
// FoleyFXImpact::rttiNameFoleyFXImpact
const char FoleyFXImpact::rttiNameFoleyFXImpact[] = "FoleyFXImpact";

// SYNTHETIC: SWBF2 0x00403ab0
// `dynamic initializer for 'FoleyFXImpact::smFreeList''
// SYNTHETIC: SWBF2 0x007687e0
// `dynamic atexit destructor for 'FoleyFXImpact::smFreeList''

// GLOBAL: SWBF2 0x007ec188
// FoleyFXImpact::smFreeList
Snd::FreeList FoleyFXImpact::smFreeList(sizeof(FoleyFXImpact), 4, true, 8);







// FUNCTION: SWBF2 0x0052ca30
FoleyFXImpact::FoleyFXImpact()
{
}

// FUNCTION: SWBF2 0x0052ca80
FoleyFXImpact::~FoleyFXImpact()
{
}

// FUNCTION: SWBF2 0x0052ca90
bool FoleyFXImpact::Read(PblFileChunk& chunk, unsigned int uChunkID)
{
    if (uChunkID != 0x1e71b297 /* "Impact" */)
    {
        return FoleyFX::Read(chunk, uChunkID);
    }

    chunk._pFile->Read8();
    mImpact.SetID(chunk._pFile->Read32(), false, false);
    return true;
}

// SYNTHETIC: SWBF2 0x0052ca50
// FoleyFXImpact::`scalar deleting destructor'
