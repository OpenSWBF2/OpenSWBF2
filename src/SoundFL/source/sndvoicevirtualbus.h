#pragma once

#include "sndsourcebus.h"
#include "sndvoicevirtual.h"

namespace Snd
{

class VoiceVirtualBus : public VoiceVirtual, public SourceBus
{
public:
    VoiceVirtualBus(unsigned int uiCustomFlags, VoiceVirtualManager* pManager,
                    bool bRestorePlaybackPos)
        : VoiceVirtual(uiCustomFlags, pManager, bRestorePlaybackPos)
    {
    }

    // FUNCTION: SWBF2 0x0059b540
    void SetGain(float fGain)
    {
        SourceBus::SetGain((Source2D3D*)this, fGain);
    }

    float GetGain() const { return SourceBus::GetGain(); }

    void Update()
    {
        SourceBus::Update((Source2D3D*)this, this);
    }
};

 

}
