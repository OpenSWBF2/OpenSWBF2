#pragma once

#include <stddef.h>

#include "sndstreamnode.h"
#include "sndstreamsourceproducer.h"
#include "win/sndsoftoutput.h"

namespace Snd
{

// SYNTHETIC: SWBF2 0x0074c820
// Snd::StreamSourceMemory::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786d00
class StreamSourceMemory : public StreamNode, public StreamSourceProducer
{
public:

    class SegmentProperties
    {
    public:
        SegmentProperties()
            : mOffset(0),
              mLength(0)
        {
        }

        unsigned int mOffset;
        unsigned int mLength;
    };

    StreamSourceMemory();
    virtual ~StreamSourceMemory();

    virtual StreamPacket* GetPacket(unsigned int uiOutput);

    void Update();

    void Setup(const StreamFormat& rFormat, void* const pData, unsigned int uiDataSize,
               unsigned int uiPacketSize, StreamPacket* pPackets, unsigned char uiNumPackets,
               bool bLooping);
    void SetLooping(unsigned char bLooping);
    void Flush()
    {
        StartFlush();
        StreamSourceProducer::Flush();
        EndFlush();
    }

private:
    StreamPacket* GetPacket(unsigned int uiOutput, bool bUpdate);

public:

    SegmentProperties mSegment;
    SegmentProperties mNextSegment;
    void* mData;
    unsigned int mDataSize;
    unsigned int mAudioFrameSize;
    StreamFormat mFormat;
    bool mLooping;
};

 
 
 
 
 

}
