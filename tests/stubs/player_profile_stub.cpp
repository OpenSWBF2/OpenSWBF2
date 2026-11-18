#include "Battlefront2/Source/PlayerProfile.h"

PlayerProfile::tProfile mDefaultProfile;

namespace
{
PlayerProfile::tProfile sStubProfiles[2];
}

PlayerProfile::tProfile::tProfile()
{
}

PlayerProfile::tProfile* PlayerProfile::GetProfile(unsigned int uiIndex)
{
    if (uiIndex >= sizeof(sStubProfiles) / sizeof(sStubProfiles[0]))
    {
        return 0;
    }
    return &sStubProfiles[uiIndex];
}

void PlayerProfile::ResetAllProfiles()
{
}
