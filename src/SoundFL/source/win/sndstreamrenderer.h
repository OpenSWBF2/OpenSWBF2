#pragma once

#include <stddef.h>

#include "SoundFL/source/sndsampleinfo.h"
#include "SoundFL/source/sndstreamnode.h"
#include "SoundFL/source/sndstreamrendererbase.h"
#include "SoundFL/source/sndstreamresampler.h"
#include "snddsbufferrenderer.h"

namespace Snd
{

// SYNTHETIC: SWBF2 0x00749150
// Snd::StreamRenderer::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786bf4
class StreamRenderer : public StreamRendererBase, public StreamNodeSource
{
public:
    StreamRenderer();

    virtual ~StreamRenderer();

    bool SetFormat(SampleInfo::Format eFormat, unsigned int uiNumChannels,
                   unsigned int uiFrequency, bool b3D, unsigned long ulFlags,
                   float fBufferMs);

    void Set3DMode(bool enable3D)
    {
        if (mFormat != SampleInfo::format_Unknown
            && Get3DMode() != enable3D && mNumChannels == 1)
        {
            SetFormat(mFormat, 1, mFrequency, enable3D, mChannelMask, 200.0f);
        }
    }

    void Flush();

    void Update();

private:
    void ConnectInternal();
    void DisconnectInternal();

protected:

    virtual StreamPacket* GetPacket(unsigned int uiOutput);

public:
    unsigned int mFrequency;
    unsigned int mNumChannels;
    SampleInfo::Format mFormat;
    unsigned long mChannelMask;
    DSBufferRenderer* mHardwareRenderer;
    unsigned char mHardwareRendererStorage[416];
    unsigned int mOutConnIdx;
    StreamResampler mResampler;
    void* mResamplerBuffer;
};

 
 
 
 
 
 

}
