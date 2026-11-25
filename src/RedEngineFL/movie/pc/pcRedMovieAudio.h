#pragma once

#include "RedEngineFL/movie/pc/pcRedMovieAudioQueue.h"
#include "SoundFL/source/sndstreampacket.h"
#include "SoundFL/source/sndstreamsourceproducer.h"
#include "SoundFL/source/win/sndstreamauxrenderer.h"

const float kMovieAudioBufferMs = 500.0f;

class MovieAudioRenderer : public Snd::StreamAuxRenderer
{
public:
    MovieAudioRenderer();

    unsigned long long PlayedBytes();
    void ResetPlayed();

private:
    virtual void UpdateObject();

    volatile long long mllPlayedBytes;
    bool mbInUpdate;
};

class MovieAudio
{
public:
    MovieAudio();
    ~MovieAudio();

    bool Open(unsigned int uRate, unsigned int uChannels);
    void Feed(const short* pSamples, unsigned int uFrames);
    unsigned long long Played();
    unsigned long long Fed() const;
    unsigned int Held() const;
    unsigned long long Dropped() const;
    bool Starved();
    void SetGain(float fGain);
    void Close();
    bool IsOpen() const;

private:
    MovieAudio(const MovieAudio&);
    MovieAudio& operator=(const MovieAudio&);

    unsigned int Queue(const short* pSamples, unsigned int uFrames);
    static unsigned int QueueThunk(void* pContext, const short* pSamples, unsigned int uFrames);

    Snd::StreamSourceProducer mProducer;
    MovieAudioRenderer mRenderer;
    Snd::StreamPacket maPackets[kMovieAudioPackets];
    short* mpBlock;
    MovieAudioHold mHold;
    int miHeap;
    unsigned int muChannels;
    unsigned int muFrameBytes;
    unsigned int muStartFrames;
    unsigned long long mullFed;
    bool mbOpen;
};
