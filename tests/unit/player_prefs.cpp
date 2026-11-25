#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string.h>

#include "Battlefront2/Source/PlayerProfile.h"

extern PlayerProfile::tProfile mDefaultProfile;

TEST_CASE_NAMED(FPlayerPrefsTest1, "Unit::PlayerPrefs::player prefs write the second profile by name", "[EngineFilter]")
{
    PlayerPrefs::OnePrefs akSaved[2];
    memcpy(akSaved, PlayerPrefs::mAllUserPrefs, sizeof(akSaved));
    unsigned char acSavedDefault[sizeof(PlayerProfile::tProfile)];
    memcpy(acSavedDefault, &mDefaultProfile, sizeof(acSavedDefault));

    memset(&mDefaultProfile, 0xA7, sizeof(PlayerProfile::tProfile));
    PlayerPrefs::mAllUserPrefs[0].pProfile = 0;
    PlayerPrefs::mAllUserPrefs[1].pProfile = 0;
    PlayerPrefs::InitAll();

    REQUIRE(PlayerProfile::GetProfile(0) != (PlayerProfile::tProfile*)0);
    REQUIRE(PlayerProfile::GetProfile(1) != (PlayerProfile::tProfile*)0);
    CHECK(PlayerProfile::GetProfile(0) != PlayerProfile::GetProfile(1));
    CHECK(PlayerPrefs::mAllUserPrefs[0].pProfile == PlayerProfile::GetProfile(0));
    CHECK(PlayerPrefs::mAllUserPrefs[1].pProfile == PlayerProfile::GetProfile(1));
    const unsigned char* pcDefault = (const unsigned char*)&mDefaultProfile;
    unsigned int uChanged = 0;
    for (size_t i = 0; i < sizeof(PlayerProfile::tProfile); ++i)
    {
        if (pcDefault[i] != 0xA7)
        {
            ++uChanged;
        }
    }
    CHECK(uChanged == 0);

    memcpy(&mDefaultProfile, acSavedDefault, sizeof(acSavedDefault));
    memcpy(PlayerPrefs::mAllUserPrefs, akSaved, sizeof(akSaved));
}
