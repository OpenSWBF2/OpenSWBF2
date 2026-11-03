#pragma once

#include <stddef.h>

#include "sndqueue.h"
#include "sndstreamnode.h"
#include "sndstreampacket.h"

namespace Snd
{

// SYNTHETIC: SWBF2 0x007420b0
// Snd::StreamVAD::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786a80
class StreamVAD : public StreamNode,
                  public StreamNodeSource,
                  public StreamNodeSink
{
public:
    StreamVAD();
    virtual ~StreamVAD();
    virtual StreamPacket* GetPacket(unsigned int uiOutput);
    void Update();
    void Flush();

    Queue<StreamPacket*, 4> mPacketQueue;
    unsigned int mSampleRate;
    unsigned int mReleaseTimeBytes;
    unsigned int mReleaseTimeBytesRemaining;
    bool mActive;
};
 
 
 
 
 

}
