#pragma once

#include "sndstreamnode.h"
#include "sndstreampacket.h"

namespace Snd
{

// VTABLE: SWBF2 0x00786c30
class SubstreamSplitter : public StreamNode,
                          public StreamNodeSource,
                          public StreamNodeSink
{
public:
    SubstreamSplitter();

    virtual ~SubstreamSplitter();

    virtual StreamPacket* GetPacket(unsigned int uiSubstream);
    void Flush();
    void Update();

    void Setup(unsigned int uiNumSubstreams, unsigned int uiSubstreamInterleave,
               StreamPacket* pPackets, unsigned short* pReadPos,
               unsigned short* pLockPos, unsigned int uiNumPackets);

protected:
    bool IsQueueFull() const;

private:

    static void __cdecl PacketCompleteCallback(StreamPacket* const pPacket,
                                               void* const pData);

public:

    unsigned int mNumPackets;
    unsigned int mPacketsPerSubstream;
    StreamPacket* mPackets;
    unsigned int mNumSubstreams;
    unsigned int mSubstreamInterleave;
    unsigned int mSubstreamBlockSize;
    unsigned short* mReadPos;
    unsigned short* mLockPos;
};

 

}
