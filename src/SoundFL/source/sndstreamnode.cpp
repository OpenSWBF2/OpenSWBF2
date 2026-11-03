#include "sndstreamnode.h"

namespace Snd
{

// FUNCTION: SWBF2 0x0073d5e0
void StreamNodeSinkNInput::Connect(StreamNodeSource* const pSource,
                                   unsigned int uiOutput, unsigned int uiInput)
{
    mConnections[uiInput].Initialize(pSource, uiOutput);
}

// FUNCTION: SWBF2 0x0073d600
StreamNodeSource* StreamNodeSinkNInput::ConnectionGetNode(unsigned int uiInput) const
{
    return mConnections[uiInput].GetInputNode();
}

// FUNCTION: SWBF2 0x0073d5c0
StreamNodeSinkNInput::StreamNodeSinkNInput()
    : mConnections(0),
      mNumConnections(0)
{
}

// FUNCTION: SWBF2 0x0073d5d0
StreamNodeSinkNInput::~StreamNodeSinkNInput()
{
}

// FUNCTION: SWBF2 0x0073d680
void StreamNodeSinkNInput::SetConnectionArray(StreamNodeConnection* const pArray,
                                              unsigned int uiCount)
{
    mConnections = pArray;
    mNumConnections = uiCount;
}

// FUNCTION: SWBF2 0x0073d670
bool StreamNode::IsStarving() const
{
    return (mFlags & 1) != 0;
}

// FUNCTION: SWBF2 0x0073d640
void StreamNode::EndFlush()
{
    mFlags &= ~2;
}

// FUNCTION: SWBF2 0x0073d660
bool StreamNode::IsFlushing() const
{
    return (mFlags & 2) != 0;
}

// FUNCTION: SWBF2 0x0073d6c0
void StreamNode::StartFlush()
{
    mFlags |= 2;
}

// FUNCTION: SWBF2 0x0073d6a0
void StreamNode::SetStarved(bool bStarved)
{
    if (bStarved)
    {
        mFlags = (unsigned char)(mFlags | 1u);
    }
    else
    {
        mFlags = (unsigned char)(mFlags & ~1u);
    }
}

// FUNCTION: SWBF2 0x0073d6d0
bool StreamNode::UpdateNode(bool bAllowPausing)
{

    bool bResult = false;
    const PlayState eDesired = QueryDesiredPlayState();

    SourceTransport::mFlags = (unsigned short)
        (SourceTransport::mFlags & ~(unsigned int)flags_updateTransport);

    switch (eDesired)
    {
    case playState_playing:
        SetPlayState(playState_playing);
        break;

    case playState_paused:
        SetStarved(false);

        SetPlayState(bAllowPausing ? playState_pausing : playState_paused);
        break;

    case playState_stopped:
        SetStarved(false);

        bResult = true;
        SourceTransport::mPlaybackPosition = 0;
        SetPlayState(playState_stopped);
        break;

    default:

        break;
    }
    return bResult;
}

}

namespace Snd
{

// FUNCTION: SWBF2 0x0073d610
StreamPacket* StreamNodeSinkNInput::ConnectionGetPacket(unsigned int uiInput) const
{
    return mConnections[uiInput].GetPacket();
}
}

namespace Snd
{

// FUNCTION: SWBF2 0x0073d650
unsigned int StreamNodeSinkNInput::GetNumInputs() const
{
    return mNumConnections;
}
}
