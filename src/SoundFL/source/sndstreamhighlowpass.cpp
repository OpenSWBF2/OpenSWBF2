#include "sndstreamhighlowpass.h"

namespace Snd
{

// FUNCTION: SWBF2 0x00747d40
StreamLowHighPass::StreamLowHighPass()
    : mBufferSize(0)
{

}

// FUNCTION: SWBF2 0x00747e00
StreamLowHighPass::~StreamLowHighPass()
{

}

// SYNTHETIC: SWBF2 0x00747e60
// Snd::StreamLowHighPass::`scalar deleting destructor'

// FUNCTION: SWBF2 0x00747ee0
void StreamLowHighPass::Flush()
{
    mInputPacket.Flush();
}

// FUNCTION: SWBF2 0x00747ef0
StreamPacket* StreamLowHighPass::GetPacket(unsigned int uiOutput)
{
    (void)uiOutput;

    StreamPacketIO* const pIO = &mInputPacket;
    if (pIO->GetPacket() == 0)
    {
        pIO->SetPacket(ConnectionGetPacket());
    }

    StreamPacket* const pInput = pIO->GetPacket();
    if (pInput == 0 || mOutputPacket.IsLocked())
    {
        return 0;
    }

    StreamPacket* const pOutput = &mOutputPacket;
    short* const pOut = (short*)pOutput->GetBuffer();
    unsigned int uiSize = pIO->GetDataLeft(pInput->GetUsed());
    if (uiSize > mBufferSize)
    {
        uiSize = mBufferSize;
    }

    DSP::HighLowPassFilter::Process16((const short*)pIO->GetData(), uiSize >> 1, pOut);

    pOutput->SetUsed(uiSize);
    pOutput->SetPosition(pInput->GetPosition() + (pIO->GetOffset() >> 1));

    if (pIO->UpdateOffset(pInput->GetUsed(), uiSize))
    {
        pOutput->SetLast(pInput->GetLast());
        if (pInput->GetDiscontinuity())
        {
            pOutput->SetDiscontinuity(true);
        }
        else
        {
            pOutput->SetDiscontinuity(false);
        }

        mInputPacket.Flush();
    }
    else
    {
        pOutput->SetLast(false);
        pOutput->SetDiscontinuity(false);
    }

    pOutput->Lock();
    return pOutput;
}

// FUNCTION: SWBF2 0x00748050
void StreamLowHighPass::SetBuffer(void* pBuffer, unsigned int uiSize)
{
    mBufferSize = uiSize;
    mOutputPacket.SetBuffer(pBuffer);
    mOutputPacket.SetUsed(0);
}

}
