#pragma once

#include <stddef.h>

#include "RedEngineFL/Memory/RedAlign.h"
#include "SoundFL/source/sndstreamgsmdecoder.h"
#include "SoundFL/source/sndstreamnode.h"
#include "SoundFL/source/sndstreampacket.h"
#include "SoundFL/source/sndstreampacketio.h"
#include "SoundFL/source/sndstreamsourceproducer.h"
#include "VoiceChatNode.h"
#include "VoiceChatUtil.h"

namespace VoiceChatLayout
{

inline unsigned int AlignedBytes(unsigned int uiBytes)
{
    return (uiBytes + 3u) & 0xFFFFFFFCu;
}

inline size_t AlignedOffset(const char* pcBlock, size_t uOffset)
{
    return (size_t)(RedAlignUp(pcBlock + uOffset, alignof(void*)) - pcBlock);
}

struct DecoderCarve
{
    size_t uProducerPackets;
    size_t uDecoders;
    size_t uDecoderBuffer;
    size_t uDecoderPackets;
    size_t uMixerConnections;
    size_t uMixerPacketIO;
    size_t uMixerBufferOffsets;
    size_t uMixerAccBuffer;
    size_t uMixerPackets;
    size_t uMixerOutBuffer;
};

inline unsigned int DecoderBlockSize(unsigned int uiSlots, unsigned int uiSamples)
{
    const unsigned int uiAligned2 = AlignedBytes(uiSamples * 2u);
    const unsigned int uiSlotBytes = sizeof(Snd::StreamSourceProducer) + sizeof(Snd::StreamPacket) * 2u
                                   + sizeof(Snd::StreamGSMDecoder) + sizeof(Snd::StreamNodeConnection)
                                   + sizeof(Snd::StreamPacketIO) + sizeof(unsigned int);
    return (uiAligned2 + uiSlotBytes) * uiSlots + sizeof(Snd::StreamPacket) * 2u + alignof(void*) * 3u
         + AlignedBytes(uiSamples * 4u) + uiAligned2 * 2u;
}

inline DecoderCarve CarveDecoder(const char* pcBlock, unsigned int uiSlots, unsigned int uiSamples)
{
    const unsigned int uiAligned2 = AlignedBytes(uiSamples * 2u);
    DecoderCarve kCarve;
    kCarve.uProducerPackets = uiSlots * sizeof(Snd::StreamSourceProducer);
    kCarve.uDecoders = kCarve.uProducerPackets + uiSlots * sizeof(Snd::StreamPacket);
    kCarve.uDecoderBuffer = kCarve.uDecoders + uiSlots * sizeof(Snd::StreamGSMDecoder);
    kCarve.uDecoderPackets = AlignedOffset(pcBlock, kCarve.uDecoderBuffer + uiAligned2 * uiSlots);
    kCarve.uMixerConnections = kCarve.uDecoderPackets + uiSlots * sizeof(Snd::StreamPacket);
    kCarve.uMixerPacketIO = kCarve.uMixerConnections + uiSlots * sizeof(Snd::StreamNodeConnection);
    kCarve.uMixerBufferOffsets = kCarve.uMixerPacketIO + uiSlots * sizeof(Snd::StreamPacketIO);
    kCarve.uMixerAccBuffer = AlignedOffset(pcBlock, kCarve.uMixerBufferOffsets + uiSlots * sizeof(unsigned int));
    kCarve.uMixerPackets = AlignedOffset(pcBlock, kCarve.uMixerAccBuffer + AlignedBytes(uiSamples * 4u));
    kCarve.uMixerOutBuffer = kCarve.uMixerPackets + sizeof(Snd::StreamPacket) * 2u;
    return kCarve;
}

struct EncoderCarve
{
    size_t uEncoderBuffer;
    size_t uCapturePackets;
    size_t uEncoderPackets;
    size_t uAudioPacket;
};

inline unsigned int EncoderBlockSize(unsigned int uiSamples, unsigned int uiEncodedBytes)
{
    return AlignedBytes(uiEncodedBytes) + AlignedBytes(uiSamples * 2u) * 2u + sizeof(Snd::StreamPacket) * 3u
         + sizeof(VoiceChat::AudioPacket) + alignof(void*);
}

inline EncoderCarve CarveEncoder(const char* pcBlock, unsigned int uiSamples, unsigned int uiEncodedBytes)
{
    EncoderCarve kCarve;
    kCarve.uEncoderBuffer = AlignedBytes(uiSamples * 2u) * 2u;
    kCarve.uCapturePackets = AlignedOffset(pcBlock, kCarve.uEncoderBuffer + AlignedBytes(uiEncodedBytes));
    kCarve.uEncoderPackets = kCarve.uCapturePackets + sizeof(Snd::StreamPacket) * 2u;
    kCarve.uAudioPacket = kCarve.uEncoderPackets + sizeof(Snd::StreamPacket);
    return kCarve;
}

struct NodeStaticCarve
{
    size_t uWriteBitmap;
    size_t uReceiveMetrics;
    size_t uSendMetrics;
    unsigned int uiClearBytes;
};

inline unsigned int NodeStaticBlockSize(unsigned int uiNodes, unsigned int uiBitmapBytes)
{
    return uiNodes * (sizeof(VoiceChat::Node::ReceiveMetrics) + sizeof(VoiceChat::Node::SendMetrics))
         + AlignedBytes(uiBitmapBytes) * 2u;
}

inline NodeStaticCarve CarveNodeStatic(unsigned int uiNodes, unsigned int uiBitmapBytes)
{
    NodeStaticCarve kCarve;
    kCarve.uWriteBitmap = AlignedBytes(uiBitmapBytes);
    kCarve.uReceiveMetrics = kCarve.uWriteBitmap + AlignedBytes(uiBitmapBytes);
    kCarve.uSendMetrics = kCarve.uReceiveMetrics + uiNodes * sizeof(VoiceChat::Node::ReceiveMetrics);
    kCarve.uiClearBytes = (unsigned int)(kCarve.uSendMetrics + uiNodes * sizeof(VoiceChat::Node::SendMetrics));
    return kCarve;
}

struct NodeCarve
{
    size_t uPackets;
    size_t uData;
    size_t uAllocatedInputs;
};

inline unsigned int NodeBlockSize(unsigned int uiNodes, unsigned int uiPackets, unsigned int uiBytes)
{
    return ((AlignedBytes(uiBytes) + sizeof(VoiceChat::AudioPacket)) * uiPackets + sizeof(VoiceChat::Node::ReceiveStream) + 4u)
             * uiNodes
         + 0x0Cu;
}

inline NodeCarve CarveNode(const char* pcBlock, unsigned int uiNodes, unsigned int uiPackets, unsigned int uiBytes)
{
    NodeCarve kCarve;
    kCarve.uPackets = uiNodes * sizeof(VoiceChat::Node::ReceiveStream);
    kCarve.uData = (size_t)(RedAlignUp(pcBlock + kCarve.uPackets + uiPackets * uiNodes * sizeof(VoiceChat::AudioPacket), 4u) - pcBlock);
    kCarve.uAllocatedInputs = kCarve.uData + AlignedBytes(uiBytes) * uiPackets * uiNodes;
    return kCarve;
}

template <class T>
constexpr size_t StorageBytes()
{
    return sizeof(T) + alignof(T);
}

template <class T>
unsigned char* StorageSlot(unsigned char* pStorage)
{
    return (unsigned char*)RedAlignUp(pStorage, alignof(T));
}

}
