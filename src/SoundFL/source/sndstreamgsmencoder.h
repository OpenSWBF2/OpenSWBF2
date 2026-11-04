#pragma once

#include <stddef.h>

#include "sndgsm.h"
#include "sndstreamcodec.h"

namespace Snd
{
namespace DSP
{
class HighLowPassFilter;
}

// SYNTHETIC: SWBF2 0x007417f0
// Snd::StreamGSMEncoder::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786a50
class StreamGSMEncoder : public StreamCodec
{
public:
    StreamGSMEncoder();
    virtual ~StreamGSMEncoder();
    // FUNCTION: SWBF2 0x00741910
    virtual StreamPacket* GetPacket(unsigned int uiOutput)
    {
        StreamPacket* const pPacket = StreamCodec::GetPacket(uiOutput);
        if (pPacket != 0)
        {
            return pPacket;
        }
        Update();
        return StreamCodec::GetPacket(uiOutput);
    }
    void Update();
    void Flush();
    void SetPackets(unsigned int uiPacketSize, StreamPacket* pPackets,
                    unsigned int uiNumPackets);

    GSM mState;
    short mInputFrame[160];
    StreamPacket* mInputPacket;
    unsigned int mInputPacketOffset;
    unsigned int mInputPacketSize;
    unsigned int mInputFrameOffset;
    unsigned int mOutputPacketOffset;
    DSP::HighLowPassFilter* mFilter;
    bool mDecimate : 1;
};

 
 
 
 
 

}
