#include <windows.h>
#include <new>
#include <stdint.h>
#include <string.h>

#include "pcRedMoviePlayer.h"
#include "pcRedMovieLayout.h"
#include "pcRedMovieAlloc.h"
#include "pcRedMovieAudio.h"
#include "pcRedMovieClock.h"
#include "pcRedMovieConvert.h"
#include "pcRedMovieDecoder.h"
#include "RedEngineFL/Graphics/PC/pcRedTexture.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "PebbleFL/Common/PblMath.h"
#include "RedEngineFL/Util/RedLog.h"
#include "Battlefront2/Platform/Pc/pcProgress.h"
#include "Battlefront2/Source/GameState.h"
#include "Battlefront2/Source/NetSetup.h"

struct MoviePlayback
{
    MoviePlayback()
        : muFrameNum(1),
          muShown(0),
#ifdef OPENSWBF2_TEST
          muWaits(0),
#endif
          mullTimerBase(0),
          mullTimerFreq(0),
          muTimerFrame(1),
          mbAudio(false),
          mbTimer(false)
    {
        memset(&mHeader, 0, sizeof(mHeader));
    }

    MovieDecoder mDecoder;
    MovieAudio mAudio;
    MovieHeaderInfo mHeader;
    unsigned int muFrameNum;
    unsigned int muShown;
#ifdef OPENSWBF2_TEST
    unsigned int muWaits;
#endif
    unsigned long long mullTimerBase;
    unsigned long long mullTimerFreq;
    unsigned int muTimerFrame;
    bool mbAudio;
    bool mbTimer;
};

#ifdef OPENSWBF2_TEST
#include "Battlefront2/Source/CommandLine.h"

void RedMovieTestOpen(const RedMoviePlayer* pPlayer);
void RedMovieTestFrame(const MoviePlayback* pPlayback, const void* pBits, int iPitch);
void RedMovieTestEnd(const MoviePlayback* pPlayback);
#endif

static MovieRejectMemo sRejected;

static void __cdecl FeedMovieAudioSink(void* pContext, const short* pSamples, unsigned int uFrames)
{
    ((MovieAudio*)pContext)->Feed(pSamples, uFrames);
}

// GLOBAL: SWBF2 0x009c8420
RedMoviePlayer* RedMoviePlayer::sMoviePlayer;

// GLOBAL: SWBF2 0x009c8424
unsigned int RedMoviePlayer::sReadSize;

// FUNCTION: SWBF2 0x006f41d0
RedMoviePlayer::RedMoviePlayer()
    : RedMoviePlayerBase(0x800)
{
    mPlayback = 0;
    mFlags = 0;

    sMoviePlayer = this;
}

// FUNCTION: SWBF2 0x006f4270
RedMoviePlayer::~RedMoviePlayer()
{
    sMoviePlayer = 0;
}

// SYNTHETIC: SWBF2 0x006f42e0
// RedMoviePlayer::`scalar deleting destructor'

// FUNCTION: SWBF2 0x006f4310
RedTexture* RedMoviePlayer::AllocTexture(const unsigned int uiNameHash,
                                         const unsigned int uiWidth,
                                         const unsigned int uiHeight,
                                         const unsigned int uiFormat,
                                         const unsigned int uiIndex,
                                         const unsigned char ucFlags)
{
    int iOldHeap = RedSetCurrentHeap(GetInfo()->GetRedHeap());
    RedTexture* pTexture = new RedTexture(uiWidth, uiHeight, D3DFMT_X8R8G8B8, 1, 0,
                                          D3DPOOL_MANAGED, uiNameHash);
    RedSetCurrentHeap(iOldHeap);
    return pTexture;
}

unsigned int __cdecl RedMoviePlayer::SourcePeek(void* pContext, unsigned char* pOut, unsigned int uSize)
{
    return ((RedMoviePlayer*)pContext)->BufferReadData(pOut, uSize, false);
}

unsigned int __cdecl RedMoviePlayer::SourceRead(void* pContext, unsigned char* pOut, unsigned int uSize)
{
    return ((RedMoviePlayer*)pContext)->BufferReadData(pOut, uSize, true);
}

int __cdecl RedMoviePlayer::SourceState(void* pContext)
{
    const RedMoviePlayer* pPlayer = (const RedMoviePlayer*)pContext;
    if (pPlayer->IsDiscontinuity())
    {
        return kMovieSourceDiscontinuity;
    }
    if (pPlayer->IsEnd())
    {
        return kMovieSourceEnd;
    }
    return kMovieSourceMore;
}

void __cdecl RedMoviePlayer::SourcePump(void* pContext)
{
    ((RedMoviePlayer*)pContext)->RedMoviePlayerBase::Update();
    Sleep(0);
}

// FUNCTION: SWBF2 0x006f4670
void RedMoviePlayer::Close()
{
    sRejected.Clear();
    DestroyMovieDecoder(true);
    CloseRenderer(GetInfo()->GetRedHeap());
    RedFreeToHeap(GetInfo()->GetRedHeap(), BufferGetBuffer(), 0);
    RedMoviePlayerBase::Close();
}

// FUNCTION: SWBF2 0x006f46c0
bool RedMoviePlayer::CreateMovieDecoder()
{
#ifdef OPENSWBF2_TEST
    if (gMovieFault == kMovieFaultTexture)
    {
        return false;
    }
#endif
    if (!CreateTextures(mPlayback->mHeader.uWidth, mPlayback->mHeader.uHeight, 0x20, 3))
    {
        return false;
    }
    return true;
}

// FUNCTION: SWBF2 0x006f46e0
void RedMoviePlayer::DestroyMovieDecoder(bool bForce)
{
    if (mPlayback)
    {
#ifdef OPENSWBF2_TEST
        const unsigned int uShown = mPlayback->muShown;
        const unsigned int uWaits = mPlayback->muWaits;
#endif
        mPlayback->mDecoder.Close();
        mPlayback->mAudio.Close();
        mPlayback->~MoviePlayback();
        MovieFree(mPlayback);
        mPlayback = 0;
#ifdef OPENSWBF2_TEST
        ++gRedMovieCloses;
        LOG(LogCore, Inform, "Movie close");
        if (gRedMovieLiveBlocks != 0)
        {
            LOG(LogCore, Error, "movie heap holds %u blocks after the movie closed", gRedMovieLiveBlocks);
        }
        LOG(LogCore, Inform, "Movie heap blocks=%u", gRedMovieLiveBlocks);
        LOG(LogCore, Inform, "Movie playback shown=%u waits=%u", uShown, uWaits);
#endif
        DestroyTextures(bForce, 3);
    }
}

void PumpWaitingMessages();

// FUNCTION: SWBF2 0x006f4720
void RedMoviePlayer::EndFrame()
{
    RedMoviePlayerRenderer::EndFrame();
    PumpWaitingMessages();
    pcProgress::Step();
}

// FUNCTION: SWBF2 0x006f4730
void RedMoviePlayer::Flush()
{
    RedMoviePlayerBase::Flush();
    DestroyMovieDecoder(false);
}

// FUNCTION: SWBF2 0x006f4750
void RedMoviePlayer::FreeTexture(RedTexture* pTexture, const unsigned int uiIndex,
                                 const unsigned char ucFlags)
{
    int iOldHeap = RedSetCurrentHeap(GetInfo()->GetRedHeap());
    delete pTexture;
    RedSetCurrentHeap(iOldHeap);
}

// FUNCTION: SWBF2 0x006f47a0
float RedMoviePlayer::GetPlaybackTime() const
{
    if (mPlayback)
    {
        return MoviePlaybackTime(mPlayback->mHeader.uFpsNum, mPlayback->mHeader.uFpsDen, mPlayback->muFrameNum);
    }
    return 0.0f;
}

// FUNCTION: SWBF2 0x006f4820
bool RedMoviePlayer::Open(const char* const pFileName, unsigned int uiNumTextures, int iHeap,
                          int iUnused, unsigned int uiArg4, unsigned int uiBufferSize,
                          unsigned int uiReadSize, unsigned int uiNumBitmaps)
{
    sRejected.Clear();
    char* pBlock = (char*)RedAllocFromHeap(
        iHeap, MovieLayout::BlockSize(uiBufferSize, uiNumTextures, uiNumBitmaps), 0);
    if (pBlock)
    {
        RedBitmapElement** ppBitmaps =
            new (pBlock + MovieLayout::BitmapArrayOffset(uiBufferSize, uiNumTextures))
                RedBitmapElement*[uiNumBitmaps];
        if (OpenRenderer(iHeap, ppBitmaps, uiNumBitmaps))
        {
            RedTexture** ppTextures = new (pBlock + MovieLayout::TextureArrayOffset(uiBufferSize))
                RedTexture*[MovieLayout::TextureSlotCount(uiNumTextures)];
            if (RedMoviePlayerBase::Open(pFileName, ppTextures, uiNumTextures, iHeap, uiArg4,
                                         pBlock, uiBufferSize, uiReadSize, 0xffffffff))
            {
                sReadSize = uiReadSize;
                return true;
            }
            CloseRenderer(iHeap);
        }
        RedFreeToHeap(iHeap, pBlock, 0);
    }
    return false;
}

// FUNCTION: SWBF2 0x006f48e0
void RedMoviePlayer::Render(const RedTexture* const pTexture, int iX, int iY,
                            unsigned int uiWidth, unsigned int uiHeight, unsigned char ucFlags)
{
    RedMoviePlayerRenderer::Render(pTexture, iX, iY, uiWidth, uiHeight, ucFlags);
}

// FUNCTION: SWBF2 0x006f48f0
void RedMoviePlayer::SeekSegment(const RedMoviePlayerBase::Segment* const pSegment, bool bFlag)
{
    RedMoviePlayerBase::SeekSegment(pSegment, bFlag);
}

// FUNCTION: SWBF2 0x006f4900
void RedMoviePlayer::SetGain(float fGain)
{
    if (mPlayback)
    {
        mPlayback->mAudio.SetGain(fGain);
    }
}

// GLOBAL: SWBF2 0x009c8430
static bool sOpeningMovieDecoder;

#ifdef OPENSWBF2_TEST
unsigned int gRedMovieCloses;
RedMovieTestRecord gRedMovieTestRecord;

static void RedMovieTestReject(const char* pcReason)
{
    ++gRedMovieTestRecord.uRejects;
    strncpy(gRedMovieTestRecord.acReject, pcReason ? pcReason : "", sizeof(gRedMovieTestRecord.acReject) - 1);
    gRedMovieTestRecord.acReject[sizeof(gRedMovieTestRecord.acReject) - 1] = 0;
}
#endif

// FUNCTION: SWBF2 0x006f4950
void RedMoviePlayer::Update()
{
    switch (GetDesiredPlayState())
    {
    case playState_stopped:
        Disable();
        SetCurrentPlayState(playState_stopped);
        DestroyMovieDecoder(false);
        Flush();
        break;
    case playState_paused:
        SetCurrentPlayState(playState_pausing);
        break;
    case playState_playing:
        SetCurrentPlayState(playState_playing);
        break;
    }

    PlayState eState = GetPlayState();
    if (eState < playState_pausing)
    {
        return;
    }
    if (eState > playState_paused)
    {
        if (eState != playState_playing)
        {
            return;
        }

        if (GetNumReallocTextures() && mPlayback)
        {
            ReallocTextures(mPlayback->mHeader.uWidth, mPlayback->mHeader.uHeight, 0x20, 3);
        }

        if (mFlags & 6)
        {
            if (!GetNumUsedTextures())
            {
                bool bEnd = (mFlags & 2) != 0;
                DestroyMovieDecoder(false);
                if (!GetLooping())
                {
                    Stop();
                }
                Notify(bEnd ? notifyType_End : notifyType_Discontinuity);
                mFlags &= ~7;
                Update();
                mFlags |= 1;
            }
        }
        else if (mPlayback && (mFlags & 1) && FixedRateFrameDue())
        {
            RedTexture* pTexture = GetTexture();
            if (pTexture)
            {
                MovieFrame kFrame;
                const int iResult = mPlayback->mDecoder.DecodeVideoFrame(&kFrame);
                if (iResult == kMovieFrameReady && kFrame.iWidth == (int)mPlayback->mHeader.uWidth &&
                    kFrame.iHeight == (int)mPlayback->mHeader.uHeight)
                {
                    IDirect3DSurface9* pSurface = 0;
                    HRESULT hrSurface = ((IDirect3DTexture9*)pTexture->m_pTexture)->GetSurfaceLevel(0, &pSurface);
#ifdef OPENSWBF2_TEST
                    if (SUCCEEDED(hrSurface) && gMovieFault == kMovieFaultSurface)
                    {
                        pSurface->Release();
                        pSurface = 0;
                        hrSurface = D3DERR_INVALIDCALL;
                    }
#endif
                    if (SUCCEEDED(hrSurface))
                    {
                        D3DLOCKED_RECT rect;
                        if (SUCCEEDED(pSurface->LockRect(&rect, 0, 0)))
                        {
                            ConvertYuv420ToXrgb(kFrame.apPlane[0], kFrame.aiStride[0], kFrame.apPlane[1],
                                                kFrame.aiStride[1], kFrame.apPlane[2], kFrame.aiStride[2],
                                                kFrame.iWidth, kFrame.iHeight, kFrame.bFullRange,
                                                (unsigned char*)rect.pBits, rect.Pitch);
#ifdef OPENSWBF2_TEST
                            RedMovieTestFrame(mPlayback, rect.pBits, rect.Pitch);
#endif
                            TextureComplete(pTexture);
                            ++mPlayback->muShown;
                            pSurface->UnlockRect();
                        }
                        pSurface->Release();
                    }
                    if (mPlayback->muFrameNum < mPlayback->mHeader.uFrames)
                    {
                        ++mPlayback->muFrameNum;
                    }
                    else
                    {
                        mFlags |= IsDiscontinuity() ? 4 : 2;
#ifdef OPENSWBF2_TEST
                        RedMovieTestEnd(mPlayback);
#endif
                    }
                }
                else
                {
                    if (iResult != kMovieFrameEnd)
                    {
                        LOG(LogCore, Warning, "Movie reject reason=frame");
#ifdef OPENSWBF2_TEST
                        RedMovieTestReject("frame");
#endif
                    }
                    mFlags |= mPlayback->mDecoder.EndState() == kMovieSourceDiscontinuity ? 4 : 2;
#ifdef OPENSWBF2_TEST
                    RedMovieTestEnd(mPlayback);
#endif
                }
            }
#ifdef OPENSWBF2_TEST
            else
            {
                ++mPlayback->muWaits;
                ++gRedMovieTestRecord.uWaits;
            }
#endif
        }
        if (mPlayback)
        {
            FeedMovieAudio();
        }
    }

    if (GetPlayState() == playState_pausing && GetReadBufferFull())
    {
        SetCurrentPlayState(playState_paused);
    }

    RedMoviePlayerBase::Update();

    if (!mPlayback && !sOpeningMovieDecoder && FixedRateBufferFull())
    {
        sOpeningMovieDecoder = true;
        if (IsEnd() || IsDiscontinuity())
        {
            ResetStart();
        }
#ifdef OPENSWBF2_TEST
        RedMovieTestOpen(this);
#endif
        const RedMoviePlayerBase::Segment* const pSegment = GetCurrentSegment();
        const bool bRejected = sRejected.Skips(pSegment);
        const char* pcReason = "memory";
        const int iOldHeap = RedSetCurrentHeap(GetInfo()->GetRedHeap());
        void* pMemory = bRejected ? 0 : MovieAlloc(sizeof(MoviePlayback));
        if (pMemory)
        {
            mPlayback = new (pMemory) MoviePlayback;
            const MovieByteSource kSource = {this, SourcePeek, SourceRead, SourceState, SourcePump};
            const unsigned int uSegmentLength = pSegment ? pSegment->mLength : 0;
            if (mPlayback->mDecoder.Open(kSource, uSegmentLength, &mPlayback->mHeader, &pcReason) != kMovieOpenOk)
            {
                mPlayback->~MoviePlayback();
                MovieFree(mPlayback);
                mPlayback = 0;
                sRejected.Remember(pSegment);
            }
        }
        if (!mPlayback)
        {
            if (!bRejected)
            {
                LOG(LogCore, Warning, "Movie reject reason=%s", pcReason);
#ifdef OPENSWBF2_TEST
                RedMovieTestReject(pcReason);
#endif
            }
            mFlags |= 2;
        }
        else
        {
            mPlayback->mbAudio = mPlayback->mHeader.uTracks != 0 &&
                                 mPlayback->mAudio.Open(mPlayback->mHeader.uRate,
                                                        (unsigned int)mPlayback->mDecoder.AudioChannels());
            if (mPlayback->mbAudio)
            {
                const MovieAudioSink kSink = {&mPlayback->mAudio, FeedMovieAudioSink};
                mPlayback->mDecoder.SetAudioSink(kSink);
            }
            mFlags |= 1;
        }
        RedSetCurrentHeap(iOldHeap);
        sOpeningMovieDecoder = false;
        if (mPlayback)
        {
            if (!CreateMovieDecoder())
            {
                LOG(LogCore, Warning, "Movie reject reason=texture");
                sRejected.Remember(GetCurrentSegment());
#ifdef OPENSWBF2_TEST
                RedMovieTestReject("texture");
#endif
                DestroyMovieDecoder(false);
                mFlags = (unsigned char)((mFlags & ~1) | 2);
            }
            else
            {
                LOG(LogCore, Inform, "Movie open");
#ifdef OPENSWBF2_TEST
                ++gRedMovieTestRecord.uOpens;
#endif
            }
        }
    }
}

// SYNTHETIC: SWBF2 0x00409120
// PblMath::`dynamic initializer for 'fOneOverHalfPi''

// SYNTHETIC: SWBF2 0x00409140
// PblMath::`dynamic initializer for 'fOneOverTwoPi''

void RedMoviePlayer::FeedMovieAudio()
{
    mPlayback->mAudio.Feed(0, 0);
}

float RedMoviePlayer::sFixedRateStart;

bool RedMoviePlayer::FixedRateFrameDue()
{
    const MovieHeaderInfo& kHeader = mPlayback->mHeader;
    if (!(g_fFixedRate > 0.0f))
    {
        if (mPlayback->mbAudio && !mPlayback->mAudio.Starved())
        {
            mPlayback->mbTimer = false;
            return MovieFrameDue(mPlayback->mAudio.Played(), kHeader.uRate, kHeader.uFpsNum, kHeader.uFpsDen,
                                 mPlayback->muFrameNum);
        }
        LARGE_INTEGER liNow;
        QueryPerformanceCounter(&liNow);
        const unsigned long long ullNow = (unsigned long long)liNow.QuadPart;
        if (!mPlayback->mbTimer)
        {
            LARGE_INTEGER liFreq;
            QueryPerformanceFrequency(&liFreq);
            mPlayback->mullTimerFreq = (unsigned long long)liFreq.QuadPart;
            mPlayback->mullTimerBase = ullNow;
            mPlayback->muTimerFrame = mPlayback->muFrameNum;
            mPlayback->mbTimer = true;
        }
        return MovieFrameDueTicks(ullNow - mPlayback->mullTimerBase, mPlayback->mullTimerFreq, kHeader.uFpsNum,
                                  kHeader.uFpsDen, mPlayback->muFrameNum - mPlayback->muTimerFrame + 1);
    }
    const double dElapsed = (double)(GameState::GetSystemTimeBase()->GetTime() - sFixedRateStart);
    return (double)(mPlayback->muFrameNum - 1) * (double)kHeader.uFpsDen <= dElapsed * (double)kHeader.uFpsNum;
}

bool RedMoviePlayer::FixedRateBufferFull()
{
    if (g_fFixedRate > 0.0f)
    {
        for (;;)
        {
            const bool bWasReading = (RedMoviePlayerBase::mFlags & flags_Reading) != 0;
            if (GetReadBufferFull())
            {
                break;
            }
            RedMoviePlayerBase::Update();
            if (!bWasReading && !(RedMoviePlayerBase::mFlags & flags_Reading))
            {
                break;
            }
            Sleep(0);
        }
        sFixedRateStart = GameState::GetSystemTimeBase()->GetTime();
    }
    return GetReadBufferFull();
}

#ifdef OPENSWBF2_TEST
unsigned int gRedMovieTestFrames;
unsigned int gRedMovieTestFrameNum;
bool gRedMovieTestEnd;

namespace
{

unsigned int suMovieSegment;

bool MovieGoldenFrame(unsigned int n, unsigned int uFrames)
{
    return n == 1 || n == 2 || n == 3 || n == 30 || n == 300 || n == uFrames;
}

}

void RedMovieTestOpen(const RedMoviePlayer* pPlayer)
{
    suMovieSegment = pPlayer->GetCurrentSegment() ? pPlayer->GetCurrentSegment()->GetHashID() : 0;
}

void RedMovieTestFrame(const MoviePlayback* pPlayback, const void* pBits, int iPitch)
{
    const unsigned int n = pPlayback->muFrameNum;
    const unsigned int uFrames = pPlayback->mHeader.uFrames;
    const unsigned int uWidth = pPlayback->mHeader.uWidth;
    const unsigned int uHeight = pPlayback->mHeader.uHeight;
    const bool bTest = gMovieTestFrames > 0;
    const bool bGolden = MovieGoldenFrame(n, uFrames);
    gRedMovieTestFrameNum = n;
    if (bTest || bGolden)
    {
        const unsigned char* pBase = (const unsigned char*)pBits;
        unsigned long long uHash = 0xCBF29CE484222325ull;
        for (unsigned int y = 0; y < uHeight; ++y)
        {
            const unsigned char* pRow = pBase + (size_t)y * (size_t)iPitch;
            for (unsigned int x = 0; x < uWidth; ++x)
            {
                for (unsigned int c = 0; c < 3; ++c)
                {
                    uHash ^= pRow[x * 4 + c];
                    uHash *= 0x100000001B3ull;
                }
            }
        }
        LOG(LogCore, Inform, "Movie frame seg=0x%08X n=%u fnv=%016llX", suMovieSegment, n, uHash);
        ++gRedMovieTestRecord.uHashed;
        if (n == 300)
        {
            gRedMovieTestRecord.uFrame300Segment = suMovieSegment;
            gRedMovieTestRecord.ullFrame300 = uHash;
        }
    }
    if (bTest)
    {
        ++gRedMovieTestFrames;
        if (gRedMovieTestFrames >= (unsigned int)gMovieTestFrames || n >= uFrames)
        {
            gRedMovieTestEnd = true;
        }
    }
}

void RedMovieTestEnd(const MoviePlayback* pPlayback)
{
    LOG(LogCore, Inform, "Movie end seg=0x%08X frames=%u shown=%u header=%u waits=%u",
        suMovieSegment, pPlayback->muFrameNum, pPlayback->muShown, pPlayback->mHeader.uFrames, pPlayback->muWaits);
    RedMovieTestRecord& rRecord = gRedMovieTestRecord;
    ++rRecord.uEnds;
    rRecord.uEndSegment = suMovieSegment;
    rRecord.uEndFrames = pPlayback->muFrameNum;
    rRecord.uEndShown = pPlayback->muShown;
    rRecord.uEndHeader = pPlayback->mHeader.uFrames;
    rRecord.uEndWaits = pPlayback->muWaits;
    if (pPlayback->muFrameNum != pPlayback->muShown || pPlayback->muFrameNum != pPlayback->mHeader.uFrames)
    {
        ++rRecord.uEndMismatches;
    }
}

#endif
