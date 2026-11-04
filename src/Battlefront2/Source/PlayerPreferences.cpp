#include "PlayerProfile.h"

namespace PlayerPrefs
{

// FUNCTION: SWBF2 0x0061c4b0
int __fastcall FindByViewport(int iValue)
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
