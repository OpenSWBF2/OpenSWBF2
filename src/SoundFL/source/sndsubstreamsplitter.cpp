#include "sndsubstreamsplitter.h"
#include <string.h>

namespace Snd
{

// FUNCTION: SWBF2 0x0074ae30
SubstreamSplitter::SubstreamSplitter()
    : mNumPackets(0),
      mPacketsPerSubstream(0),
      mPackets(0),
      mNumSubstreams(0),
      mSubstreamInterleave(0),
      mReadPos(0),
      mLockPos(0)
{

}

// FUNCTION: SWBF2 0x0074ae90
SubstreamSplitter::~SubstreamSplitter()
{
}

}

namespace Snd
{

// FUNCTION: SWBF2 0x0074aed0
void SubstreamSplitter::Flush()
{
    PlayState ePlayState = GetCurrentPlayState();
    SetPlayState(playState_playing);

    for (unsigned int i = 0; i < mNumSubstreams; ++i)
    {
        StreamPacket* pPacket;
        do
        {
            pPacket = GetPacket(i);
            if (pPacket)
            {
                pPacket->Unlock();
            }
        } while (pPacket);
    }

    SetPlayState(ePlayState);
}

// FUNCTION: SWBF2 0x0074af60
StreamPacket* SubstreamSplitter::GetPacket(unsigned int uiSubstream)
{
    Update();

    if (GetCurrentPlayState() == playState_playing && mPackets != 0 && mLockPos != 0)
    {
        unsigned short& rPos = mLockPos[uiSubstream];
        StreamPacket* pPacket = &mPackets[rPos + uiSubstream * mPacketsPerSubstream];
        if (pPacket->GetUsed() != 0 && !pPacket->IsLocked())
        {
            if (++rPos == mPacketsPerSubstream)
            {
                rPos = 0;
            }
            pPacket->Lock();
            return pPacket;
        }
    }
    return 0;
}

}

namespace Snd
{

// FUNCTION: SWBF2 0x0074afe0
bool SubstreamSplitter::IsQueueFull() const
{
    bool bFull = false;
    for (unsigned int i = 0; i < mNumSubstreams; ++i)
    {
        bFull |= mPackets[mReadPos[i] + i * mPacketsPerSubstream].GetUsed() != 0;
    }
    return bFull;
}

// FUNCTION: SWBF2 0x0074b030
void __cdecl SubstreamSplitter::PacketCompleteCallback(StreamPacket* const  ,
                                                       void* const pData)
{
    ((StreamPacket*)pData)->Unlock();
}

// FUNCTION: SWBF2 0x0074b0d0
void SubstreamSplitter::Update()
{
    const PlayState eDesired = QueryDesiredPlayState();

    SourceTransport::mFlags = (unsigned short)
        (SourceTransport::mFlags & ~(unsigned int)flags_updateTransport);
    PlayState eCurrent = GetCurrentPlayState();

    switch (eDesired)
    {
    case playState_playing:
    case playState_stopped:
        eCurrent = eDesired;
        SetPlayState(eDesired);
        break;
    case playState_paused:
        if (IsQueueFull())
        {
            SetPlayState(playState_paused);
            return;
        }
        SetPlayState(playState_pausing);
        Pause();
        goto fill;
    }

    if (eCurrent != playState_playing && eCurrent != playState_pausing)
    {
        return;
    }

fill:
    while (!IsQueueFull())
    {
        StreamPacket* const pPacket = ConnectionGetPacket();
        if (pPacket == 0)
        {
            break;
        }

        const unsigned int uiShort =
            (mSubstreamBlockSize - pPacket->GetUsed()) / mNumSubstreams;

        for (unsigned int i = 0; i < mNumSubstreams; ++i)
        {
            unsigned short& rPos = mReadPos[i];
            StreamPacket& rSub = mPackets[rPos + i * mPacketsPerSubstream];

            pPacket->Lock();
            rSub.SetCallback(PacketCompleteCallback, pPacket);
            rSub.SetBuffer((char*)pPacket->GetBuffer() + i * mSubstreamInterleave);
            rSub.SetUsed(mSubstreamInterleave - uiShort);
            if (pPacket->GetDiscontinuity())
            {
                rSub.SetDiscontinuity(true);
            }
            else
            {
                rSub.SetDiscontinuity(false);
            }
            if (pPacket->GetLast())
            {
                rSub.SetLast(true);
            }
            else
            {
                rSub.SetLast(false);
            }
            rSub.SetPosition(pPacket->GetPosition() / mNumSubstreams);

            if (++rPos == mPacketsPerSubstream)
            {
                rPos = 0;
            }
        }

        pPacket->Unlock();
    }
}

// SYNTHETIC: SWBF2 0x0074aea0
// Snd::SubstreamSplitter::`scalar deleting destructor'

}

namespace Snd
{

// FUNCTION: SWBF2 0x0074b070
void SubstreamSplitter::Setup(unsigned int uiNumSubstreams, unsigned int uiSubstreamInterleave,
                              StreamPacket* pPackets, unsigned short* pReadPos,
                              unsigned short* pLockPos, unsigned int uiNumPackets)
{
    mNumPackets = uiNumPackets;
    mPacketsPerSubstream = uiNumPackets / uiNumSubstreams;
    mPackets = pPackets;
    mNumSubstreams = uiNumSubstreams;
    mSubstreamInterleave = uiSubstreamInterleave;
    mSubstreamBlockSize = uiNumSubstreams * uiSubstreamInterleave;
    mReadPos = pReadPos;
    mLockPos = pLockPos;
    memset(pReadPos, 0, uiNumSubstreams * sizeof(unsigned short));
    memset(pLockPos, 0, uiNumSubstreams * sizeof(unsigned short));
}
}
