#include "third_party/doctest/doctest.h"

#include <windows.h>

#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"
#include "unit/movie_testdata.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/movie/pc/pcRedMovieAlloc.h"

extern "C"
{
    void __cdecl av_log_set_level(int iLevel);
}

namespace
{

int siMovieOomFailures;

void CountMovieOomFailure(int iHeap, unsigned int uSize)
{
    (void)iHeap;
    (void)uSize;
    ++siMovieOomFailures;
}

class MovieOomFailureCount
{
public:
    MovieOomFailureCount() : mpSaved(OnFailure)
    {
        siMovieOomFailures = 0;
        OnFailure = CountMovieOomFailure;
    }

    ~MovieOomFailureCount()
    {
        OnFailure = mpSaved;
    }

private:
    void (*mpSaved)(int heap, unsigned int size);
};

class MovieOomHeap
{
public:
    explicit MovieOomHeap(size_t uSize) : miHeap(-1), mpArena(0)
    {
        av_log_set_level(-8);
        mpArena = (char*)VirtualAlloc(0, uSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        REQUIRE(mpArena != nullptr);
        miHeap = RedCreateHeap(mpArena, mpArena + uSize, 0x10, "MovieOomTest");
        REQUIRE(miHeap >= 0);
    }

    ~MovieOomHeap()
    {
        RedDestroyHeap(miHeap);
        VirtualFree(mpArena, 0, MEM_RELEASE);
    }

    unsigned int Used()
    {
        RedHeapStats kStats;
        memset(&kStats, 0, sizeof(kStats));
        REQUIRE(RedGetHeapStats(miHeap, &kStats));
        return kStats.uUsed;
    }

    int miHeap;

private:
    MovieOomHeap(const MovieOomHeap&);
    MovieOomHeap& operator=(const MovieOomHeap&);

    char* mpArena;
};

class MovieOomCurrent
{
public:
    explicit MovieOomCurrent(int iHeap) : miSaved(RedSetCurrentHeap(iHeap))
    {
    }

    ~MovieOomCurrent()
    {
        RedSetCurrentHeap(miSaved);
    }

private:
    int miSaved;
};

const int kOomLedgerSize = 8192;

struct OomBlock
{
    void* p;
    size_t uSize;
};

OomBlock saOomLedger[kOomLedgerSize];
int siOomLive;
int siOomCalls;
int siOomFailAt;
bool sbOomOverflow;

void OomAdd(void* p, size_t uSize)
{
    if (siOomLive == kOomLedgerSize)
    {
        sbOomOverflow = true;
        return;
    }
    saOomLedger[siOomLive].p = p;
    saOomLedger[siOomLive].uSize = uSize;
    ++siOomLive;
}

void OomRemove(void* p)
{
    for (int i = 0; i < siOomLive; ++i)
    {
        if (saOomLedger[i].p == p)
        {
            saOomLedger[i] = saOomLedger[--siOomLive];
            return;
        }
    }
}

bool OomFailNow()
{
    ++siOomCalls;
    return siOomCalls == siOomFailAt;
}

int siOomFullHeap = -1;
bool sbOomFullGave;

void* OomFailOnFullHeap(size_t uSize)
{
    MovieOomCurrent kFull(siOomFullHeap);
    void* p = MovieAlloc(uSize);
    if (p)
    {
        sbOomFullGave = true;
        MovieFree(p);
    }
    return 0;
}

void* __cdecl OomAlloc(size_t uSize)
{
    if (OomFailNow())
    {
        return OomFailOnFullHeap(uSize);
    }
    void* p = MovieAlloc(uSize);
    if (p)
    {
        OomAdd(p, uSize);
    }
    return p;
}

void* __cdecl OomRealloc(void* pOld, size_t uSize)
{
    if (OomFailNow())
    {
        return OomFailOnFullHeap(uSize);
    }
    void* p = MovieRealloc(pOld, uSize);
    if (p)
    {
        if (pOld)
        {
            OomRemove(pOld);
        }
        OomAdd(p, uSize);
    }
    return p;
}

void __cdecl OomFree(void* p)
{
    if (p)
    {
        OomRemove(p);
    }
    MovieFree(p);
}

class OomAllocatorScope
{
public:
    OomAllocatorScope() : mpArena(0)
    {
        MovieAllocRegister();
        const size_t uArena = 0x10000;
        mpArena = (char*)VirtualAlloc(0, uArena, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        REQUIRE(mpArena != nullptr);
        siOomFullHeap = RedCreateHeap(mpArena, mpArena + uArena, 0x10, "MovieOomFull");
        REQUIRE(siOomFullHeap >= 0);
        for (size_t uBlock = 0x8000; uBlock >= 0x10; uBlock >>= 1)
        {
            while (RedTryAllocFromHeap(siOomFullHeap, uBlock))
            {
            }
        }
        sbOomFullGave = false;
        ossf2_set_allocator(OomAlloc, OomRealloc, OomFree);
    }

    ~OomAllocatorScope()
    {
        ossf2_set_allocator(MovieAlloc, MovieRealloc, MovieFree);
        RedDestroyHeap(siOomFullHeap);
        siOomFullHeap = -1;
        VirtualFree(mpArena, 0, MEM_RELEASE);
    }

private:
    char* mpArena;
};

unsigned long long OomFnv(unsigned long long uHash, const unsigned char* p, size_t uSize)
{
    for (size_t i = 0; i < uSize; ++i)
    {
        uHash ^= p[i];
        uHash *= 0x100000001B3ull;
    }
    return uHash;
}

unsigned long long OomFrameHash(const MovieFrame& kFrame)
{
    unsigned long long uHash = 0xCBF29CE484222325ull;
    for (int iPlane = 0; iPlane < 3; ++iPlane)
    {
        const int iWidth = iPlane == 0 ? kFrame.iWidth : (kFrame.iWidth + 1) / 2;
        const int iHeight = iPlane == 0 ? kFrame.iHeight : (kFrame.iHeight + 1) / 2;
        for (int y = 0; y < iHeight; ++y)
        {
            uHash = OomFnv(uHash, kFrame.apPlane[iPlane] + (ptrdiff_t)y * kFrame.aiStride[iPlane], (size_t)iWidth);
        }
    }
    return uHash;
}

struct OomRun
{
    std::string sReason;
    int iOpen;
    int iEnd;
    int iCalls;
    int iLive;
    std::vector<unsigned long long> vOutput;
};

OomRun RunWithFailure(MovieOomHeap& kHeap, const unsigned char* pBytes, unsigned int uSize,
                      unsigned int uSegmentLength, int iFailAt)
{
    OomRun kRun;
    kRun.vOutput.reserve(64);
    siOomLive = 0;
    siOomCalls = 0;
    siOomFailAt = iFailAt;
    MemoryMovieSource kSource(pBytes, uSize, 0x10000, 0);
    MovieDecoder kDecoder;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    {
        MovieOomCurrent kCurrent(kHeap.miHeap);
        kRun.iOpen = kDecoder.Open(kSource.Source(), uSegmentLength, &kInfo, &pcReason);
    }
    MovieAudioCollector kAudio;
    kAudio.Attach(kDecoder);
    kRun.sReason = pcReason ? pcReason : "";
    kRun.iEnd = kMovieFrameError;
    while (kRun.iOpen == kMovieOpenOk && kRun.vOutput.size() < 60)
    {
        MovieFrame kFrame;
        const short* pSamples = 0;
        unsigned int uFrames = 0;
        int iResult;
        {
            MovieOomCurrent kCurrent(kHeap.miHeap);
            iResult = kDecoder.DecodeVideoFrame(&kFrame);
            uFrames = kAudio.Take(&pSamples);
        }
        unsigned long long uAudio = OomFnv(0xCBF29CE484222325ull, (const unsigned char*)pSamples,
                                           (size_t)uFrames * kDecoder.AudioChannels() * sizeof(short));
        if (iResult != kMovieFrameReady)
        {
            kRun.iEnd = iResult;
            kRun.vOutput.push_back(uAudio);
            break;
        }
        kRun.vOutput.push_back(OomFrameHash(kFrame) ^ uAudio);
    }
    {
        MovieOomCurrent kCurrent(kHeap.miHeap);
        kDecoder.Close();
    }
    kRun.iCalls = siOomCalls;
    kRun.iLive = siOomLive;
    return kRun;
}

void SweepEveryAllocation(const std::vector<unsigned char>& v, unsigned int uSegmentLength, unsigned int uMinFrames,
                          bool bCompare)
{
    MovieOomHeap kHeap(0x1000000);
    const unsigned int uBefore = kHeap.Used();
    OomAllocatorScope kScope;
    MovieOomFailureCount kCount;
    sbOomOverflow = false;
    const OomRun kClean = RunWithFailure(kHeap, v.data(), (unsigned int)v.size(), uSegmentLength, -1);
    REQUIRE(kClean.iOpen == kMovieOpenOk);
    REQUIRE(kClean.iEnd == (bCompare ? kMovieFrameEnd : kMovieFrameError));
    REQUIRE(kClean.vOutput.size() >= uMinFrames + 1);
    REQUIRE(kClean.iLive == 0);
    unsigned int uRejected = 0;
    unsigned int uFailedFrames = 0;
    unsigned int uAbsorbed = 0;
    unsigned int uLeaks = 0;
    unsigned int uDiffer = 0;
    unsigned int uNotMemory = 0;
    for (int iFail = 1; iFail <= kClean.iCalls; ++iFail)
    {
        WatchdogArm("movie oom sweep");
        const OomRun kRun = RunWithFailure(kHeap, v.data(), (unsigned int)v.size(), uSegmentLength, iFail);
        if (kRun.iLive != 0)
        {
            ++uLeaks;
            MESSAGE("failing allocation " << iFail << " left " << kRun.iLive << " blocks");
        }
        if (kRun.iOpen != kMovieOpenOk)
        {
            ++uRejected;
            if (kRun.sReason != "memory")
            {
                ++uNotMemory;
                MESSAGE("failing allocation " << iFail << " rejected the open with reason " << kRun.sReason);
            }
            continue;
        }
        if (!bCompare)
        {
            ++uFailedFrames;
            continue;
        }
        bool bSame = kRun.vOutput.size() <= kClean.vOutput.size();
        for (size_t i = 0; bSame && i + 1 < kRun.vOutput.size(); ++i)
        {
            bSame = kRun.vOutput[i] == kClean.vOutput[i];
        }
        if (kRun.iEnd == kMovieFrameEnd)
        {
            ++uAbsorbed;
            bSame = bSame && kRun.vOutput == kClean.vOutput;
        }
        else
        {
            ++uFailedFrames;
            bSame = bSame && kRun.iEnd == kMovieFrameError;
        }
        if (!bSame)
        {
            ++uDiffer;
            MESSAGE("failing allocation " << iFail << " gave different output, end " << kRun.iEnd);
        }
    }
    MESSAGE("allocations " << kClean.iCalls << " rejected " << uRejected << " frame errors " << uFailedFrames
                           << " absorbed " << uAbsorbed);
    CHECK(!sbOomOverflow);
    CHECK(uLeaks == 0);
    CHECK(uDiffer == 0);
    CHECK(uNotMemory == 0);
    CHECK(!sbOomFullGave);
    CHECK(uRejected > 0);
    CHECK(uFailedFrames > 0);
    CHECK(siMovieOomFailures == 0);
    CHECK(kHeap.Used() == uBefore);
}

unsigned int Read32(const unsigned char* p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

bool RetailShellCut(unsigned int uFrames, std::vector<unsigned char>& v, unsigned int* puSegmentLength)
{
    const std::string sDir = MovieDataDir();
    if (sDir.empty())
    {
        return false;
    }
    const std::string sPath = sDir + "\\shell.mvs";
    std::vector<MvsSpan> vSpans;
    if (!ListMvsSegments(sPath, vSpans) || vSpans.empty())
    {
        return false;
    }
    std::vector<unsigned char> vHead;
    const unsigned int uHeadSize = vSpans[0].uLength < kMovieHeaderMaxBytes ? vSpans[0].uLength : kMovieHeaderMaxBytes;
    REQUIRE(ReadMvsBytes(sPath, vSpans[0].llOffset, uHeadSize, vHead));
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    REQUIRE(CheckMovieHeader(vHead.data(), uHeadSize, vSpans[0].uLength, &kInfo, &pcReason) == kMovieHeaderOk);
    REQUIRE(kInfo.uFrames > uFrames);
    const unsigned int uIndexAt = kInfo.uHeaderEnd - 4 * (kInfo.uFrames + 1);
    const unsigned int uCut = Read32(vHead.data() + uIndexAt + 4 * uFrames) & ~1u;
    REQUIRE(ReadMvsBytes(sPath, vSpans[0].llOffset, uCut, v));
    *puSegmentLength = vSpans[0].uLength;
    return true;
}

}

TEST_CASE("movie decoder survives every failed allocation of a synthetic stream without a leak" *
          doctest::test_suite("unit"))
{
    const std::vector<unsigned char> v = BuildSyntheticMovie(SyntheticSpec(), 0);
    SweepEveryAllocation(v, (unsigned int)v.size(), 0, false);
}

TEST_CASE("movie decoder survives every failed allocation of eight shell frames with the same output" *
          doctest::test_suite("unit"))
{
    std::vector<unsigned char> v;
    unsigned int uSegmentLength = 0;
    SKIP_IF(!RetailShellCut(8, v, &uSegmentLength), "GameData movies not found");
    SweepEveryAllocation(v, uSegmentLength, 8, true);
}

TEST_CASE("movie decoder rejects a heap that runs out mid-open without the failure handler" *
          doctest::test_suite("unit"))
{
    const std::vector<unsigned char> v = BuildSyntheticMovie(SyntheticSpec(), 0);
    unsigned int uRejects = 0;
    unsigned int uLeaks = 0;
    unsigned int uFailures = 0;
    bool bOpened = false;
    for (size_t uArena = 0x10000; uArena <= 0x400000 && !bOpened; uArena += 0x1000)
    {
        WatchdogArm("movie oom arena");
        MovieOomHeap kHeap(uArena);
        const unsigned int uBefore = kHeap.Used();
        MovieOomFailureCount kCount;
        MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), 4096, 0);
        MovieDecoder kDecoder;
        MovieHeaderInfo kInfo;
        const char* pcReason = 0;
        int iOpen;
        {
            MovieOomCurrent kCurrent(kHeap.miHeap);
            iOpen = kDecoder.Open(kSource.Source(), (unsigned int)v.size(), &kInfo, &pcReason);
        }
        if (iOpen == kMovieOpenOk)
        {
            bOpened = true;
        }
        else
        {
            ++uRejects;
            CHECK(std::string(pcReason ? pcReason : "") == "memory");
        }
        {
            MovieOomCurrent kCurrent(kHeap.miHeap);
            kDecoder.Close();
        }
        uFailures += (unsigned int)siMovieOomFailures;
        if (kHeap.Used() != uBefore)
        {
            ++uLeaks;
        }
    }
    CHECK(bOpened);
    CHECK(uRejects > 0);
    CHECK(uLeaks == 0);
    CHECK(uFailures == 0);
}

TEST_CASE("movie decoder fails a frame when the heap runs out mid-decode without the failure handler" *
          doctest::test_suite("unit"))
{
    std::vector<unsigned char> v;
    unsigned int uSegmentLength = 0;
    SKIP_IF(!RetailShellCut(8, v, &uSegmentLength), "GameData movies not found");
    MovieOomHeap kHeap(0x1000000);
    const unsigned int uBefore = kHeap.Used();
    MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), 0x10000, 0);
    std::vector<void*> vFill;
    vFill.reserve(0x10000);
    int iOpen = kMovieOpenReject;
    int iFirst = kMovieFrameError;
    int iFull = kMovieFrameReady;
    int iFailures = -1;
    {
        MovieDecoder kDecoder;
        MovieHeaderInfo kInfo;
        const char* pcReason = 0;
        MovieFrame kFrame;
        {
            MovieOomCurrent kCurrent(kHeap.miHeap);
            iOpen = kDecoder.Open(kSource.Source(), uSegmentLength, &kInfo, &pcReason);
            if (iOpen == kMovieOpenOk)
            {
                iFirst = kDecoder.DecodeVideoFrame(&kFrame);
            }
        }
        if (iOpen == kMovieOpenOk && iFirst == kMovieFrameReady)
        {
            for (size_t uBlock = 0x100000; uBlock >= 0x10 && vFill.size() < vFill.capacity(); uBlock >>= 1)
            {
                while (vFill.size() < vFill.capacity())
                {
                    void* p = RedTryAllocFromHeap(kHeap.miHeap, uBlock);
                    if (!p)
                    {
                        break;
                    }
                    vFill.push_back(p);
                }
            }
            MovieOomFailureCount kCount;
            {
                MovieOomCurrent kCurrent(kHeap.miHeap);
                for (int i = 0; i < 4 && iFull == kMovieFrameReady; ++i)
                {
                    iFull = kDecoder.DecodeVideoFrame(&kFrame);
                }
            }
            iFailures = siMovieOomFailures;
            for (size_t i = 0; i < vFill.size(); ++i)
            {
                RedFreeToHeap(kHeap.miHeap, vFill[i], 0);
            }
        }
        MovieOomCurrent kCurrent(kHeap.miHeap);
        kDecoder.Close();
    }
    REQUIRE(iOpen == kMovieOpenOk);
    REQUIRE(iFirst == kMovieFrameReady);
    REQUIRE(!vFill.empty());
    REQUIRE(vFill.size() < vFill.capacity());
    CHECK(iFull == kMovieFrameError);
    CHECK(iFailures == 0);
    CHECK(kHeap.Used() == uBefore);
}

TEST_CASE("movie open rejected for memory is remembered until the next open and then plays" * doctest::test_suite("unit"))
{
    std::vector<unsigned char> v;
    unsigned int uSegmentLength = 0;
    SKIP_IF(!RetailShellCut(8, v, &uSegmentLength), "GameData movies not found");
    MovieRejectMemo kMemo = {0};
    const void* pSegment = &v;
    {
        MovieOomHeap kSmall(0x10000);
        const unsigned int uBefore = kSmall.Used();
        MovieOomFailureCount kCount;
        MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), 0x10000, 0);
        MovieDecoder kDecoder;
        MovieHeaderInfo kInfo;
        const char* pcReason = 0;
        MovieOomCurrent kCurrent(kSmall.miHeap);
        CHECK(kDecoder.Open(kSource.Source(), uSegmentLength, &kInfo, &pcReason) == kMovieOpenReject);
        CHECK(std::string(pcReason ? pcReason : "") == "memory");
        kMemo.Remember(pSegment);
        kDecoder.Close();
        CHECK(kSmall.Used() == uBefore);
        CHECK(siMovieOomFailures == 0);
    }
    CHECK(kMemo.Skips(pSegment));
    kMemo.Clear();
    REQUIRE_FALSE(kMemo.Skips(pSegment));
    MovieOomHeap kLarge(0x1000000);
    MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), 0x10000, 0);
    MovieDecoder kDecoder;
    MovieAudioCollector kAudio;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    MovieOomCurrent kCurrent(kLarge.miHeap);
    REQUIRE(kDecoder.Open(kSource.Source(), uSegmentLength, &kInfo, &pcReason) == kMovieOpenOk);
    kAudio.Attach(kDecoder);
    unsigned int uShown = 0;
    unsigned int uAudio = 0;
    for (int i = 0; i < 8; ++i)
    {
        MovieFrame kFrame;
        if (kDecoder.DecodeVideoFrame(&kFrame) == kMovieFrameReady)
        {
            ++uShown;
        }
        const short* pSamples = 0;
        uAudio += kAudio.Take(&pSamples);
    }
    kDecoder.Close();
    CHECK(uShown == 8);
    CHECK(uAudio > 0);
}
