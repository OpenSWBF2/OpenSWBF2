#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>

#include <string.h>
#include <string>
#include <vector>

#include "RedEngineFL/movie/pc/pcRedMovieHeader.h"

namespace
{

struct StreamSpec
{
    char acMagic[4];
    unsigned int uFrames;
    unsigned int uLargest;
    unsigned int uWidth;
    unsigned int uHeight;
    unsigned int uFpsNum;
    unsigned int uFpsDen;
    unsigned int uTracks;
    unsigned int auRate[8];
    unsigned int auFlags[8];
    unsigned int uSpan;
    unsigned int uLastSpan;
    int iIndex0Shift;
    int iLastShift;
    int iSwapFrame;
};

StreamSpec RetailSpec(unsigned int uRate)
{
    StreamSpec k;
    memset(&k, 0, sizeof(k));
    memcpy(k.acMagic, "BIKi", 4);
    k.uFrames = 7314;
    k.uLargest = 135192;
    k.uWidth = 800;
    k.uHeight = 600;
    k.uFpsNum = 10000000;
    k.uFpsDen = 333667;
    k.uTracks = 1;
    for (int i = 0; i < 8; ++i)
    {
        k.auRate[i] = uRate;
        k.auFlags[i] = 0x7000;
    }
    k.uSpan = 10000;
    k.uLastSpan = 10000;
    k.iSwapFrame = -1;
    return k;
}

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

struct Built
{
    std::vector<unsigned char> vBytes;
    unsigned int uFileSize;
};

Built Build(const StreamSpec& k)
{
    std::vector<unsigned char> v;
    v.insert(v.end(), k.acMagic, k.acMagic + 4);
    const size_t uSizeAt = v.size();
    Put32(v, 0);
    Put32(v, k.uFrames);
    Put32(v, k.uLargest);
    Put32(v, k.uFrames);
    Put32(v, k.uWidth);
    Put32(v, k.uHeight);
    Put32(v, k.uFpsNum);
    Put32(v, k.uFpsDen);
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
        Put16(v, k.auRate[i]);
        Put16(v, k.auFlags[i]);
    }
    for (unsigned int i = 0; i < k.uTracks; ++i)
    {
        Put32(v, 0);
    }
    const unsigned int uHeaderEnd = (unsigned int)v.size() + 4 * (k.uFrames + 1);
    std::vector<unsigned int> vIndex;
    unsigned int uPos = uHeaderEnd;
    for (unsigned int i = 0; i <= k.uFrames; ++i)
    {
        vIndex.push_back(uPos);
        uPos += (i + 1 == k.uFrames) ? k.uLastSpan : k.uSpan;
    }
    const unsigned int uFileSize = vIndex[k.uFrames];
    vIndex[0] = (unsigned int)((int)vIndex[0] + k.iIndex0Shift) | 1u;
    vIndex[k.uFrames] = (unsigned int)((int)vIndex[k.uFrames] + k.iLastShift);
    if (k.iSwapFrame >= 0)
    {
        const unsigned int uTmp = vIndex[k.iSwapFrame];
        vIndex[k.iSwapFrame] = vIndex[k.iSwapFrame + 1];
        vIndex[k.iSwapFrame + 1] = uTmp;
    }
    for (size_t i = 0; i < vIndex.size(); ++i)
    {
        Put32(v, vIndex[i]);
    }
    const unsigned int uSizeField = uFileSize - 8;
    v[uSizeAt] = (unsigned char)uSizeField;
    v[uSizeAt + 1] = (unsigned char)(uSizeField >> 8);
    v[uSizeAt + 2] = (unsigned char)(uSizeField >> 16);
    v[uSizeAt + 3] = (unsigned char)(uSizeField >> 24);
    Built kOut;
    kOut.vBytes = v;
    kOut.uFileSize = uFileSize;
    return kOut;
}

int Check(const Built& k, unsigned int uSegment, MovieHeaderInfo* pInfo, std::string* psReason)
{
    const char* pcReason = 0;
    const int iResult =
        CheckMovieHeader(k.vBytes.data(), (unsigned int)k.vBytes.size(), uSegment, pInfo, &pcReason);
    *psReason = pcReason ? pcReason : "";
    return iResult;
}

void RequireOk(const StreamSpec& kSpec)
{
    const Built k = Build(kSpec);
    MovieHeaderInfo kInfo;
    std::string sReason;
    CHECK(Check(k, k.uFileSize, &kInfo, &sReason) == kMovieHeaderOk);
    CHECK(sReason == "");
}

void RequireReject(const StreamSpec& kSpec, const char* pcReason)
{
    const Built k = Build(kSpec);
    MovieHeaderInfo kInfo;
    std::string sReason;
    CHECK(Check(k, k.uFileSize, &kInfo, &sReason) == kMovieHeaderReject);
    CHECK(sReason == pcReason);
}

void RequireRejectSegment(const StreamSpec& kSpec, int iSegmentShift, const char* pcReason)
{
    const Built k = Build(kSpec);
    MovieHeaderInfo kInfo;
    std::string sReason;
    CHECK(Check(k, (unsigned int)((int)k.uFileSize + iSegmentShift), &kInfo, &sReason) == kMovieHeaderReject);
    CHECK(sReason == pcReason);
}

void SetSizeField(Built& k, unsigned int uField)
{
    k.vBytes[4] = (unsigned char)uField;
    k.vBytes[5] = (unsigned char)(uField >> 8);
    k.vBytes[6] = (unsigned char)(uField >> 16);
    k.vBytes[7] = (unsigned char)(uField >> 24);
}

}

TEST_CASE_NAMED(FMovieHeaderTest1, "Unit::MovieHeader::movie header accepts retail shaped streams", "[SmokeFilter]")
{
    const unsigned int auRates[2] = {48000, 44100};
    for (int r = 0; r < 2; ++r)
    {
        const Built k = Build(RetailSpec(auRates[r]));
        CHECK(k.vBytes.size() == 29316);
        CHECK(k.uFileSize == 29316 + 7314 * 10000);
        MovieHeaderInfo kInfo;
        std::string sReason;
        CHECK(Check(k, 29316 + 7314 * 10000, &kInfo, &sReason) == kMovieHeaderOk);
        CHECK(sReason == "");
        CHECK(kInfo.cRevision == 'i');
        CHECK(kInfo.uWidth == 800);
        CHECK(kInfo.uHeight == 600);
        CHECK(kInfo.uFrames == 7314);
        CHECK(kInfo.uLargest == 135192);
        CHECK(kInfo.uFpsNum == 10000000);
        CHECK(kInfo.uFpsDen == 333667);
        CHECK(kInfo.uVideoFlags == 0);
        CHECK(kInfo.uTracks == 1);
        CHECK(kInfo.uRate == auRates[r]);
        CHECK(kInfo.uAudioFlags == 0x7000);
        CHECK(kInfo.uHeaderEnd == 29316);
        CHECK(kInfo.uFileSize == 29316 + 7314 * 10000);
        CHECK(kInfo.uIndex0 == 29317);
    }
}

TEST_CASE_NAMED(FMovieHeaderTest2, "Unit::MovieHeader::movie header accepts every field at its cap", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uWidth = 1024;
    k.uHeight = 768;
    RequireOk(k);

    k = RetailSpec(48000);
    k.uFrames = 16000;
    RequireOk(k);

    k = RetailSpec(48000);
    k.uFrames = 1;
    k.uLargest = 1;
    k.uWidth = 1;
    k.uHeight = 1;
    k.uFpsNum = 1;
    k.uFpsDen = 1;
    k.uLastSpan = 4;
    RequireOk(k);

    k = RetailSpec(48000);
    k.uFpsNum = 120;
    k.uFpsDen = 1;
    RequireOk(k);

    k = RetailSpec(48000);
    k.uTracks = 0;
    k.uFrames = 1;
    k.uLargest = 2;
    k.uLastSpan = 2;
    RequireOk(k);

    k = RetailSpec(48000);
    k.uLargest = 262144;
    k.uSpan = 262144;
    k.uLastSpan = 262144;
    k.uFrames = 3;
    RequireOk(k);

    k = RetailSpec(48000);
    k.uTracks = 6;
    k.auRate[5] = 8000;
    k.auFlags[5] = 0;
    RequireOk(k);

    k = RetailSpec(8000);
    k.uTracks = 0;
    RequireOk(k);

    const char acRevisions[6] = {'b', 'f', 'g', 'h', 'i', 'k'};
    for (int i = 0; i < 6; ++i)
    {
        k = RetailSpec(48000);
        k.acMagic[3] = acRevisions[i];
        const Built kBuilt = Build(k);
        CHECK(kBuilt.vBytes.size() == (acRevisions[i] == 'k' ? 29320u : 29316u));
        MovieHeaderInfo kInfo;
        std::string sReason;
        CHECK(Check(kBuilt, kBuilt.uFileSize, &kInfo, &sReason) == kMovieHeaderOk);
        CHECK(kInfo.cRevision == acRevisions[i]);
    }

    const Built kBuilt = Build(RetailSpec(48000));
    MovieHeaderInfo kInfo;
    std::string sReason;
    CHECK(Check(kBuilt, 0xFFFFFFFFu, &kInfo, &sReason) == kMovieHeaderOk);

    Built kOdd = Build(RetailSpec(48000));
    SetSizeField(kOdd, 29316 + 7314 * 10000 - 8 + 1);
    CHECK(Check(kOdd, 29316 + 7314 * 10000 + 1, &kInfo, &sReason) == kMovieHeaderOk);
    CHECK(kInfo.uFileSize == 29316 + 7314 * 10000 + 1);
}

TEST_CASE_NAMED(FMovieHeaderTest3, "Unit::MovieHeader::movie header rejects a bad magic with its reason", "[SmokeFilter]")
{
    const char* apcMagic[4] = {"KB2i", "SMUS", "BIJi", "bIKi"};
    for (int i = 0; i < 4; ++i)
    {
        StreamSpec k = RetailSpec(48000);
        memcpy(k.acMagic, apcMagic[i], 4);
        RequireReject(k, "magic");
    }
}

TEST_CASE_NAMED(FMovieHeaderTest4, "Unit::MovieHeader::movie header rejects a revision outside the demuxer set with its reason", "[SmokeFilter]")
{
    const char acRevisions[5] = {'a', 'c', 'e', 'j', 'l'};
    for (int i = 0; i < 5; ++i)
    {
        StreamSpec k = RetailSpec(48000);
        k.acMagic[3] = acRevisions[i];
        RequireReject(k, "revision");
    }
}

TEST_CASE_NAMED(FMovieHeaderTest5, "Unit::MovieHeader::movie header rejects a file size past its segment with its reason", "[SmokeFilter]")
{
    RequireRejectSegment(RetailSpec(48000), -1, "size");
    Built k = Build(RetailSpec(48000));
    SetSizeField(k, 0xFFFFFFFFu);
    MovieHeaderInfo kInfo;
    std::string sReason;
    CHECK(Check(k, 0xFFFFFFFFu, &kInfo, &sReason) == kMovieHeaderReject);
    CHECK(sReason == "size");
}

TEST_CASE_NAMED(FMovieHeaderTest6, "Unit::MovieHeader::movie header rejects a frame count past its cap with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uFrames = 16001;
    RequireReject(k, "frames");
    Built kZero = Build(RetailSpec(48000));
    kZero.vBytes[8] = 0;
    kZero.vBytes[9] = 0;
    kZero.vBytes[10] = 0;
    kZero.vBytes[11] = 0;
    MovieHeaderInfo kInfo;
    std::string sReason;
    CHECK(Check(kZero, kZero.uFileSize, &kInfo, &sReason) == kMovieHeaderReject);
    CHECK(sReason == "frames");
}

TEST_CASE_NAMED(FMovieHeaderTest7, "Unit::MovieHeader::movie header rejects a largest frame past its cap with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uLargest = 262145;
    RequireReject(k, "largest");
    k.uLargest = 0;
    RequireReject(k, "largest");
}

TEST_CASE_NAMED(FMovieHeaderTest8, "Unit::MovieHeader::movie header rejects a width past its cap with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uWidth = 1025;
    RequireReject(k, "width");
    k.uWidth = 0;
    RequireReject(k, "width");
}

TEST_CASE_NAMED(FMovieHeaderTest9, "Unit::MovieHeader::movie header rejects a height past its cap with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uHeight = 769;
    RequireReject(k, "height");
    k.uHeight = 0;
    RequireReject(k, "height");
}

TEST_CASE_NAMED(FMovieHeaderTest10, "Unit::MovieHeader::movie header rejects a zero frame rate with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uFpsNum = 0;
    RequireReject(k, "fps");
    k = RetailSpec(48000);
    k.uFpsDen = 0;
    RequireReject(k, "fps");
    k.uFpsNum = 0;
    RequireReject(k, "fps");
}

TEST_CASE_NAMED(FMovieHeaderTest11, "Unit::MovieHeader::movie header accepts the three retail frame rates", "[SmokeFilter]")
{
    const unsigned int auNum[3] = {10000000, 5000000, 10000000};
    const unsigned int auDen[3] = {333333, 166833, 333667};
    for (int i = 0; i < 3; ++i)
    {
        StreamSpec k = RetailSpec(48000);
        k.uFpsNum = auNum[i];
        k.uFpsDen = auDen[i];
        RequireOk(k);
    }
}

TEST_CASE_NAMED(FMovieHeaderTest12, "Unit::MovieHeader::movie header rejects a frame rate below 1 fps with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uFpsNum = 0xFFFFFFFFu;
    k.uFpsDen = 0xFFFFFFFFu;
    RequireOk(k);
    k.uFpsNum = 0xFFFFFFFEu;
    RequireReject(k, "fps");
    k.uFpsNum = 1;
    k.uFpsDen = 2;
    RequireReject(k, "fps");
    k.uFpsNum = 1;
    k.uFpsDen = 0xFFFFFFFFu;
    RequireReject(k, "fps");
}

TEST_CASE_NAMED(FMovieHeaderTest13, "Unit::MovieHeader::movie header rejects a frame rate above 120 fps with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uFpsNum = 4294967280u;
    k.uFpsDen = 35791394u;
    RequireOk(k);
    k.uFpsNum = 4294967281u;
    RequireReject(k, "fps");
    k.uFpsNum = 121;
    k.uFpsDen = 1;
    RequireReject(k, "fps");
    k.uFpsNum = 0xFFFFFFFFu;
    k.uFpsDen = 1;
    RequireReject(k, "fps");
}

TEST_CASE_NAMED(FMovieHeaderTest14, "Unit::MovieHeader::movie header rejects a largest frame past the file size with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uFrames = 1;
    k.uLastSpan = 4;
    const unsigned int uFileSize = Build(k).uFileSize;
    CHECK(uFileSize == 44 + 12 + 8 + 4);
    k.uLargest = uFileSize;
    RequireOk(k);
    k.uLargest = uFileSize + 1;
    RequireReject(k, "largest");
}

TEST_CASE_NAMED(FMovieHeaderTest15, "Unit::MovieHeader::movie header rejects a frame shorter than its audio sizes with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uTracks = 2;
    k.uFrames = 3;
    k.uLargest = 8;
    k.uSpan = 8;
    k.uLastSpan = 8;
    RequireOk(k);
    k.uLastSpan = 6;
    RequireReject(k, "order");
    k.uLastSpan = 8;
    k.uSpan = 6;
    RequireReject(k, "order");
    k = RetailSpec(48000);
    k.uTracks = 6;
    k.uFrames = 2;
    k.uLargest = 24;
    k.uSpan = 24;
    k.uLastSpan = 24;
    RequireOk(k);
    k.uLastSpan = 22;
    RequireReject(k, "order");
}

TEST_CASE_NAMED(FMovieHeaderTest16, "Unit::MovieHeader::movie header rejects a track count past its cap with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.uTracks = 7;
    RequireReject(k, "tracks");
}

TEST_CASE_NAMED(FMovieHeaderTest17, "Unit::MovieHeader::movie header rejects a sample rate outside its range with its reason", "[SmokeFilter]")
{
    RequireReject(RetailSpec(7999), "rate");
    RequireReject(RetailSpec(48001), "rate");
    StreamSpec k = RetailSpec(48000);
    k.uTracks = 6;
    k.auRate[5] = 48001;
    RequireReject(k, "rate");
}

TEST_CASE_NAMED(FMovieHeaderTest18, "Unit::MovieHeader::movie header rejects an audio flag outside the bink set with its reason", "[SmokeFilter]")
{
    const unsigned int auFlags[4] = {0x7800, 0x8000, 0x0001, 0xFFFF};
    for (int i = 0; i < 4; ++i)
    {
        StreamSpec k = RetailSpec(48000);
        k.auFlags[0] = auFlags[i];
        RequireReject(k, "aflags");
    }
    StreamSpec k = RetailSpec(48000);
    k.uTracks = 2;
    k.auFlags[1] = 0x7001;
    RequireReject(k, "aflags");
}

TEST_CASE_NAMED(FMovieHeaderTest19, "Unit::MovieHeader::movie header rejects a first index off the header end with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.iIndex0Shift = 2;
    RequireReject(k, "index");
    k.iIndex0Shift = -2;
    RequireReject(k, "index");
}

TEST_CASE_NAMED(FMovieHeaderTest20, "Unit::MovieHeader::movie header rejects an index out of order with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.iSwapFrame = 100;
    RequireReject(k, "order");
    k = RetailSpec(48000);
    k.uSpan = 0;
    k.uFrames = 3;
    k.uLargest = 10000;
    RequireReject(k, "order");
    k = RetailSpec(48000);
    k.uFrames = 3;
    k.uSpan = 262146;
    RequireReject(k, "order");
    k = RetailSpec(48000);
    k.uFrames = 3;
    k.uLastSpan = 262146;
    RequireReject(k, "order");
}

TEST_CASE_NAMED(FMovieHeaderTest21, "Unit::MovieHeader::movie header rejects a last index off the file end with its reason", "[SmokeFilter]")
{
    StreamSpec k = RetailSpec(48000);
    k.iLastShift = -2;
    Built kShort = Build(k);
    MovieHeaderInfo kInfo;
    std::string sReason;
    CHECK(Check(kShort, kShort.uFileSize, &kInfo, &sReason) == kMovieHeaderReject);
    CHECK(sReason == "range");
    k.iLastShift = 2;
    Built kLong = Build(k);
    CHECK(Check(kLong, kLong.uFileSize + 2, &kInfo, &sReason) == kMovieHeaderReject);
    CHECK(sReason == "range");
}

TEST_CASE_NAMED(FMovieHeaderTest22, "Unit::MovieHeader::movie header asks for more bytes at every truncation point without reading past them", "[SmokeFilter]")
{
    SYSTEM_INFO kSys;
    GetSystemInfo(&kSys);
    const size_t uPage = kSys.dwPageSize;
    const size_t uRegion = 32768 + uPage;
    unsigned char* pRegion =
        (unsigned char*)VirtualAlloc(0, uRegion + uPage, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    REQUIRE(pRegion);
    DWORD dwOld = 0;
    REQUIRE(VirtualProtect(pRegion + uRegion, uPage, PAGE_NOACCESS, &dwOld));
    unsigned char* pEnd = pRegion + uRegion;

    struct Shape
    {
        StreamSpec kSpec;
        unsigned int auNeed[4];
        unsigned int uFull;
    };
    Shape akShapes[3];
    akShapes[0].kSpec = RetailSpec(48000);
    akShapes[0].auNeed[0] = 4;
    akShapes[0].auNeed[1] = 44;
    akShapes[0].auNeed[2] = 56;
    akShapes[0].auNeed[3] = 29316;
    akShapes[0].uFull = 29316;
    akShapes[1].kSpec = RetailSpec(48000);
    akShapes[1].kSpec.acMagic[3] = 'k';
    akShapes[1].kSpec.uTracks = 2;
    akShapes[1].kSpec.uFrames = 3;
    akShapes[1].kSpec.uLargest = 10000;
    akShapes[1].auNeed[0] = 4;
    akShapes[1].auNeed[1] = 48;
    akShapes[1].auNeed[2] = 72;
    akShapes[1].auNeed[3] = 88;
    akShapes[1].uFull = 88;
    akShapes[2].kSpec = RetailSpec(48000);
    akShapes[2].kSpec.uTracks = 0;
    akShapes[2].kSpec.uFrames = 2;
    akShapes[2].kSpec.uLargest = 10000;
    akShapes[2].auNeed[0] = 4;
    akShapes[2].auNeed[1] = 44;
    akShapes[2].auNeed[2] = 44;
    akShapes[2].auNeed[3] = 56;
    akShapes[2].uFull = 56;

    for (int s = 0; s < 3; ++s)
    {
        const Built k = Build(akShapes[s].kSpec);
        REQUIRE(k.vBytes.size() == akShapes[s].uFull);
        int iWrong = 0;
        for (unsigned int n = 0; n <= akShapes[s].uFull; ++n)
        {
            unsigned char* pAt = pEnd - n;
            memcpy(pAt, k.vBytes.data(), n);
            MovieHeaderInfo kInfo;
            const char* pcReason = 0;
            const int iResult = CheckMovieHeader(pAt, n, k.uFileSize, &kInfo, &pcReason);
            unsigned int uWant = 0;
            for (int i = 0; i < 4 && !uWant; ++i)
            {
                if (n < akShapes[s].auNeed[i])
                {
                    uWant = akShapes[s].auNeed[i];
                }
            }
            const bool bOk = uWant ? (iResult == kMovieHeaderNeedMore && kInfo.uNeeded == uWant && pcReason == 0)
                                   : (iResult == kMovieHeaderOk && pcReason == 0);
            if (!bOk && iWrong++ < 4)
            {
                INFO("shape " << s << " size " << n << " result " << iResult << " needed " << kInfo.uNeeded);
                CHECK(bOk);
            }
        }
        CHECK(iWrong == 0);
    }
    VirtualFree(pRegion, 0, MEM_RELEASE);
}
