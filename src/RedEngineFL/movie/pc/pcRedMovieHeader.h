#pragma once

struct MovieHeaderInfo
{
    char cRevision;
    unsigned int uWidth;
    unsigned int uHeight;
    unsigned int uFrames;
    unsigned int uLargest;
    unsigned int uFpsNum;
    unsigned int uFpsDen;
    unsigned int uVideoFlags;
    unsigned int uTracks;
    unsigned int uRate;
    unsigned int uAudioFlags;
    unsigned int uHeaderEnd;
    unsigned int uFileSize;
    unsigned int uIndex0;
    unsigned int uNeeded;
};

const int kMovieHeaderOk = 0;
const int kMovieHeaderNeedMore = 1;
const int kMovieHeaderReject = 2;

const unsigned int kMovieMaxWidth = 1024;
const unsigned int kMovieMaxHeight = 768;
const unsigned int kMovieMaxFrames = 16000;
const unsigned int kMovieMaxLargest = 0x40000;
const unsigned int kMovieMaxTracks = 6;
const unsigned int kMovieMinRate = 8000;
const unsigned int kMovieMaxRate = 48000;
const unsigned int kMovieAudioFlagMask = 0x7000;
const unsigned int kMovieMinFps = 1;
const unsigned int kMovieMaxFps = 120;

int CheckMovieHeader(const unsigned char* pBytes, unsigned int uSize, unsigned int uSegmentLength,
                     MovieHeaderInfo* pInfo, const char** ppcReason);
