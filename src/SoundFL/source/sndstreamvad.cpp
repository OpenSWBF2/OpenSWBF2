
#include "snddsp.h"
#include "sndqueue.h"
#include "sndstreampacket.h"
#include "sndstreamvad.h"

namespace Snd
{

// FUNCTION: SWBF2 0x00741fc0
StreamVAD::StreamVAD()
    : mSampleRate(0x1F40),
      mReleaseTimeBytesRemaining(0),
      mActive(false)
{
    mReleaseTimeBytes = 0x0AF0;
}

// FUNCTION: SWBF2 0x00742050
StreamVAD::~StreamVAD()
{
}



// FUNCTION: SWBF2 0x007421a0
StreamPacket* StreamVAD::GetPacket(unsigned int uiOutput)
{
    typedef StreamPacket* (__thiscall* GetPacketFn)(StreamNodeSource* pThis,
                                                    unsigned int uiOutput);
    (void)uiOutput;

    if (GetCurrentPlayState() != playState_playing)
    {
        return 0;
    }

    StreamPacket* pPacket = 0;
    if (!mActive || !mPacketQueue.IsFull())
    {

        StreamNodeSource* const pNode = mConnection.mInputNode;
        pPacket = pNode
            ? pNode->GetPacket(mConnection.mInputNodeOutputIndex)
            : 0;

        if (pPacket != 0)
        {
            unsigned int uiBytes = pPacket->GetUsed();
            bool bVoice = false;

            if (DSP::SignalPower16((const short*)pPacket->GetBuffer(),
                                   pPacket->GetUsed() >> 1) > 30000)
            {
                const unsigned int uiNumSamples = pPacket->GetUsed() >> 1;
                const unsigned int uiFrequency = DSP::TurningPointsFrequency16(
                    (const short*)pPacket->GetBuffer(), uiNumSamples,
                    mSampleRate);
                if ((uiFrequency > 120 && uiFrequency < 7000) ||
                    uiFrequency - 10001 <= 1998)
                {
                    if (!mActive)
                    {
                        mActive = true;
                        mReleaseTimeBytesRemaining = mReleaseTimeBytes;
                    }
                    bVoice = true;
                }
            }

            if (mActive)
            {
                mPacketQueue.Add(pPacket);
                mPlaybackPosition = pPacket->GetPosition();

                if (!bVoice)
                {
                    short* const pSamples = (short*)pPacket->GetBuffer();
                    if (uiBytes > mReleaseTimeBytesRemaining)
                    {
                        uiBytes = mReleaseTimeBytesRemaining;
                    }

                    const unsigned int uiNumSamples = uiBytes >> 1;
                    for (unsigned int i = 0; i < uiNumSamples; i++)
                    {
                        const unsigned int uiGain =
                            (mReleaseTimeBytesRemaining * 0x7FFF) / mReleaseTimeBytes;
                        mReleaseTimeBytesRemaining -= 2;
                        pSamples[i] = (short)((pSamples[i] * (int)uiGain) >> 16);
                    }

                    if (mReleaseTimeBytesRemaining == 0)
                    {
                        pPacket->SetDiscontinuity(true);
                        mActive = false;
                    }
                }
            }
            else
            {
                pPacket->Unlock();
                pPacket = 0;
            }
        }
    }

    if (!mPacketQueue.IsEmpty())
    {
        mPacketQueue.Get(pPacket);
        if (pPacket->GetLast())
        {
            Stop();
        }
    }

    return pPacket;
}

}

// FUNCTION: SWBF2 0x00742120
void Snd::StreamVAD::Flush()
{

    while (!mPacketQueue.IsEmpty())
    {
        Snd::StreamPacket* pPacket;
        mPacketQueue.Get(pPacket);
        pPacket->Unlock();
    }

    mReleaseTimeBytesRemaining = 0;
    mActive = false;
}

// FUNCTION: SWBF2 0x00742350
void Snd::StreamVAD::Update()
{
    if (StreamNode::UpdateNode(false))
    {
        Flush();
    }
}
