#include "ProfileCRC.h"

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
    LOG(LogLoad, Warning, "OPENSWBF2 SAVE profile CRC mismatch \"%s.profile\"", acName);
    return false;
}
