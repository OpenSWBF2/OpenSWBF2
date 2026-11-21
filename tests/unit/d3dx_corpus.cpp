#include "unit/d3dx_corpus.h"

#include "PebbleFL/pc/pcD3dxPort.h"

#include <math.h>
#include <stdio.h>
#include <vector>

namespace
{
void PortMatrixMultiply(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixMultiply(PcD3dx::MatrixMultiply, puIn, uVariant, puOut);
}

void PortVec3TransformCoord(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::Vec3Transform(PcD3dx::Vec3TransformCoord, puIn, uVariant, puOut);
}

void PortVec3TransformNormal(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::Vec3Transform(PcD3dx::Vec3TransformNormal, puIn, uVariant, puOut);
}

void PortVec3Normalize(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::Vec3Normalize(PcD3dx::Vec3Normalize, puIn, uVariant, puOut);
}

void PortQuaternionNormalize(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::QuaternionNormalize(PcD3dx::QuaternionNormalize, puIn, uVariant, puOut);
}

void PortMatrixRotationQuaternion(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotationQuaternion(PcD3dx::MatrixRotationQuaternion, puIn, uVariant, puOut);
}

void PortMatrixScaling(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixScaling(PcD3dx::MatrixScaling, puIn, uVariant, puOut);
}

void PortVec4Transform(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::Vec4Transform(PcD3dx::Vec4Transform, puIn, uVariant, puOut);
}

void PortMatrixInverse(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixInverse(PcD3dx::MatrixInverse, puIn, uVariant, puOut);
}

const D3dxCorpusFn sBaseFns[] = {
    {"MatrixMultiply", "mm", 32, 16, 5, true, false, PortMatrixMultiply},
    {"Vec3TransformCoord", "vm", 19, 3, 4, false, false, PortVec3TransformCoord},
    {"Vec3TransformNormal", "vm", 19, 3, 4, true, false, PortVec3TransformNormal},
    {"Vec3Normalize", "v", 3, 3, 2, false, false, PortVec3Normalize},
    {"QuaternionNormalize", "q", 4, 4, 2, false, false, PortQuaternionNormalize},
    {"MatrixRotationQuaternion", "q", 4, 16, 1, true, false, PortMatrixRotationQuaternion},
    {"MatrixScaling", "s", 3, 16, 1, true, false, PortMatrixScaling},
    {"Vec4Transform", "qm", 20, 4, 4, true, false, PortVec4Transform},
    {"MatrixInverse", "n", 16, 18, 4, false, false, PortMatrixInverse},
};

const unsigned int sSpecials[] = {
    0x00000000u, 0x80000000u, 0x00000001u, 0x80000001u, 0x007FFFFFu, 0x807FFFFFu, 0x00800000u, 0x80800000u,
    0x7F7FFFFFu, 0xFF7FFFFFu, 0x7F800000u, 0xFF800000u, 0x7FC00000u, 0xFFC00000u, 0x7F800001u, 0xFF800001u,
    0x7FBFFFFFu, 0xFFBFFFFFu, 0x7FC00001u, 0xFFC00001u, 0x7FFFFFFFu, 0xFFFFFFFFu, 0x7FA5A5A5u, 0xFFE12345u,
    0x3F800000u, 0xBF800000u, 0x3F000000u, 0x28800000u, 0x287FFFFFu, 0x34000000u, 0x1F800000u, 0x5F800000u,
};

const unsigned int SPECIAL_COUNT = sizeof(sSpecials) / sizeof(sSpecials[0]);

struct Rng
{
    unsigned int uState;

    unsigned int Next()
    {
        uState ^= uState << 13;
        uState ^= uState >> 17;
        uState ^= uState << 5;
        return uState;
    }
};

unsigned int NameSeed(const char* pcName)
{
    unsigned int uHash = 2166136261u;
    for (const char* p = pcName; *p; ++p)
    {
        uHash = (uHash ^ (unsigned char)*p) * 16777619u;
    }
    return uHash;
}

unsigned int Moderate(Rng& kRng, unsigned int uLoExp, unsigned int uSpan)
{
    const unsigned int uBits = kRng.Next();
    const unsigned int uExp = uLoExp + (kRng.Next() % uSpan);
    return (uBits & 0x807FFFFFu) | (uExp << 23);
}

unsigned int Special(Rng& kRng)
{
    return sSpecials[kRng.Next() % SPECIAL_COUNT];
}

unsigned int Word(Rng& kRng, unsigned int uMode)
{
    if (uMode == 0)
    {
        return kRng.Next();
    }
    if (uMode == 1)
    {
        return Moderate(kRng, 100, 40);
    }
    if ((kRng.Next() & 1) == 0)
    {
        return Special(kRng);
    }
    return (kRng.Next() & 1) ? kRng.Next() : Moderate(kRng, 100, 40);
}

const float kOne = 1.0f;

unsigned int FloatWord(float f)
{
    unsigned int u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

void StructuredMatrix(Rng& kRng, unsigned int* puOut)
{
    const unsigned int uKind = kRng.Next() % 5;
    for (unsigned int i = 0; i < 16; ++i)
    {
        puOut[i] = (i % 5) == 0 ? FloatWord(kOne) : 0u;
    }
    if (uKind == 1)
    {
        for (unsigned int i = 0; i < 16; ++i)
        {
            if ((kRng.Next() & 3) == 0)
            {
                puOut[i] = Moderate(kRng, 90, 30);
            }
        }
    }
    else if (uKind == 2)
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            puOut[8 + i] = puOut[4 + i];
        }
        puOut[4 + (kRng.Next() & 3)] = Moderate(kRng, 120, 8);
    }
    else if (uKind == 3)
    {
        for (unsigned int i = 0; i < 16; ++i)
        {
            puOut[i] = Moderate(kRng, 120, 10);
        }
        puOut[3] = 0u;
        puOut[7] = 0u;
        puOut[11] = 0u;
        puOut[15] = FloatWord(kOne);
    }
    else if (uKind == 4)
    {
        puOut[(kRng.Next() % 4) * 5] = Moderate(kRng, 1, 20);
    }
}

unsigned int DoubledWord(unsigned int uWord)
{
    float f;
    memcpy(&f, &uWord, sizeof(f));
    return FloatWord(f * 2.0f);
}

void StructuredSingular(Rng& kRng, unsigned int* puOut)
{
    StructuredMatrix(kRng, puOut);
    const unsigned int uKind = kRng.Next() % 5;
    const unsigned int uDst = kRng.Next() % 4;
    const unsigned int uSrc = (uDst + 1 + kRng.Next() % 3) % 4;
    for (unsigned int i = 0; i < 4; ++i)
    {
        if (uKind == 1)
        {
            puOut[uDst * 4 + i] = 0u;
        }
        else if (uKind == 2 || uKind == 4)
        {
            puOut[uDst * 4 + i] = puOut[uSrc * 4 + i];
        }
        else if (uKind == 3)
        {
            puOut[i * 4 + uDst] = DoubledWord(puOut[i * 4 + uSrc]);
        }
    }
    if (uKind == 4)
    {
        puOut[uDst * 4 + (kRng.Next() & 3)] ^= 1u;
    }
}

void StructuredVector(Rng& kRng, unsigned int* puOut, unsigned int uCount)
{
    const unsigned int uKind = kRng.Next() % 4;
    for (unsigned int i = 0; i < uCount; ++i)
    {
        if (uKind == 0)
        {
            puOut[i] = (kRng.Next() & 1) ? 0x80000000u : 0u;
        }
        else if (uKind == 1)
        {
            puOut[i] = Moderate(kRng, 102, 4);
        }
        else if (uKind == 2)
        {
            puOut[i] = Moderate(kRng, 1, 40);
        }
        else
        {
            puOut[i] = Moderate(kRng, 124, 6);
        }
    }
}

unsigned int Nudged(Rng& kRng, unsigned int uWord, unsigned int uSpan)
{
    return uWord + (kRng.Next() % (2 * uSpan + 1)) - uSpan;
}

unsigned int StructuredAngle(Rng& kRng)
{
    const unsigned int uSign = (kRng.Next() & 1) << 31;
    const unsigned int uKind = kRng.Next() % 4;
    if (uKind == 0)
    {
        const double dQuarter = 1.5707963267948966;
        const unsigned int uMultiple = kRng.Next() % (1u << (kRng.Next() % 25));
        return uSign | Nudged(kRng, FloatWord((float)(uMultiple * dQuarter)) & 0x7FFFFFFFu, 3);
    }
    if (uKind == 1)
    {
        return uSign | Nudged(kRng, 0x5F000000u, 48);
    }
    if (uKind == 2)
    {
        return uSign | Nudged(kRng, 0x42BC8000u, 4);
    }
    return Moderate(kRng, 143, 112);
}

void StructuredAxis(Rng& kRng, unsigned int* puOut)
{
    const unsigned int uKind = kRng.Next() % 6;
    if (uKind == 0)
    {
        double ad[3];
        double dLen = 0.0;
        for (unsigned int i = 0; i < 3; ++i)
        {
            ad[i] = (double)(int)(kRng.Next() % 2001u) - 1000.0;
            dLen += ad[i] * ad[i];
        }
        if (dLen == 0.0)
        {
            ad[kRng.Next() % 3] = 1.0;
            dLen = 1.0;
        }
        for (unsigned int i = 0; i < 3; ++i)
        {
            puOut[i] = FloatWord((float)(ad[i] / sqrt(dLen)));
        }
        return;
    }
    for (unsigned int i = 0; i < 3; ++i)
    {
        if (uKind == 1)
        {
            puOut[i] = Moderate(kRng, 100, 60);
        }
        else if (uKind == 2)
        {
            puOut[i] = (kRng.Next() & 1) ? 0x80000000u : 0u;
        }
        else if (uKind == 3)
        {
            puOut[i] = kRng.Next() & 0x807FFFFFu;
        }
        else if (uKind == 4)
        {
            puOut[i] = Moderate(kRng, 200, 55);
        }
        else
        {
            puOut[i] = Moderate(kRng, 120, 10);
        }
    }
    if (uKind == 5)
    {
        puOut[kRng.Next() % 3] = Special(kRng);
    }
}

void StructuredCase(const D3dxCorpusFn& kFn, Rng& kRng, unsigned int* puIn)
{
    unsigned int uWord = 0;
    for (const char* p = kFn.pcLayout; *p; ++p)
    {
        if (*p == 'm')
        {
            StructuredMatrix(kRng, puIn + uWord);
            uWord += 16;
        }
        else if (*p == 'n')
        {
            StructuredSingular(kRng, puIn + uWord);
            uWord += 16;
        }
        else if (*p == 'a')
        {
            puIn[uWord] = StructuredAngle(kRng);
            uWord += 1;
        }
        else if (*p == 'x')
        {
            StructuredAxis(kRng, puIn + uWord);
            uWord += 3;
        }
        else if (*p == 'v')
        {
            StructuredVector(kRng, puIn + uWord, 3);
            uWord += 3;
        }
        else if (*p == 'q')
        {
            StructuredVector(kRng, puIn + uWord, 4);
            uWord += 4;
        }
        else
        {
            StructuredVector(kRng, puIn + uWord, 3);
            uWord += 3;
        }
    }
}
}

const D3dxCorpusFn* D3dxCorpusFns(unsigned int& uCount)
{
    static const std::vector<D3dxCorpusFn> vFns = []() {
        std::vector<D3dxCorpusFn> v(sBaseFns, sBaseFns + sizeof(sBaseFns) / sizeof(sBaseFns[0]));
        unsigned int uX87 = 0;
        const D3dxCorpusFn* pX87 = D3dxX87Fns(uX87);
        v.insert(v.end(), pX87, pX87 + uX87);
        return v;
    }();
    uCount = (unsigned int)vFns.size();
    return &vFns[0];
}

const D3dxCorpusFn* D3dxCorpusFind(const char* pcName)
{
    unsigned int uCount = 0;
    const D3dxCorpusFn* pFns = D3dxCorpusFns(uCount);
    for (unsigned int i = 0; i < uCount; ++i)
    {
        if (strcmp(pFns[i].pcName, pcName) == 0)
        {
            return &pFns[i];
        }
    }
    return 0;
}

bool D3dxCorpusHasPort(const D3dxCorpusFn& kFn)
{
    return kFn.pfnPort != 0 && (!kFn.bX64PortOnly || sizeof(void*) == 8);
}

void D3dxCorpusInputs(const D3dxCorpusFn& kFn, unsigned int uFirst, unsigned int uCount, unsigned int* puIn)
{
    const unsigned int uSeed = NameSeed(kFn.pcName);
    for (unsigned int c = 0; c < uCount; ++c)
    {
        const unsigned int uCase = uFirst + c;
        Rng kRng = {(uSeed ^ (uCase * 2654435761u)) | 1u};
        for (int i = 0; i < 4; ++i)
        {
            kRng.Next();
        }
        unsigned int* puCase = puIn + (size_t)c * kFn.uInWords;
        const unsigned int uMode = (uCase / kFn.uVariants) % 4;
        if (uMode == 3)
        {
            StructuredCase(kFn, kRng, puCase);
            continue;
        }
        for (unsigned int i = 0; i < kFn.uInWords; ++i)
        {
            puCase[i] = Word(kRng, uMode);
        }
    }
}

unsigned int D3dxCorpusVariant(const D3dxCorpusFn& kFn, unsigned int uCase)
{
    return uCase % kFn.uVariants;
}

unsigned int D3dxFnv1a(unsigned int uHash, const unsigned int* puWords, size_t uCount)
{
    const unsigned char* puc = (const unsigned char*)puWords;
    for (size_t i = 0; i < uCount * sizeof(unsigned int); ++i)
    {
        uHash = (uHash ^ puc[i]) * 16777619u;
    }
    return uHash;
}

unsigned int D3dxCorpusHash(const D3dxCorpusFn& kFn, D3dxEvalFn pfnEval)
{
    std::vector<unsigned int> in((size_t)D3DX_CORPUS_BLOCK * kFn.uInWords);
    std::vector<unsigned int> out(kFn.uOutWords);
    unsigned int uHash = 2166136261u;
    for (unsigned int uFirst = 0; uFirst < D3DX_CORPUS_CASES; uFirst += D3DX_CORPUS_BLOCK)
    {
        D3dxCorpusInputs(kFn, uFirst, D3DX_CORPUS_BLOCK, &in[0]);
        for (unsigned int c = 0; c < D3DX_CORPUS_BLOCK; ++c)
        {
            pfnEval(&in[(size_t)c * kFn.uInWords], D3dxCorpusVariant(kFn, uFirst + c), &out[0]);
            uHash = D3dxFnv1a(uHash, &out[0], kFn.uOutWords);
        }
    }
    return uHash;
}

std::string D3dxPinsText(D3dxEvalFn (*pfnRefFor)(const char*))
{
    std::string sText;
    unsigned int uCount = 0;
    const D3dxCorpusFn* pFns = D3dxCorpusFns(uCount);
    for (unsigned int i = 0; i < uCount; ++i)
    {
        if (!pFns[i].bPinned)
        {
            continue;
        }
        const D3dxEvalFn pfnRef = pfnRefFor(pFns[i].pcName);
        if (!pfnRef)
        {
            return std::string();
        }
        char acLine[128];
        _snprintf(acLine, sizeof(acLine), "%s\t%u\t%08X\r\n", pFns[i].pcName, D3DX_CORPUS_CASES,
                  D3dxCorpusHash(pFns[i], pfnRef));
        acLine[sizeof(acLine) - 1] = 0;
        sText += acLine;
    }
    return sText;
}

bool D3dxReadPin(const char* pcName, unsigned int& uCases, unsigned int& uHash)
{
    FILE* pFile = fopen("corpus\\d3dx\\pins.txt", "rb");
    if (!pFile)
    {
        return false;
    }
    char acLine[256];
    bool bFound = false;
    while (!bFound && fgets(acLine, sizeof(acLine), pFile))
    {
        char acName[128];
        unsigned int uC = 0;
        unsigned int uH = 0;
        if (sscanf(acLine, "%127[^\t]\t%u\t%x", acName, &uC, &uH) == 3 && strcmp(acName, pcName) == 0)
        {
            uCases = uC;
            uHash = uH;
            bFound = true;
        }
    }
    fclose(pFile);
    return bFound;
}
