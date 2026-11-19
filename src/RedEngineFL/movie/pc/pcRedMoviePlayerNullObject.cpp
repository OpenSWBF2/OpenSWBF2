#include "pcRedMoviePlayer.h"

RedMoviePlayer* RedMoviePlayer::sMoviePlayer;

RedMoviePlayer::RedMoviePlayer()
    : RedMoviePlayerBase(0x800)
{
    mBink = 0;
    mFlags = 0;
    sMoviePlayer = this;
}

RedMoviePlayer::~RedMoviePlayer()
{
    sMoviePlayer = 0;
}

RedTexture* RedMoviePlayer::AllocTexture(const unsigned int uiNameHash,
                                         const unsigned int uiWidth,
                                         const unsigned int uiHeight,
                                         const unsigned int uiFormat,
                                         const unsigned int uiIndex,
                                         const unsigned char ucFlags)
{
    return 0;
}

void RedMoviePlayer::FreeTexture(RedTexture* pTexture, const unsigned int uiIndex,
                                 const unsigned char ucFlags)
{
}
