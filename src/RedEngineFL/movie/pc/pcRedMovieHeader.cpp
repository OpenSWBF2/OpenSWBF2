#include "RedEngineFL/movie/pc/pcRedMovieHeader.h"

#include <stddef.h>
#include <string.h>

namespace
{

unsigned int ReadLE32(const unsigned char* pBytes)
{
    return (unsigned int)pBytes[0] | ((unsigned int)pBytes[1] << 8) | ((unsigned int)pBytes[2] << 16) |
           ((unsigned int)pBytes[3] << 24);
}

unsigned int ReadLE16(const unsigned char* pBytes)
{
    return (unsigned int)pBytes[0] | ((unsigned int)pBytes[1] << 8);
}

bool IsBinkRevision(char cRevision)
{
    return cRevision == 'b' || cRevision == 'f' || cRevision == 'g' || cRevision == 'h' || cRevision == 'i' ||
           cRevision == 'k';
}

int Reject(const char* pcReason, const char** ppcReason)
{
    *ppcReason = pcReason;
    return kMovieHeaderReject;
}

int NeedMore(unsigned long long u64Needed, MovieHeaderInfo* pInfo)
{
    pInfo->uNeeded = (unsigned int)u64Needed;
    return kMovieHeaderNeedMore;
}

}

int CheckMovieHeader(const unsigned char* pBytes, unsigned int uSize, unsigned int uSegmentLength,
                     MovieHeaderInfo* pInfo, const char** ppcReason)
{
    memset(pInfo, 0, sizeof(*pInfo));
    *ppcReason = 0;

    if (uSize < 4)
    {
        return NeedMore(4, pInfo);
    }
    if (pBytes[0] != 'B' || pBytes[1] != 'I' || pBytes[2] != 'K')
    {
        return Reject("magic", ppcReason);
    }
    const char cRevision = (char)pBytes[3];
    if (!IsBinkRevision(cRevision))
    {
        return Reject("revision", ppcReason);
    }
    pInfo->cRevision = cRevision;

    const unsigned long long u64Fixed = 44ull + (cRevision == 'k' ? 4ull : 0ull);
    if (uSize < u64Fixed)
    {
        return NeedMore(u64Fixed, pInfo);
    }

    const unsigned long long u64FileSize = (unsigned long long)ReadLE32(pBytes + 4) + 8ull;
    const unsigned int uFrames = ReadLE32(pBytes + 8);
    const unsigned int uLargest = ReadLE32(pBytes + 12);
    const unsigned int uWidth = ReadLE32(pBytes + 20);
    const unsigned int uHeight = ReadLE32(pBytes + 24);
    const unsigned int uFpsNum = ReadLE32(pBytes + 28);
    const unsigned int uFpsDen = ReadLE32(pBytes + 32);
    const unsigned int uTracks = ReadLE32(pBytes + 40);

    if (u64FileSize > uSegmentLength)
    {
        return Reject("size", ppcReason);
    }
    if (uFrames < 1 || uFrames > kMovieMaxFrames)
    {
        return Reject("frames", ppcReason);
    }
    if (uLargest < 1 || uLargest > kMovieMaxLargest)
    {
        return Reject("largest", ppcReason);
    }
    if (uLargest > u64FileSize)
    {
        return Reject("largest", ppcReason);
    }
    if (uWidth < 1 || uWidth > kMovieMaxWidth)
    {
        return Reject("width", ppcReason);
    }
    if (uHeight < 1 || uHeight > kMovieMaxHeight)
    {
        return Reject("height", ppcReason);
    }
    if (uFpsNum == 0 || uFpsDen == 0)
    {
        return Reject("fps", ppcReason);
    }
    if ((unsigned long long)uFpsNum < (unsigned long long)kMovieMinFps * uFpsDen)
    {
        return Reject("fps", ppcReason);
    }
    if ((unsigned long long)uFpsNum > (unsigned long long)kMovieMaxFps * uFpsDen)
    {
        return Reject("fps", ppcReason);
    }
    if (uTracks > kMovieMaxTracks)
    {
        return Reject("tracks", ppcReason);
    }

    pInfo->uFileSize = (unsigned int)u64FileSize;
    pInfo->uFrames = uFrames;
    pInfo->uLargest = uLargest;
    pInfo->uWidth = uWidth;
    pInfo->uHeight = uHeight;
    pInfo->uFpsNum = uFpsNum;
    pInfo->uFpsDen = uFpsDen;
    pInfo->uVideoFlags = ReadLE32(pBytes + 36);
    pInfo->uTracks = uTracks;

    const unsigned long long u64Tables = u64Fixed + 12ull * uTracks;
    if (uSize < u64Tables)
    {
        return NeedMore(u64Tables, pInfo);
    }
    const unsigned char* pTrackInfo = pBytes + (size_t)u64Fixed + 4 * (size_t)uTracks;
    for (unsigned int i = 0; i < uTracks; ++i)
    {
        const unsigned int uRate = ReadLE16(pTrackInfo + 4 * i);
        if (uRate < kMovieMinRate || uRate > kMovieMaxRate)
        {
            return Reject("rate", ppcReason);
        }
        const unsigned int uFlags = ReadLE16(pTrackInfo + 4 * i + 2);
        if (uFlags & ~kMovieAudioFlagMask)
        {
            return Reject("aflags", ppcReason);
        }
        if (i == 0)
        {
            pInfo->uRate = uRate;
            pInfo->uAudioFlags = uFlags;
        }
    }

    const unsigned long long u64HeaderEnd = u64Tables + 4ull * ((unsigned long long)uFrames + 1ull);
    pInfo->uHeaderEnd = (unsigned int)u64HeaderEnd;
    if (uSize < u64HeaderEnd)
    {
        return NeedMore(u64HeaderEnd, pInfo);
    }

    const unsigned char* pIndex = pBytes + (size_t)u64Tables;
    const unsigned int uIndex0 = ReadLE32(pIndex);
    pInfo->uIndex0 = uIndex0;
    if ((unsigned long long)(uIndex0 & ~1u) != u64HeaderEnd)
    {
        return Reject("index", ppcReason);
    }
    unsigned long long u64Prev = uIndex0 & ~1u;
    for (unsigned int i = 1; i <= uFrames; ++i)
    {
        const unsigned long long u64Pos = ReadLE32(pIndex + 4 * (size_t)i) & ~1u;
        if (u64Pos <= u64Prev || u64Pos - u64Prev > kMovieMaxLargest)
        {
            return Reject("order", ppcReason);
        }
        if (u64Pos - u64Prev < 4ull * uTracks)
        {
            return Reject("order", ppcReason);
        }
        u64Prev = u64Pos;
    }
    if (u64Prev != (u64FileSize & ~1ull))
    {
        return Reject("range", ppcReason);
    }
    return kMovieHeaderOk;
}
