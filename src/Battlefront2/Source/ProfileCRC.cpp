#include "ProfileCRC.h"
#include "LuaSaveItems.h"

#include <stddef.h>

#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "RedEngineFL/Util/RedLog.h"

bool PlayerProfile::CheckLoadedCRC(const tProfile& kProfile, const unsigned short* pusName)
{
    tProfile kClean = kProfile;
    kClean.mIsDirty = false;
    if (CalcCRC(&kClean, offsetof(tProfile, mCRC), 0) == kProfile.mCRC)
    {
        return true;
    }
    char acName[128] = {};
    if (pusName)
    {
        PblUnicode::ustrncnv(acName, pusName, sizeof(acName) - 1);
    }
    LOG(LogLoad, Warning, "Save data profile CRC mismatch \"%s.profile\"", acName);
#ifdef OPENSWBF2_TEST
    SaveRejects::Note();
#endif
    return false;
}
