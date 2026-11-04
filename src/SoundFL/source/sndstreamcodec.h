#pragma once

#include "sndstreamnode.h"
#include "sndstreamsourceproducer.h"

namespace Snd
{

// SYNTHETIC: SWBF2 0x0073d110
// Snd::StreamCodec::`scalar deleting destructor'

// VTABLE: SWBF2 0x0078689c
class StreamCodec : public StreamNode,
                    public StreamNodeSink,
                    public StreamSourceProducer
{
public:

    // FUNCTION: SWBF2 0x0073d100
    virtual ~StreamCodec()
    {

    }

    // FUNCTION: SWBF2 0x0073d200
    virtual StreamPacket* GetPacket(unsigned int uiOutput)
    {
        if (StreamNode::GetCurrentPlayState() == playState_playing)
        {
            StreamPacket* const pPacket = StreamSourceProducer::GetPacket(uiOutput);
            if (pPacket != 0)
            {
                if (pPacket->GetLast())
                {
                    StreamNode::Stop();
                }
                return pPacket;
            }
        }

        return 0;
    }

protected:

    bool BufferPackets();
    bool Update();
};

 

}
