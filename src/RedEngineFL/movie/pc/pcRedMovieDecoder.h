#pragma once

#include "RedEngineFL/movie/pc/pcRedMovieHeader.h"

struct MovieByteSource
{
    void* pContext;
    unsigned int (*Peek)(void* pContext, unsigned char* pOut, unsigned int uSize);
    unsigned int (*Read)(void* pContext, unsigned char* pOut, unsigned int uSize);
    int (*State)(void* pContext);
    void (*Pump)(void* pContext);
};

const int kMovieSourceMore = 0;
const int kMovieSourceEnd = 1;
const int kMovieSourceDiscontinuity = 2;

const int kMovieOpenOk = 0;
const int kMovieOpenReject = 1;

struct MovieAudioSink
{
    void* pContext;
    void (*Feed)(void* pContext, const short* pSamples, unsigned int uFrames);
};

const unsigned int kMovieAudioChunkFrames = 2048;

struct MovieRejectMemo
{
    const void* mpSegment;

    void Remember(const void* pSegment)
    {
        mpSegment = pSegment;
    }

    bool Skips(const void* pSegment) const
    {
        return pSegment != 0 && pSegment == mpSegment;
    }

    void Clear()
    {
        mpSegment = 0;
    }
};

const int kMovieFrameReady = 0;
const int kMovieFrameEnd = 1;
const int kMovieFrameError = 2;

const unsigned int kMovieHeaderMaxBytes = 48 + 12 * kMovieMaxTracks + 4 * (kMovieMaxFrames + 1);
const int kMovieIoBufferSize = 32768;

struct MovieFrame
{
    const unsigned char* apPlane[3];
    int aiStride[3];
    int iWidth;
    int iHeight;
    bool bFullRange;
};

class MovieDecoder
{
public:
    MovieDecoder();
    ~MovieDecoder();

    int Open(const MovieByteSource& kSource, unsigned int uSegmentLength, MovieHeaderInfo* pInfo,
             const char** ppcReason);
    int DecodeVideoFrame(MovieFrame* pFrame);
    void SetAudioSink(const MovieAudioSink& kSink);
    int AudioChannels() const;
    int ProbeScore() const;
    int EndState() const;
    void Close();

private:
    MovieDecoder(const MovieDecoder&);
    MovieDecoder& operator=(const MovieDecoder&);

    static int __cdecl ReadCb(void* pOpaque, unsigned char* pBuf, int iSize);
    int PeekHeader(unsigned int uSegmentLength, MovieHeaderInfo* pInfo, const char** ppcReason);
    int OpenCodecs();
    int DecodeFrame(MovieFrame* pFrame);
    int DecodeAudioPacket();

    MovieByteSource mSource;
    void* mpFormat;
    void* mpIo;
    void* mpVideo;
    void* mpAudio;
    void* mpPacket;
    void* mpVideoFrame;
    void* mpAudioFrame;
    short* mpAudioChunk;
    MovieAudioSink mSink;
    int miAudioStream;
    int miAudioChannels;
    int miEndState;
    int miHeap;
};
