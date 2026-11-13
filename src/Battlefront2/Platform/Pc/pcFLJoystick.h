#pragma once

#include "Battlefront2/Source/FLJoystick.h"

extern const int RedKeyTo_BindID[144];

extern bool gInvertMouse;

unsigned short __cdecl RawInputToActionKey(unsigned short usRawInput);
unsigned char __cdecl ActionKeyToRawInput(unsigned short usActionKey);
