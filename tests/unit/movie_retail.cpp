#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <vector>

#include "testutil.h"
#include "unit/movie_testdata.h"
#include "RedEngineFL/movie/pc/pcRedMovieConvert.h"

namespace
{

struct MvsSegment
{
    unsigned int uName;
    unsigned int uLength;
    unsigned int uGap;
};

struct MvsWalk
{
    FILE* pFile;
    long long llFileSize;
    std::vector<unsigned char> vInfo;
    long long llDataPos;
    bool bInfo;
    bool bData;
};

unsigned int Le32(const unsigned char* p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

unsigned int Le16(const unsigned char* p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

bool ReadAt(FILE* pFile, long long llPos, void* pOut, size_t uSize)
{
    return _fseeki64(pFile, llPos, SEEK_SET) == 0 && fread(pOut, 1, uSize, pFile) == uSize;
}

bool WalkChunks(MvsWalk& kWalk, long long llStart, long long llEnd, int iDepth)
{
    if (iDepth > 4)
    {
        return false;
    }
    long long llPos = llStart;
    while (llPos + 8 <= llEnd)
    {
        unsigned char acHead[8];
        if (!ReadAt(kWalk.pFile, llPos, acHead, 8))
        {
            return false;
        }
        const unsigned int uId = Le32(acHead);
        const long long llSize = Le32(acHead + 4);
        const long long llBody = llPos + 8;
        if (llBody + llSize > llEnd)
        {
            return false;
        }
        if (uId == 0x2f1f7060u || uId == 0xf9f40518u)
        {
            if (!WalkChunks(kWalk, llBody, llBody + llSize, iDepth + 1))
            {
                return false;
            }
        }
        else if (uId == 0x0fb40705u)
        {
            if (llSize > 0x10000 || kWalk.bInfo)
            {
                return false;
            }
            kWalk.vInfo.resize((size_t)llSize);
            if (llSize && !ReadAt(kWalk.pFile, llBody, kWalk.vInfo.data(), (size_t)llSize))
            {
                return false;
            }
            kWalk.bInfo = true;
        }
        else if (uId == 0xd872e2a5u)
        {
            kWalk.llDataPos = llBody;
            kWalk.bData = true;
        }
        llPos = llBody + ((llSize + 3) & ~3ll);
    }
    return true;
}

bool ReadSegments(const std::vector<unsigned char>& vInfo, std::vector<MvsSegment>& vOut)
{
    const size_t uWords = vInfo.size() / 4;
    unsigned int uCount = 0;
    bool bCount = false;
    MvsSegment kCur = {0, 0, 0};
    bool bOpen = false;
    for (size_t i = 0; i < uWords; ++i)
    {
        const unsigned int uWord = Le32(&vInfo[i * 4]);
        const bool bHasValue = i + 1 < uWords;
        const unsigned int uValue = bHasValue ? Le32(&vInfo[(i + 1) * 4]) : 0;
        if (uWord == 0x40fbdebdu && bHasValue)
        {
            uCount = uValue;
            bCount = true;
            ++i;
        }
        else if (uWord == 0xf3c342e6u)
        {
            kCur.uName = 0;
            kCur.uLength = 0;
            kCur.uGap = 0;
            bOpen = true;
        }
        else if (bOpen && uWord == 0x8d39bde6u && bHasValue)
        {
            kCur.uName = uValue;
            ++i;
        }
        else if (bOpen && uWord == 0x83d03615u && bHasValue)
        {
            kCur.uLength = uValue;
            ++i;
        }
        else if (bOpen && uWord == 0x809608b6u && bHasValue)
        {
            kCur.uGap = uValue;
            ++i;
        }
        else if (bOpen && uWord == 0x7db1d043u)
        {
            vOut.push_back(kCur);
            bOpen = false;
        }
    }
    return bCount && uCount == vOut.size() && uCount > 0;
}

struct RetailTally
{
    int iStreams;
    int i48000;
    int i44100;
    int iFailures;
};

void Fail(RetailTally& kTally, const std::string& sFile, unsigned int uSegment, const char* pcField,
          unsigned long long ullValue)
{
    ++kTally.iFailures;
    if (kTally.iFailures <= 20)
    {
        char acSeg[16];
        sprintf(acSeg, "%08X", uSegment);
        FAIL_CHECK(sFile << " segment " << acSeg << " field " << pcField << " value " << ullValue);
    }
}

void ScanSegment(MvsWalk& kWalk, const std::string& sFile, const MvsSegment& kSeg, long long llStart,
                 RetailTally& kTally)
{
    ++kTally.iStreams;
    const unsigned int uSeg = kSeg.uName;
    unsigned char acHead[56];
    if (kSeg.uLength < sizeof(acHead) || llStart + (long long)sizeof(acHead) > kWalk.llFileSize ||
        !ReadAt(kWalk.pFile, llStart, acHead, sizeof(acHead)))
    {
        Fail(kTally, sFile, uSeg, "read", kSeg.uLength);
        return;
    }
    if (memcmp(acHead, "BIKi", 4) != 0)
    {
        Fail(kTally, sFile, uSeg, "revision", Le32(acHead));
    }
    const unsigned long long ullFileSize = (unsigned long long)Le32(acHead + 4) + 8;
    const unsigned int uFrames = Le32(acHead + 8);
    const unsigned int uLargest = Le32(acHead + 12);
    const unsigned int uWidth = Le32(acHead + 20);
    const unsigned int uHeight = Le32(acHead + 24);
    const unsigned int uFpsNum = Le32(acHead + 28);
    const unsigned int uFpsDen = Le32(acHead + 32);
    const unsigned int uVideoFlags = Le32(acHead + 36);
    const unsigned int uTracks = Le32(acHead + 40);
    const unsigned int uRate = Le16(acHead + 48);
    const unsigned int uTrackFlags = Le16(acHead + 50);
    const unsigned int uTrackId = Le32(acHead + 52);
    if (uWidth != 800)
    {
        Fail(kTally, sFile, uSeg, "width", uWidth);
    }
    if (uHeight != 600)
    {
        Fail(kTally, sFile, uSeg, "height", uHeight);
    }
    if (!(uFpsNum == 5000000 && uFpsDen == 166833) && !(uFpsNum == 10000000 && uFpsDen == 333667) &&
        !(uFpsNum == 10000000 && uFpsDen == 333333))
    {
        Fail(kTally, sFile, uSeg, "fps", uFpsDen);
    }
    if (uVideoFlags != 0)
    {
        Fail(kTally, sFile, uSeg, "vflags", uVideoFlags);
    }
    if (uTracks != 1)
    {
        Fail(kTally, sFile, uSeg, "tracks", uTracks);
        return;
    }
    if (uTrackFlags != 0x7000)
    {
        Fail(kTally, sFile, uSeg, "aflags", uTrackFlags);
    }
    if (uTrackId != 0)
    {
        Fail(kTally, sFile, uSeg, "trackid", uTrackId);
    }
    if (uRate == 48000)
    {
        ++kTally.i48000;
    }
    else if (uRate == 44100)
    {
        ++kTally.i44100;
    }
    else
    {
        Fail(kTally, sFile, uSeg, "rate", uRate);
    }
    if (uFrames < 1 || uFrames > 7314)
    {
        Fail(kTally, sFile, uSeg, "frames", uFrames);
        return;
    }
    if (uLargest > 135192)
    {
        Fail(kTally, sFile, uSeg, "largest", uLargest);
    }
    if ((unsigned long long)kSeg.uLength < ullFileSize)
    {
        Fail(kTally, sFile, uSeg, "length", kSeg.uLength);
    }
    const unsigned long long ullHeaderEnd = 44ull + 12ull + 4ull * ((unsigned long long)uFrames + 1ull);
    unsigned char acIndex[4];
    if (!ReadAt(kWalk.pFile, llStart + 56, acIndex, 4))
    {
        Fail(kTally, sFile, uSeg, "read", 56);
        return;
    }
    const unsigned int uIndex0 = Le32(acIndex);
    if ((unsigned long long)(uIndex0 & ~1u) != ullHeaderEnd)
    {
        Fail(kTally, sFile, uSeg, "index0", uIndex0);
    }
}

bool ScanMvs(const std::string& sDir, const std::string& sFile, RetailTally& kTally)
{
    MvsWalk kWalk;
    kWalk.pFile = fopen((sDir + "\\" + sFile).c_str(), "rb");
    kWalk.llDataPos = 0;
    kWalk.bInfo = false;
    kWalk.bData = false;
    if (!kWalk.pFile)
    {
        return false;
    }
    bool bOk = _fseeki64(kWalk.pFile, 0, SEEK_END) == 0;
    kWalk.llFileSize = bOk ? _ftelli64(kWalk.pFile) : 0;
    unsigned char acHead[8];
    bOk = bOk && kWalk.llFileSize >= 8 && ReadAt(kWalk.pFile, 0, acHead, 8) && memcmp(acHead, "ucfb", 4) == 0;
    const long long llEnd = bOk ? 8 + (long long)Le32(acHead + 4) : 0;
    bOk = bOk && llEnd <= kWalk.llFileSize && WalkChunks(kWalk, 8, llEnd, 0) && kWalk.bInfo && kWalk.bData;
    std::vector<MvsSegment> vSegments;
    bOk = bOk && ReadSegments(kWalk.vInfo, vSegments);
    if (bOk)
    {
        long long llPos = kWalk.llDataPos;
        for (size_t i = 0; i < vSegments.size(); ++i)
        {
            ScanSegment(kWalk, sFile, vSegments[i], llPos, kTally);
            llPos += (long long)vSegments[i].uGap + (long long)vSegments[i].uLength;
        }
    }
    fclose(kWalk.pFile);
    return bOk;
}

}

std::string MovieDataDir()
{
    const char* pcRoot = FindGameDataRoot();
    if (pcRoot == 0)
    {
        return std::string();
    }
    const std::string sDir = std::string(pcRoot) + "\\data\\_lvl_pc\\movies";
    const DWORD dwAttr = GetFileAttributesA(sDir.c_str());
    if (dwAttr == INVALID_FILE_ATTRIBUTES || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        return std::string();
    }
    return sDir;
}

bool ListMvsSegments(const std::string& sPath, std::vector<MvsSpan>& vOut)
{
    MvsWalk kWalk;
    kWalk.pFile = fopen(sPath.c_str(), "rb");
    kWalk.llDataPos = 0;
    kWalk.bInfo = false;
    kWalk.bData = false;
    if (!kWalk.pFile)
    {
        return false;
    }
    bool bOk = _fseeki64(kWalk.pFile, 0, SEEK_END) == 0;
    kWalk.llFileSize = bOk ? _ftelli64(kWalk.pFile) : 0;
    unsigned char acHead[8];
    bOk = bOk && kWalk.llFileSize >= 8 && ReadAt(kWalk.pFile, 0, acHead, 8) && memcmp(acHead, "ucfb", 4) == 0;
    const long long llEnd = bOk ? 8 + (long long)Le32(acHead + 4) : 0;
    bOk = bOk && llEnd <= kWalk.llFileSize && WalkChunks(kWalk, 8, llEnd, 0) && kWalk.bInfo && kWalk.bData;
    std::vector<MvsSegment> vSegments;
    bOk = bOk && ReadSegments(kWalk.vInfo, vSegments);
    if (bOk)
    {
        long long llPos = kWalk.llDataPos;
        for (size_t i = 0; i < vSegments.size(); ++i)
        {
            MvsSpan kSpan;
            kSpan.uName = vSegments[i].uName;
            kSpan.llOffset = llPos;
            kSpan.uLength = vSegments[i].uLength;
            bOk = bOk && llPos + (long long)kSpan.uLength <= kWalk.llFileSize;
            vOut.push_back(kSpan);
            llPos += (long long)vSegments[i].uGap + (long long)vSegments[i].uLength;
        }
    }
    fclose(kWalk.pFile);
    return bOk;
}

bool ReadMvsBytes(const std::string& sPath, long long llOffset, unsigned int uSize, std::vector<unsigned char>& vOut)
{
    vOut.resize(uSize);
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return false;
    }
    bool bOk = true;
    const unsigned int uBlock = 0x100000;
    for (unsigned int uDone = 0; bOk && uDone < uSize; uDone += uBlock)
    {
        const unsigned int uPart = uSize - uDone < uBlock ? uSize - uDone : uBlock;
        bOk = ReadAt(pFile, llOffset + uDone, vOut.data() + uDone, uPart);
    }
    fclose(pFile);
    return bOk;
}

FileMovieSource::FileMovieSource() : mpFile(0), mllOffset(0), muLength(0), muPos(0), mbFailed(false)
{
}

FileMovieSource::~FileMovieSource()
{
    if (mpFile)
    {
        fclose((FILE*)mpFile);
    }
}

bool FileMovieSource::Open(const std::string& sPath, long long llOffset, unsigned int uLength)
{
    mpFile = fopen(sPath.c_str(), "rb");
    mllOffset = llOffset;
    muLength = uLength;
    muPos = 0;
    mbFailed = false;
    return mpFile != 0;
}

MovieByteSource FileMovieSource::Source()
{
    MovieByteSource k;
    k.pContext = this;
    k.Peek = Peek;
    k.Read = Read;
    k.State = State;
    k.Pump = Pump;
    return k;
}

unsigned int FileMovieSource::Fetch(unsigned char* pOut, unsigned int uSize)
{
    const unsigned int uAvail = muLength - muPos;
    const unsigned int uCopy = uSize < uAvail ? uSize : uAvail;
    if (uCopy == 0 || mbFailed)
    {
        return 0;
    }
    if (!ReadAt((FILE*)mpFile, mllOffset + muPos, pOut, uCopy))
    {
        mbFailed = true;
        return 0;
    }
    return uCopy;
}

unsigned int FileMovieSource::Peek(void* pContext, unsigned char* pOut, unsigned int uSize)
{
    return ((FileMovieSource*)pContext)->Fetch(pOut, uSize);
}

unsigned int FileMovieSource::Read(void* pContext, unsigned char* pOut, unsigned int uSize)
{
    FileMovieSource* p = (FileMovieSource*)pContext;
    const unsigned int uCopy = p->Fetch(pOut, uSize < 0x10000u ? uSize : 0x10000u);
    p->muPos += uCopy;
    return uCopy;
}

int FileMovieSource::State(void* pContext)
{
    FileMovieSource* p = (FileMovieSource*)pContext;
    if (p->mbFailed)
    {
        return kMovieSourceDiscontinuity;
    }
    return p->muPos >= p->muLength ? kMovieSourceEnd : kMovieSourceMore;
}

void FileMovieSource::Pump(void* pContext)
{
    (void)pContext;
}

unsigned long long MovieFrameFnv(const unsigned char* pPixels, int iPitch, int iWidth, int iHeight)
{
    unsigned long long ullHash = 0xcbf29ce484222325ull;
    for (int iRow = 0; iRow < iHeight; ++iRow)
    {
        const unsigned char* pRow = pPixels + (size_t)iRow * (size_t)iPitch;
        for (int iCol = 0; iCol < iWidth; ++iCol)
        {
            for (int k = 0; k < 3; ++k)
            {
                ullHash ^= pRow[iCol * 4 + k];
                ullHash *= 0x100000001b3ull;
            }
        }
    }
    return ullHash;
}

namespace
{

struct FrameRow
{
    std::string sMvs;
    std::string sSegment;
    unsigned int uFrame;
    std::string sHash;
};

bool LoadFrameRows(const char* pcRel, const char* pcSection, std::vector<FrameRow>& vOut, std::string* psModel)
{
    std::string sText;
    if (!ReadRepoText(pcRel, sText))
    {
        return false;
    }
    std::string sSection;
    size_t uStart = 0;
    while (uStart < sText.size())
    {
        size_t uEnd = sText.find('\n', uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uStart, uEnd - uStart);
        uStart = uEnd + 1;
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        if (sLine.compare(0, 3, "## ") == 0)
        {
            sSection = sLine.substr(3);
            continue;
        }
        if (sLine.empty() || sLine[0] == '#')
        {
            continue;
        }
        if (psModel && sSection == "colour" && sLine.compare(0, 6, "model=") == 0)
        {
            *psModel = sLine.substr(6);
            continue;
        }
        if (sSection != pcSection)
        {
            continue;
        }
        FrameRow kRow;
        const size_t uTab1 = sLine.find('\t');
        const size_t uTab2 = uTab1 == std::string::npos ? uTab1 : sLine.find('\t', uTab1 + 1);
        const size_t uTab3 = uTab2 == std::string::npos ? uTab2 : sLine.find('\t', uTab2 + 1);
        if (uTab3 == std::string::npos || sLine.find('\t', uTab3 + 1) != std::string::npos)
        {
            return false;
        }
        kRow.sMvs = sLine.substr(0, uTab1);
        kRow.sSegment = sLine.substr(uTab1 + 1, uTab2 - uTab1 - 1);
        kRow.uFrame = (unsigned int)strtoul(sLine.substr(uTab2 + 1, uTab3 - uTab2 - 1).c_str(), 0, 10);
        kRow.sHash = sLine.substr(uTab3 + 1);
        if (kRow.sHash.size() != 16 || kRow.sHash.find_first_not_of("0123456789ABCDEF") != std::string::npos)
        {
            return false;
        }
        vOut.push_back(kRow);
    }
    return true;
}

bool FindSpan(const std::string& sPath, unsigned int uName, MvsSpan* pOut)
{
    std::vector<MvsSpan> vSpans;
    if (!ListMvsSegments(sPath, vSpans))
    {
        return false;
    }
    for (size_t i = 0; i < vSpans.size(); ++i)
    {
        if (vSpans[i].uName == uName)
        {
            *pOut = vSpans[i];
            return true;
        }
    }
    return false;
}

int DecodeFrameHashes(const std::string& sPath, const MvsSpan& kSpan, std::vector<std::string>& vOut,
                      unsigned int* puHeaderFrames)
{
    FileMovieSource kSource;
    REQUIRE(kSource.Open(sPath, kSpan.llOffset, kSpan.uLength));
    MovieScratchHeap kHeap;
    MovieDecoder kDecoder;
    MovieHeaderInfo kInfo;
    const char* pcReason = 0;
    REQUIRE(kHeap.Open(kDecoder, kSource.Source(), kSpan.uLength, &kInfo, &pcReason) == kMovieOpenOk);
    *puHeaderFrames = kInfo.uFrames;
    std::vector<unsigned char> vPixels;
    int iResult = kMovieFrameReady;
    while (iResult == kMovieFrameReady && vOut.size() <= kInfo.uFrames)
    {
        MovieFrame kFrame;
        iResult = kHeap.Decode(kDecoder, &kFrame);
        if (iResult != kMovieFrameReady)
        {
            break;
        }
        const int iPitch = kFrame.iWidth * 4 + 64;
        vPixels.assign((size_t)iPitch * (size_t)kFrame.iHeight, 0xEE);
        ConvertYuv420ToXrgb(kFrame.apPlane[0], kFrame.aiStride[0], kFrame.apPlane[1], kFrame.aiStride[1],
                            kFrame.apPlane[2], kFrame.aiStride[2], kFrame.iWidth, kFrame.iHeight, kFrame.bFullRange,
                            vPixels.data(), iPitch);
        char acHash[24];
        sprintf(acHash, "%016llX", MovieFrameFnv(vPixels.data(), iPitch, kFrame.iWidth, kFrame.iHeight));
        vOut.push_back(acHash);
    }
    kHeap.Close(kDecoder);
    return iResult;
}

}

TEST_CASE_NAMED(FMovieRetailTest1, "Unit::MovieRetail::movie frame hashes match the pinned FFmpeg path", "[SmokeFilter]")
{
    const std::string sDir = MovieDataDir();
    SKIP_IF(sDir.empty(), "GameData movies not found");
    std::vector<FrameRow> vGolden;
    std::string sModel;
    REQUIRE(LoadFrameRows("tests\\baseline\\v143\\movie-goldens.txt", "binkw32 frames", vGolden, &sModel));
    std::vector<FrameRow> vPins;
    REQUIRE(LoadFrameRows("tests\\baseline\\v143\\movie-frames.txt", "ffmpeg frames", vPins, 0));
    REQUIRE(vGolden.size() == 2941);
    REQUIRE(vPins.size() == vGolden.size());
    const bool bExact = sModel == "exact";
    unsigned int uPinMiss = 0;
    unsigned int uGoldenMiss = 0;
    unsigned int uSegments = 0;
    size_t i = 0;
    while (i < vPins.size())
    {
        size_t j = i;
        while (j < vPins.size() && vPins[j].sMvs == vPins[i].sMvs && vPins[j].sSegment == vPins[i].sSegment)
        {
            REQUIRE(vPins[j].uFrame == j - i + 1);
            REQUIRE(vGolden[j].sMvs == vPins[j].sMvs);
            REQUIRE(vGolden[j].sSegment == vPins[j].sSegment);
            REQUIRE(vGolden[j].uFrame == vPins[j].uFrame);
            ++j;
        }
        ++uSegments;
        INFO(vPins[i].sMvs << " " << vPins[i].sSegment);
        MvsSpan kSpan;
        const std::string sPath = sDir + "\\" + vPins[i].sMvs;
        REQUIRE(FindSpan(sPath, (unsigned int)strtoul(vPins[i].sSegment.c_str(), 0, 16), &kSpan));
        std::vector<std::string> vHashes;
        unsigned int uHeaderFrames = 0;
        const int iResult = DecodeFrameHashes(sPath, kSpan, vHashes, &uHeaderFrames);
        CHECK(iResult == kMovieFrameEnd);
        CHECK(uHeaderFrames == j - i);
        REQUIRE(vHashes.size() == j - i);
        for (size_t k = 0; k < vHashes.size(); ++k)
        {
            if (vHashes[k] != vPins[i + k].sHash)
            {
                ++uPinMiss;
                if (uPinMiss <= 10)
                {
                    FAIL_CHECK("frame " << (k + 1) << " hash " << vHashes[k] << " pin " << vPins[i + k].sHash);
                }
            }
            if (bExact && vHashes[k] != vGolden[i + k].sHash)
            {
                ++uGoldenMiss;
                if (uGoldenMiss <= 10)
                {
                    FAIL_CHECK("frame " << (k + 1) << " hash " << vHashes[k] << " binkw32 " << vGolden[i + k].sHash);
                }
            }
        }
        i = j;
    }
    CHECK(uSegments == 3);
    CHECK(uPinMiss == 0);
    CHECK(uGoldenMiss == 0);
}

TEST_CASE_NAMED(FMovieRetailTest2, "Unit::MovieRetail::movie retail layout matches the measured set", "[SmokeFilter]")
{
    const char* pcRoot = FindGameDataRoot();
    SKIP_IF(pcRoot == 0, "GameData not found");
    const std::string sDir = std::string(pcRoot) + "\\data\\_lvl_pc\\movies";
    const DWORD dwAttr = GetFileAttributesA(sDir.c_str());
    SKIP_IF(dwAttr == INVALID_FILE_ATTRIBUTES || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0,
            "data\\_lvl_pc\\movies not found");

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

    RetailTally kTally = {0, 0, 0, 0};
    for (size_t i = 0; i < vFiles.size(); ++i)
    {
        INFO(vFiles[i]);
        CHECK(ScanMvs(sDir, vFiles[i], kTally));
    }
    CHECK(kTally.iStreams == 281);
    CHECK(kTally.i48000 == 269);
    CHECK(kTally.i44100 == 12);
    CHECK(kTally.iFailures == 0);
}
