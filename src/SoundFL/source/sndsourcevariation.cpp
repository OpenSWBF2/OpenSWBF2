#include "sndsourcevariation.h"

#include "sndglobals.h"
#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblFile.h"
#include "PebbleFL/Common/PblRandom.h"

namespace Snd
{
extern PblRandom sndRandom;
}

namespace Snd
{

// FUNCTION: SWBF2 0x00743970
SourceVariation::SourceVariation()
    : mGainDev(0.0f),
      mPitchDev(0.0f)
{
}

// FUNCTION: SWBF2 0x00743aa0
bool SourceVariation::Read(PblFileChunk& chunk, unsigned int uiPropertyID)
{
    switch (uiPropertyID)
    {
    case 0x35119EB8u:
        chunk._pFile->Read8();
        mPitchDev = chunk._pFile->ReadFloat();
        return true;
    case 0xF60B6465u:
        chunk._pFile->Read8();
        mGainDev = Clamp(chunk._pFile->ReadFloat(), 0.0f, 1.0f);
        return true;
    }
    return SourceTransportControl::Read(chunk, uiPropertyID);
}

}

namespace Snd
{

// FUNCTION: SWBF2 0x00743990
float SourceVariation::CalculateGain(float fGain) const
{
    return Clamp((sndRandom.GetFloat() - 0.5f) * mGainDev * 2.0f + fGain, 0.0f, 1.0f);
}
}

namespace Snd
{

// FUNCTION: SWBF2 0x00743a30
float SourceVariation::CalculatePitch(float fPitch) const
{
    return (sndRandom.GetFloat() - 0.5f) * mPitchDev * 2.0f + fPitch;
}
}
