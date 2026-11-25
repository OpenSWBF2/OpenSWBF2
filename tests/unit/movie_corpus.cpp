#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>

#include <stdio.h>
#include <algorithm>
#include <string>
#include <vector>

#include "testutil.h"
#include "unit/movie_testdata.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/movie/pc/pcRedMovieConvert.h"

namespace
{

struct CorpusPerturb
{
    CorpusPerturb()
    {
        RedPerturbSet(51, kRedPerturbPad);
    }
    ~CorpusPerturb()
    {
        RedPerturbSet(0, 0);
    }
};

struct CorpusTally
{
    unsigned int uStreams;
    unsigned int uFrames;
    unsigned long long ullSamples;
    unsigned int uFailures;
};

void DecodeCorpusStream(const std::string& sFile, const std::string& sPath, const MvsSpan& kSpan,
                        std::vector<unsigned char>& vPixels, CorpusTally& kTally)
{
    ++kTally.uStreams;
    char acLabel[64];
    sprintf(acLabel, "movie corpus %.24s %08X", sFile.c_str(), kSpan.uName);
    WatchdogArm(acLabel);
    FileMovieSource kSource;
    MovieScratchHeap kHeap;
    MovieDecoder kDecoder;
    MovieAudioCollector kAudio;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    unsigned int uFrames = 0;
    unsigned long long ullSamples = 0;
    int iResult = kMovieFrameError;
    int iOpen = kMovieOpenReject;
    if (kSource.Open(sPath, kSpan.llOffset, kSpan.uLength))
    {
        iOpen = kHeap.Open(kDecoder, kSource.Source(), kSpan.uLength, &kInfo, &pcReason);
        kAudio.Attach(kDecoder);
    }
    if (iOpen == kMovieOpenOk)
    {
        iResult = kMovieFrameReady;
        while (iResult == kMovieFrameReady && uFrames <= kInfo.uFrames)
        {
            MovieFrame kFrame;
            iResult = kHeap.Decode(kDecoder, &kFrame);
            const short* pSamples = 0;
            ullSamples += kAudio.Take(&pSamples);
            if (iResult != kMovieFrameReady)
            {
                break;
            }
            ++uFrames;
            WatchdogArm(acLabel);
            const int iPitch = kFrame.iWidth * 4;
            const size_t uBytes = (size_t)iPitch * (size_t)kFrame.iHeight;
            if (vPixels.size() < uBytes)
            {
                vPixels.resize(uBytes);
            }
            ConvertYuv420ToXrgb(kFrame.apPlane[0], kFrame.aiStride[0], kFrame.apPlane[1], kFrame.aiStride[1],
                                kFrame.apPlane[2], kFrame.aiStride[2], kFrame.iWidth, kFrame.iHeight,
                                kFrame.bFullRange, vPixels.data(), iPitch);
        }
    }
    const int iEndState = kDecoder.EndState();
    kHeap.Close(kDecoder);
    const bool bOk = iOpen == kMovieOpenOk && iResult == kMovieFrameEnd && uFrames == kInfo.uFrames;
    printf("movie-corpus %s %08X frames=%u/%u samples=%llu open=%d result=%d end=%d %s\n", sFile.c_str(),
           kSpan.uName, uFrames, iOpen == kMovieOpenOk ? kInfo.uFrames : 0u, ullSamples, iOpen, iResult, iEndState,
           bOk ? "ok" : "FAIL");
    fflush(stdout);
    kTally.uFrames += uFrames;
    kTally.ullSamples += ullSamples;
    if (!bOk)
    {
        ++kTally.uFailures;
        FAIL_CHECK(sFile << " segment " << kSpan.uName << " frames " << uFrames << " open " << iOpen << " result "
                         << iResult << " end " << iEndState << " reason " << (pcReason ? pcReason : "-"));
    }
}

}

TEST_CASE_NAMED(FMovieCorpusTest1, "Unit::MovieCorpus::movie corpus decodes every retail stream", "[SmokeFilter][OnRequest]")
{
    const std::string sDir = MovieDataDir();
    SKIP_IF(sDir.empty(), "GameData movies not found");
    std::vector<std::string> vFiles;
    WIN32_FIND_DATAA kData;
    HANDLE hFind = FindFirstFileA((sDir + "\\*.mvs").c_str(), &kData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                vFiles.push_back(kData.cFileName);
            }
        } while (FindNextFileA(hFind, &kData));
        FindClose(hFind);
    }
    std::sort(vFiles.begin(), vFiles.end());
    CHECK(vFiles.size() == 24);

    CorpusPerturb kPerturb;
    REQUIRE(RedPerturbModes() == kRedPerturbPad);
    CorpusTally kTally = {0, 0, 0, 0};
    std::vector<unsigned char> vPixels;
    for (size_t i = 0; i < vFiles.size(); ++i)
    {
        const std::string sPath = sDir + "\\" + vFiles[i];
        std::vector<MvsSpan> vSpans;
        CHECK_MESSAGE(ListMvsSegments(sPath, vSpans), vFiles[i]);
        for (size_t s = 0; s < vSpans.size(); ++s)
        {
            DecodeCorpusStream(vFiles[i], sPath, vSpans[s], vPixels, kTally);
        }
    }
    printf("movie-corpus files=%u streams=%u frames=%u samples=%llu failures=%u\n", (unsigned int)vFiles.size(),
           kTally.uStreams, kTally.uFrames, kTally.ullSamples, kTally.uFailures);
    fflush(stdout);
    CHECK(kTally.uStreams == 281);
    CHECK(kTally.uFrames == 257340);
    CHECK(kTally.uFailures == 0);
}
