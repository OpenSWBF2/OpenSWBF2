#pragma once

#include <stddef.h>
#include <string.h>

#include "RedEngineFL/movie/pc/pcRedMovieClock.h"

const unsigned int kMovieAudioPackets = 16;
const unsigned int kMovieAudioPacketFrames = 4800;
const unsigned int kMovieAudioHoldFrames = 48000;
const unsigned int kMovieAudioMaxChannels = 2;

struct MovieAudioHold
{
    short* pHold;
    unsigned int uChannels;
    unsigned int uHeld;
    unsigned long long ullDropped;
};

typedef unsigned int (*MovieAudioQueueFn)(void* pContext, const short* pSamples, unsigned int uFrames);

inline void MovieAudioFeed(MovieAudioHold& kHold, MovieAudioQueueFn pfnQueue, void* pContext, const short* pSamples,
                           unsigned int uFrames)
{
    const size_t uFrameBytes = sizeof(short) * kHold.uChannels;
    if (kHold.uHeld != 0)
    {
        const unsigned int uMoved = pfnQueue(pContext, kHold.pHold, kHold.uHeld);
        kHold.uHeld -= uMoved;
        if (kHold.uHeld != 0 && uMoved != 0)
        {
            memmove(kHold.pHold, kHold.pHold + (size_t)uMoved * kHold.uChannels, (size_t)kHold.uHeld * uFrameBytes);
        }
    }
    if (uFrames == 0 || pSamples == 0)
    {
        return;
    }
    const unsigned int uDone = kHold.uHeld == 0 ? pfnQueue(pContext, pSamples, uFrames) : 0;
    const unsigned int uRest = uFrames - uDone;
    const unsigned int uRoom = kMovieAudioHoldFrames - kHold.uHeld;
    const unsigned int uKeep = uRest < uRoom ? uRest : uRoom;
    memcpy(kHold.pHold + (size_t)kHold.uHeld * kHold.uChannels, pSamples + (size_t)uDone * kHold.uChannels,
           (size_t)uKeep * uFrameBytes);
    kHold.uHeld += uKeep;
    kHold.ullDropped += uRest - uKeep;
}

inline unsigned int MovieAudioStartFrames(unsigned int uBufferBytes, unsigned int uFrameBytes)
{
    return (uBufferBytes / 2 + uFrameBytes - 1) / uFrameBytes;
}

inline bool MovieAudioStarved(unsigned long long ullPlayed, unsigned long long ullFed, unsigned int uHeld,
                              bool bPlaying, unsigned int uStartFrames)
{
    const unsigned long long ullPending = (ullPlayed < ullFed ? ullFed - ullPlayed : 0) + uHeld;
    if (ullPending == 0)
    {
        return true;
    }
    return !bPlaying && ullPending < uStartFrames;
}

inline unsigned int MovieAudioPlayedStep(unsigned int uLeftBefore, unsigned int uPosBefore, unsigned int uSize,
                                         unsigned int uLeftAfter, unsigned int uPosAfter, bool bStopping)
{
    if (bStopping)
    {
        return 0;
    }
    if (uLeftAfter == 0)
    {
        return uLeftBefore;
    }
    const unsigned int uAdvance = MovieCursorAdvance(uPosBefore, uPosAfter, uSize);
    return uAdvance < uLeftBefore ? uAdvance : uLeftBefore;
}
