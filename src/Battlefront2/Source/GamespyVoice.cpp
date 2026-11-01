#include "SoundFL/source/sndstreamcodec.h"

namespace Snd
{

// FUNCTION: SWBF2 0x0073d200
StreamPacket* StreamCodec::GetPacket(unsigned int uiOutput)
{
    if (StreamNode::GetCurrentPlayState() != playState_playing)
    {
        return 0;
    }

    StreamPacket* pPacket = StreamSourceProducer::GetPacket(uiOutput);
    if (pPacket == 0)
    {
        return 0;
    }

    if (pPacket->GetLast())
    {
        StreamNode::Stop();
    }

    return pPacket;
}

}
