#pragma once

#include "sndglobals.h"
#include "PebbleFL/Common/PblVector.h"

namespace Snd
{

template <class VOICE>
inline void ReleaseHardwareVoice(VOICE* pVoice)
{
    pVoice->Lock(false);
    pVoice->SetPosVel3D(&Globals::smNullVector, &Globals::smNullVector);
}

template <class VOICE>
inline void KeepReleasedVoicePosVel(VOICE* pVoice, ::PblVector3* pPos, ::PblVector3* pVel)
{
    *pPos = *pVoice->mPos3D;
    *pVel = *pVoice->mVel3D;
    pVoice->SetPosition(pPos);
    pVoice->SetVelocity(pVel);
    if (pVoice->mVoice)
    {
        pVoice->mVoice->SetPosVel3D(pPos, pVel);
    }
}

}
