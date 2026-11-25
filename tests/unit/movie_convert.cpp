#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string.h>

#include "RedEngineFL/movie/pc/pcRedMovieConvert.h"

namespace
{

struct Bgr
{
    int iB;
    int iG;
    int iR;
};

Bgr ConvertOne(int iY, int iU, int iV, bool bFullRange)
{
    unsigned char ucY = (unsigned char)iY;
    unsigned char ucU = (unsigned char)iU;
    unsigned char ucV = (unsigned char)iV;
    unsigned char acOut[4] = {0xEE, 0xEE, 0xEE, 0xEE};
    ConvertYuv420ToXrgb(&ucY, 1, &ucU, 1, &ucV, 1, 1, 1, bFullRange, acOut, 4);
    CHECK(acOut[3] == 0);
    Bgr k = {acOut[0], acOut[1], acOut[2]};
    return k;
}

void CheckPixel(int iY, int iU, int iV, bool bFullRange, int iB, int iG, int iR)
{
    const Bgr k = ConvertOne(iY, iU, iV, bFullRange);
    INFO("yuv " << iY << " " << iU << " " << iV << " full " << bFullRange);
    CHECK(k.iB == iB);
    CHECK(k.iG == iG);
    CHECK(k.iR == iR);
}

}

TEST_CASE_NAMED(FMovieConvertTest1, "Unit::MovieConvert::movie convert maps mpeg range black and white to the bink literals", "[SmokeFilter]")
{
    CheckPixel(16, 128, 128, false, 0, 1, 0);
    CheckPixel(235, 128, 128, false, 254, 255, 254);
    CheckPixel(180, 128, 128, false, 190, 192, 190);
}

TEST_CASE_NAMED(FMovieConvertTest2, "Unit::MovieConvert::movie convert maps mpeg range primaries and bars to the bink literals", "[SmokeFilter]")
{
    CheckPixel(81, 90, 240, false, 0, 1, 255);
    CheckPixel(145, 54, 34, false, 0, 255, 0);
    CheckPixel(41, 240, 110, false, 255, 0, 0);
    CheckPixel(162, 44, 142, false, 0, 193, 192);
    CheckPixel(131, 156, 44, false, 190, 191, 0);
    CheckPixel(112, 72, 58, false, 0, 192, 0);
    CheckPixel(84, 184, 198, false, 192, 0, 192);
    CheckPixel(65, 100, 212, false, 0, 1, 193);
    CheckPixel(35, 212, 114, false, 192, 0, 0);
}

TEST_CASE_NAMED(FMovieConvertTest3, "Unit::MovieConvert::movie convert clamps mpeg range extremes", "[SmokeFilter]")
{
    CheckPixel(0, 0, 0, false, 0, 138, 0);
    CheckPixel(255, 255, 255, false, 255, 124, 255);
}

TEST_CASE_NAMED(FMovieConvertTest4, "Unit::MovieConvert::movie convert maps full range black and white and rounds", "[SmokeFilter]")
{
    CheckPixel(0, 128, 128, true, 0, 0, 0);
    CheckPixel(255, 128, 128, true, 255, 255, 255);
    CheckPixel(81, 90, 240, true, 14, 14, 238);
    CheckPixel(112, 72, 58, true, 13, 181, 14);
    CheckPixel(0, 0, 0, true, 0, 135, 0);
    CheckPixel(255, 255, 255, true, 255, 121, 255);
}

TEST_CASE_NAMED(FMovieConvertTest5, "Unit::MovieConvert::movie convert takes nearest chroma on an odd size image", "[SmokeFilter]")
{
    const unsigned char acY[9] = {128, 128, 128, 128, 128, 128, 128, 128, 128};
    const unsigned char acU[4] = {100, 130, 145, 160};
    const unsigned char acV[4] = {100, 130, 160, 190};
    unsigned char acOut[3 * 3 * 4];
    memset(acOut, 0xEE, sizeof(acOut));
    ConvertYuv420ToXrgb(acY, 3, acU, 2, acV, 2, 3, 3, false, acOut, 12);
    const int aiChroma[9] = {0, 0, 1, 0, 0, 1, 2, 2, 3};
    const int aaiBgr[4][3] = {{73, 165, 84}, {133, 129, 133}, {164, 98, 182}, {194, 68, 230}};
    for (int i = 0; i < 9; ++i)
    {
        INFO("pixel " << i);
        CHECK(acOut[i * 4] == aaiBgr[aiChroma[i]][0]);
        CHECK(acOut[i * 4 + 1] == aaiBgr[aiChroma[i]][1]);
        CHECK(acOut[i * 4 + 2] == aaiBgr[aiChroma[i]][2]);
        CHECK(acOut[i * 4 + 3] == 0);
    }
}

TEST_CASE_NAMED(FMovieConvertTest6, "Unit::MovieConvert::movie convert keeps the destination pitch padding", "[SmokeFilter]")
{
    const int iWidth = 4;
    const int iHeight = 4;
    const int iPitch = iWidth * 4 + 12;
    unsigned char acY[16];
    unsigned char acU[4];
    unsigned char acV[4];
    for (int i = 0; i < 16; ++i)
    {
        acY[i] = (unsigned char)(16 + i * 13);
    }
    memset(acU, 128, sizeof(acU));
    memset(acV, 128, sizeof(acV));
    unsigned char acOut[iPitch * iHeight];
    memset(acOut, 0x5A, sizeof(acOut));
    ConvertYuv420ToXrgb(acY, iWidth, acU, 2, acV, 2, iWidth, iHeight, false, acOut, iPitch);
    const int aaiGreen[4][2] = {{1, 46}, {61, 106}, {122, 167}, {182, 228}};
    for (int iRow = 0; iRow < iHeight; ++iRow)
    {
        for (int iByte = iWidth * 4; iByte < iPitch; ++iByte)
        {
            INFO("row " << iRow << " byte " << iByte);
            CHECK(acOut[iRow * iPitch + iByte] == 0x5A);
        }
        INFO("row " << iRow);
        CHECK(acOut[iRow * iPitch + 1] == aaiGreen[iRow][0]);
        CHECK(acOut[iRow * iPitch + 13] == aaiGreen[iRow][1]);
    }
}

TEST_CASE_NAMED(FMovieConvertTest7, "Unit::MovieConvert::movie convert turns float audio into clamped rounded s16", "[SmokeFilter]")
{
    const float afIn[7] = {0.0f, 1.0f, -1.0f, 2.0f, -2.0f, 0.5f, -0.5f};
    const short asWant[7] = {0, 32767, -32767, 32767, -32768, 16384, -16384};
    const float* apPlane[1] = {afIn};
    short asOut[7];
    memset(asOut, 0x11, sizeof(asOut));
    ConvertAudioToS16(apPlane, 1, 1, 7, asOut);
    for (int i = 0; i < 7; ++i)
    {
        INFO("sample " << i);
        CHECK(asOut[i] == asWant[i]);
    }
}

TEST_CASE_NAMED(FMovieConvertTest8, "Unit::MovieConvert::movie convert gives the same s16 from planar and interleaved audio", "[SmokeFilter]")
{
    const float afLeft[3] = {0.25f, -2.0f, 0.75f};
    const float afRight[3] = {-0.25f, 3.0f, 0.0f};
    const float afMixed[6] = {0.25f, -0.25f, -2.0f, 3.0f, 0.75f, 0.0f};
    const short asWant[6] = {8192, -8192, -32768, 32767, 24575, 0};
    const float* apPlanar[2] = {afLeft, afRight};
    const float* apPacked[1] = {afMixed};
    short asPlanar[6];
    short asPacked[6];
    ConvertAudioToS16(apPlanar, 1, 2, 3, asPlanar);
    ConvertAudioToS16(apPacked, 0, 2, 3, asPacked);
    for (int i = 0; i < 6; ++i)
    {
        INFO("sample " << i);
        CHECK(asPlanar[i] == asWant[i]);
        CHECK(asPacked[i] == asWant[i]);
    }
}
