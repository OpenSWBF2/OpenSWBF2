#pragma once

#include <string>
#include <vector>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/movie/pc/pcRedMovieDecoder.h"

struct MvsSpan
{
    unsigned int uName;
    long long llOffset;
    unsigned int uLength;
};

std::string MovieDataDir();
bool ListMvsSegments(const std::string& sPath, std::vector<MvsSpan>& vOut);
bool ReadMvsBytes(const std::string& sPath, long long llOffset, unsigned int uSize, std::vector<unsigned char>& vOut);

struct SyntheticMovieSpec
{
    char acMagic[4];
    unsigned int uWidth;
    unsigned int uHeight;
    unsigned int uFrames;
    unsigned int uTracks;
    unsigned int uRate;
    unsigned int uAudioFlags;
    unsigned int uSpan;
    int iIndex0Shift;
    unsigned int uSeed;
};

SyntheticMovieSpec SyntheticSpec();
std::vector<unsigned char> BuildSyntheticMovie(const SyntheticMovieSpec& kSpec, unsigned int* puHeaderEnd);

class MemoryMovieSource
{
public:
    MemoryMovieSource(const unsigned char* pBytes, unsigned int uSize, unsigned int uChunk, unsigned int uArrival);
    MovieByteSource Source();

    unsigned int muReads;
    unsigned int muPeeks;
    unsigned int muPumps;
    unsigned int muZeroReads;

private:
    static unsigned int Peek(void* pContext, unsigned char* pOut, unsigned int uSize);
    static unsigned int Read(void* pContext, unsigned char* pOut, unsigned int uSize);
    static int State(void* pContext);
    static void Pump(void* pContext);

    const unsigned char* mpBytes;
    unsigned int muSize;
    unsigned int muChunk;
    unsigned int muArrival;
    unsigned int muArrived;
    unsigned int muPos;
};

class FileMovieSource
{
public:
    FileMovieSource();
    ~FileMovieSource();

    bool Open(const std::string& sPath, long long llOffset, unsigned int uLength);
    MovieByteSource Source();

private:
    FileMovieSource(const FileMovieSource&);
    FileMovieSource& operator=(const FileMovieSource&);

    static unsigned int Peek(void* pContext, unsigned char* pOut, unsigned int uSize);
    static unsigned int Read(void* pContext, unsigned char* pOut, unsigned int uSize);
    static int State(void* pContext);
    static void Pump(void* pContext);
    unsigned int Fetch(unsigned char* pOut, unsigned int uSize);

    void* mpFile;
    long long mllOffset;
    unsigned int muLength;
    unsigned int muPos;
    bool mbFailed;
};

class MovieAudioCollector
{
public:
    MovieAudioCollector();
    void Attach(MovieDecoder& kDecoder);
    unsigned int Take(const short** ppSamples);

private:
    static void Feed(void* pContext, const short* pSamples, unsigned int uFrames);

    std::vector<short> mvIn;
    std::vector<short> mvOut;
    int miChannels;
    int miHeap;
};

unsigned long long MovieFrameFnv(const unsigned char* pPixels, int iPitch, int iWidth, int iHeight);

class MovieScratchHeap
{
public:
    MovieScratchHeap();
    ~MovieScratchHeap();

    int Open(MovieDecoder& kDecoder, const MovieByteSource& kSource, unsigned int uSegmentLength,
             MovieHeaderInfo* pInfo, const char** ppcReason);
    int Decode(MovieDecoder& kDecoder, MovieFrame* pFrame);
    void Close(MovieDecoder& kDecoder);
    RedHeapStats Stats();

    int miHeap;

private:
    MovieScratchHeap(const MovieScratchHeap&);
    MovieScratchHeap& operator=(const MovieScratchHeap&);

    char* mpArena;
};
