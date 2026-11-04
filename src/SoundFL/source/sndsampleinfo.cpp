#include "sndsampleinfo.h"

namespace Snd
{

// GLOBAL: SWBF2 0x007e39f4
// SampleInfo::smSamplesPerFrame
unsigned int SampleInfo::smSamplesPerFrame[] = { 28, 1, 1, 64, 64 };

// GLOBAL: SWBF2 0x007e3a08
// SampleInfo::smFrameSize
unsigned int SampleInfo::smFrameSize[] = { 16, 2, 1, 36, 36 };

// GLOBAL: SWBF2 0x007e3a1c
// SampleInfo::smBitDepth
unsigned char SampleInfo::smBitDepth[] = { 4, 16, 8, 4, 4 };

// FUNCTION: SWBF2 0x00748f00
unsigned int __cdecl SampleInfo::GetBitDepth(Format eFormat)
{
    return smBitDepth[eFormat - 1];
}

// FUNCTION: SWBF2 0x00748f10
unsigned int __cdecl SampleInfo::GetFrameSize(Format eFormat)
{
    return smFrameSize[eFormat - 1];
}

// FUNCTION: SWBF2 0x00748f20
unsigned int __cdecl SampleInfo::GetSamplesPerFrame(Format eFormat)
{
    return smSamplesPerFrame[eFormat - 1];
}

}
