#include "RedShadowBlob.h"
#include "PebbleFL/Common/PblRandom.h"
#include "PebbleFL/Common/PblMath.h"

// GLOBAL: SWBF2 0x009c8954
RedShadowBlob* RedShadowBlob::sInstance;

// SYNTHETIC: SWBF2 0x00409520
// `dynamic initializer for 'shadowBlobRandom''

// GLOBAL: SWBF2 0x009c8958
// shadowBlobRandom
PblRandom shadowBlobRandom;

// FUNCTION: SWBF2 0x006fc810
bool RedShadowBlob::Create()
{
    return false;
}

// GLOBAL: SWBF2 0x007e1900

float gShadowBlurNeighborWeight = 0.1f;
// GLOBAL: SWBF2 0x007e1904

int gShadowBlurPassCount = 3;

// FUNCTION: SWBF2 0x006fc840
void RedShadowBlob::Update()
{
    for (int row = 0; row < 15; ++row)
    {
        for (int col = 0; col < 20; ++col)
        {
            mData[col][row].blurValue = mData[col][row].intensity = mData[col][row].setValue;
        }
    }

    const int iNumPasses = gShadowBlurPassCount;
    for (int pass = 0; pass < iNumPasses; ++pass)
    {
        const float fWeight = gShadowBlurNeighborWeight;
        for (int row = 1; row < 14; ++row)
        {
            for (int col = 1; col < 19; ++col)
            {
                float intensity = mData[col + 1][row].blurValue * fWeight;
                intensity += mData[col][row].intensity;
                intensity += mData[col][row + 1].blurValue * fWeight;
                intensity += mData[col - 1][row].blurValue * fWeight;
                intensity += mData[col][row - 1].blurValue * fWeight;
                intensity = Min(intensity, 1.0f);
                mData[col][row].intensity = intensity;
            }
        }
        for (int row = 1; row < 14; ++row)
        {
            for (int col = 1; col < 19; ++col)
            {
                mData[col][row].blurValue = mData[col][row].intensity;
            }
        }
    }

    for (int row = 0; row < 15; ++row)
    {
        for (int col = 0; col < 20; ++col)
        {
            float& value = mData[col][row].setValue;
            value += (0.0f - value) * 0.02f;
        }
    }
    SetZOrder(0xfe);
}

// FUNCTION: SWBF2 0x006fc820
bool RedShadowBlob::Destroy()
{
    if (!RedShadowBlob::sInstance)
    {
        return false;
    }
    delete RedShadowBlob::sInstance;
    RedShadowBlob::sInstance = 0;
    return true;
}
