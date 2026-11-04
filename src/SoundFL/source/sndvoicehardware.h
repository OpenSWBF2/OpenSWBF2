#pragma once

#include "sndsourcei3dl2.h"
#include "sndvoicebase.h"

namespace Snd
{

class Voice;

class VoiceHardware : public VoiceBase, public SourceI3DL2Interpolated
{
public:

    VoiceHardware() {}

protected:
    // FUNCTION: SWBF2 0x00745a80
    ~VoiceHardware() {}

    void Update(Voice* pVoice);
public:
};

 

}
