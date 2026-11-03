#pragma once

#include <stddef.h>

#include "sndsourcetransport.h"
#include "sndstreamnode.h"
#include "sndstreampacket.h"

namespace Snd
{

class StreamFormat;

// SYNTHETIC: SWBF2 0x0074c030
// Snd::StreamCaptureSourceBase::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786cd0
class StreamCaptureSourceBase : public StreamNode,
                                public StreamNodeSource
{
public:
    enum Flags
    {
        flags_open = 1
    };

    StreamCaptureSourceBase();
    virtual ~StreamCaptureSourceBase();
    void Close(bool bFlush);
    void Flush();
    virtual StreamPacket* GetPacket(unsigned int uiOutput);
    bool Open(unsigned char ucDeviceIndex, StreamFormat const& cFormat,
              unsigned int uiPacketSize, unsigned char ucNumPackets,
              StreamPacket* pPackets);

protected:
    SourceTransport::PlayState Update(bool bHaveDevice);

public:
    unsigned int mPacketSize;
    StreamPacket* mPackets;
    unsigned char mNumPackets;
    unsigned char mReadPacketIndex;
    unsigned char mLockPacketIndex;
    unsigned char mFlags;
    float mGain;
};
 
 
 
 
 
 
 
 

}
