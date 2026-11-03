#pragma once

#include "sndsource2d3d.h"
#include "sndsource3d.h"
#include "sndsourcei3dl2.h"
#include "sndstreamconsumer.h"
#include "sndstreamnode.h"

namespace Snd
{

class StreamRendererBase : public StreamNode,
                           public StreamConsumer,
                           public Source2D3D,
                           public Source3D,
                           public SourceI3DL2Interpolated
{
public:
    // FUNCTION: SWBF2 0x00748fe0
    StreamRendererBase()
        : Source3D(0)
    {
        Source3D::mProps = static_cast<Source3D::Properties*>(this);
    }

    // FUNCTION: SWBF2 0x00749110
    ~StreamRendererBase() {}
};

 

}
