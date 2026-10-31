#pragma once

#include <string.h>
#include "SoundFL/source/sndstreamnode.h"

#ifndef _WIN64
static_assert(sizeof(Snd::StreamNodeConnection) == 8, "stream node connection size");
#endif

namespace GameVoiceChat
{

inline void ClearMixerConnection(Snd::StreamNodeConnection* pConnections, unsigned int uiSlot)
{
    if (&pConnections[uiSlot])
    {
        memset(&pConnections[uiSlot], 0, sizeof(Snd::StreamNodeConnection));
    }
}

}
