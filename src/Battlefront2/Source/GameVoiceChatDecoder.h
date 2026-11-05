#pragma once

#include <stddef.h>

#include "VoiceChatNode.h"
#include "SoundFL/source/sndstreamsourceproducer.h"
#include "SoundFL/source/sndstreammixer.h"
#include "SoundFL/source/win/sndstreamauxrenderer.h"

namespace Snd
{
class StreamGSMDecoder;
class StreamPacketIO;
struct StreamPacket;
struct StreamNodeConnection;
}

namespace GameVoiceChat
{

// SYNTHETIC: SWBF2 0x0053c780
// GameVoiceChat::Decoder::`scalar deleting destructor'

// VTABLE: SWBF2 0x007a0108
class Decoder : public VoiceChat::Decoder,
                public Snd::StreamSourceNProducer
{
public:
    Decoder();
    virtual ~Decoder();

    void Play();
    bool IsEnabled();
    bool IsEnabled(unsigned int uiDevice);
    bool Update();
    bool ReceivePacket(unsigned int uiInput, const VoiceChat::AudioPacket* pPacket);
    bool IsStarving(unsigned int uiInput, bool bForce);

    bool Open(unsigned int playbackDevice, float fPacketMs, unsigned int minBuffers, unsigned int iSlots);
    void FreeBuffers();
    void Close();
    void Flush();
    void Stop();
    bool IsPlaying();

    void SetGain(float fGain, unsigned int uiOutput);
    float GetGain(unsigned int uiOutput) const;
    unsigned int GetNumOutputs() const;

    Snd::StreamSourceProducer* mProducers;
    Snd::StreamGSMDecoder* mDecoders;
    Snd::StreamMixer mMixer;
    Snd::StreamAuxRenderer mAuxRenderer;
    Snd::StreamPacket* mProducerPackets;
    unsigned char* mDecoderBuffer;
    Snd::StreamPacket* mDecoderPackets;
    int* mMixerAccBuffer;
    short* mMixerOutBuffer;
    Snd::StreamPacket* mMixerPackets;
    Snd::StreamNodeConnection* mMixerConnections;
    Snd::StreamPacketIO* mMixerPacketIO;
    unsigned int* mMixerBufferOffsets;
    unsigned int mNumInputStreams;
    float mRenderGain[1];
    unsigned int mNumBuffers;
    unsigned char mRenderDeviceIndex;
    bool mPlaying : 1;
    bool mEnabled : 1;
};
}

 
 
 
 
 
 
 
 
 
