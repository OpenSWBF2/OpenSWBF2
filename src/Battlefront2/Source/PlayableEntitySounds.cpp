#include "PlayableEntitySounds.h"

// FUNCTION: SWBF2 0x00619b60
PlayableEntitySounds::PlayableEntitySounds(PlayableEntitySoundsClass* pClass)
    : mPESClass(pClass)
{
}

// FUNCTION: SWBF2 0x00619b80
PlayableEntitySounds::~PlayableEntitySounds()
{
}

// FUNCTION: SWBF2 0x00619b90
PlayableEntitySoundsClass::PlayableEntitySoundsClass()
{
}

// FUNCTION: SWBF2 0x00619c10
PlayableEntitySoundsClass::PlayableEntitySoundsClass(const PlayableEntitySoundsClass* pBase)
{
    for (int i = 0; i < command_count; ++i)
    {
        mAICommands[i] = pBase->mAICommands[i];
        mPlayerCommands[i] = pBase->mPlayerCommands[i];
    }
    mSCResponseYessirSound = pBase->mSCResponseYessirSound;
    mSCResponseNosirSound = pBase->mSCResponseNosirSound;
}

// FUNCTION: SWBF2 0x00619d00
bool PlayableEntitySoundsClass::SetProperty(unsigned int uProperty, const char* pszValue)
{
    switch (uProperty)
    {
    case 0x150a38e9: /* "AISCFieldMoveOutSound" */ mAICommands[command_fieldMoveOut].SetID(pszValue, false, false); return true;
    case 0x8018b09d: /* "AISCFieldHoldSound" */ mAICommands[command_fieldHold].SetID(pszValue, false, false); return true;
    case 0x1d988641: /* "AISCFieldFollowSound" */ mAICommands[command_fieldFollow].SetID(pszValue, false, false); return true;
    case 0xe0999151: /* "AISCDriverGetInSound" */ mAICommands[command_driverGetIn].SetID(pszValue, false, false); return true;
    case 0x3b62b992: /* "AISCDriverGetOutSound" */ mAICommands[command_driverGetOut].SetID(pszValue, false, false); return true;
    case 0x2512bb6d: /* "AISCPassengerMoveOutSound" */ mAICommands[command_passengerMoveOut].SetID(pszValue, false, false); return true;
    case 0xa473e5ea: /* "AISCPassengerStopSound" */ mAICommands[command_passengerStop].SetID(pszValue, false, false); return true;
    case 0xf9d92705: /* "AISCPassengerGetInSound" */ mAICommands[command_passengerGetIn].SetID(pszValue, false, false); return true;
    case 0x45912fae: /* "AISCPassengerGetOutSound" */ mAICommands[command_passengerGetOut].SetID(pszValue, false, false); return true;
    case 0x22987695: /* "AISCGunnerAllClearSound" */ mAICommands[command_gunnerAllClear].SetID(pszValue, false, false); return true;
    case 0x1aec5f13: /* "AISCGunnerSteadySound" */ mAICommands[command_gunnerSteady].SetID(pszValue, false, false); return true;
    case 0x7ef094d0: /* "AISCGunnerGetInSound" */ mAICommands[command_gunnerGetIn].SetID(pszValue, false, false); return true;
    case 0xdf526f61: /* "AISCGunnerGetOutSound" */ mAICommands[command_gunnerGetOut].SetID(pszValue, false, false); return true;

    case 0xe0e7aa4f: /* "SCFieldMoveOutSound" */ mPlayerCommands[command_fieldMoveOut].SetID(pszValue, false, false); return true;
    case 0xb2a1a19f: /* "SCFieldHoldSound" */ mPlayerCommands[command_fieldHold].SetID(pszValue, false, false); return true;
    case 0x491b3917: /* "SCFieldFollowSound" */ mPlayerCommands[command_fieldFollow].SetID(pszValue, false, false); return true;
    case 0x47041baf: /* "SCDriverGetInSound" */ mPlayerCommands[command_driverGetIn].SetID(pszValue, false, false); return true;
    case 0x4efe8d94: /* "SCDriverGetOutSound" */ mPlayerCommands[command_driverGetOut].SetID(pszValue, false, false); return true;
    case 0x5b7c85cb: /* "SCPassengerMoveOutSound" */ mPlayerCommands[command_passengerMoveOut].SetID(pszValue, false, false); return true;
    case 0x5d42f4c8: /* "SCPassengerStopSound" */ mPlayerCommands[command_passengerStop].SetID(pszValue, false, false); return true;
    case 0x6c89065f: /* "SCPassengerGetInSound" */ mPlayerCommands[command_passengerGetIn].SetID(pszValue, false, false); return true;
    case 0xf260f2a4: /* "SCPassengerGetOutSound" */ mPlayerCommands[command_passengerGetOut].SetID(pszValue, false, false); return true;
    case 0xa3d45943: /* "SCGunnerAllClearSound" */ mPlayerCommands[command_gunnerAllClear].SetID(pszValue, false, false); return true;
    case 0x981f3d5d: /* "SCGunnerSteadySound" */ mPlayerCommands[command_gunnerSteady].SetID(pszValue, false, false); return true;
    case 0xd1f9b472: /* "SCGunnerGetInSound" */ mPlayerCommands[command_gunnerGetIn].SetID(pszValue, false, false); return true;
    case 0xc27c316f: /* "SCGunnerGetOutSound" */ mPlayerCommands[command_gunnerGetOut].SetID(pszValue, false, false); return true;

    case 0x8e131444: /* "AISCResponseYessirSound" */ mSCResponseYessirSound.SetID(pszValue, false, false); return true;
    case 0x60681218: /* "AISCResponseNosirSound" */ mSCResponseNosirSound.SetID(pszValue, false, false); return true;

    default:
        return false;
    }
}
