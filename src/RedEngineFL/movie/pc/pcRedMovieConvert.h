#pragma once

void ConvertYuv420ToXrgb(const unsigned char* pY, int iYStride, const unsigned char* pU, int iUStride,
                         const unsigned char* pV, int iVStride, int iWidth, int iHeight, bool bFullRange,
                         unsigned char* pDst, int iDstPitch);

void ConvertAudioToS16(const float* const* ppPlanes, int iPlanar, int iChannels, int iSamples, short* pOut);
