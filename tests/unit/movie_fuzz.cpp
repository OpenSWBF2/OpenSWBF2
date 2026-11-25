#include "third_party/doctest/doctest.h"

#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "testutil.h"
#include "unit/movie_testdata.h"

namespace
{

const char* const kapcMovieReasons[] = {"magic", "revision", "size",   "frames", "largest", "width", "height",
                                        "fps",   "tracks",   "rate",   "aflags", "index",   "order", "range"};

const unsigned int kauMovieFieldValues[] = {0u,
                                            1u,
                                            2u,
                                            kMovieMaxWidth,
                                            kMovieMaxWidth + 1,
                                            kMovieMaxHeight,
                                            kMovieMaxHeight + 1,
                                            kMovieMaxFrames,
                                            kMovieMaxFrames + 1,
                                            kMovieMaxLargest,
                                            kMovieMaxLargest + 1,
                                            kMovieMaxTracks,
                                            kMovieMaxTracks + 1,
                                            kMovieMinRate - 1,
                                            kMovieMaxRate,
                                            kMovieMaxRate + 1,
                                            0x7000u,
                                            0x7FFFFFFFu,
                                            0x80000000u,
                                            0xFFFFFFFFu};

bool KnownReason(const char* pcReason)
{
    if (!pcReason)
    {
        return false;
    }
    for (size_t i = 0; i < sizeof(kapcMovieReasons) / sizeof(kapcMovieReasons[0]); ++i)
    {
        if (strcmp(pcReason, kapcMovieReasons[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

unsigned int FuzzFnv(unsigned int uSum, const unsigned char* p, size_t uSize)
{
    for (size_t i = 0; i < uSize; ++i)
    {
        uSum = (uSum ^ p[i]) * 16777619u;
    }
    return (uSum ^ (unsigned int)uSize) * 16777619u;
}

unsigned int FuzzMix(unsigned int uSum, unsigned int uValue)
{
    return FuzzFnv(uSum, (const unsigned char*)&uValue, sizeof(uValue));
}

void Mutate(std::mt19937& rng, std::vector<unsigned char>& b, size_t uProtect)
{
    const int iMutations = 1 + (int)(rng() % 6);
    for (int m = 0; m < iMutations && !b.empty(); ++m)
    {
        const bool bAnywhere = uProtect >= b.size() || rng() % 10 == 0;
        const size_t uFrom = bAnywhere ? 0 : uProtect;
        const size_t uPos = uFrom + rng() % (b.size() - uFrom);
        switch (rng() % 8)
        {
        case 0:
        case 1:
            b[uPos] ^= (unsigned char)(1u << (rng() % 8));
            break;
        case 2:
        case 3:
        {
            static const unsigned char acSet[4] = {0x00, 0xFF, 0x7F, 0x80};
            b[uPos] = acSet[rng() % 4];
            break;
        }
        case 4:
        case 5:
        {
            const size_t uAt = (uPos & ~(size_t)3) + 4 <= b.size() ? (uPos & ~(size_t)3) : uPos;
            if (uAt + 4 > b.size())
            {
                break;
            }
            const unsigned int uValue = (rng() & 1)
                                            ? kauMovieFieldValues[rng() % (sizeof(kauMovieFieldValues) /
                                                                           sizeof(kauMovieFieldValues[0]))]
                                            : (unsigned int)rng();
            memcpy(&b[uAt], &uValue, 4);
            break;
        }
        case 6:
            b[uPos] = (unsigned char)rng();
            break;
        default:
            if (rng() % 4 == 0)
            {
                b.resize(uFrom + rng() % (b.size() - uFrom + 1));
            }
            break;
        }
    }
}

struct HeaderBase
{
    std::vector<unsigned char> vBytes;
    unsigned int uSegment;
};

HeaderBase MakeHeaderBase(std::mt19937& rng)
{
    SyntheticMovieSpec kSpec = SyntheticSpec();
    static const char acRevisions[4] = {'b', 'i', 'i', 'k'};
    kSpec.acMagic[3] = acRevisions[rng() % 4];
    kSpec.uFrames = 1 + rng() % 64;
    kSpec.uTracks = rng() % 3;
    kSpec.uWidth = 1 + rng() % kMovieMaxWidth;
    kSpec.uHeight = 1 + rng() % kMovieMaxHeight;
    kSpec.uSpan = 8 + rng() % 64;
    kSpec.uRate = (rng() & 1) ? 48000 : 44100;
    kSpec.uSeed = rng();
    unsigned int uHeaderEnd = 0;
    HeaderBase k;
    k.vBytes = BuildSyntheticMovie(kSpec, &uHeaderEnd);
    k.uSegment = (unsigned int)k.vBytes.size();
    k.vBytes.resize(uHeaderEnd);
    return k;
}

std::vector<unsigned char> HeaderInput(unsigned int uSeed, unsigned int uIter, unsigned int* puSegment)
{
    std::mt19937 rng(uSeed ^ (uIter * 0x9E3779B1u));
    HeaderBase k = MakeHeaderBase(rng);
    Mutate(rng, k.vBytes, 0);
    *puSegment = (rng() % 16 == 0) ? (unsigned int)rng() : k.uSegment;
    return k.vBytes;
}

class GuardRegion
{
public:
    GuardRegion() : mpRegion(0), muSize(0)
    {
        SYSTEM_INFO kSys;
        GetSystemInfo(&kSys);
        const size_t uPage = kSys.dwPageSize;
        muSize = ((kMovieHeaderMaxBytes + 4096 + uPage - 1) / uPage) * uPage;
        mpRegion = (unsigned char*)VirtualAlloc(0, muSize + uPage, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        REQUIRE(mpRegion != nullptr);
        DWORD dwOld = 0;
        REQUIRE(VirtualProtect(mpRegion + muSize, uPage, PAGE_NOACCESS, &dwOld));
    }

    ~GuardRegion()
    {
        VirtualFree(mpRegion, 0, MEM_RELEASE);
    }

    const unsigned char* Place(const std::vector<unsigned char>& v)
    {
        unsigned char* pAt = mpRegion + muSize - v.size();
        if (!v.empty())
        {
            memcpy(pAt, v.data(), v.size());
        }
        return pAt;
    }

    size_t SizeLimit() const
    {
        return muSize;
    }

private:
    unsigned char* mpRegion;
    size_t muSize;
};

bool RunHeaderFuzz(unsigned int uSeed, unsigned int uCount, unsigned int* puSum = 0)
{
    GuardRegion kRegion;
    unsigned int uSum = 2166136261u;
    unsigned int auResults[3] = {0, 0, 0};
    for (unsigned int uIter = 0; uIter < uCount; ++uIter)
    {
        char acLabel[64];
        sprintf(acLabel, "movie header seed=%u iter=%u", uSeed, uIter);
        WatchdogArm(acLabel);
        unsigned int uSegment = 0;
        const std::vector<unsigned char> v = HeaderInput(uSeed, uIter, &uSegment);
        if (v.size() > kRegion.SizeLimit())
        {
            printf("movie header fuzz FAIL oversize: %s\n", acLabel);
            return false;
        }
        const unsigned char* pBytes = kRegion.Place(v);
        MovieHeaderInfo kInfo;
        const char* pcReason = 0;
        const int iResult = CheckMovieHeader(pBytes, (unsigned int)v.size(), uSegment, &kInfo, &pcReason);
        bool bOk = false;
        if (iResult == kMovieHeaderOk)
        {
            bOk = pcReason == 0 && kInfo.uWidth >= 1 && kInfo.uWidth <= kMovieMaxWidth && kInfo.uHeight >= 1 &&
                  kInfo.uHeight <= kMovieMaxHeight && kInfo.uFrames >= 1 && kInfo.uFrames <= kMovieMaxFrames &&
                  kInfo.uLargest >= 1 && kInfo.uLargest <= kMovieMaxLargest && kInfo.uTracks <= kMovieMaxTracks &&
                  kInfo.uHeaderEnd <= v.size() && (kInfo.uIndex0 & ~1u) == kInfo.uHeaderEnd &&
                  kInfo.uFileSize <= uSegment && kInfo.uLargest <= kInfo.uFileSize &&
                  (unsigned long long)kInfo.uFpsNum >= kInfo.uFpsDen &&
                  (unsigned long long)kInfo.uFpsNum <= 120ull * kInfo.uFpsDen;
        }
        else if (iResult == kMovieHeaderNeedMore)
        {
            bOk = pcReason == 0 && kInfo.uNeeded > v.size() && kInfo.uNeeded <= kMovieHeaderMaxBytes;
        }
        else if (iResult == kMovieHeaderReject)
        {
            bOk = KnownReason(pcReason);
        }
        if (!bOk)
        {
            printf("movie header fuzz FAIL result=%d reason=%s: %s\n", iResult, pcReason ? pcReason : "-", acLabel);
            fflush(stdout);
            return false;
        }
        if (iResult >= 0 && iResult < 3)
        {
            ++auResults[iResult];
        }
        uSum = FuzzMix(FuzzFnv(uSum, v.data(), v.size()), (unsigned int)iResult);
    }
    printf("movie-header-fuzz seed=%u count=%u ok=%u more=%u reject=%u sum=0x%08x\n", uSeed, uCount, auResults[0],
           auResults[1], auResults[2], uSum);
    fflush(stdout);
    if (puSum)
    {
        *puSum = uSum;
    }
    return true;
}

struct DecoderBase
{
    std::vector<unsigned char> vBytes;
    unsigned int uSegment;
    unsigned int uHeaderEnd;
};

bool ReadRetailHead(const std::string& sPath, const MvsSpan& kSpan, DecoderBase* pOut)
{
    const unsigned int uSize = kSpan.uLength < 0x40000 ? kSpan.uLength : 0x40000;
    if (!ReadMvsBytes(sPath, kSpan.llOffset, uSize, pOut->vBytes))
    {
        return false;
    }
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    if (CheckMovieHeader(pOut->vBytes.data(), uSize, kSpan.uLength, &kInfo, &pcReason) != kMovieHeaderOk)
    {
        return false;
    }
    pOut->uSegment = kSpan.uLength;
    pOut->uHeaderEnd = kInfo.uHeaderEnd;
    return true;
}

std::vector<DecoderBase> DecoderBases(unsigned int* puRetail)
{
    std::vector<DecoderBase> vBases;
    SyntheticMovieSpec kSpec = SyntheticSpec();
    for (unsigned int i = 0; i < 2; ++i)
    {
        DecoderBase k;
        if (i == 1)
        {
            kSpec.acMagic[3] = 'k';
            kSpec.uTracks = 2;
            kSpec.uWidth = 40;
            kSpec.uHeight = 24;
            kSpec.uFrames = 6;
            kSpec.uRate = 44100;
            kSpec.uAudioFlags = 0x5000;
        }
        kSpec.uSeed = 77 + i;
        k.vBytes = BuildSyntheticMovie(kSpec, &k.uHeaderEnd);
        k.uSegment = (unsigned int)k.vBytes.size();
        vBases.push_back(k);
    }
    *puRetail = 0;
    const std::string sDir = MovieDataDir();
    if (sDir.empty())
    {
        return vBases;
    }
    const char* apcFiles[2] = {"shell.mvs", "ingame.mvs"};
    for (int i = 0; i < 2; ++i)
    {
        const std::string sPath = sDir + "\\" + apcFiles[i];
        std::vector<MvsSpan> vSpans;
        DecoderBase k;
        REQUIRE(ListMvsSegments(sPath, vSpans));
        REQUIRE(!vSpans.empty());
        REQUIRE(ReadRetailHead(sPath, vSpans[0], &k));
        vBases.push_back(k);
        ++*puRetail;
    }
    std::vector<std::string> vFiles;
    WIN32_FIND_DATAA kData;
    HANDLE hFind = FindFirstFileA((sDir + "\\*.mvs").c_str(), &kData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            vFiles.push_back(kData.cFileName);
        } while (FindNextFileA(hFind, &kData));
        FindClose(hFind);
    }
    std::sort(vFiles.begin(), vFiles.end());
    for (size_t f = 0; f < vFiles.size(); ++f)
    {
        const std::string sPath = sDir + "\\" + vFiles[f];
        std::vector<MvsSpan> vSpans;
        REQUIRE(ListMvsSegments(sPath, vSpans));
        for (size_t s = 0; s < vSpans.size(); ++s)
        {
            std::vector<unsigned char> vHead;
            REQUIRE(ReadMvsBytes(sPath, vSpans[s].llOffset, 56, vHead));
            if (((unsigned int)vHead[48] | ((unsigned int)vHead[49] << 8)) == 44100)
            {
                DecoderBase k;
                REQUIRE(ReadRetailHead(sPath, vSpans[s], &k));
                vBases.push_back(k);
                ++*puRetail;
                return vBases;
            }
        }
    }
    FAIL("no 44100 Hz retail segment");
    return vBases;
}

std::vector<unsigned char> DecoderInput(const std::vector<DecoderBase>& vBases, unsigned int uSeed, unsigned int uIter,
                                        unsigned int* puSegment, unsigned int* puChunk)
{
    std::mt19937 rng(uSeed ^ (uIter * 0x9E3779B1u));
    const DecoderBase& kBase = vBases[rng() % vBases.size()];
    std::vector<unsigned char> v = kBase.vBytes;
    Mutate(rng, v, kBase.uHeaderEnd);
    static const unsigned int auChunks[4] = {7, 4096, 32768, 0x40000};
    *puChunk = auChunks[rng() % 4];
    *puSegment = kBase.uSegment;
    return v;
}

const unsigned int kMovieFuzzFrames = 8;

bool RunDecoderFuzz(unsigned int uSeed, unsigned int uCount, unsigned int* puSum = 0)
{
    unsigned int uRetail = 0;
    const std::vector<DecoderBase> vBases = DecoderBases(&uRetail);
    MovieScratchHeap kHeap;
    const unsigned int uUsedStart = kHeap.Stats().uUsed;
    unsigned int uSum = 2166136261u;
    unsigned int uOpened = 0;
    unsigned int uRejected = 0;
    unsigned int uFrames = 0;
    unsigned int uEnds = 0;
    unsigned int uErrors = 0;
    for (unsigned int uIter = 0; uIter < uCount; ++uIter)
    {
        char acLabel[64];
        sprintf(acLabel, "movie decoder seed=%u iter=%u", uSeed, uIter);
        WatchdogArm(acLabel);
        unsigned int uSegment = 0;
        unsigned int uChunk = 0;
        const std::vector<unsigned char> v = DecoderInput(vBases, uSeed, uIter, &uSegment, &uChunk);
        MemoryMovieSource kSource(v.data(), (unsigned int)v.size(), uChunk, 0);
        MovieDecoder kDecoder;
        MovieHeaderInfo kInfo;
        const char* pcReason = 0;
        const int iOpen = kHeap.Open(kDecoder, kSource.Source(), uSegment, &kInfo, &pcReason);
        MovieAudioCollector kAudio;
        kAudio.Attach(kDecoder);
        uSum = FuzzMix(FuzzFnv(uSum, v.data(), v.size()), (unsigned int)iOpen);
        if (iOpen == kMovieOpenReject)
        {
            if (!KnownReason(pcReason) && !(pcReason && strcmp(pcReason, "decoder") == 0))
            {
                printf("movie decoder fuzz FAIL reason=%s: %s\n", pcReason ? pcReason : "-", acLabel);
                fflush(stdout);
                return false;
            }
            ++uRejected;
        }
        else if (iOpen == kMovieOpenOk)
        {
            ++uOpened;
            if (kDecoder.ProbeScore() != 0)
            {
                printf("movie decoder fuzz FAIL probe=%d: %s\n", kDecoder.ProbeScore(), acLabel);
                fflush(stdout);
                return false;
            }
            int iResult = kMovieFrameReady;
            for (unsigned int f = 0; f < kMovieFuzzFrames && iResult == kMovieFrameReady; ++f)
            {
                MovieFrame kFrame;
                iResult = kHeap.Decode(kDecoder, &kFrame);
                const short* pSamples = 0;
                const unsigned int uAudio = kAudio.Take(&pSamples);
                uSum = FuzzMix(FuzzMix(uSum, (unsigned int)iResult), uAudio);
                if (iResult == kMovieFrameReady)
                {
                    ++uFrames;
                    if (kFrame.iWidth < 1 || kFrame.iWidth > (int)kMovieMaxWidth || kFrame.iHeight < 1 ||
                        kFrame.iHeight > (int)kMovieMaxHeight || !kFrame.apPlane[0] || !kFrame.apPlane[1] ||
                        !kFrame.apPlane[2])
                    {
                        printf("movie decoder fuzz FAIL frame %dx%d: %s\n", kFrame.iWidth, kFrame.iHeight, acLabel);
                        fflush(stdout);
                        return false;
                    }
                }
                else if (iResult == kMovieFrameEnd)
                {
                    ++uEnds;
                }
                else if (iResult == kMovieFrameError)
                {
                    ++uErrors;
                }
                else
                {
                    printf("movie decoder fuzz FAIL result=%d: %s\n", iResult, acLabel);
                    fflush(stdout);
                    return false;
                }
            }
        }
        else
        {
            printf("movie decoder fuzz FAIL open=%d: %s\n", iOpen, acLabel);
            fflush(stdout);
            return false;
        }
        kHeap.Close(kDecoder);
        if (kHeap.Stats().uUsed != uUsedStart)
        {
            printf("movie decoder fuzz FAIL used=%u start=%u: %s\n", kHeap.Stats().uUsed, uUsedStart, acLabel);
            fflush(stdout);
            return false;
        }
    }
    printf("movie-decoder-fuzz seed=%u count=%u bases=%u retail=%u opened=%u rejected=%u frames=%u ends=%u "
           "errors=%u sum=0x%08x\n",
           uSeed, uCount, (unsigned int)vBases.size(), uRetail, uOpened, uRejected, uFrames, uEnds, uErrors, uSum);
    fflush(stdout);
    if (puSum)
    {
        *puSum = uSum;
    }
    return true;
}

bool DumpInput(const std::vector<unsigned char>& v)
{
    REQUIRE(gFuzzOutPath[0] != 0);
    FILE* pFile = fopen(gFuzzOutPath, "wb");
    REQUIRE(pFile);
    const bool bOk = v.empty() || fwrite(v.data(), 1, v.size(), pFile) == v.size();
    fclose(pFile);
    return bOk;
}

}

TEST_CASE("movie header fuzz default seeds" * doctest::test_suite("fuzz"))
{
    const unsigned int auSeeds[4] = {1, 2, 3, 0xC0FFEE};
    unsigned int auSums[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i)
    {
        CHECK(RunHeaderFuzz(auSeeds[i], 15000, &auSums[i]));
    }
    unsigned int uAgain = 0;
    CHECK(RunHeaderFuzz(auSeeds[0], 15000, &uAgain));
    CHECK(uAgain == auSums[0]);
}

TEST_CASE("movie decoder fuzz default seeds" * doctest::test_suite("fuzz"))
{
    const unsigned int auSeeds[4] = {1, 2, 3, 0xC0FFEE};
    unsigned int auSums[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i)
    {
        CHECK(RunDecoderFuzz(auSeeds[i], 75, &auSums[i]));
    }
    unsigned int uAgain = 0;
    CHECK(RunDecoderFuzz(auSeeds[0], 75, &uAgain));
    CHECK(uAgain == auSums[0]);
}

TEST_CASE("movie header fuzz soak" * doctest::test_suite("stress"))
{
    if (gFuzzDump)
    {
        unsigned int uSegment = 0;
        CHECK(DumpInput(HeaderInput(gFuzzSeed, gFuzzIter, &uSegment)));
        return;
    }
    CHECK(RunHeaderFuzz(gFuzzSeed, gFuzzCount ? gFuzzCount : 1000000));
}

TEST_CASE("movie decoder fuzz soak" * doctest::test_suite("stress"))
{
    if (gFuzzDump)
    {
        unsigned int uRetail = 0;
        unsigned int uSegment = 0;
        unsigned int uChunk = 0;
        CHECK(DumpInput(DecoderInput(DecoderBases(&uRetail), gFuzzSeed, gFuzzIter, &uSegment, &uChunk)));
        return;
    }
    CHECK(RunDecoderFuzz(gFuzzSeed, gFuzzCount ? gFuzzCount : 10000));
}
