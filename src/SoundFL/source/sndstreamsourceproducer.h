#pragma once

#include <stddef.h>
#include "sndstreamnode.h"
#include "sndstreampacket.h"

namespace Snd
{

// SYNTHETIC: SWBF2 0x0073c870
// Snd::StreamSourceProducer::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786878
class StreamSourceProducer : public StreamNodeSource
{
public:

    StreamSourceProducer();
    virtual ~StreamSourceProducer();

    virtual StreamPacket* GetPacket(unsigned int uiOutput);

    void SetPackets(unsigned int uiPacketSize, StreamPacket* pPackets,
                    unsigned int uiNumPackets);

    void SetPackets(unsigned int uiPacketSize, StreamPacket* pPackets, unsigned int uiNumPackets,
                    const bool bCheckPacketBuffers);

    void Flush();

    StreamPacket* GetReadPacket();
    void PacketComplete(StreamPacket* const pPacket, unsigned int uiBufferUsed,
                        unsigned int uiUnused, bool bFlagBit0, bool bFlagBit1,
                        unsigned int uiPos);

    unsigned int GetNumQueuedPackets() const;

protected:
    unsigned int GetPacketSize() const { return mPacketSize; }

private:
    friend class StreamSourceNProducer;

    StreamPacket* mPackets;
    unsigned int mNumPackets;
    unsigned int mPacketSize;
    unsigned int mReadPacketIndex;
    unsigned int mLockPacketIndex;
};

 
 

// SYNTHETIC: SWBF2 0x0073c840
// Snd::StreamSourceNProducer::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786884
class StreamSourceNProducer : public StreamNodeSource
{
public:
    StreamSourceNProducer();
    virtual ~StreamSourceNProducer();
    virtual StreamPacket* GetPacket(unsigned int uiOutput);
    void SetProducers(StreamSourceProducer* pProducers,
                      unsigned int uiNumProducers);

    StreamPacket* GetReadPacket(const unsigned int uiOutput) const;
    StreamSourceProducer* GetProducer(const unsigned int uiOutput) const
    {
        return mProducers + uiOutput;
    }

    void PacketComplete(StreamPacket* const pPacket, unsigned int uiBufferUsed,
                                       unsigned int uiUnused, bool bFlagBit0, bool bFlagBit1,
                                       unsigned int uiPos, unsigned int uiOutput);

    StreamSourceProducer* mProducers;
    unsigned int mNumProducers;
};

 
 
 

}
