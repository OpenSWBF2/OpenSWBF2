#pragma once

struct MovieWide
{
    unsigned long long uHi;
    unsigned long long uLo;
};

inline MovieWide MovieMulWide(unsigned long long uA, unsigned long long uB)
{
    const unsigned long long uMask = 0xFFFFFFFFull;
    const unsigned long long uLowLow = (uA & uMask) * (uB & uMask);
    const unsigned long long uLowHigh = (uA & uMask) * (uB >> 32);
    const unsigned long long uHighLow = (uA >> 32) * (uB & uMask);
    const unsigned long long uMid = (uLowLow >> 32) + (uLowHigh & uMask) + (uHighLow & uMask);
    MovieWide k;
    k.uLo = (uLowLow & uMask) | (uMid << 32);
    k.uHi = (uA >> 32) * (uB >> 32) + (uLowHigh >> 32) + (uHighLow >> 32) + (uMid >> 32);
    return k;
}

inline bool MovieWideAtLeast(const MovieWide& kA, const MovieWide& kB)
{
    return kA.uHi != kB.uHi ? kA.uHi > kB.uHi : kA.uLo >= kB.uLo;
}

inline bool MovieFrameDue(unsigned long long uPlayed, unsigned int uRate, unsigned int uFpsNum, unsigned int uFpsDen,
                          unsigned int uFrame)
{
    if (uRate == 0 || uFpsNum == 0 || uFpsDen == 0 || uFrame == 0)
    {
        return true;
    }
    return MovieWideAtLeast(MovieMulWide(uPlayed, uFpsNum), MovieMulWide((unsigned long long)(uFrame - 1) * uFpsDen, uRate));
}

inline bool MovieFrameDueTicks(unsigned long long uTicks, unsigned long long uFreq, unsigned int uFpsNum,
                               unsigned int uFpsDen, unsigned int uFrame)
{
    if (uFreq == 0 || uFpsNum == 0 || uFpsDen == 0 || uFrame == 0)
    {
        return true;
    }
    return MovieWideAtLeast(MovieMulWide(uTicks, uFpsNum), MovieMulWide((unsigned long long)(uFrame - 1) * uFpsDen, uFreq));
}

inline float MoviePlaybackTime(unsigned int uFpsNum, unsigned int uFpsDen, unsigned int uFrame)
{
    return (float)uFpsDen * (float)uFrame / (float)uFpsNum;
}

inline unsigned int MovieCursorAdvance(unsigned int uPrev, unsigned int uCur, unsigned int uSize)
{
    if (uCur >= uPrev)
    {
        return uCur - uPrev;
    }
    return uSize - uPrev + uCur;
}
