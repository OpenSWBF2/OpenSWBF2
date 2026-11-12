#include <string.h>

#include "sndstreamresampler.h"

namespace Snd
{

// FUNCTION: SWBF2 0x0074cb60
StreamResampler::StreamResampler()
    : mBufferSize(0),
      mStepSize(0x10000),
      mInputPacket(0),
      mInputPacketReadPos(0),
      mSampleOffset(0),
      mPrevSample(0)
{

}

// FUNCTION: SWBF2 0x0074ceb0
void StreamResampler::SetBuffer(void* pBuffer, unsigned int uiSize)
{
    mBufferSize = uiSize;
    mOutputPacket.SetBuffer(pBuffer);
    mOutputPacket.SetUsed(0);
}


// FUNCTION: SWBF2 0x0074cc10
void StreamResampler::CompletePacket()
{
    bool bLast = false;
    bool bDiscontinuity = false;
    unsigned int uiInputSamples = mInputPacket->GetUsed() >> 1;

    if (mOutputPacket.GetUsed() != 0)
    {
        mOutputPacket.Lock();
    }
    StreamPacket* const pInput = mInputPacket;
    mOutputPacket.SetPosition(pInput->GetPosition());

    if (mInputPacketReadPos >= uiInputSamples)
    {
        bLast = pInput->GetLast();
        bDiscontinuity = pInput->GetDiscontinuity();
        mInputPacketReadPos -= uiInputSamples;
        pInput->Unlock();
        mInputPacket = 0;
    }

    mOutputPacket.SetLast(bLast);
    mOutputPacket.SetDiscontinuity(bDiscontinuity);
}

// FUNCTION: SWBF2 0x0074ccc0
void StreamResampler::Flush()
{
    mPrevSample = 0;
    mSampleOffset = 0;
    if (mInputPacket != 0)
    {
        if (mInputPacket->IsLocked())
        {
            mInputPacket->Unlock();
        }
        mInputPacket = 0;
        mInputPacketReadPos = 0;
    }
}

// FUNCTION: SWBF2 0x0074cd30
StreamPacket* StreamResampler::GetPacket(unsigned int)
{
    if (mStepSize == 0x10000)
    {
        StreamPacket* const pInput = mInputPacket;
        if (pInput != 0)
        {
            if (mOutputPacket.IsLocked())
            {
                return 0;
            }
            unsigned int uiCount = (pInput->GetUsed() >> 1) - mInputPacketReadPos;
            if (uiCount > mBufferSize >> 1)
            {
                uiCount = mBufferSize >> 1;
            }
            memcpy(mOutputPacket.GetBuffer(), (short*)pInput->GetBuffer() + mInputPacketReadPos, uiCount * 2);
            mInputPacketReadPos += uiCount;
            mOutputPacket.SetUsed(uiCount);
            CompletePacket();
            if (mOutputPacket.IsLocked())
            {
                return &mOutputPacket;
            }
        }
        return ConnectionGetPacket();
    }

    if (mStepSize == 0 || mOutputPacket.IsLocked())
    {
        return 0;
    }

    StreamPacket* pInput = mInputPacket;
    if (pInput == 0)
    {
        pInput = ConnectionGetPacket();
        if (pInput == 0)
        {
            return 0;
        }
    }

    if (mInputPacketReadPos < pInput->GetUsed())
    {
        const short* pSrc = (const short*)pInput->GetBuffer() + mInputPacketReadPos;
        const short* const pSrcEnd = (const short*)((const char*)pInput->GetBuffer() + pInput->GetUsed());
        short* pDst = (short*)mOutputPacket.GetBuffer();
        short* const pDstEnd = (short*)((char*)pDst + mBufferSize);
        while (pSrc < pSrcEnd && pDst < pDstEnd)
        {
            *pDst++ = (short)(((*pSrc * mSampleOffset) >> 16) + (((0xFFFF - mSampleOffset) * mPrevSample) >> 16));
            mSampleOffset += mStepSize;
            if (mSampleOffset & 0xFFFF0000)
            {
                int iAdvance = mSampleOffset >> 16;
                mPrevSample = *pSrc;
                mSampleOffset &= 0xFFFF;
                pSrc += iAdvance;
            }
        }
        mOutputPacket.SetUsed((char*)pDst - (char*)mOutputPacket.GetBuffer());
        mInputPacketReadPos = pSrc - (const short*)pInput->GetBuffer();
    }
    else
    {
        mInputPacketReadPos -= pInput->GetUsed();
    }

    mInputPacket = pInput;
    CompletePacket();
    if (mOutputPacket.IsLocked())
    {
        return &mOutputPacket;
    }
    return 0;
}

}

namespace Snd
{

// FUNCTION: SWBF2 0x0074ced0
void StreamResampler::Update()
{
    UpdateNode(false);
}
}

namespace Snd
{

// FUNCTION: SWBF2 0x0074cbd0
StreamResampler::~StreamResampler()
{

}
}
