#include "unit/d3dx_corpus.h"

#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "testutil.h"

namespace
{
void RefMatrixMultiply(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixMultiply(D3DXMatrixMultiply, puIn, uVariant, puOut);
}

void RefVec3TransformCoord(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::Vec3Transform(D3DXVec3TransformCoord, puIn, uVariant, puOut);
}

void RefVec3TransformNormal(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::Vec3Transform(D3DXVec3TransformNormal, puIn, uVariant, puOut);
}

void RefVec3Normalize(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::Vec3Normalize(D3DXVec3Normalize, puIn, uVariant, puOut);
}

void RefQuaternionNormalize(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::QuaternionNormalize(D3DXQuaternionNormalize, puIn, uVariant, puOut);
}

void RefMatrixRotationQuaternion(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotationQuaternion(D3DXMatrixRotationQuaternion, puIn, uVariant, puOut);
}

void RefMatrixScaling(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixScaling(D3DXMatrixScaling, puIn, uVariant, puOut);
}

void RefVec4Transform(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::Vec4Transform(D3DXVec4Transform, puIn, uVariant, puOut);
}

void RefMatrixInverse(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixInverse(D3DXMatrixInverse, puIn, uVariant, puOut);
}

void RefMatrixRotationX(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotation(D3DXMatrixRotationX, puIn, uVariant, puOut);
}

void RefMatrixRotationY(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotation(D3DXMatrixRotationY, puIn, uVariant, puOut);
}

void RefMatrixRotationZ(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotation(D3DXMatrixRotationZ, puIn, uVariant, puOut);
}

void RefMatrixRotationAxis(const unsigned int* puIn, unsigned int uVariant, unsigned int* puOut)
{
    D3dxEval::MatrixRotationAxis(D3DXMatrixRotationAxis, puIn, uVariant, puOut);
}

struct RefEntry
{
    const char* pcName;
    D3dxEvalFn pfnRef;
};

const RefEntry sRefs[] = {
    {"MatrixMultiply", RefMatrixMultiply},
    {"Vec3TransformCoord", RefVec3TransformCoord},
    {"Vec3TransformNormal", RefVec3TransformNormal},
    {"Vec3Normalize", RefVec3Normalize},
    {"QuaternionNormalize", RefQuaternionNormalize},
    {"MatrixRotationQuaternion", RefMatrixRotationQuaternion},
    {"MatrixScaling", RefMatrixScaling},
    {"Vec4Transform", RefVec4Transform},
    {"MatrixInverse", RefMatrixInverse},
    {"MatrixRotationX", RefMatrixRotationX},
    {"MatrixRotationY", RefMatrixRotationY},
    {"MatrixRotationZ", RefMatrixRotationZ},
    {"MatrixRotationAxis", RefMatrixRotationAxis},
};

D3dxEvalFn RefFor(const char* pcName)
{
    for (size_t i = 0; i < sizeof(sRefs) / sizeof(sRefs[0]); ++i)
    {
        if (strcmp(sRefs[i].pcName, pcName) == 0)
        {
            return sRefs[i].pfnRef;
        }
    }
    return 0;
}

bool FileSha256(const char* pcPath, std::string& sHex)
{
    sHex.clear();
    FILE* pFile = fopen(pcPath, "rb");
    if (!pFile)
    {
        return false;
    }
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    bool bOk = CryptAcquireContextA(&hProv, 0, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
               CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
    unsigned char aucBuf[65536];
    size_t uRead;
    while (bOk && (uRead = fread(aucBuf, 1, sizeof(aucBuf), pFile)) > 0)
    {
        bOk = CryptHashData(hHash, aucBuf, (DWORD)uRead, 0) != 0;
    }
    bOk = bOk && ferror(pFile) == 0;
    fclose(pFile);
    unsigned char aucDigest[32];
    DWORD uLen = sizeof(aucDigest);
    bOk = bOk && CryptGetHashParam(hHash, HP_HASHVAL, aucDigest, &uLen, 0) && uLen == sizeof(aucDigest);
    if (hHash)
    {
        CryptDestroyHash(hHash);
    }
    if (hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    if (!bOk)
    {
        return false;
    }
    char acHex[65];
    for (int i = 0; i < 32; ++i)
    {
        _snprintf(acHex + i * 2, 3, "%02x", aucDigest[i]);
    }
    acHex[64] = 0;
    sHex = acHex;
    return true;
}

std::string PinnedWin32Sha()
{
    std::string sProps;
    if (!ReadRepoText("lib\\vendor-pins.props", sProps))
    {
        return std::string();
    }
    const std::string sOpen = "<D3dxWin32Sha256>";
    const size_t uAt = sProps.find(sOpen);
    if (uAt == std::string::npos)
    {
        return std::string();
    }
    const size_t uEnd = sProps.find('<', uAt + sOpen.size());
    if (uEnd == std::string::npos)
    {
        return std::string();
    }
    return sProps.substr(uAt + sOpen.size(), uEnd - uAt - sOpen.size());
}

std::string DirOf(const std::string& sPath)
{
    const size_t uSlash = sPath.find_last_of("\\/");
    return uSlash == std::string::npos ? std::string() : sPath.substr(0, uSlash);
}

std::string LowerPath(std::string s)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
    return s;
}

bool CheckPinnedDll(std::string& sWhy, std::string& sDll)
{
    D3DXMATRIX kA;
    D3DXMatrixScaling(&kA, 1.0f, 1.0f, 1.0f);
    HMODULE hDll = GetModuleHandleA("d3dx9_43.dll");
    if (!hDll)
    {
        sWhy = "d3dx9_43.dll is not loaded";
        return false;
    }
    char acDll[MAX_PATH];
    char acExe[MAX_PATH];
    const DWORD uDll = GetModuleFileNameA(hDll, acDll, sizeof(acDll));
    const DWORD uExe = GetModuleFileNameA(0, acExe, sizeof(acExe));
    if (uDll == 0 || uDll >= sizeof(acDll) || uExe == 0 || uExe >= sizeof(acExe))
    {
        sWhy = "cannot read the module paths";
        return false;
    }
    sDll = acDll;
    if (LowerPath(DirOf(acDll)) != LowerPath(DirOf(acExe)))
    {
        sWhy = std::string("loaded ") + acDll + " is not next to the exe";
        return false;
    }
    const std::string sPin = PinnedWin32Sha();
    if (sPin.size() != 64)
    {
        sWhy = "no D3dxWin32Sha256 in lib\\vendor-pins.props";
        return false;
    }
    std::string sSha;
    if (!FileSha256(acDll, sSha))
    {
        sWhy = std::string("cannot hash ") + acDll;
        return false;
    }
    if (sSha != sPin)
    {
        sWhy = std::string("loaded ") + acDll + " sha256 " + sSha + " is not the pin " + sPin;
        return false;
    }
    return true;
}

bool RefBlock(const D3dxCorpusFn& kFn, D3dxEvalFn pfnRef, unsigned int uBlock, std::vector<unsigned int>& vOut)
{
    const unsigned int uFirst = uBlock * D3DX_CORPUS_BLOCK;
    if (uFirst >= D3DX_CORPUS_CASES)
    {
        return false;
    }
    std::vector<unsigned int> in((size_t)D3DX_CORPUS_BLOCK * kFn.uInWords);
    vOut.resize((size_t)D3DX_CORPUS_BLOCK * kFn.uOutWords);
    D3dxCorpusInputs(kFn, uFirst, D3DX_CORPUS_BLOCK, &in[0]);
    for (unsigned int c = 0; c < D3DX_CORPUS_BLOCK; ++c)
    {
        pfnRef(&in[(size_t)c * kFn.uInWords], D3dxCorpusVariant(kFn, uFirst + c), &vOut[(size_t)c * kFn.uOutWords]);
    }
    return true;
}

std::string sRefDll;
}

int D3dxRefWritePins(const std::string& sOutDir)
{
    const std::string sText = D3dxPinsText(RefFor);
    if (sText.empty())
    {
        printf("gen-corpus: d3dx reference missing\n");
        return 1;
    }
    const std::string sDir = sOutDir + "\\d3dx";
    CreateDirectoryA(sDir.c_str(), 0);
    FILE* pFile = fopen((sDir + "\\pins.txt").c_str(), "wb");
    if (!pFile)
    {
        printf("gen-corpus: cannot write %s\\pins.txt\n", sDir.c_str());
        return 1;
    }
    const bool bOk = fwrite(sText.data(), 1, sText.size(), pFile) == sText.size();
    fclose(pFile);
    printf("gen-corpus: d3dx pins written\n");
    return bOk ? 0 : 1;
}

bool D3dxReferenceOpen(std::string& sWhy)
{
    std::string sReason;
    if (!CheckPinnedDll(sReason, sRefDll))
    {
        sWhy = "d3dx reference: " + sReason;
        return false;
    }
    unsigned int uCount = 0;
    const D3dxCorpusFn* pFns = D3dxCorpusFns(uCount);
    for (unsigned int f = 0; f < uCount; ++f)
    {
        if (!RefFor(pFns[f].pcName))
        {
            sWhy = std::string("d3dx reference: no reference for ") + pFns[f].pcName;
            return false;
        }
    }
    return true;
}

bool D3dxReferenceBlock(unsigned int uFunction, unsigned int uBlock, std::vector<unsigned int>& vOut,
                        std::string& sWhy)
{
    unsigned int uCount = 0;
    const D3dxCorpusFn* pFns = D3dxCorpusFns(uCount);
    if (uFunction >= uCount || !RefBlock(pFns[uFunction], RefFor(pFns[uFunction].pcName), uBlock, vOut))
    {
        sWhy = "d3dx reference: no block";
        return false;
    }
    return true;
}

std::string D3dxReferenceSource()
{
    return "in-process " + sRefDll;
}

void D3dxReferenceClose()
{
}

int D3dxRefWriteFile(const char* pcPath)
{
    std::string sWhy;
    std::string sDll;
    if (!CheckPinnedDll(sWhy, sDll))
    {
        printf("d3dx-ref: %s\n", sWhy.c_str());
        return 3;
    }
    unsigned int uCount = 0;
    const D3dxCorpusFn* pFns = D3dxCorpusFns(uCount);
    FILE* pFile = fopen(pcPath, "wb");
    if (!pFile)
    {
        printf("d3dx-ref: cannot write %s\n", pcPath);
        return 4;
    }
    const unsigned int auHead[3] = {D3DX_CORPUS_CASES, D3DX_CORPUS_BLOCK, uCount};
    bool bOk = fwrite(D3DX_REF_MAGIC, 1, sizeof(D3DX_REF_MAGIC), pFile) == sizeof(D3DX_REF_MAGIC) &&
               fwrite(auHead, sizeof(auHead), 1, pFile) == 1;
    std::vector<unsigned int> out;
    for (unsigned int f = 0; bOk && f < uCount; ++f)
    {
        const D3dxCorpusFn& kFn = pFns[f];
        const D3dxEvalFn pfnRef = RefFor(kFn.pcName);
        if (!pfnRef)
        {
            printf("d3dx-ref: no reference for %s\n", kFn.pcName);
            fclose(pFile);
            return 5;
        }
        char acName[D3DX_REF_NAME];
        memset(acName, 0, sizeof(acName));
        strncpy(acName, kFn.pcName, sizeof(acName) - 1);
        const unsigned int auFn[3] = {kFn.uInWords, kFn.uOutWords, kFn.uVariants};
        bOk = fwrite(acName, sizeof(acName), 1, pFile) == 1 && fwrite(auFn, sizeof(auFn), 1, pFile) == 1;
        for (unsigned int b = 0; bOk && b < D3DX_CORPUS_CASES / D3DX_CORPUS_BLOCK; ++b)
        {
            RefBlock(kFn, pfnRef, b, out);
            bOk = fwrite(&out[0], sizeof(unsigned int), out.size(), pFile) == out.size();
        }
    }
    bOk = fclose(pFile) == 0 && bOk;
    if (!bOk)
    {
        printf("d3dx-ref: write failed %s\n", pcPath);
        return 4;
    }
    printf("d3dx-ref: %u functions from %s\n", uCount, sDll.c_str());
    return 0;
}
