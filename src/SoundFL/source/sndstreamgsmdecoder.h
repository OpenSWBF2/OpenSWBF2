#pragma once

#include <stddef.h>

#include "sndgsm.h"
#include "sndstreamcodec.h"

namespace Snd
{

// SYNTHETIC: SWBF2 0x0073d140
// Snd::StreamGSMDecoder::`scalar deleting destructor'

// VTABLE: SWBF2 0x007868a8
class StreamGSMDecoder : public StreamCodec
{
public:
    StreamGSMDecoder();

    virtual ~StreamGSMDecoder() {}
    // FUNCTION: SWBF2 0x0073d250
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
    void Flush();
    void Update();
    void SetPackets(unsigned int uiPacketSize, StreamPacket* pPackets,
                    unsigned int uiNumPackets);

    GSM mState;
    StreamPacket* mInputPacket;
    unsigned int mInputPacketOffset;
    unsigned int mInputPacketSize;
    unsigned int mOutputPacketOffset;
    short (*mFrame)[160];
    unsigned int mFrameOffset;
    bool mDecimate : 1;
};

 
 
 
 
 

}
