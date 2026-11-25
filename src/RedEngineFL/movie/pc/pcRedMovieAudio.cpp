#include "RedEngineFL/movie/pc/pcRedMovieAudio.h"

#include <windows.h>
#include <string.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/movie/RedMovieGain.h"
#include "RedEngineFL/movie/pc/pcRedMovieAlloc.h"
#include "RedEngineFL/movie/pc/pcRedMovieClock.h"
#include "SoundFL/source/sndsampleinfo.h"
#include "SoundFL/source/sndstreamformat.h"
#include "SoundFL/source/win/sndengine.h"

namespace
{
const unsigned char kMovieHardwareGain = 0x10;
}

MovieAudioRenderer::MovieAudioRenderer()
    : mllPlayedBytes(0),
      mbInUpdate(false)
{
}

unsigned long long MovieAudioRenderer::PlayedBytes()
{
    return (unsigned long long)InterlockedCompareExchange64(&mllPlayedBytes, 0, 0);
}

void MovieAudioRenderer::ResetPlayed()
{
    InterlockedExchange64(&mllPlayedBytes, 0);
}

void MovieAudioRenderer::UpdateObject()
{
    IDirectSound* const pDevice = mDevice;
    if (pDevice == 0)
    {
        return;
    }
    Snd::Engine::UpdateCooperativeLevel(pDevice, false);
    Snd::DSBufferRenderer::mFlags |= kMovieHardwareGain;
    if (mbInUpdate)
    {
        Snd::DSBufferRenderer::UpdateObject();
        return;
    }
    mbInUpdate = true;
    const unsigned int uLeft = mLeftToPlay;
    const unsigned int uPrev = mPrevPlayPos;
    const unsigned int uSize = mBufferSize;
    const bool bStopping = Snd::StreamNode::QueryDesiredPlayState() == Snd::SourceTransport::playState_stopped;
    Snd::DSBufferRenderer::UpdateObject();
    const unsigned int uPlayed = MovieAudioPlayedStep(uLeft, uPrev, uSize, mLeftToPlay, mPrevPlayPos, bStopping);
    if (uPlayed != 0)
    {
        InterlockedExchangeAdd64(&mllPlayedBytes, (long long)uPlayed);
    }
    mbInUpdate = false;
}

MovieAudio::MovieAudio()
    : mpBlock(0),
      miHeap(-1),
      muChannels(0),
      muFrameBytes(0),
      muStartFrames(0),
      mullFed(0),
      mbOpen(false)
{
    memset(&mHold, 0, sizeof(mHold));
}

MovieAudio::~MovieAudio()
{
    Close();
}

bool MovieAudio::Open(unsigned int uRate, unsigned int uChannels)
{
    Close();
    if (uRate == 0 || uChannels == 0 || uChannels > kMovieAudioMaxChannels || Snd::Engine::smDirectSound == 0 ||
        Snd::Engine::GetMixConfig() == Snd::Engine::mixConfig_Disabled)
    {
        return false;
    }
    const unsigned int uFrameBytes = (unsigned int)sizeof(short) * uChannels;
    const size_t uPacketSamples = (size_t)kMovieAudioPacketFrames * uChannels;
    const size_t uSamples = kMovieAudioPackets * uPacketSamples + (size_t)kMovieAudioHoldFrames * uChannels;
    const int iHeap = __RedCurrHeap;
    short* const pBlock = (short*)RedTryAllocFromHeap(iHeap, uSamples * sizeof(short));
    if (pBlock == 0)
    {
        return false;
    }
    for (unsigned int i = 0; i < kMovieAudioPackets; ++i)
    {
        maPackets[i].SetBuffer(pBlock + i * uPacketSamples);
    }
    mProducer.SetPackets((unsigned int)(uPacketSamples * sizeof(short)), maPackets, kMovieAudioPackets);
    mRenderer.ResetPlayed();
    const Snd::StreamFormat cFormat(Snd::SampleInfo::format_PCM16, uRate, uChannels);
    if (!mRenderer.Open(0xFE, cFormat, kMovieAudioBufferMs))
    {
        mProducer.SetPackets(0, 0, 0);
        RedFreeToHeap(iHeap, pBlock, 0);
        return false;
    }
    mpBlock = pBlock;
#ifdef OPENSWBF2_TEST
    ++gRedMovieLiveBlocks;
#endif
    mHold.pHold = pBlock + kMovieAudioPackets * uPacketSamples;
    mHold.uChannels = uChannels;
    mHold.uHeld = 0;
    mHold.ullDropped = 0;
    miHeap = iHeap;
    muChannels = uChannels;
    muFrameBytes = uFrameBytes;
    muStartFrames = MovieAudioStartFrames(mRenderer.mBufferSize, uFrameBytes);
    mullFed = 0;
    mRenderer.Connect(&mProducer, 0);
    mRenderer.Snd::StreamNode::Play(true);
    mbOpen = true;
    return true;
}

unsigned int MovieAudio::Queue(const short* pSamples, unsigned int uFrames)
{
    unsigned int uDone = 0;
    while (uDone < uFrames)
    {
        Snd::StreamPacket* const pPacket = mProducer.GetReadPacket();
        if (pPacket == 0)
        {
            break;
        }
        unsigned int uChunk = uFrames - uDone;
        if (uChunk > kMovieAudioPacketFrames)
        {
            uChunk = kMovieAudioPacketFrames;
        }
        memcpy(pPacket->GetBuffer(), pSamples + (size_t)uDone * muChannels, (size_t)uChunk * muFrameBytes);
        mProducer.PacketComplete(pPacket, uChunk * muFrameBytes, 0, false, false, 0);
        uDone += uChunk;
        mullFed += uChunk;
    }
    return uDone;
}

unsigned int MovieAudio::QueueThunk(void* pContext, const short* pSamples, unsigned int uFrames)
{
    return ((MovieAudio*)pContext)->Queue(pSamples, uFrames);
}

void MovieAudio::Feed(const short* pSamples, unsigned int uFrames)
{
    if (!mbOpen)
    {
        return;
    }
    MovieAudioFeed(mHold, QueueThunk, this, pSamples, uFrames);
}

unsigned long long MovieAudio::Played()
{
    if (!mbOpen)
    {
        return 0;
    }
    return mRenderer.PlayedBytes() / muFrameBytes;
}

unsigned long long MovieAudio::Fed() const
{
    return mullFed;
}

unsigned int MovieAudio::Held() const
{
    return mHold.uHeld;
}

unsigned long long MovieAudio::Dropped() const
{
    return mHold.ullDropped;
}

bool MovieAudio::Starved()
{
    if (!mbOpen)
    {
        return true;
    }
    return MovieAudioStarved(Played(), mullFed, mHold.uHeld,
                             mRenderer.GetCurrentPlayState() == Snd::SourceTransport::playState_playing, muStartFrames);
}

void MovieAudio::SetGain(float fGain)
{
    if (!mbOpen)
    {
        return;
    }
    mRenderer.SetGain(MovieStreamGain(fGain));
}

void MovieAudio::Close()
{
    if (!mbOpen)
    {
        return;
    }
    mRenderer.Connect(0, 0);
    if (mRenderer.GetPlayState() != Snd::SourceTransport::playState_stopped)
    {
        mRenderer.Snd::SourceTransport::Stop();
    }
    mRenderer.Close();
    mProducer.SetPackets(0, 0, 0);
    RedFreeToHeap(miHeap, mpBlock, 0);
#ifdef OPENSWBF2_TEST
    --gRedMovieLiveBlocks;
#endif
    mpBlock = 0;
    mHold.pHold = 0;
    mHold.uHeld = 0;
    miHeap = -1;
    mbOpen = false;
}

bool MovieAudio::IsOpen() const
{
    return mbOpen;
}
