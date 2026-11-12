#include <windows.h>
#include <objbase.h>
#include <string.h>

#include "snddsdmo.h"

#include "sndengine.h"
#include "sndsoftoutput.h"

// GLOBAL: SWBF2 0x00787dcc
// IID_IMediaBuffer
// GLOBAL: SWBF2 0x00787ddc
// IID_IMediaObject
// GLOBAL: SWBF2 0x00787dfc
// IID_IMediaObjectInPlace
// GLOBAL: SWBF2 0x00787e5c
// kMediaTypeAudio
// GLOBAL: SWBF2 0x00787e6c
// kMediaSubTypePcm
// GLOBAL: SWBF2 0x00787e7c
// kFormatWaveFormatEx
// GLOBAL: SWBF2 0x00787e8c
// IID_IUnknown

namespace Snd
{

// FUNCTION: SWBF2 0x0074d530
bool DSDMO::DeliverInputData(void* const pData, unsigned int uiSize)
{
    mInputData.SetBuffer(pData, uiSize, uiSize);
    return SUCCEEDED(mObject->ProcessInput(0, (IMediaBuffer*)&mInputData, 1, 0, 0));
}

// FUNCTION: SWBF2 0x0074d5c0
unsigned int DSDMO::GetOutputData(void* const pData, unsigned int uiSize)
{
    DSMediaBuffer buffer;
    IMediaBuffer* pMediaBuffer;
    DSMediaBuffer::Create((unsigned char*)pData, uiSize, IID_IMediaBuffer,
                          (void**)&pMediaBuffer, &buffer);

    DMO_OUTPUT_DATA_BUFFER output = { pMediaBuffer, 0, 0, 0 };
    DWORD dwStatus;
    const HRESULT hr = mObject->ProcessOutput(1, 1, &output, &dwStatus);
    unsigned int uiLength = buffer.GetLength();
    pMediaBuffer->Release();
    if (Engine::Failed(hr, "source\\win\\snddsdmo.cpp", 252))
    {
        uiLength = 0;
    }
    return uiLength;
}

static const GUID kMediaTypeAudio =
    { 0x73647561, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

static const GUID kMediaSubTypePcm =
    { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

static const GUID kFormatWaveFormatEx =
    { 0x05589F81, 0xC356, 0x11CE, { 0xBF, 0x01, 0x00, 0xAA, 0x00, 0x55, 0x59, 0x5A } };

// FUNCTION: SWBF2 0x0074d230
MediaType::MediaType(const StreamFormat& rFormat)
    : mWaveformat(rFormat.GetFormat(), rFormat.GetNumChannels(),
                  rFormat.GetFrequency(), false, 0)
{
    memset((DMO_MEDIA_TYPE*)this, 0, sizeof(DMO_MEDIA_TYPE));

    majortype = kMediaTypeAudio;
    subtype = kMediaSubTypePcm;

    pbFormat = (BYTE*)&mWaveformat;
    lSampleSize = mWaveformat.Format.wBitsPerSample >> 3;

    formattype = kFormatWaveFormatEx;
    cbFormat = sizeof(WAVEFORMATEX);
    bFixedSizeSamples = TRUE;
    bTemporalCompression = FALSE;
    pUnk = 0;
}

// FUNCTION: SWBF2 0x0074d370
bool DSDMO::Create(const _GUID& rCLSID, const StreamFormat* const pInput,
                   const StreamFormat* const pOutput)
{
    if (Engine::Failed(CoCreateInstance(rCLSID, 0, CLSCTX_INPROC,
                                        IID_IMediaObject, (void**)&mObject),
                       "source\\win\\snddsdmo.cpp", 88)
        || !mObject)
    {
        return false;
    }

    if (FAILED(mObject->QueryInterface(IID_IMediaObjectInPlace,
                                       (void**)&mObjectInPlace)))
    {
        mObjectInPlace = 0;
    }

    MediaType input(*pInput);
    MediaType output(*pOutput);

    if (Engine::Succeeded(mObject->SetInputType(0, &input, DMO_SET_TYPEF_TEST_ONLY),
                          "source\\win\\snddsdmo.cpp", 105)
        && Engine::Succeeded(mObject->SetInputType(0, &input, 0), "source\\win\\snddsdmo.cpp", 108)
        && Engine::Succeeded(mObject->SetOutputType(0, &output, DMO_SET_TYPEF_TEST_ONLY),
                             "source\\win\\snddsdmo.cpp", 110)
        && Engine::Succeeded(mObject->SetOutputType(0, &output, 0), "source\\win\\snddsdmo.cpp", 113)
        && Engine::Succeeded(mObject->AllocateStreamingResources(), "source\\win\\snddsdmo.cpp", 115))
    {
        return true;
    }

    Destroy();
    return false;
}

// FUNCTION: SWBF2 0x0074d570
void DSDMO::Destroy()
{
    const bool bHadInPlace = (mObjectInPlace != 0);

    if (mObjectInPlace)
    {
        mObjectInPlace->Release();
        mObjectInPlace = 0;
    }

    if (mObject)
    {
        if (!bHadInPlace)
        {
            mObject->Flush();
        }
        mObject->Release();
        mObject = 0;
    }
}

// FUNCTION: SWBF2 0x0074d5b0
IMediaObject* DSDMO::GetMediaObject() const
{
    return mObject;
}

// FUNCTION: SWBF2 0x0074d6d0
bool DSDMO::IsValid() const
{
    if (mObject == 0)
    {
        return false;
    }

    return true;
}

// FUNCTION: SWBF2 0x0074d6a0
bool DSDMO::IsInputDataComplete()
{
    if (mInputData.GetRefCount() == 1)
    {
        mInputData.SetBuffer(0, 0, 0);
        return true;
    }
    return false;
}

}

namespace Snd
{

// FUNCTION: SWBF2 0x0074d1b0
DSDMO::DSDMO()
    : mObject(NULL),
      mObjectInPlace(NULL)
{
    void* pBuffer;
    DSMediaBuffer::Create(NULL, 0, IID_IMediaBuffer, &pBuffer, &mInputData);
}
}

namespace Snd
{

// FUNCTION: SWBF2 0x0074d2d0
DSDMO::~DSDMO()
{
    IMediaBuffer* const pInputData = &mInputData;
    pInputData->Release();
    if (IsValid())
    {
        Destroy();
    }

}
}
