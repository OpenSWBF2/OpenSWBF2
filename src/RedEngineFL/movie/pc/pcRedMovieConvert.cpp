#include "RedEngineFL/movie/pc/pcRedMovieConvert.h"

#include <math.h>
#include <stddef.h>

namespace
{

unsigned char LimitedChannel(int iSum)
{
    if (iSum > 32767)
    {
        iSum = 32767;
    }
    iSum -= 28672;
    if (iSum < 0)
    {
        iSum = 0;
    }
    return (unsigned char)(iSum >> 4);
}

unsigned char FullChannel(int iSum)
{
    if (iSum < 0)
    {
        return 0;
    }
    iSum >>= 16;
    return (unsigned char)(iSum > 255 ? 255 : iSum);
}

void PixelBink(int iY, int iU, int iV, unsigned char* pOut)
{
    const int iYt = (iY * 64 * 19081) >> 16;
    const int iUt = (iU * 16 * 26640) >> 16;
    const int iVt = (iV * 32 * 26640) >> 16;
    pOut[0] = LimitedChannel(5 * iUt + iYt + 24205);
    pOut[1] = LimitedChannel(iYt - (iUt + iVt) + 30887);
    pOut[2] = LimitedChannel(2 * iVt + iYt + 25040);
}

void PixelFull(int iY, int iU, int iV, unsigned char* pOut)
{
    const int iD = iU - 128;
    const int iE = iV - 128;
    const int iRound = 32768;
    pOut[0] = FullChannel(65536 * iY + 116130 * iD + iRound);
    pOut[1] = FullChannel(65536 * iY - 22554 * iD - 46802 * iE + iRound);
    pOut[2] = FullChannel(65536 * iY + 91881 * iE + iRound);
}

short PcmSample(float fScaled)
{
    if (fScaled >= 32767.0f)
    {
        return 32767;
    }
    if (fScaled <= -32768.0f)
    {
        return -32768;
    }
    if (fScaled != fScaled)
    {
        return 0;
    }
    return (short)lrintf(fScaled);
}

}

void ConvertYuv420ToXrgb(const unsigned char* pY, int iYStride, const unsigned char* pU, int iUStride,
                         const unsigned char* pV, int iVStride, int iWidth, int iHeight, bool bFullRange,
                         unsigned char* pDst, int iDstPitch)
{
    for (int iRow = 0; iRow < iHeight; ++iRow)
    {
        const unsigned char* pYRow = pY + (ptrdiff_t)iRow * iYStride;
        const unsigned char* pURow = pU + (ptrdiff_t)(iRow >> 1) * iUStride;
        const unsigned char* pVRow = pV + (ptrdiff_t)(iRow >> 1) * iVStride;
        unsigned char* pOut = pDst + (ptrdiff_t)iRow * iDstPitch;
        for (int iCol = 0; iCol < iWidth; ++iCol)
        {
            const int iChroma = iCol >> 1;
            if (bFullRange)
            {
                PixelFull(pYRow[iCol], pURow[iChroma], pVRow[iChroma], pOut);
            }
            else
            {
                PixelBink(pYRow[iCol], pURow[iChroma], pVRow[iChroma], pOut);
            }
            pOut[3] = 0;
            pOut += 4;
        }
    }
}

void ConvertAudioToS16(const float* const* ppPlanes, int iPlanar, int iChannels, int iSamples, short* pOut)
{
    for (int iSample = 0; iSample < iSamples; ++iSample)
    {
        for (int iChannel = 0; iChannel < iChannels; ++iChannel)
        {
            const ptrdiff_t iAt = (ptrdiff_t)iSample * iChannels + iChannel;
            pOut[iAt] = PcmSample(32767.0f * (iPlanar ? ppPlanes[iChannel][iSample] : ppPlanes[0][iAt]));
        }
    }
}
