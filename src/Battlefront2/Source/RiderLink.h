#pragma once

#ifdef OPENSWBF2_RIDERLINK

namespace RiderLink
{
void NoteSwitch();
void NoteHandsOff();
void NoteHold(bool bEveryMission);
void SetPublishDir(char const* pcDir);
void Start(bool bNoRender);
void Service();
void Stop();
void __cdecl Tap(char const* pcText);
}

#else

extern bool gLinkSwitchIgnored;

#endif
