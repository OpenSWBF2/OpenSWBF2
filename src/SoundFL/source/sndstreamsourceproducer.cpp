#include <new>

#include "sndstreamsourceproducer.h"

namespace Snd
{

// FUNCTION: SWBF2 0x0073c740
StreamSourceNProducer::StreamSourceNProducer()
    : mProducers(0), mNumProducers(0)
{
}

// FUNCTION: SWBF2 0x0073c760
StreamSourceProducer::StreamSourceProducer()
{
    mPackets = 0;
    mNumPackets = 0;
    mPacketSize = 0;
    mWriteIndex = 0;
    mReadIndex = 0;
}

// FUNCTION: SWBF2 0x0073c790
StreamNodeSource::~StreamNodeSource()
{
}

// FUNCTION: SWBF2 0x0073c7a0
StreamSourceNProducer::~StreamSourceNProducer()
{
}

// FUNCTION: SWBF2 0x0073c7b0
StreamSourceProducer::~StreamSourceProducer()
{

    Flush();
}

// FUNCTION: SWBF2 0x0073cb10
void Snd::StreamSourceProducer::SetPackets(unsigned int uiPacketSize, StreamPacket* pPackets, unsigned int uiNumPackets, int iUnread)
{
    (void)iUnread;

    if (uiPacketSize == 0 || uiNumPackets == 0 || pPackets == 0)
    {
        mPackets = 0;
        mNumPackets = 0;
        mPacketSize = 0;
        return;
    }

    mPackets = pPackets;
    mNumPackets = uiNumPackets;
    mPacketSize = uiPacketSize;

    for (unsigned int i = 0; i < mNumPackets; ++i)
    {
        pPackets[i].mBufferUsed = 0;
    }
}

// FUNCTION: SWBF2 0x0073cb80
void StreamSourceProducer::SetPackets(unsigned int uiPacketSize,
                                      StreamPacket* pPackets,
                                      unsigned int uiNumPackets)
{
    SetPackets(uiPacketSize, pPackets, uiNumPackets, 1);
}

// FUNCTION: SWBF2 0x0073cba0
void StreamSourceNProducer::SetProducers(StreamSourceProducer* pProducers,
                                         unsigned int uiNumProducers)
{
    if (pProducers == 0 || uiNumProducers == 0)
    {
        mProducers = 0;
        mNumProducers = 0;
        return;
    }

    mProducers = pProducers;
    mNumProducers = uiNumProducers;
}

// FUNCTION: SWBF2 0x0073c8e0
void StreamSourceProducer::Flush()
{
    for (;;)
    {

        StreamPacket* const pPacket = GetPacket(0);

        if (pPacket == 0)
        {
            break;
        }

        pPacket->Unlock();
    }
}

// FUNCTION: SWBF2 0x0073c9a0
StreamPacket* StreamSourceProducer::GetPacket(unsigned int uiOutput)
{
    (void)uiOutput;

    StreamPacket* const pPackets = mPackets;
    if (pPackets == 0)
    {
        return 0;
    }

    unsigned int& uiRead = mReadIndex;
    StreamPacket* const pPacket = &pPackets[uiRead];

    if (pPacket->mRefCount != 0)
    {
        return 0;
    }
    if (pPacket->mBufferUsed == 0)
    {
        return 0;
    }

    ++pPacket->mRefCount;

    ++uiRead;
    if (uiRead == mNumPackets)
    {
        uiRead = 0;
    }

    return pPacket;
}

// FUNCTION: SWBF2 0x0073c930
unsigned int StreamSourceProducer::GetNumQueuedPackets() const
{
    const StreamPacket* const pPackets = mPackets;
    const unsigned int uiNumPackets = mNumPackets;
    unsigned int uiIndex = mWriteIndex;
    unsigned int uiQueued = 0;

    for (unsigned int i = uiNumPackets; i != 0; --i)
    {
        const StreamPacket& packet = pPackets[uiIndex++];
        if (packet.mRefCount != 0 || packet.mBufferUsed != 0)
        {
            ++uiQueued;
        }
        if (uiIndex == uiNumPackets)
        {
            uiIndex = 0;
        }
    }
    return uiQueued;
}

// FUNCTION: SWBF2 0x0073c970
StreamPacket* StreamNodeSource::GetPacket(unsigned int uiOutput)
{
    (void)uiOutput;
    return 0;
}

// FUNCTION: SWBF2 0x0073c980
StreamPacket* StreamSourceNProducer::GetPacket(unsigned int uiOutput)
{
    StreamSourceProducer* const pOut = mProducers + uiOutput;
    return pOut->GetPacket(0);
}

// FUNCTION: SWBF2 0x0073ca10
StreamPacket* StreamSourceProducer::GetReadPacket()
{

    StreamPacket* const pPackets = mPackets;
    StreamPacket* const pPacket = &pPackets[mWriteIndex];

    if (pPacket->mBufferUsed != 0)
    {
        return 0;
    }
    if (pPacket->mRefCount != 0)
    {
        return 0;
    }

    return pPacket;
}

// FUNCTION: SWBF2 0x0073ca90
void StreamSourceProducer::PacketComplete(StreamPacket* const pPacket,
                                          unsigned int uiBufferUsed,
                                          unsigned int uiUnused,
                                          bool bFlagBit0, bool bFlagBit1,
                                          unsigned int uiPos)
{
    (void)uiUnused;

    pPacket->mBufferUsed = uiBufferUsed;
    pPacket->mPos = uiPos;

    if (bFlagBit0)
    {
        pPacket->mFlags |= 1;
    }
    else
    {
        pPacket->mFlags &= 0xFFFE;
    }

    if (bFlagBit1)
    {
        pPacket->mFlags |= 2;
    }
    else
    {
        pPacket->mFlags &= 0xFFFD;
    }

    ++mWriteIndex;
    if (mWriteIndex == mNumPackets)
    {
        mWriteIndex = 0;
    }
}

// FUNCTION: SWBF2 0x0073caf0
void StreamPacket::SetDiscontinuity(bool bDiscontinuity)
{
    if (bDiscontinuity)
    {
        mFlags |= 2;
    }
    else
    {
        mFlags &= 0xFFFD;
    }
}

}

namespace Snd {

// FUNCTION: SWBF2 0x0073c9e0
StreamPacket* StreamSourceNProducer::GetWritablePacket(unsigned int uiOutput)
{
    const StreamSourceProducer& producer = mProducers[uiOutput];
    StreamPacket* pPacket = producer.mPackets + producer.mWriteIndex;
    return pPacket->mBufferUsed == 0 && pPacket->mRefCount == 0 ? pPacket : 0;
}
}

namespace Snd {

// FUNCTION: SWBF2 0x0073ca30
unsigned int StreamSourceNProducer::CompleteWrittenPacket(StreamPacket* pPacket,
                                   unsigned int uiBufferUsed, unsigned int uiUnused,
                                   bool bFlagBit0, bool bFlagBit1, unsigned int uiPos,
                                   unsigned int uiOutput)
{
    StreamSourceProducer& producer = mProducers[uiOutput];
    pPacket->mBufferUsed = uiBufferUsed;
    pPacket->mPos = uiPos;
    if (bFlagBit0)
    {
        pPacket->mFlags |= 1;
    }
    else
    {
        pPacket->mFlags &= ~1u;
    }
    if (bFlagBit1)
    {
        pPacket->mFlags |= 2;
    }
    else
    {
        pPacket->mFlags &= ~2u;
    }
    unsigned int& rIndex = producer.mWriteIndex;
    unsigned int result = ++rIndex;
    if (result == producer.mNumPackets)
    {
        rIndex = 0;
    }
    return result;
}
}
