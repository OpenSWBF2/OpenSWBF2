#include "pcRedMoviePlayer.h"
#include "RedEngineFL/Util/RedLog.h"

unsigned int RedMoviePlayer::sReadSize;

#ifdef OPENSWBF2_TEST
unsigned int gRedMovieCloses;
#endif

static bool sMovieOffLogged;

void RedMoviePlayer::Close()
{
}

void RedMoviePlayer::EndFrame()
{
}

void RedMoviePlayer::Flush()
{
}

float RedMoviePlayer::GetPlaybackTime() const
{
    return 0.0f;
}

bool RedMoviePlayer::Open(const char* const pFileName, unsigned int uiNumTextures, int iHeap,
                          int iUnused, unsigned int uiArg4, unsigned int uiBufferSize,
                          unsigned int uiReadSize, unsigned int uiNumBitmaps)
{
    if (!sMovieOffLogged)
    {
        sMovieOffLogged = true;
        LOG(LogCore, Warning, "OPENSWBF2 MOVIE off: no movie decoder in this build");
    }
    return false;
}

void RedMoviePlayer::Render(const RedTexture* const pTexture, int iX, int iY,
                            unsigned int uiWidth, unsigned int uiHeight, unsigned char ucFlags)
{
}

void RedMoviePlayer::SeekSegment(const RedMoviePlayerBase::Segment* const pSegment, bool bFlag)
{
}

void RedMoviePlayer::SetGain(float fGain)
{
}

void RedMoviePlayer::Update()
{
}
