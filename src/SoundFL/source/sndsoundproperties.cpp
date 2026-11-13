#include "sndsoundproperties.h"

#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblFile.h"

namespace Snd
{

// FUNCTION: SWBF2 0x0074b750
bool SoundProperties::Read(PblFileChunk& chunk, unsigned int uiPropertyID)
{
    switch (uiPropertyID)
    {
    case 0x5FB91E8Cu: /* "Group" */
        chunk._pFile->Read8();
        mGroupID = chunk._pFile->Read32();
        return true;
    case 0x5FD56DF9u: /* "bus" */
    {
        chunk._pFile->Read8();
        Bus* const pBus = Bus::FindByHashID(chunk._pFile->Read32());
        if (pBus)
        {
            mBus = pBus;
        }
        return true;
    }
    case 0x8D39BDE6u: /* "name" */
        chunk._pFile->Read8();
        mHashID = chunk._pFile->Read32();
        return true;
    }
    return false;
}

}
