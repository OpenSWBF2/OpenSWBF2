#pragma once

#include <stddef.h>

#include "snddsbufferrenderer.h"

namespace Snd
{

class StreamFormat;

// SYNTHETIC: SWBF2 0x0073f3b0
// Snd::StreamAuxRenderer::`scalar deleting destructor'

// VTABLE: SWBF2 0x00786964
class StreamAuxRenderer : public DSBufferRenderer
{
public:
    StreamAuxRenderer();
    virtual ~StreamAuxRenderer();
    bool IsOpen() const { return mIsOpen; }
    bool Open(unsigned char ucDeviceIndex, StreamFormat const& cFormat,
              float fBufferMs);
    void Close();

private:
    virtual void UpdateObject();

public:
    bool mIsOpen;
    IDirectSound* mDevice;
};
 
 
 


}
