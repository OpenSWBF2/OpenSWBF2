#include "RedEngineFL/movie/pc/pcRedMovieDecoder.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/movie/pc/pcRedMovieAlloc.h"
#include "RedEngineFL/movie/pc/pcRedMovieConvert.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

namespace
{

unsigned char sacHeaderPeek[kMovieHeaderMaxBytes];

int Fail(unsigned int uFailuresBefore, const char** ppcReason)
{
    *ppcReason = MovieAllocFailures() != uFailuresBefore ? "memory" : "decoder";
    return kMovieOpenReject;
}

AVCodecContext* OpenCodec(const AVStream* pStream)
{
    const AVCodec* pCodec = avcodec_find_decoder(pStream->codecpar->codec_id);
    if (!pCodec)
    {
        return 0;
    }
    AVCodecContext* pContext = avcodec_alloc_context3(pCodec);
    if (!pContext)
    {
        return 0;
    }
    if (avcodec_parameters_to_context(pContext, pStream->codecpar) < 0)
    {
        avcodec_free_context(&pContext);
        return 0;
    }
    pContext->thread_count = 1;
    if (avcodec_open2(pContext, pCodec, 0) < 0)
    {
        avcodec_free_context(&pContext);
        return 0;
    }
    return pContext;
}

}

MovieDecoder::MovieDecoder()
    : mpFormat(0), mpIo(0), mpVideo(0), mpAudio(0), mpPacket(0), mpVideoFrame(0), mpAudioFrame(0), mpAudioChunk(0),
      miAudioStream(-1), miAudioChannels(0), miEndState(kMovieSourceMore), miHeap(-1)
{
    memset(&mSource, 0, sizeof(mSource));
    memset(&mSink, 0, sizeof(mSink));
}

MovieDecoder::~MovieDecoder()
{
    Close();
}

int MovieDecoder::PeekHeader(unsigned int uSegmentLength, MovieHeaderInfo* pInfo, const char** ppcReason)
{
    unsigned int uWant = 4;
    for (;;)
    {
        const int iState = mSource.State(mSource.pContext);
        const unsigned int uHave = mSource.Peek(mSource.pContext, sacHeaderPeek, uWant);
        const int iResult = CheckMovieHeader(sacHeaderPeek, uHave, uSegmentLength, pInfo, ppcReason);
        if (iResult != kMovieHeaderNeedMore)
        {
            return iResult;
        }
        if (pInfo->uNeeded > kMovieHeaderMaxBytes || (iState != kMovieSourceMore && uHave < uWant))
        {
            *ppcReason = "size";
            return kMovieHeaderReject;
        }
        if (uHave < uWant)
        {
            mSource.Pump(mSource.pContext);
        }
        uWant = pInfo->uNeeded;
    }
}

int MovieDecoder::Open(const MovieByteSource& kSource, unsigned int uSegmentLength, MovieHeaderInfo* pInfo,
                       const char** ppcReason)
{
    Close();
    *ppcReason = 0;
    mSource = kSource;
    miEndState = kMovieSourceMore;

    if (PeekHeader(uSegmentLength, pInfo, ppcReason) != kMovieHeaderOk)
    {
        return kMovieOpenReject;
    }

    MovieAllocRegister();
    miHeap = __RedCurrHeap;
    const unsigned int uFailures = MovieAllocFailures();

    unsigned char* pIoBuffer = (unsigned char*)av_malloc(kMovieIoBufferSize);
    if (!pIoBuffer)
    {
        return Fail(uFailures, ppcReason);
    }
    AVIOContext* pIo = avio_alloc_context(pIoBuffer, kMovieIoBufferSize, 0, this, ReadCb, 0, 0);
    if (!pIo)
    {
        av_free(pIoBuffer);
        return Fail(uFailures, ppcReason);
    }
    mpIo = pIo;

    AVFormatContext* pFormat = avformat_alloc_context();
    if (!pFormat)
    {
        Close();
        return Fail(uFailures, ppcReason);
    }
    pFormat->pb = pIo;
    if (avformat_open_input(&pFormat, "", av_find_input_format("bink"), 0) < 0)
    {
        Close();
        return Fail(uFailures, ppcReason);
    }
    mpFormat = pFormat;

    if (OpenCodecs() != kMovieOpenOk)
    {
        Close();
        return Fail(uFailures, ppcReason);
    }
    return kMovieOpenOk;
}

int MovieDecoder::OpenCodecs()
{
    AVFormatContext* pFormat = (AVFormatContext*)mpFormat;
    if (pFormat->nb_streams < 1 || pFormat->streams[0]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
    {
        return kMovieOpenReject;
    }
    mpVideo = OpenCodec(pFormat->streams[0]);
    if (!mpVideo)
    {
        return kMovieOpenReject;
    }
    for (unsigned int i = 1; i < pFormat->nb_streams; ++i)
    {
        if (pFormat->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            AVCodecContext* pAudio = OpenCodec(pFormat->streams[i]);
            if (!pAudio)
            {
                return kMovieOpenReject;
            }
            mpAudio = pAudio;
            miAudioStream = (int)i;
            miAudioChannels = pAudio->ch_layout.nb_channels;
            if (miAudioChannels < 1 || miAudioChannels > 2)
            {
                return kMovieOpenReject;
            }
            mpAudioChunk = (short*)av_malloc((size_t)kMovieAudioChunkFrames * miAudioChannels * sizeof(short));
            if (!mpAudioChunk)
            {
                return kMovieOpenReject;
            }
            break;
        }
    }
    mpPacket = av_packet_alloc();
    mpVideoFrame = av_frame_alloc();
    mpAudioFrame = av_frame_alloc();
    if (!mpPacket || !mpVideoFrame || !mpAudioFrame)
    {
        return kMovieOpenReject;
    }
    return kMovieOpenOk;
}

int __cdecl MovieDecoder::ReadCb(void* pOpaque, unsigned char* pBuf, int iSize)
{
    MovieDecoder* pDecoder = (MovieDecoder*)pOpaque;
    if (iSize <= 0)
    {
        return AVERROR(EINVAL);
    }
    for (;;)
    {
        const unsigned int uRead = pDecoder->mSource.Read(pDecoder->mSource.pContext, pBuf, (unsigned int)iSize);
        if (uRead != 0)
        {
            return (int)uRead;
        }
        const int iState = pDecoder->mSource.State(pDecoder->mSource.pContext);
        if (iState != kMovieSourceMore)
        {
            pDecoder->miEndState = iState;
            return AVERROR_EOF;
        }
        pDecoder->mSource.Pump(pDecoder->mSource.pContext);
    }
}

int MovieDecoder::DecodeAudioPacket()
{
    AVCodecContext* pAudio = (AVCodecContext*)mpAudio;
    AVFrame* pFrame = (AVFrame*)mpAudioFrame;
    int iResult = avcodec_send_packet(pAudio, (AVPacket*)mpPacket);
    if (iResult < 0)
    {
        return iResult;
    }
    for (;;)
    {
        iResult = avcodec_receive_frame(pAudio, pFrame);
        if (iResult == AVERROR(EAGAIN))
        {
            return 0;
        }
        if (iResult < 0)
        {
            return iResult;
        }
        const int iPlanar = pFrame->format == AV_SAMPLE_FMT_FLTP ? 1 : 0;
        if ((!iPlanar && pFrame->format != AV_SAMPLE_FMT_FLT) || pFrame->ch_layout.nb_channels != miAudioChannels ||
            pFrame->nb_samples < 0)
        {
            av_frame_unref(pFrame);
            return AVERROR_INVALIDDATA;
        }
        const unsigned int uSamples = (unsigned int)pFrame->nb_samples;
        for (unsigned int uDone = 0; uDone < uSamples;)
        {
            const unsigned int uChunk =
                uSamples - uDone < kMovieAudioChunkFrames ? uSamples - uDone : kMovieAudioChunkFrames;
            const float* apPlanes[2] = {0, 0};
            for (int c = 0; c < miAudioChannels; ++c)
            {
                apPlanes[c] = iPlanar ? (const float*)pFrame->extended_data[c] + uDone
                                      : (const float*)pFrame->extended_data[0] + (size_t)uDone * miAudioChannels;
            }
            ConvertAudioToS16(apPlanes, iPlanar, miAudioChannels, (int)uChunk, mpAudioChunk);
            if (mSink.Feed)
            {
                mSink.Feed(mSink.pContext, mpAudioChunk, uChunk);
            }
            uDone += uChunk;
        }
        av_frame_unref(pFrame);
    }
}

int MovieDecoder::DecodeVideoFrame(MovieFrame* pOut)
{
    if (!mpFormat)
    {
        return kMovieFrameError;
    }
    const int iOldHeap = RedSetCurrentHeap(miHeap);
    const int iResult = DecodeFrame(pOut);
    RedSetCurrentHeap(iOldHeap);
    return iResult;
}

int MovieDecoder::DecodeFrame(MovieFrame* pOut)
{
    AVFormatContext* pFormat = (AVFormatContext*)mpFormat;
    AVCodecContext* pVideo = (AVCodecContext*)mpVideo;
    AVPacket* pPacket = (AVPacket*)mpPacket;
    AVFrame* pFrame = (AVFrame*)mpVideoFrame;
    for (;;)
    {
        int iResult = av_read_frame(pFormat, pPacket);
        if (iResult == AVERROR_EOF)
        {
            return kMovieFrameEnd;
        }
        if (iResult < 0)
        {
            return kMovieFrameError;
        }
        if (pPacket->stream_index == miAudioStream)
        {
            iResult = DecodeAudioPacket();
            av_packet_unref(pPacket);
            if (iResult < 0)
            {
                return kMovieFrameError;
            }
            continue;
        }
        if (pPacket->stream_index != 0)
        {
            av_packet_unref(pPacket);
            continue;
        }
        iResult = avcodec_send_packet(pVideo, pPacket);
        av_packet_unref(pPacket);
        if (iResult < 0)
        {
            return kMovieFrameError;
        }
        iResult = avcodec_receive_frame(pVideo, pFrame);
        if (iResult == AVERROR(EAGAIN))
        {
            continue;
        }
        if (iResult < 0)
        {
            return kMovieFrameError;
        }
        if (pFrame->format != AV_PIX_FMT_YUV420P && pFrame->format != AV_PIX_FMT_YUVA420P)
        {
            return kMovieFrameError;
        }
        for (int i = 0; i < 3; ++i)
        {
            pOut->apPlane[i] = pFrame->data[i];
            pOut->aiStride[i] = pFrame->linesize[i];
        }
        pOut->iWidth = pFrame->width;
        pOut->iHeight = pFrame->height;
        pOut->bFullRange = pFrame->color_range == AVCOL_RANGE_JPEG;
        return kMovieFrameReady;
    }
}

void MovieDecoder::SetAudioSink(const MovieAudioSink& kSink)
{
    mSink = kSink;
}

int MovieDecoder::AudioChannels() const
{
    return miAudioChannels;
}

int MovieDecoder::ProbeScore() const
{
    return mpFormat ? ((const AVFormatContext*)mpFormat)->probe_score : -1;
}

int MovieDecoder::EndState() const
{
    return miEndState;
}

void MovieDecoder::Close()
{
    AVCodecContext* pVideo = (AVCodecContext*)mpVideo;
    AVCodecContext* pAudio = (AVCodecContext*)mpAudio;
    AVFormatContext* pFormat = (AVFormatContext*)mpFormat;
    AVIOContext* pIo = (AVIOContext*)mpIo;
    AVPacket* pPacket = (AVPacket*)mpPacket;
    AVFrame* pVideoFrame = (AVFrame*)mpVideoFrame;
    AVFrame* pAudioFrame = (AVFrame*)mpAudioFrame;
    if (pVideo)
    {
        avcodec_free_context(&pVideo);
    }
    if (pAudio)
    {
        avcodec_free_context(&pAudio);
    }
    if (pFormat)
    {
        avformat_close_input(&pFormat);
    }
    if (pIo)
    {
        av_freep(&pIo->buffer);
        avio_context_free(&pIo);
    }
    if (pPacket)
    {
        av_packet_free(&pPacket);
    }
    if (pVideoFrame)
    {
        av_frame_free(&pVideoFrame);
    }
    if (pAudioFrame)
    {
        av_frame_free(&pAudioFrame);
    }
    if (mpAudioChunk)
    {
        av_free(mpAudioChunk);
    }
    mpVideo = 0;
    mpAudio = 0;
    mpFormat = 0;
    mpIo = 0;
    mpPacket = 0;
    mpVideoFrame = 0;
    mpAudioFrame = 0;
    mpAudioChunk = 0;
    memset(&mSink, 0, sizeof(mSink));
    miAudioStream = -1;
    miAudioChannels = 0;
}
