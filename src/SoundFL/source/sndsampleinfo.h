#pragma once

namespace Snd
{

class SampleInfo
{
public:
    enum Format
    {
        format_Unknown = 0,
        format_VAG = 1,
        format_PCM16 = 2,
        format_PCM8 = 3,
        format_XADPCM = 4,
        format_IMAADPCM = 5
    };

    SampleInfo(Format eFormat, unsigned int uiFrequency,
               unsigned int uiLength, unsigned int uiHashID)
        : mFormat((char)eFormat),
          mFrequency(uiFrequency),
          mLength(uiLength),
          mHashID(uiHashID)
    {
    }

    static unsigned int __cdecl GetBitDepth(Format eFormat);
    static unsigned int __cdecl GetFrameSize(Format eFormat);
    static unsigned int __cdecl GetSamplesPerFrame(Format eFormat);

private:
    static unsigned int smSamplesPerFrame[];
    static unsigned int smFrameSize[];
    static unsigned char smBitDepth[];

public:

    char mFormat;
    unsigned int mFrequency;
    unsigned int mLength;
    unsigned int mHashID;
};
 

}
