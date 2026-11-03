#pragma once

#include "sndsource.h"
#include "sndsource2d.h"
#include "sndsource3d.h"

namespace Snd
{

class Source2D3D : public Source, public Source2D, public Source3D::Properties
{
public:
    enum Flags
    {
        flags_mode3D = 1,
        flags_oldMode3D = 2,
        flags_updateMode3D = 4
    };

    Source2D3D() : mFlags(0) {}

    // FUNCTION: SWBF2 0x0059b300
    void Set3DMode(bool b3DMode)
    {
        const unsigned char f = (unsigned char)(mFlags & (unsigned char)~2u);

        mFlags = (unsigned char)(f | (unsigned char)((f & 1) << 1));

        if (b3DMode)
        {
            mFlags |= 1;
        }
        else
        {
            mFlags &= (unsigned char)~1u;
        }

        mFlags |= 4;
    }

    bool Get3DMode() const { return (mFlags & 1) != 0; }

protected:

    void CopyParameters(const Source2D3D& rOther)
    {
        Set3DMode(rOther.Get3DMode());
    }

    bool Read(PblFileChunk& chunk, unsigned int uiPropertyID);

    bool Get3DModeChanged()
    {
        const bool bChanged = (mFlags & 4) != 0;
        mFlags &= (unsigned char)~4u;
        return bChanged;
    }
    float CalculatePitch(const Source3D* pSource3D) const;

private:
    unsigned char mFlags;
};

 

}
