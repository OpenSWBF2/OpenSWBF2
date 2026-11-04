#include "sndsource2d3d.h"
#include "PebbleFL/Common/PblFile.h"
#include "PebbleFL/Common/PblChunk.h"

namespace Snd
{

// FUNCTION: SWBF2 0x0074b2a0
float Source2D3D::CalculatePitch(const Source3D* pSource3D) const
{
    if (Get3DMode())
    {
        const float fPitch = mPitch;
        return pSource3D->CalculatePitch() * fPitch;
    }
    return mPitch;
}

// FUNCTION: SWBF2 0x0074b2d0
bool Source2D3D::Read(PblFileChunk& chunk, unsigned int uiPropertyID)
{
    bool bResult = true;
    if (uiPropertyID != 0x1A8B16B5u)
    {
        bResult = Source::Read(chunk, uiPropertyID);
        if (!bResult)
        {
            bResult = Source2D::Read(chunk, uiPropertyID);
        }
        if (!bResult)
        {
            bResult = Source3D::Properties::Read(chunk, uiPropertyID);
        }
    }
    else
    {
        chunk._pFile->Read8();
        Set3DMode(chunk._pFile->ReadFloat() > 0.0f);
    }
    return bResult;
}

}
