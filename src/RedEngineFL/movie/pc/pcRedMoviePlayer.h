#pragma once

#include "RedEngineFL/movie/RedMoviePlayerBase.h"
#include "RedEngineFL/movie/RedMoviePlayerRenderer.h"

class RedBitmapElement;
struct MoviePlayback;

#ifdef OPENSWBF2_TEST
extern unsigned int gRedMovieCloses;
extern unsigned int gRedMovieTestFrames;
extern unsigned int gRedMovieTestFrameNum;
extern bool gRedMovieTestEnd;
#endif

// VTABLE: SWBF2 0x00783ff0
class RedMoviePlayer : public RedMoviePlayerBase, public RedMoviePlayerRenderer
{
public:

    // FUNCTION: SWBF2 0x006f41d0
    RedMoviePlayer();

    // FUNCTION: SWBF2 0x006f4270
    virtual ~RedMoviePlayer();

    bool Open(const char* const pFileName, unsigned int uiNumTextures, int iHeap,
              int iUnused, unsigned int uiArg4, unsigned int uiBufferSize,
              unsigned int uiReadSize, unsigned int uiNumBitmaps);
    void Close();
    void Flush();
    void EndFrame();
    void Update();
    float GetPlaybackTime() const;
    void Render(const RedTexture* const pTexture, int iX, int iY,
                unsigned int uiWidth, unsigned int uiHeight, unsigned char ucFlags);
    void SeekSegment(const RedMoviePlayerBase::Segment* const pSegment, bool bFlag);
    void SetGain(float fGain);

    virtual RedTexture* AllocTexture(const unsigned int uiNameHash,
                                     const unsigned int uiWidth,
                                     const unsigned int uiHeight,
                                     const unsigned int uiFormat,
                                     const unsigned int uiIndex,
                                     const unsigned char ucFlags);
    virtual void FreeTexture(RedTexture* pTexture, const unsigned int uiIndex,
                             const unsigned char ucFlags);

    bool CreateMovieDecoder();
    void DestroyMovieDecoder(bool bForce);
    bool FixedRateFrameDue();
    bool FixedRateBufferFull();
    void FeedMovieAudio();

    static unsigned int __cdecl SourcePeek(void* pContext, unsigned char* pOut, unsigned int uSize);
    static unsigned int __cdecl SourceRead(void* pContext, unsigned char* pOut, unsigned int uSize);
    static int __cdecl SourceState(void* pContext);
    static void __cdecl SourcePump(void* pContext);

    static RedMoviePlayer* sMoviePlayer;

    static unsigned int sReadSize;

    static float sFixedRateStart;

    MoviePlayback* mPlayback;

    unsigned char mFlags;
};

 
