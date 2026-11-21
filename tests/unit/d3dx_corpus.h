#pragma once

#include <d3dx9math.h>
#include <string.h>
#include <string>
#include <vector>

typedef void (*D3dxEvalFn)(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut);

struct D3dxCorpusFn
{
    const char* pcName;
    const char* pcLayout;
    unsigned int uInWords;
    unsigned int uOutWords;
    unsigned int uVariants;
    bool bPinned;
    bool bX64PortOnly;
    D3dxEvalFn pfnPort;
    const char* pcNoPort;
};

static const unsigned int D3DX_CORPUS_CASES = 262144;
static const unsigned int D3DX_CORPUS_BLOCK = 4096;

const D3dxCorpusFn* D3dxCorpusFns(unsigned int& uCount);
const D3dxCorpusFn* D3dxX87Fns(unsigned int& uCount);
const D3dxCorpusFn* D3dxCorpusFind(const char* pcName);
bool D3dxCorpusHasPort(const D3dxCorpusFn& kFn);
void D3dxCorpusInputs(const D3dxCorpusFn& kFn, unsigned int uFirst, unsigned int uCount, unsigned int* puIn);
unsigned int D3dxCorpusVariant(const D3dxCorpusFn& kFn, unsigned int uCase);
unsigned int D3dxFnv1a(unsigned int uHash, const unsigned int* puWords, size_t uCount);
unsigned int D3dxCorpusHash(const D3dxCorpusFn& kFn, D3dxEvalFn pfnEval);
std::string D3dxPinsText(D3dxEvalFn (*pfnRefFor)(const char*));
bool D3dxReadPin(const char* pcName, unsigned int& uCases, unsigned int& uHash);
int D3dxRefWritePins(const std::string& sOutDir);
int D3dxRefWriteFile(const char* pcPath);

static const char D3DX_REF_MAGIC[8] = {'D', '3', 'D', 'X', 'R', 'E', 'F', '1'};
static const unsigned int D3DX_REF_NAME = 64;

bool D3dxReferenceOpen(std::string& sWhy);
bool D3dxReferenceBlock(unsigned int uFunction, unsigned int uBlock, std::vector<unsigned int>& vOut,
                        std::string& sWhy);
std::string D3dxReferenceSource();
void D3dxReferenceClose();

namespace D3dxEval
{
struct Buffer
{
    alignas(16) float af[64];
};

template <class F> void MatrixMultiply(F fn, const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    Buffer kBuf;
    float* pA = kBuf.af;
    float* pB = kBuf.af + 16;
    float* pO = kBuf.af + 32;
    memcpy(pA, puIn, 16 * sizeof(float));
    memcpy(pB, puIn + 16, 16 * sizeof(float));
    if (uVariant == 1)
    {
        pO = pA;
    }
    else if (uVariant == 2)
    {
        pO = pB;
    }
    else if (uVariant == 3)
    {
        pB = pA;
    }
    else if (uVariant == 4)
    {
        pB = pA;
        pO = pA;
    }
    fn((D3DXMATRIX*)pO, (const D3DXMATRIX*)pA, (const D3DXMATRIX*)pB);
    memcpy(puOut, pO, 16 * sizeof(float));
}

template <class F> void Vec3Transform(F fn, const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    Buffer kBuf;
    float* pV = kBuf.af;
    float* pO = kBuf.af + 4;
    float* pM = (uVariant & 1) ? kBuf.af + 9 : kBuf.af + 8;
    memcpy(pV, puIn, 3 * sizeof(float));
    memcpy(pM, puIn + 3, 16 * sizeof(float));
    if (uVariant & 2)
    {
        pO = pV;
    }
    fn((D3DXVECTOR3*)pO, (const D3DXVECTOR3*)pV, (const D3DXMATRIX*)pM);
    memcpy(puOut, pO, 3 * sizeof(float));
}

template <class F> void Vec3Normalize(F fn, const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    Buffer kBuf;
    float* pV = kBuf.af;
    float* pO = uVariant ? pV : kBuf.af + 4;
    memcpy(pV, puIn, 3 * sizeof(float));
    fn((D3DXVECTOR3*)pO, (const D3DXVECTOR3*)pV);
    memcpy(puOut, pO, 3 * sizeof(float));
}

template <class F> void QuaternionNormalize(F fn, const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    Buffer kBuf;
    float* pQ = kBuf.af;
    float* pO = uVariant ? pQ : kBuf.af + 4;
    memcpy(pQ, puIn, 4 * sizeof(float));
    fn((D3DXQUATERNION*)pO, (const D3DXQUATERNION*)pQ);
    memcpy(puOut, pO, 4 * sizeof(float));
}

template <class F> void MatrixRotationQuaternion(F fn, const unsigned int* puIn, unsigned int, unsigned int* puOut)
{
    Buffer kBuf;
    float* pQ = kBuf.af;
    float* pO = kBuf.af + 16;
    memcpy(pQ, puIn, 4 * sizeof(float));
    fn((D3DXMATRIX*)pO, (const D3DXQUATERNION*)pQ);
    memcpy(puOut, pO, 16 * sizeof(float));
}

template <class F> void Vec4Transform(F fn, const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    Buffer kBuf;
    float* pV = kBuf.af;
    float* pO = kBuf.af + 4;
    float* pM = (uVariant & 1) ? kBuf.af + 9 : kBuf.af + 8;
    memcpy(pV, puIn, 4 * sizeof(float));
    memcpy(pM, puIn + 4, 16 * sizeof(float));
    if (uVariant & 2)
    {
        pO = pV;
    }
    fn((D3DXVECTOR4*)pO, (const D3DXVECTOR4*)pV, (const D3DXMATRIX*)pM);
    memcpy(puOut, pO, 4 * sizeof(float));
}

template <class F> void MatrixInverse(F fn, const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    Buffer kBuf;
    const unsigned int uSentinel = 0xA5A5A5A5u;
    for (unsigned int i = 16; i < 33; ++i)
    {
        memcpy(&kBuf.af[i], &uSentinel, sizeof(float));
    }
    float* pM = kBuf.af;
    float* pO = kBuf.af + 16;
    float* pDet = kBuf.af + 32;
    memcpy(pM, puIn, 16 * sizeof(float));
    if (uVariant == 1)
    {
        pDet = 0;
    }
    else if (uVariant == 2)
    {
        pO = pM;
    }
    else if (uVariant == 3)
    {
        pDet = pO + 5;
    }
    const D3DXMATRIX* pRet = fn((D3DXMATRIX*)pO, pDet, (const D3DXMATRIX*)pM);
    memcpy(puOut, pO, 16 * sizeof(float));
    memcpy(puOut + 16, &kBuf.af[32], sizeof(float));
    puOut[17] = pRet == (const D3DXMATRIX*)pO ? 1u : (pRet == 0 ? 0u : 2u);
}

template <class F> void MatrixRotation(F fn, const unsigned int* puIn, unsigned int, unsigned int* puOut)
{
    Buffer kBuf;
    float fAngle;
    memcpy(&fAngle, puIn, sizeof(fAngle));
    const D3DXMATRIX* pRet = fn((D3DXMATRIX*)kBuf.af, fAngle);
    memcpy(puOut, kBuf.af, 16 * sizeof(float));
    puOut[16] = pRet == (const D3DXMATRIX*)kBuf.af ? 1u : 0u;
}

template <class F> void MatrixRotationAxis(F fn, const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    Buffer kBuf;
    float* pO = kBuf.af;
    float* pV = uVariant ? kBuf.af : kBuf.af + 16;
    float fAngle;
    memcpy(pV, puIn, 3 * sizeof(float));
    memcpy(&fAngle, puIn + 3, sizeof(fAngle));
    const D3DXMATRIX* pRet = fn((D3DXMATRIX*)pO, (const D3DXVECTOR3*)pV, fAngle);
    memcpy(puOut, pO, 16 * sizeof(float));
    puOut[16] = pRet == (const D3DXMATRIX*)pO ? 1u : 0u;
}

template <class F> void MatrixScaling(F fn, const unsigned int* puIn, unsigned int, unsigned int* puOut)
{
    Buffer kBuf;
    float af[3];
    memcpy(af, puIn, sizeof(af));
    fn((D3DXMATRIX*)kBuf.af, af[0], af[1], af[2]);
    memcpy(puOut, kBuf.af, 16 * sizeof(float));
}
}
