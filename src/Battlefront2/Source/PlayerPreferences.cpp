#include "PlayerProfile.h"

#include "CommandLine.h"

extern PlayerProfile::tProfile mDefaultProfile;

namespace PlayerPrefs
{

// SYNTHETIC: SWBF2 0x00405f30
// PlayerPrefs::`dynamic initializer for 'mAllUserPrefs''

// GLOBAL: SWBF2 0x01ef7fa0
// mAllUserPrefs
OnePrefs mAllUserPrefs[1];
}

// FUNCTION: SWBF2 0x0061c410
void PlayerPrefs::OnePrefs::Clear()
{
    mPlayerId = -1;
    mViewport = 0;
    mJoystick = 0;
    mTeamNum = PlayerPrefs::gDefaultTeamNum;
    mDifficulty = (eGAME_DIFFICULTY)PlayerPrefs::gDefaultDifficulty;
    pProfile = 0;
    hasChallenge = false;
}

// FUNCTION: SWBF2 0x0061c440
void PlayerPrefs::InitAll()
{
    PlayerProfile::ResetAllProfiles();
    PlayerPrefs::mAllUserPrefs[0].Clear();

    PlayerPrefs::mAllUserPrefs[0].mViewport = 0;
    PlayerPrefs::mAllUserPrefs[0].mPlayerId = 0;
    PlayerPrefs::mAllUserPrefs[0].mJoystick = 0;

    PlayerPrefs::mAllUserPrefs[0].pProfile = PlayerProfile::GetProfile(0);
    *(PlayerProfile::tProfile**)((char*)&mDefaultProfile + 0x10) = PlayerProfile::GetProfile(1);
}

// FUNCTION: SWBF2 0x0061c490
int PlayerPrefs::FindByPlayerId(int iPlayer)
{
    for (unsigned int i = 0; i < 1; ++i)
    {
        if ((&PlayerPrefs::mAllUserPrefs[0].mPlayerId)[i * 11] == iPlayer)
        {
            return i;
        }
    }
    return -1;
}

namespace PlayerPrefs
{

// FUNCTION: SWBF2 0x0061c4b0
int FindByViewport(int iValue)
{
    for (unsigned int i = 0; i < 1; ++i)
    {
        if (PlayerPrefs::mAllUserPrefs[i].mViewport == iValue)
        {
            return i;
        }
    }
    return -1;
}

int FindByJoystick(int aJoystick)
{
    for (unsigned int i = 0; i < sizeof(mAllUserPrefs) / sizeof(mAllUserPrefs[0]); ++i)
    {
        if (mAllUserPrefs[i].mJoystick == aJoystick)
        {
            return i;
        }
    }
    return -1;
}

}
