#pragma once

#include <stddef.h>

#include "SoundFL/source/sndbufferio.h"
#include "SoundFL/source/sndstreampacketio.h"
#include "SoundFL/source/sndstreamnode.h"
#include "SoundFL/source/sndstreamsourceproducer.h"
#include "snddsdmo.h"

namespace Snd
{

class StreamFormat;

// SYNTHETIC: SWBF2 0x0074d820
// Snd::StreamDSDMO::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786d60
class StreamDSDMO : public StreamNode, public StreamSourceProducer, public StreamNodeSink
{
public:
    StreamDSDMO();
    virtual ~StreamDSDMO();

    virtual StreamPacket* GetPacket(unsigned int uiOutput);
    bool CreateDMO(const _GUID& rCLSID, const StreamFormat& rIn,
                   const StreamFormat& rOut);
    unsigned int GetInputAligment() const;
    unsigned int GetMinInputSize() const;
    void Flush();

    void SetInputBuffer(void* pBuffer, unsigned int uiSize);

    void Update();

    DSDMO mDMO;

    StreamPacketIO mInputPacket;
    BufferIO mBuffer;
    StreamPacketIOLite mOutputPacket;
    unsigned int mAlignment;
    unsigned int mMinSize;
    unsigned int mAudioFrameSize;
    unsigned int mInputFrameSize;
    unsigned char mFlags;
};

 
 
 
 
 
 
 
 

}
