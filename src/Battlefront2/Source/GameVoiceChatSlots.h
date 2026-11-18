#pragma once

#include <string.h>
#include "SoundFL/source/sndstreamnode.h"

static_assert(sizeof(Snd::StreamNodeConnection) == 2 * sizeof(void*), "stream node connection size");

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
