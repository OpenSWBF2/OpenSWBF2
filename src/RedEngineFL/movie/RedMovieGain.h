#pragma once

#include "SoundFL/source/sndbus.h"

inline float MovieStreamGain(float fGain)
{
    if (!(fGain > 0.0f))
    {
        return 0.0f;
    }
    const int iVolume = fGain >= 1.0f ? 32767 : (int)(fGain * 32767.0f);
    return (float)iVolume / 32768.0f;
}

inline bool MovieBusGain(unsigned int uiBusHash, float* pfGain)
{
    if (uiBusHash == 0)
    {
        return false;
    }
    const Snd::Bus* const pBus = Snd::Bus::FindByHashID(uiBusHash);
    if (pBus == 0)
    {
        return false;
    }
    *pfGain = pBus->mFinalGain;
    return true;
}
