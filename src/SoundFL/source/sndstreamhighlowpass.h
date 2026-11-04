#pragma once

#include "snddsp.h"
#include "sndstreampacketio.h"
#include "sndstreamsourceproducer.h"

namespace Snd
{

// VTABLE: SWBF2 0x00786b94
class StreamLowHighPass : public StreamNode, public StreamNodeSink, public StreamSourceProducer, public DSP::HighLowPassFilter
{
public:
    StreamLowHighPass();
    virtual ~StreamLowHighPass();

    void SetBuffer(void* pBuffer, unsigned int uiSize);

    void Flush();

protected:

    virtual StreamPacket* GetPacket(unsigned int uiOutput);

public:
    unsigned int mBufferSize;
    StreamPacket mOutputPacket;
    StreamPacketIO mInputPacket;
};

 

}
