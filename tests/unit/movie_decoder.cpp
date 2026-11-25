#include "third_party/doctest/doctest.h"

#include <windows.h>

#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"
#include "unit/movie_testdata.h"
#include "RedEngineFL/movie/pc/pcRedMovieConvert.h"
#include "RedEngineFL/movie/pc/pcRedMovieAudioQueue.h"

extern "C"
{
    void __cdecl av_log_set_level(int iLevel);
}

namespace
{

void Put32(std::vector<unsigned char>& v, unsigned int u)
{
    v.push_back((unsigned char)u);
    v.push_back((unsigned char)(u >> 8));
    v.push_back((unsigned char)(u >> 16));
    v.push_back((unsigned char)(u >> 24));
}

void Put16(std::vector<unsigned char>& v, unsigned int u)
{
    v.push_back((unsigned char)u);
    v.push_back((unsigned char)(u >> 8));
}

void Set32(std::vector<unsigned char>& v, size_t uAt, unsigned int u)
{
    v[uAt] = (unsigned char)u;
    v[uAt + 1] = (unsigned char)(u >> 8);
    v[uAt + 2] = (unsigned char)(u >> 16);
    v[uAt + 3] = (unsigned char)(u >> 24);
}

unsigned int Get32(const unsigned char* p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

unsigned int NextRandom(unsigned int& uState)
{
    uState = uState * 1664525u + 1013904223u;
    return uState >> 8;
}

struct RetailSegment
{
    std::string sPath;
    MvsSpan kSpan;
};

bool FirstSegment(const char* pcFile, RetailSegment* pOut)
{
    const std::string sDir = MovieDataDir();
    if (sDir.empty())
    {
        return false;
    }
    pOut->sPath = sDir + "\\" + pcFile;
    std::vector<MvsSpan> vSpans;
    if (!ListMvsSegments(pOut->sPath, vSpans) || vSpans.empty())
    {
        return false;
    }
    pOut->kSpan = vSpans[0];
    return true;
}

bool ShortestSegment(const char* pcFile, RetailSegment* pOut)
{
    const std::string sDir = MovieDataDir();
    if (sDir.empty())
    {
        return false;
    }
    pOut->sPath = sDir + "\\" + pcFile;
    std::vector<MvsSpan> vSpans;
    if (!ListMvsSegments(pOut->sPath, vSpans) || vSpans.empty())
    {
        return false;
    }
    pOut->kSpan = vSpans[0];
    for (size_t i = 1; i < vSpans.size(); ++i)
    {
        if (vSpans[i].uLength < pOut->kSpan.uLength)
        {
            pOut->kSpan = vSpans[i];
        }
    }
    return true;
}

int RejectCase(SyntheticMovieSpec kSpec, std::string* psReason, unsigned int* puReads, bool* pbNoAlloc)
{
    const std::vector<unsigned char> v = BuildSyntheticMovie(kSpec, 0);
    MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), 0x10000, 0);
    MovieScratchHeap kHeap;
    const RedHeapStats kBefore = kHeap.Stats();
    MovieDecoder kDecoder;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    const int iResult = kHeap.Open(kDecoder, kSource.Source(), (unsigned int)v.size(), &kInfo, &pcReason);
    const RedHeapStats kAfter = kHeap.Stats();
    kHeap.Close(kDecoder);
    *psReason = pcReason ? pcReason : "";
    *puReads = kSource.muReads;
    *pbNoAlloc = kAfter.uAllocs == kBefore.uAllocs && kAfter.uUsed == kBefore.uUsed;
    return iResult;
}

}

SyntheticMovieSpec SyntheticSpec()
{
    SyntheticMovieSpec k;
    memset(&k, 0, sizeof(k));
    memcpy(k.acMagic, "BIKi", 4);
    k.uWidth = 64;
    k.uHeight = 48;
    k.uFrames = 4;
    k.uTracks = 1;
    k.uRate = 48000;
    k.uAudioFlags = 0x7000;
    k.uSpan = 600;
    k.uSeed = 1;
    return k;
}

std::vector<unsigned char> BuildSyntheticMovie(const SyntheticMovieSpec& k, unsigned int* puHeaderEnd)
{
    std::vector<unsigned char> v;
    v.insert(v.end(), k.acMagic, k.acMagic + 4);
    Put32(v, 0);
    Put32(v, k.uFrames);
    Put32(v, k.uSpan);
    Put32(v, k.uFrames);
    Put32(v, k.uWidth);
    Put32(v, k.uHeight);
    Put32(v, 30);
    Put32(v, 1);
    Put32(v, 0);
    Put32(v, k.uTracks);
    if (k.acMagic[3] == 'k')
    {
        Put32(v, 0);
    }
    for (unsigned int i = 0; i < k.uTracks; ++i)
    {
        Put32(v, 0x1000);
    }
    for (unsigned int i = 0; i < k.uTracks; ++i)
    {
        Put16(v, k.uRate);
        Put16(v, k.uAudioFlags);
    }
    for (unsigned int i = 0; i < k.uTracks; ++i)
    {
        Put32(v, i);
    }
    const unsigned int uHeaderEnd = (unsigned int)v.size() + 4 * (k.uFrames + 1);
    for (unsigned int i = 0; i <= k.uFrames; ++i)
    {
        unsigned int uPos = uHeaderEnd + i * k.uSpan;
        if (i == 0)
        {
            uPos = (unsigned int)((int)uPos + k.iIndex0Shift) | 1u;
        }
        Put32(v, uPos);
    }
    unsigned int uState = k.uSeed;
    for (unsigned int f = 0; f < k.uFrames; ++f)
    {
        const size_t uStart = v.size();
        for (unsigned int t = 0; t < k.uTracks; ++t)
        {
            const unsigned int uAudio = (NextRandom(uState) & 1) ? 0 : 64;
            Put32(v, uAudio);
            if (uAudio)
            {
                Put32(v, 2048);
                for (unsigned int i = 4; i < uAudio; ++i)
                {
                    v.push_back((unsigned char)NextRandom(uState));
                }
            }
        }
        while (v.size() < uStart + k.uSpan)
        {
            v.push_back((unsigned char)NextRandom(uState));
        }
    }
    Set32(v, 4, (unsigned int)v.size() - 8);
    if (puHeaderEnd)
    {
        *puHeaderEnd = uHeaderEnd;
    }
    return v;
}

MemoryMovieSource::MemoryMovieSource(const unsigned char* pBytes, unsigned int uSize, unsigned int uChunk,
                                     unsigned int uArrival)
    : muReads(0), muPeeks(0), muPumps(0), muZeroReads(0), mpBytes(pBytes), muSize(uSize), muChunk(uChunk),
      muArrival(uArrival), muArrived(uArrival ? 0 : uSize), muPos(0)
{
}

MovieByteSource MemoryMovieSource::Source()
{
    MovieByteSource k;
    k.pContext = this;
    k.Peek = Peek;
    k.Read = Read;
    k.State = State;
    k.Pump = Pump;
    return k;
}

unsigned int MemoryMovieSource::Peek(void* pContext, unsigned char* pOut, unsigned int uSize)
{
    MemoryMovieSource* p = (MemoryMovieSource*)pContext;
    ++p->muPeeks;
    const unsigned int uAvail = p->muArrived - p->muPos;
    const unsigned int uCopy = uSize < uAvail ? uSize : uAvail;
    memcpy(pOut, p->mpBytes + p->muPos, uCopy);
    return uCopy;
}

unsigned int MemoryMovieSource::Read(void* pContext, unsigned char* pOut, unsigned int uSize)
{
    MemoryMovieSource* p = (MemoryMovieSource*)pContext;
    ++p->muReads;
    const unsigned int uAvail = p->muArrived - p->muPos;
    unsigned int uCopy = uSize < uAvail ? uSize : uAvail;
    if (uCopy > p->muChunk)
    {
        uCopy = p->muChunk;
    }
    if (uCopy == 0)
    {
        ++p->muZeroReads;
    }
    memcpy(pOut, p->mpBytes + p->muPos, uCopy);
    p->muPos += uCopy;
    return uCopy;
}

int MemoryMovieSource::State(void* pContext)
{
    MemoryMovieSource* p = (MemoryMovieSource*)pContext;
    return p->muArrived >= p->muSize ? kMovieSourceEnd : kMovieSourceMore;
}

void MemoryMovieSource::Pump(void* pContext)
{
    MemoryMovieSource* p = (MemoryMovieSource*)pContext;
    ++p->muPumps;
    p->muArrived = p->muSize - p->muArrived < p->muArrival ? p->muSize : p->muArrived + p->muArrival;
}

MovieAudioCollector::MovieAudioCollector() : miChannels(0), miHeap(-1)
{
}

void MovieAudioCollector::Attach(MovieDecoder& kDecoder)
{
    miChannels = kDecoder.AudioChannels();
    miHeap = __RedCurrHeap;
    mvIn.clear();
    const MovieAudioSink kSink = {this, Feed};
    kDecoder.SetAudioSink(kSink);
}

void MovieAudioCollector::Feed(void* pContext, const short* pSamples, unsigned int uFrames)
{
    MovieAudioCollector* p = (MovieAudioCollector*)pContext;
    const int iSaved = RedSetCurrentHeap(p->miHeap);
    p->mvIn.insert(p->mvIn.end(), pSamples, pSamples + (size_t)uFrames * p->miChannels);
    RedSetCurrentHeap(iSaved);
}

unsigned int MovieAudioCollector::Take(const short** ppSamples)
{
    mvOut.swap(mvIn);
    mvIn.clear();
    *ppSamples = mvOut.empty() ? 0 : mvOut.data();
    return miChannels ? (unsigned int)(mvOut.size() / miChannels) : 0;
}

MovieScratchHeap::MovieScratchHeap() : miHeap(-1), mpArena(0)
{
    av_log_set_level(-8);
    const size_t uArena = 0x1000000;
    mpArena = (char*)VirtualAlloc(0, uArena, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    REQUIRE(mpArena != nullptr);
    miHeap = RedCreateHeap(mpArena, mpArena + uArena, 0x10, "MovieDecoderTest");
    REQUIRE(miHeap >= 0);
}

MovieScratchHeap::~MovieScratchHeap()
{
    if (miHeap >= 0)
    {
        RedDestroyHeap(miHeap);
    }
    if (mpArena)
    {
        VirtualFree(mpArena, 0, MEM_RELEASE);
    }
}

int MovieScratchHeap::Open(MovieDecoder& kDecoder, const MovieByteSource& kSource, unsigned int uSegmentLength,
                           MovieHeaderInfo* pInfo, const char** ppcReason)
{
    const int iSaved = RedSetCurrentHeap(miHeap);
    const int iResult = kDecoder.Open(kSource, uSegmentLength, pInfo, ppcReason);
    RedSetCurrentHeap(iSaved);
    return iResult;
}

int MovieScratchHeap::Decode(MovieDecoder& kDecoder, MovieFrame* pFrame)
{
    const int iSaved = RedSetCurrentHeap(miHeap);
    const int iResult = kDecoder.DecodeVideoFrame(pFrame);
    RedSetCurrentHeap(iSaved);
    return iResult;
}

void MovieScratchHeap::Close(MovieDecoder& kDecoder)
{
    const int iSaved = RedSetCurrentHeap(miHeap);
    kDecoder.Close();
    RedSetCurrentHeap(iSaved);
}

RedHeapStats MovieScratchHeap::Stats()
{
    RedHeapStats kStats;
    memset(&kStats, 0, sizeof(kStats));
    REQUIRE(RedGetHeapStats(miHeap, &kStats));
    return kStats;
}

TEST_CASE("movie decoder rejects a capped header before ffmpeg reads or allocates" * doctest::test_suite("unit"))
{
    SyntheticMovieSpec kWidth = SyntheticSpec();
    kWidth.uWidth = kMovieMaxWidth + 1;
    SyntheticMovieSpec kMagic = SyntheticSpec();
    memcpy(kMagic.acMagic, "XIKi", 4);
    SyntheticMovieSpec kIndex = SyntheticSpec();
    kIndex.iIndex0Shift = 4;
    const SyntheticMovieSpec akSpecs[3] = {kWidth, kMagic, kIndex};
    const char* apcReasons[3] = {"width", "magic", "index"};
    for (int i = 0; i < 3; ++i)
    {
        INFO("cap " << apcReasons[i]);
        std::string sReason;
        unsigned int uReads = 0;
        bool bNoAlloc = false;
        CHECK(RejectCase(akSpecs[i], &sReason, &uReads, &bNoAlloc) == kMovieOpenReject);
        CHECK(sReason == apcReasons[i]);
        CHECK(uReads == 0);
        CHECK(bNoAlloc);
    }
}

TEST_CASE("movie decoder rejects a stream that ends inside its header with size" * doctest::test_suite("unit"))
{
    const std::vector<unsigned char> v = BuildSyntheticMovie(SyntheticSpec(), 0);
    MemoryMovieSource kSource(v.data(), 40, 0x10000, 7);
    MovieScratchHeap kHeap;
    const RedHeapStats kBefore = kHeap.Stats();
    MovieDecoder kDecoder;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    CHECK(kHeap.Open(kDecoder, kSource.Source(), (unsigned int)v.size(), &kInfo, &pcReason) == kMovieOpenReject);
    CHECK(std::string(pcReason ? pcReason : "") == "size");
    CHECK(kSource.muReads == 0);
    CHECK(kSource.muPumps == 6);
    CHECK(kHeap.Stats().uAllocs == kBefore.uAllocs);
}

TEST_CASE("movie decoder opens a garbage stream unprobed and fails or ends cleanly" * doctest::test_suite("unit"))
{
    SyntheticMovieSpec kSpec = SyntheticSpec();
    for (unsigned int uSeed = 1; uSeed <= 4; ++uSeed)
    {
        INFO("seed " << uSeed);
        kSpec.uSeed = uSeed;
        const std::vector<unsigned char> v = BuildSyntheticMovie(kSpec, 0);
        MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), 13, 0);
        MovieScratchHeap kHeap;
        const RedHeapStats kBefore = kHeap.Stats();
        {
            MovieDecoder kDecoder;
            MovieHeaderInfo kInfo;
            const char* pcReason = 0;
            REQUIRE(kHeap.Open(kDecoder, kSource.Source(), (unsigned int)v.size(), &kInfo, &pcReason) ==
                    kMovieOpenOk);
            CHECK(kDecoder.ProbeScore() == 0);
            CHECK(kDecoder.AudioChannels() == 2);
            int iResult = kMovieFrameReady;
            unsigned int uCalls = 0;
            while (iResult == kMovieFrameReady && uCalls < kSpec.uFrames + 2)
            {
                MovieFrame kFrame;
                iResult = kHeap.Decode(kDecoder, &kFrame);
                ++uCalls;
                if (iResult == kMovieFrameReady)
                {
                    CHECK(kFrame.iWidth == 64);
                    CHECK(kFrame.iHeight == 48);
                }
            }
            CHECK(iResult != kMovieFrameReady);
            CHECK(uCalls <= kSpec.uFrames + 1);
            kHeap.Close(kDecoder);
        }
        CHECK(kHeap.Stats().uUsed == kBefore.uUsed);
    }
}

TEST_CASE("movie decoder streams 30 shell frames through 7 byte reads unprobed" * doctest::test_suite("unit"))
{
    RetailSegment kSeg;
    SKIP_IF(!FirstSegment("shell.mvs", &kSeg), "GameData movies not found");
    std::vector<unsigned char> vHead;
    const unsigned int uHeadSize =
        kSeg.kSpan.uLength < kMovieHeaderMaxBytes ? kSeg.kSpan.uLength : kMovieHeaderMaxBytes;
    REQUIRE(ReadMvsBytes(kSeg.sPath, kSeg.kSpan.llOffset, uHeadSize, vHead));
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    REQUIRE(CheckMovieHeader(vHead.data(), uHeadSize, kSeg.kSpan.uLength, &kInfo, &pcReason) == kMovieHeaderOk);
    REQUIRE(kInfo.uFrames > 30);
    const unsigned int uIndexAt = kInfo.uHeaderEnd - 4 * (kInfo.uFrames + 1);
    const unsigned int uCut = Get32(vHead.data() + uIndexAt + 4 * 30) & ~1u;
    std::vector<unsigned char> v;
    REQUIRE(ReadMvsBytes(kSeg.sPath, kSeg.kSpan.llOffset, uCut, v));

    MemoryMovieSource kSource(v.data(), uCut, 7, 4099);
    MovieScratchHeap kHeap;
    const RedHeapStats kBefore = kHeap.Stats();
    std::vector<unsigned char> vPixels(800 * 600 * 4);
    unsigned int uFrames = 0;
    unsigned int uAudio = 0;
    int iEnd = kMovieFrameReady;
    {
        MovieDecoder kDecoder;
        MovieAudioCollector kAudio;
        REQUIRE(kHeap.Open(kDecoder, kSource.Source(), kSeg.kSpan.uLength, &kInfo, &pcReason) == kMovieOpenOk);
        kAudio.Attach(kDecoder);
        CHECK(kDecoder.ProbeScore() == 0);
        CHECK(kDecoder.AudioChannels() == 2);
        for (int i = 0; i < 40 && iEnd == kMovieFrameReady; ++i)
        {
            MovieFrame kFrame;
            iEnd = kHeap.Decode(kDecoder, &kFrame);
            if (iEnd != kMovieFrameReady)
            {
                break;
            }
            ++uFrames;
            CHECK(kFrame.iWidth == 800);
            CHECK(kFrame.iHeight == 600);
            CHECK(!kFrame.bFullRange);
            ConvertYuv420ToXrgb(kFrame.apPlane[0], kFrame.aiStride[0], kFrame.apPlane[1], kFrame.aiStride[1],
                                kFrame.apPlane[2], kFrame.aiStride[2], kFrame.iWidth, kFrame.iHeight,
                                kFrame.bFullRange, vPixels.data(), 800 * 4);
            const short* pSamples = 0;
            uAudio += kAudio.Take(&pSamples);
        }
        CHECK(kDecoder.EndState() == kMovieSourceEnd);
        MovieFrame kFrame;
        CHECK(kHeap.Decode(kDecoder, &kFrame) == kMovieFrameEnd);
        kHeap.Close(kDecoder);
    }
    CHECK(uFrames == 30);
    CHECK(iEnd == kMovieFrameEnd);
    CHECK(uAudio > 0);
    CHECK(kSource.muZeroReads > 0);
    CHECK(kSource.muPumps > 0);
    CHECK(kHeap.Stats().uUsed == kBefore.uUsed);
}

TEST_CASE("movie decoder decodes every frame of the shortest shell segment" * doctest::test_suite("unit"))
{
    RetailSegment kSeg;
    SKIP_IF(!ShortestSegment("shell.mvs", &kSeg), "GameData movies not found");
    REQUIRE(kSeg.kSpan.uLength < 0x2000000);
    std::vector<unsigned char> v;
    REQUIRE(ReadMvsBytes(kSeg.sPath, kSeg.kSpan.llOffset, kSeg.kSpan.uLength, v));
    MemoryMovieSource kSource(v.data(), kSeg.kSpan.uLength, 0x10000, 0);
    MovieScratchHeap kHeap;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    unsigned int uFrames = 0;
    int iResult = kMovieFrameReady;
    {
        MovieDecoder kDecoder;
        REQUIRE(kHeap.Open(kDecoder, kSource.Source(), kSeg.kSpan.uLength, &kInfo, &pcReason) == kMovieOpenOk);
        while (iResult == kMovieFrameReady && uFrames <= kInfo.uFrames)
        {
            MovieFrame kFrame;
            iResult = kHeap.Decode(kDecoder, &kFrame);
            if (iResult == kMovieFrameReady)
            {
                ++uFrames;
            }
        }
        kHeap.Close(kDecoder);
    }
    INFO("segment " << kSeg.kSpan.uName << " frames " << kInfo.uFrames);
    CHECK(iResult == kMovieFrameEnd);
    CHECK(uFrames == kInfo.uFrames);
}

TEST_CASE("movie decoder keeps its blocks on the heap it opened on" * doctest::test_suite("unit"))
{
    const std::vector<unsigned char> v = BuildSyntheticMovie(SyntheticSpec(), 0);
    MovieScratchHeap kOpenHeap;
    MovieScratchHeap kOtherHeap;
    const RedHeapStats kOpenStart = kOpenHeap.Stats();
    MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), 0x10000, 0);
    MovieDecoder kDecoder;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    REQUIRE(kOpenHeap.Open(kDecoder, kSource.Source(), (unsigned int)v.size(), &kInfo, &pcReason) == kMovieOpenOk);
    const RedHeapStats kOpened = kOpenHeap.Stats();
    const RedHeapStats kOtherBefore = kOtherHeap.Stats();
    int iResult = kMovieFrameReady;
    for (int i = 0; i < 4 && iResult != kMovieFrameEnd; ++i)
    {
        MovieFrame kFrame;
        iResult = kOtherHeap.Decode(kDecoder, &kFrame);
    }
    const RedHeapStats kOtherAfter = kOtherHeap.Stats();
    const RedHeapStats kDecoded = kOpenHeap.Stats();
    kOpenHeap.Close(kDecoder);
    CHECK(kOtherAfter.uAllocs == kOtherBefore.uAllocs);
    CHECK(kOtherAfter.uUsed == kOtherBefore.uUsed);
    CHECK(kDecoded.uAllocs > kOpened.uAllocs);
    CHECK(kOpenHeap.Stats().uUsed == kOpenStart.uUsed);
}

namespace
{

struct LeBits
{
    std::vector<unsigned char> v;
    size_t uBit;

    LeBits() : uBit(0)
    {
    }

    void Put(unsigned int uValue, unsigned int uCount)
    {
        for (unsigned int i = 0; i < uCount; ++i, ++uBit)
        {
            if ((uBit >> 3) >= v.size())
            {
                v.push_back(0);
            }
            if ((uValue >> i) & 1)
            {
                v[uBit >> 3] |= (unsigned char)(1u << (uBit & 7));
            }
        }
    }

    void Align32()
    {
        while (uBit & 31)
        {
            Put(0, 1);
        }
    }
};

std::vector<unsigned char> AudioBlocksMovie(unsigned int uBlocks)
{
    SyntheticMovieSpec kSpec = SyntheticSpec();
    kSpec.uFrames = 1;
    kSpec.uSpan = 8192;
    unsigned int uHeaderEnd = 0;
    std::vector<unsigned char> v = BuildSyntheticMovie(kSpec, &uHeaderEnd);
    LeBits kBits;
    kBits.Put(uBlocks * 1920u * 4u, 32);
    for (unsigned int b = 0; b < uBlocks; ++b)
    {
        kBits.Put(0, 2);
        for (int ch = 0; ch < 2; ++ch)
        {
            kBits.Put(0, 29);
            kBits.Put(0, 29);
            for (int band = 0; band < 25; ++band)
            {
                kBits.Put(0, 8);
            }
            for (int run = 0; run < 4; ++run)
            {
                kBits.Put(1, 1);
                kBits.Put(15, 4);
                kBits.Put(0, 4);
            }
        }
        kBits.Align32();
    }
    const unsigned int uPacket = (unsigned int)kBits.v.size();
    REQUIRE(uPacket == 4 + 76 * uBlocks);
    REQUIRE(4 + uPacket < kSpec.uSpan);
    v[uHeaderEnd] = (unsigned char)uPacket;
    v[uHeaderEnd + 1] = (unsigned char)(uPacket >> 8);
    v[uHeaderEnd + 2] = 0;
    v[uHeaderEnd + 3] = 0;
    memcpy(&v[uHeaderEnd + 4], kBits.v.data(), uPacket);
    return v;
}

struct AudioBlocksRun
{
    unsigned int uFrames;
    unsigned int uPeakGrowth;
    unsigned int uUsedAfter;
    unsigned int uUsedBefore;
};

AudioBlocksRun DecodeAudioBlocks(unsigned int uBlocks)
{
    const std::vector<unsigned char> v = AudioBlocksMovie(uBlocks);
    MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), 0x10000, 0);
    MovieScratchHeap kHeap;
    AudioBlocksRun kRun;
    kRun.uUsedBefore = kHeap.Stats().uUsed;
    MovieDecoder kDecoder;
    MovieAudioCollector kAudio;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    REQUIRE(kHeap.Open(kDecoder, kSource.Source(), (unsigned int)v.size(), &kInfo, &pcReason) == kMovieOpenOk);
    REQUIRE(kDecoder.AudioChannels() == 2);
    kAudio.Attach(kDecoder);
    const unsigned int uPeakOpen = kHeap.Stats().uPeak;
    MovieFrame kFrame;
    kHeap.Decode(kDecoder, &kFrame);
    const short* pSamples = 0;
    kRun.uFrames = kAudio.Take(&pSamples);
    kRun.uPeakGrowth = kHeap.Stats().uPeak - uPeakOpen;
    kHeap.Close(kDecoder);
    kRun.uUsedAfter = kHeap.Stats().uUsed;
    return kRun;
}

}

TEST_CASE("movie decoder hands every decoded audio frame to the player without growing a buffer" * doctest::test_suite("unit"))
{
    const AudioBlocksRun kSmall = DecodeAudioBlocks(10);
    const AudioBlocksRun kLarge = DecodeAudioBlocks(66);
    CHECK(kSmall.uFrames == 10u * 1920u);
    CHECK(kLarge.uFrames == 66u * 1920u);
    CHECK(kLarge.uFrames > 16u * 4800u + 48000u);
    CHECK(kLarge.uPeakGrowth < kLarge.uFrames * 2u * (unsigned int)sizeof(short));
    MESSAGE("peak growth " << kSmall.uPeakGrowth << " and " << kLarge.uPeakGrowth << " for " << kLarge.uFrames * 4u << " audio bytes");
    CHECK(kSmall.uUsedAfter == kSmall.uUsedBefore);
    CHECK(kLarge.uUsedAfter == kLarge.uUsedBefore);
}

TEST_CASE("movie header caps fit the x86 runtime heap budget" * doctest::test_suite("unit"))
{
    const unsigned long long ullHeadroom = 12902464ull;
    const unsigned long long ullPixels = (unsigned long long)kMovieMaxWidth * kMovieMaxHeight;
    const unsigned long long ullFixed = 77829ull;
    const unsigned long long ullPixelBytes = ullPixels * 13075ull / 1000ull;
    const unsigned long long ullIndexBytes = (unsigned long long)kMovieMaxFrames * 493ull / 10ull;
    const unsigned long long ullPacketBytes = 2ull * kMovieMaxLargest;
    const unsigned long long ullRingBytes = 262240ull;
    const unsigned long long ullAudioChunk = (unsigned long long)kMovieAudioChunkFrames * kMovieAudioMaxChannels * 2ull;
    const unsigned long long ullAudioBlock =
        ((unsigned long long)kMovieAudioPackets * kMovieAudioPacketFrames + kMovieAudioHoldFrames) * kMovieAudioMaxChannels * 2ull;
    const unsigned long long ullTotal =
        ullFixed + ullPixelBytes + ullIndexBytes + ullPacketBytes + ullRingBytes + ullAudioChunk + ullAudioBlock;
    MESSAGE("caps " << kMovieMaxWidth << "x" << kMovieMaxHeight << " frames " << kMovieMaxFrames << " largest "
                    << kMovieMaxLargest << " total " << ullTotal << " headroom " << ullHeadroom);
    CHECK(ullAudioBlock == 499200ull);
    CHECK(ullTotal <= ullHeadroom);
    CHECK(kMovieHeaderMaxBytes <= 0x20000u);
}

TEST_CASE("movie reject memo skips its segment until cleared" * doctest::test_suite("unit"))
{
    MovieRejectMemo kMemo = {0};
    int aiSegment[2] = {0, 0};
    CHECK_FALSE(kMemo.Skips(&aiSegment[0]));
    CHECK_FALSE(kMemo.Skips(0));
    kMemo.Remember(&aiSegment[0]);
    CHECK(kMemo.Skips(&aiSegment[0]));
    CHECK_FALSE(kMemo.Skips(&aiSegment[1]));
    CHECK_FALSE(kMemo.Skips(0));
    kMemo.Clear();
    CHECK_FALSE(kMemo.Skips(&aiSegment[0]));
}

TEST_CASE("movie player remembers every open reject until the next open or close" * doctest::test_suite("unit"))
{
    std::string sText;
    REQUIRE(ReadRepoText("src/RedEngineFL/movie/pc/pcRedMoviePlayer.cpp", sText));
    std::string sFlat;
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (sText[i] != '\r')
        {
            sFlat += sText[i];
        }
    }
    const size_t uOpen = sFlat.find("bool RedMoviePlayer::Open(");
    const size_t uClose = sFlat.find("void RedMoviePlayer::Close()");
    REQUIRE(uOpen != std::string::npos);
    REQUIRE(uClose != std::string::npos);
    CHECK(sFlat.substr(uOpen, sFlat.find("\n}", uOpen) - uOpen).find("sRejected.Clear();") != std::string::npos);
    CHECK(sFlat.substr(uClose, sFlat.find("\n}", uClose) - uClose).find("sRejected.Clear();") != std::string::npos);
    CHECK(sFlat.find("const bool bRejected = sRejected.Skips(pSegment);") != std::string::npos);
    CHECK(sFlat.find("mPlayback = 0;\n                sRejected.Remember(pSegment);\n") != std::string::npos);
    CHECK(sFlat.find("\"OPENSWBF2 MOVIE reject reason=texture\");\n                sRejected.Remember(GetCurrentSegment());\n") !=
          std::string::npos);
}
