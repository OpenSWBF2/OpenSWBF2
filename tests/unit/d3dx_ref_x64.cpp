#include "unit/d3dx_corpus.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"

namespace
{
const char* const kPrefix = "d3dx reference child";

struct RefEntry
{
    std::string sName;
    unsigned int uIn;
    unsigned int uOut;
    unsigned int uVariants;
    long long iOffset;
};

std::string sExe;
std::string sFile;
FILE* pRefFile = 0;
std::vector<RefEntry> vEntries;
std::vector<size_t> vMap;

std::string Fail(const std::string& sReason)
{
    return std::string(kPrefix) + ": " + sExe + ": " + sReason;
}

bool ReadEntries(std::string& sWhy)
{
    char acMagic[sizeof(D3DX_REF_MAGIC)];
    unsigned int auHead[3];
    if (fread(acMagic, sizeof(acMagic), 1, pRefFile) != 1 || memcmp(acMagic, D3DX_REF_MAGIC, sizeof(acMagic)) != 0 ||
        fread(auHead, sizeof(auHead), 1, pRefFile) != 1)
    {
        sWhy = Fail("bad header in " + sFile);
        return false;
    }
    if (auHead[0] != D3DX_CORPUS_CASES || auHead[1] != D3DX_CORPUS_BLOCK)
    {
        sWhy = Fail("corpus size differs in " + sFile);
        return false;
    }
    for (unsigned int e = 0; e < auHead[2]; ++e)
    {
        char acName[D3DX_REF_NAME];
        unsigned int auFn[3];
        if (fread(acName, sizeof(acName), 1, pRefFile) != 1 || fread(auFn, sizeof(auFn), 1, pRefFile) != 1)
        {
            sWhy = Fail("short file " + sFile);
            return false;
        }
        acName[sizeof(acName) - 1] = 0;
        RefEntry kEntry = {acName, auFn[0], auFn[1], auFn[2], _ftelli64(pRefFile)};
        vEntries.push_back(kEntry);
        if (_fseeki64(pRefFile, (long long)D3DX_CORPUS_CASES * auFn[1] * sizeof(unsigned int), SEEK_CUR) != 0)
        {
            sWhy = Fail("short file " + sFile);
            return false;
        }
    }
    const long long iEnd = _ftelli64(pRefFile);
    if (_fseeki64(pRefFile, 0, SEEK_END) != 0 || _ftelli64(pRefFile) != iEnd)
    {
        sWhy = Fail("file size differs from its entries in " + sFile);
        return false;
    }
    unsigned int uCount = 0;
    const D3dxCorpusFn* pFns = D3dxCorpusFns(uCount);
    for (unsigned int f = 0; f < uCount; ++f)
    {
        size_t uAt = vEntries.size();
        for (size_t e = 0; e < vEntries.size(); ++e)
        {
            if (vEntries[e].sName == pFns[f].pcName)
            {
                uAt = e;
            }
        }
        if (uAt == vEntries.size())
        {
            sWhy = Fail(std::string("no entry for ") + pFns[f].pcName);
            return false;
        }
        const RefEntry& kEntry = vEntries[uAt];
        if (kEntry.uIn != pFns[f].uInWords || kEntry.uOut != pFns[f].uOutWords ||
            kEntry.uVariants != pFns[f].uVariants)
        {
            sWhy = Fail(std::string("entry layout differs for ") + pFns[f].pcName);
            return false;
        }
        vMap.push_back(uAt);
    }
    return true;
}
}

int D3dxRefWritePins(const std::string&)
{
    printf("gen-corpus: d3dx pins come from the Win32 build\n");
    return 0;
}

int D3dxRefWriteFile(const char*)
{
    printf("d3dx-ref: the reference comes from the Win32 build\n");
    return 2;
}

bool D3dxReferenceOpen(std::string& sWhy)
{
    D3dxReferenceClose();
    if (!RunWin32Child("d3dx-ref", kPrefix, sExe, sFile, sWhy))
    {
        return false;
    }
    pRefFile = fopen(sFile.c_str(), "rb");
    if (!pRefFile)
    {
        sWhy = Fail("cannot open " + sFile);
        return false;
    }
    return ReadEntries(sWhy);
}

bool D3dxReferenceBlock(unsigned int uFunction, unsigned int uBlock, std::vector<unsigned int>& vOut,
                        std::string& sWhy)
{
    if (!pRefFile || uFunction >= vMap.size() || uBlock >= D3DX_CORPUS_CASES / D3DX_CORPUS_BLOCK)
    {
        sWhy = Fail("no block");
        return false;
    }
    const RefEntry& kEntry = vEntries[vMap[uFunction]];
    vOut.resize((size_t)D3DX_CORPUS_BLOCK * kEntry.uOut);
    const long long iAt = kEntry.iOffset + (long long)uBlock * D3DX_CORPUS_BLOCK * kEntry.uOut * sizeof(unsigned int);
    if (_fseeki64(pRefFile, iAt, SEEK_SET) != 0 ||
        fread(&vOut[0], sizeof(unsigned int), vOut.size(), pRefFile) != vOut.size())
    {
        sWhy = Fail("short file " + sFile);
        return false;
    }
    return true;
}

std::string D3dxReferenceSource()
{
    return "child " + sExe;
}

void D3dxReferenceClose()
{
    if (pRefFile)
    {
        fclose(pRefFile);
        pRefFile = 0;
    }
    if (!sFile.empty())
    {
        DeleteFileA(sFile.c_str());
        sFile.clear();
    }
    vEntries.clear();
    vMap.clear();
}
