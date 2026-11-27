#include <windows.h>
#include <bcrypt.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <iterator>

#include "gate/gate_keys.h"

#pragma comment(lib, "bcrypt.lib")

namespace Gate
{

namespace
{

const char* const kComparedTables[] = {
    "tests\\baseline\\v143\\cloth-headroom.txt",
    "tests\\baseline\\v143\\mem-peaks.txt",
    "tests\\baseline\\v143\\pool-sizes.txt",
    "tests\\baseline\\v143\\smoke-all-digests.txt",
    "tests\\baseline\\v143\\lua-bins.txt",
    "tests\\baseline\\v143\\lua-hist.txt",
    "tests\\baseline\\v143\\offthread.txt",
    "tests\\baseline\\v143\\digests.txt",
    "tests\\baseline\\v143\\digests-hog.txt",
    "tests\\baseline\\v143\\digest-trace-geo1c_con.txt",
    "tests\\baseline\\v143\\digest-trace-hot1g_con.txt",
    "tests\\baseline\\v143\\digest-trace-nab2g_c.txt",
    "tests\\baseline\\v143\\digest-trace-spa3c_ass.txt",
    "tests\\baseline\\v143\\digest-trace-dag1c_ctf.txt",
    "tests\\baseline\\v143\\digest-trace-kas2c_uber.txt",
    "tests\\baseline\\v143\\factory-geo1c_con.txt",
    "tests\\baseline\\v143\\factory-hot1g_con.txt",
    "tests\\baseline\\v143\\factory-nab2g_c.txt",
    "tests\\baseline\\v143\\factory-spa3c_ass.txt",
    "tests\\baseline\\v143\\initializers.txt",
    "tests\\baseline\\v143\\initializers-di.txt",
    "tests\\baseline\\v143\\initializers-discarded.txt",
    "tests\\baseline\\v143\\digests-debug.txt",
    "tests\\baseline\\v143\\x87-inventory.tsv",
    "tests\\baseline\\v143-x64\\mem-peaks.txt",
    "tests\\baseline\\v143-x64\\commit.txt",
    "tests\\baseline\\v143-x64\\lua-hist.txt",
    "tests\\baseline\\v143-x64\\cloth-headroom.txt",
    "tests\\baseline\\v143-x64\\pool-sizes.txt",
    "tests\\baseline\\v143-x64\\lua-bins.txt",
    "tests\\baseline\\v143-x64\\offthread.txt",
};

const char* const kExcludedDocs[] = {"tests\\README.md"};

BCRYPT_ALG_HANDLE Sha256Provider()
{
    static BCRYPT_ALG_HANDLE shAlg = 0;
    static bool sbOpened = false;
    if (!sbOpened)
    {
        BCRYPT_ALG_HANDLE hAlg = 0;
        if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0)))
        {
            shAlg = hAlg;
        }
        sbOpened = true;
    }
    return shAlg;
}

struct HashState
{
    BCRYPT_HASH_HANDLE hHash;
    bool bOk;
};

HashState BeginHash()
{
    HashState k;
    k.hHash = 0;
    k.bOk = false;
    BCRYPT_ALG_HANDLE hAlg = Sha256Provider();
    if (hAlg && BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &k.hHash, NULL, 0, NULL, 0, 0)))
    {
        k.bOk = true;
    }
    return k;
}

void AddHash(HashState& k, const void* pData, size_t uSize)
{
    const unsigned char* pBytes = (const unsigned char*)pData;
    while (k.bOk && uSize > 0)
    {
        const ULONG uChunk = uSize > 0x40000000u ? 0x40000000u : (ULONG)uSize;
        if (!BCRYPT_SUCCESS(BCryptHashData(k.hHash, (PUCHAR)pBytes, uChunk, 0)))
        {
            k.bOk = false;
        }
        pBytes += uChunk;
        uSize -= uChunk;
    }
}

std::string EndHash(HashState& k)
{
    unsigned char acDigest[32];
    std::string sHex;
    if (k.bOk && BCRYPT_SUCCESS(BCryptFinishHash(k.hHash, acDigest, sizeof(acDigest), 0)))
    {
        static const char kcHex[] = "0123456789abcdef";
        for (size_t i = 0; i < sizeof(acDigest); ++i)
        {
            sHex += kcHex[acDigest[i] >> 4];
            sHex += kcHex[acDigest[i] & 15];
        }
    }
    if (k.hHash)
    {
        BCryptDestroyHash(k.hHash);
    }
    return sHex;
}

bool ReadU16(const std::vector<unsigned char>& kBytes, unsigned long long uAt, unsigned int& uValue)
{
    if (uAt + 2 > kBytes.size())
    {
        return false;
    }
    uValue = (unsigned int)kBytes[(size_t)uAt] | ((unsigned int)kBytes[(size_t)uAt + 1] << 8);
    return true;
}

bool ReadU32(const std::vector<unsigned char>& kBytes, unsigned long long uAt, unsigned long long& uValue)
{
    if (uAt + 4 > kBytes.size())
    {
        return false;
    }
    const size_t i = (size_t)uAt;
    uValue = (unsigned long long)kBytes[i] | ((unsigned long long)kBytes[i + 1] << 8) |
             ((unsigned long long)kBytes[i + 2] << 16) | ((unsigned long long)kBytes[i + 3] << 24);
    return true;
}

void ClearBytes(std::vector<unsigned char>& kBytes, unsigned long long uAt, size_t uCount)
{
    for (size_t i = 0; i < uCount && uAt + i < kBytes.size(); ++i)
    {
        kBytes[(size_t)uAt + i] = 0;
    }
}

bool ClassMatch(const std::string& sPattern, size_t& uAt, char cText)
{
    size_t i = uAt + 1;
    bool bHit = false;
    const char cLow = (char)tolower((unsigned char)cText);
    while (i < sPattern.size() && sPattern[i] != ']')
    {
        char cFrom = (char)tolower((unsigned char)sPattern[i]);
        if (i + 2 < sPattern.size() && sPattern[i + 1] == '-' && sPattern[i + 2] != ']')
        {
            const char cTo = (char)tolower((unsigned char)sPattern[i + 2]);
            if (cLow >= cFrom && cLow <= cTo)
            {
                bHit = true;
            }
            i += 3;
        }
        else
        {
            if (cLow == cFrom)
            {
                bHit = true;
            }
            ++i;
        }
    }
    uAt = i < sPattern.size() ? i + 1 : i;
    return bHit;
}

bool LikeFrom(const std::string& sText, size_t uText, const std::string& sPattern, size_t uPat)
{
    while (uPat < sPattern.size())
    {
        const char cPat = sPattern[uPat];
        if (cPat == '*')
        {
            for (size_t i = uText; i <= sText.size(); ++i)
            {
                if (LikeFrom(sText, i, sPattern, uPat + 1))
                {
                    return true;
                }
            }
            return false;
        }
        if (uText >= sText.size())
        {
            return false;
        }
        if (cPat == '?')
        {
            ++uText;
            ++uPat;
            continue;
        }
        if (cPat == '[')
        {
            size_t uNext = uPat;
            if (!ClassMatch(sPattern, uNext, sText[uText]))
            {
                return false;
            }
            uPat = uNext;
            ++uText;
            continue;
        }
        if (tolower((unsigned char)cPat) != tolower((unsigned char)sText[uText]))
        {
            return false;
        }
        ++uText;
        ++uPat;
    }
    return uText == sText.size();
}

std::string WithSlash(const std::string& sRoot)
{
    if (!sRoot.empty() && sRoot[sRoot.size() - 1] == '\\')
    {
        return sRoot;
    }
    return sRoot + "\\";
}

std::string RelUnder(const std::string& sPath, const std::string& sRootSlash)
{
    if (StartsWithNoCase(sPath, sRootSlash))
    {
        return LowerText(sPath.substr(sRootSlash.size()));
    }
    return std::string();
}

bool EndsWith(const std::string& sText, const std::string& sTail)
{
    return sText.size() >= sTail.size() && sText.compare(sText.size() - sTail.size(), sTail.size(), sTail) == 0;
}

bool IsHeaderPath(const std::string& sLower)
{
    return EndsWith(sLower, ".h") || EndsWith(sLower, ".hpp") || EndsWith(sLower, ".inl");
}

bool InvalidFieldValue(const std::string& sValue)
{
    if (sValue == "-")
    {
        return true;
    }
    size_t uStart = 0;
    while (uStart <= sValue.size())
    {
        size_t uEnd = sValue.find_first_of(";,", uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sValue.size();
        }
        const std::string sPart = sValue.substr(uStart, uEnd - uStart);
        if (sPart.size() >= 2 && EndsWith(sPart, "=-") && sPart.find('=') == sPart.size() - 2)
        {
            return true;
        }
        uStart = uEnd + 1;
    }
    return false;
}

bool DecisionInvalidValue(const std::string& sValue)
{
    if (sValue == "-")
    {
        return true;
    }
    size_t uAt = sValue.find("=-");
    while (uAt != std::string::npos)
    {
        const size_t uNext = uAt + 2;
        if (uNext == sValue.size() || sValue[uNext] == ';' || sValue[uNext] == ',')
        {
            return true;
        }
        uAt = sValue.find("=-", uAt + 1);
    }
    return false;
}

}

std::string Sha256Hex(const void* pData, size_t uSize)
{
    HashState k = BeginHash();
    AddHash(k, pData, uSize);
    return EndHash(k);
}

std::string TextSha(const std::string& sText)
{
    return Sha256Hex(sText.data(), sText.size());
}

std::wstring Utf8ToWide(const std::string& sText)
{
    if (sText.empty())
    {
        return std::wstring();
    }
    const int iLen = MultiByteToWideChar(CP_UTF8, 0, sText.data(), (int)sText.size(), NULL, 0);
    std::wstring sOut((size_t)(iLen > 0 ? iLen : 0), L'\0');
    if (iLen > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, sText.data(), (int)sText.size(), &sOut[0], iLen);
    }
    return sOut;
}

std::string WideToUtf8(const std::wstring& sText)
{
    if (sText.empty())
    {
        return std::string();
    }
    const int iLen = WideCharToMultiByte(CP_UTF8, 0, sText.data(), (int)sText.size(), NULL, 0, NULL, NULL);
    std::string sOut((size_t)(iLen > 0 ? iLen : 0), '\0');
    if (iLen > 0)
    {
        WideCharToMultiByte(CP_UTF8, 0, sText.data(), (int)sText.size(), &sOut[0], iLen, NULL, NULL);
    }
    return sOut;
}

bool FileSha(const std::string& sPath, std::string& sHash)
{
    HANDLE hFile = CreateFileW(Utf8ToWide(sPath).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    HashState k = BeginHash();
    std::vector<unsigned char> kBuffer(1 << 20);
    bool bOk = true;
    for (;;)
    {
        DWORD uRead = 0;
        if (!ReadFile(hFile, &kBuffer[0], (DWORD)kBuffer.size(), &uRead, NULL))
        {
            bOk = false;
            break;
        }
        if (uRead == 0)
        {
            break;
        }
        AddHash(k, &kBuffer[0], uRead);
    }
    CloseHandle(hFile);
    sHash = EndHash(k);
    return bOk && !sHash.empty();
}

bool ReadFileBytes(const std::string& sPath, std::vector<unsigned char>& kBytes)
{
    kBytes.clear();
    FILE* pFile = _wfopen(Utf8ToWide(sPath).c_str(), L"rb");
    if (!pFile)
    {
        return false;
    }
    unsigned char acBuffer[65536];
    size_t uRead = 0;
    while ((uRead = fread(acBuffer, 1, sizeof(acBuffer), pFile)) > 0)
    {
        kBytes.insert(kBytes.end(), acBuffer, acBuffer + uRead);
    }
    const bool bOk = ferror(pFile) == 0;
    fclose(pFile);
    return bOk;
}

bool ReadTextFile(const std::string& sPath, std::string& sText)
{
    std::vector<unsigned char> kBytes;
    if (!ReadFileBytes(sPath, kBytes))
    {
        return false;
    }
    sText = DecodeText(kBytes);
    return true;
}

static bool PutFile(const std::string& sPath, const std::string& sText, const wchar_t* pcMode)
{
    FILE* pFile = _wfopen(Utf8ToWide(sPath).c_str(), pcMode);
    if (!pFile)
    {
        return false;
    }
    const size_t uWritten = sText.empty() ? 0 : fwrite(sText.data(), 1, sText.size(), pFile);
    const bool bOk = uWritten == sText.size() && ferror(pFile) == 0;
    return fclose(pFile) == 0 && bOk;
}

bool WriteTextFile(const std::string& sPath, const std::string& sText)
{
    return PutFile(sPath, sText, L"wb");
}

bool AppendTextFile(const std::string& sPath, const std::string& sText)
{
    return PutFile(sPath, sText, L"ab");
}

bool FileExists(const std::string& sPath)
{
    const DWORD uAttr = GetFileAttributesW(Utf8ToWide(sPath).c_str());
    return uAttr != INVALID_FILE_ATTRIBUTES && (uAttr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool DirExists(const std::string& sPath)
{
    const DWORD uAttr = GetFileAttributesW(Utf8ToWide(sPath).c_str());
    return uAttr != INVALID_FILE_ATTRIBUTES && (uAttr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool MakeDirs(const std::string& sPath)
{
    if (sPath.empty() || DirExists(sPath))
    {
        return true;
    }
    const size_t uSlash = sPath.find_last_of('\\');
    if (uSlash != std::string::npos && uSlash > 2)
    {
        if (!MakeDirs(sPath.substr(0, uSlash)))
        {
            return false;
        }
    }
    return CreateDirectoryW(Utf8ToWide(sPath).c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool FileWriteTime(const std::string& sPath, unsigned long long& uTime)
{
    WIN32_FILE_ATTRIBUTE_DATA kData;
    if (!GetFileAttributesExW(Utf8ToWide(sPath).c_str(), GetFileExInfoStandard, &kData))
    {
        return false;
    }
    uTime = ((unsigned long long)kData.ftLastWriteTime.dwHighDateTime << 32) | kData.ftLastWriteTime.dwLowDateTime;
    return true;
}

std::string FullPath(const std::string& sPath)
{
    const std::wstring sWide = Utf8ToWide(sPath);
    const DWORD uLen = GetFullPathNameW(sWide.c_str(), 0, NULL, NULL);
    if (uLen == 0)
    {
        return sPath;
    }
    std::wstring sOut(uLen, L'\0');
    const DWORD uGot = GetFullPathNameW(sWide.c_str(), uLen, &sOut[0], NULL);
    sOut.resize(uGot);
    return WideToUtf8(sOut);
}

std::string JoinPath(const std::string& sDir, const std::string& sRel)
{
    if (sRel.size() >= 2 && (sRel[1] == ':' || (sRel[0] == '\\' && sRel[1] == '\\')))
    {
        return sRel;
    }
    if (sDir.empty())
    {
        return sRel;
    }
    return WithSlash(sDir) + sRel;
}

std::string LowerText(const std::string& sText)
{
    std::string sOut = sText;
    for (size_t i = 0; i < sOut.size(); ++i)
    {
        if (sOut[i] >= 'A' && sOut[i] <= 'Z')
        {
            sOut[i] = (char)(sOut[i] - 'A' + 'a');
        }
    }
    return sOut;
}

std::string UpperText(const std::string& sText)
{
    std::string sOut = sText;
    for (size_t i = 0; i < sOut.size(); ++i)
    {
        if (sOut[i] >= 'a' && sOut[i] <= 'z')
        {
            sOut[i] = (char)(sOut[i] - 'a' + 'A');
        }
    }
    return sOut;
}

std::string TrimText(const std::string& sText)
{
    size_t uStart = 0;
    size_t uEnd = sText.size();
    while (uStart < uEnd && isspace((unsigned char)sText[uStart]))
    {
        ++uStart;
    }
    while (uEnd > uStart && isspace((unsigned char)sText[uEnd - 1]))
    {
        --uEnd;
    }
    return sText.substr(uStart, uEnd - uStart);
}

bool StartsWithNoCase(const std::string& sText, const std::string& sPrefix)
{
    return sText.size() >= sPrefix.size() && LowerText(sText.substr(0, sPrefix.size())) == LowerText(sPrefix);
}

bool EqualNoCase(const std::string& sA, const std::string& sB)
{
    return sA.size() == sB.size() && LowerText(sA) == LowerText(sB);
}

std::vector<std::string> SplitText(const std::string& sText, char cSep)
{
    std::vector<std::string> kOut;
    size_t uStart = 0;
    for (;;)
    {
        const size_t uAt = sText.find(cSep, uStart);
        if (uAt == std::string::npos)
        {
            kOut.push_back(sText.substr(uStart));
            return kOut;
        }
        kOut.push_back(sText.substr(uStart, uAt - uStart));
        uStart = uAt + 1;
    }
}

std::vector<std::string> SplitLines(const std::string& sText)
{
    std::vector<std::string> kOut;
    size_t uStart = 0;
    size_t i = 0;
    while (i < sText.size())
    {
        if (sText[i] == '\r' || sText[i] == '\n')
        {
            kOut.push_back(sText.substr(uStart, i - uStart));
            if (sText[i] == '\r' && i + 1 < sText.size() && sText[i + 1] == '\n')
            {
                ++i;
            }
            ++i;
            uStart = i;
        }
        else
        {
            ++i;
        }
    }
    if (uStart < sText.size())
    {
        kOut.push_back(sText.substr(uStart));
    }
    return kOut;
}

std::string JoinText(const std::vector<std::string>& kParts, const std::string& sSep)
{
    std::string sOut;
    for (size_t i = 0; i < kParts.size(); ++i)
    {
        if (i)
        {
            sOut += sSep;
        }
        sOut += kParts[i];
    }
    return sOut;
}

std::string DecodeText(const std::vector<unsigned char>& kBytes)
{
    if (kBytes.size() >= 2 && kBytes[0] == 0xFF && kBytes[1] == 0xFE)
    {
        std::wstring sWide((kBytes.size() - 2) / 2, L'\0');
        for (size_t i = 0; i < sWide.size(); ++i)
        {
            sWide[i] = (wchar_t)(kBytes[2 + i * 2] | (kBytes[3 + i * 2] << 8));
        }
        return WideToUtf8(sWide);
    }
    if (kBytes.size() >= 3 && kBytes[0] == 0xEF && kBytes[1] == 0xBB && kBytes[2] == 0xBF)
    {
        return std::string(kBytes.begin() + 3, kBytes.end());
    }
    return std::string(kBytes.begin(), kBytes.end());
}

bool PeNormalise(std::vector<unsigned char>& kBytes, std::string& sError)
{
    const unsigned long long n = kBytes.size();
    if (n < 0x40 || kBytes[0] != 0x4D || kBytes[1] != 0x5A)
    {
        sError = "not a PE file: no MZ header";
        return false;
    }
    unsigned long long uE = 0;
    ReadU32(kBytes, 0x3C, uE);
    const long long iE = (long long)(int)(unsigned int)uE;
    unsigned long long uSig = 0;
    if (iE < 0x40 || (unsigned long long)iE + 24 > n || !ReadU32(kBytes, (unsigned long long)iE, uSig) || uSig != 0x4550)
    {
        sError = "not a PE file: no PE signature";
        return false;
    }
    const unsigned long long e = (unsigned long long)iE;
    unsigned int uSections = 0;
    unsigned int uOptSize = 0;
    ReadU16(kBytes, e + 6, uSections);
    ReadU16(kBytes, e + 20, uOptSize);
    const unsigned long long uOpt = e + 24;
    if (uOpt + uOptSize > n || uOptSize < 96)
    {
        sError = "not a PE file: optional header truncated";
        return false;
    }
    unsigned int uMagic = 0;
    ReadU16(kBytes, uOpt, uMagic);
    unsigned long long uCountAt = 0;
    unsigned long long uDirAt = 0;
    if (uMagic == 0x10B)
    {
        uCountAt = uOpt + 92;
        uDirAt = uOpt + 96;
    }
    else if (uMagic == 0x20B)
    {
        uCountAt = uOpt + 108;
        uDirAt = uOpt + 112;
    }
    else
    {
        char acMsg[64];
        _snprintf(acMsg, sizeof(acMsg), "not a PE file: optional header magic 0x%X", uMagic);
        acMsg[sizeof(acMsg) - 1] = 0;
        sError = acMsg;
        return false;
    }
    ClearBytes(kBytes, e + 8, 4);
    ClearBytes(kBytes, uOpt + 64, 4);
    unsigned long long uDirCount = 0;
    if (!ReadU32(kBytes, uCountAt, uDirCount))
    {
        sError = "not a PE file: optional header truncated";
        return false;
    }
    if (uDirCount <= 6 || uDirAt + 56 > uOpt + uOptSize)
    {
        return true;
    }
    unsigned long long uDbgRva = 0;
    unsigned long long uDbgSize = 0;
    ReadU32(kBytes, uDirAt + 48, uDbgRva);
    ReadU32(kBytes, uDirAt + 52, uDbgSize);
    if (uDbgRva == 0 || uDbgSize == 0)
    {
        return true;
    }
    const unsigned long long uSecAt = uOpt + uOptSize;
    if (uSecAt + 40ull * uSections > n)
    {
        sError = "not a PE file: section table truncated";
        return false;
    }
    long long iDbgOff = -1;
    for (unsigned int i = 0; i < uSections; ++i)
    {
        const unsigned long long s = uSecAt + 40ull * i;
        unsigned long long uVa = 0;
        unsigned long long uRaw = 0;
        unsigned long long uPtr = 0;
        ReadU32(kBytes, s + 12, uVa);
        ReadU32(kBytes, s + 16, uRaw);
        ReadU32(kBytes, s + 20, uPtr);
        if (uDbgRva >= uVa && uDbgRva < uVa + uRaw)
        {
            iDbgOff = (long long)(uPtr + uDbgRva - uVa);
            break;
        }
    }
    if (iDbgOff < 0 || (unsigned long long)iDbgOff + uDbgSize > n)
    {
        sError = "not a PE file: debug directory outside the file";
        return false;
    }
    const unsigned long long uOff = (unsigned long long)iDbgOff;
    for (unsigned long long d = uOff; d + 28 <= uOff + uDbgSize; d += 28)
    {
        ClearBytes(kBytes, d + 4, 4);
        unsigned long long uType = 0;
        unsigned long long uDataSize = 0;
        unsigned long long uDataPtr = 0;
        ReadU32(kBytes, d + 12, uType);
        ReadU32(kBytes, d + 16, uDataSize);
        ReadU32(kBytes, d + 24, uDataPtr);
        unsigned long long uSig2 = 0;
        if (uType == 2 && uDataSize >= 24 && uDataPtr + 24 <= n && ReadU32(kBytes, uDataPtr, uSig2) &&
            uSig2 == 0x53445352)
        {
            ClearBytes(kBytes, uDataPtr + 4, 20);
        }
    }
    return true;
}

bool PeNormalisedSha(const std::string& sPath, std::string& sHash, std::string& sError)
{
    std::vector<unsigned char> kBytes;
    if (!ReadFileBytes(sPath, kBytes))
    {
        sError = "cannot read " + sPath;
        return false;
    }
    if (!PeNormalise(kBytes, sError))
    {
        return false;
    }
    sHash = Sha256Hex(kBytes.empty() ? "" : (const char*)&kBytes[0], kBytes.size());
    return true;
}

bool LikeMatch(const std::string& sText, const std::string& sPattern)
{
    return LikeFrom(sText, 0, sPattern, 0);
}

bool InputMatches(const std::string& sPath, const std::vector<std::string>& kInputs)
{
    for (size_t i = 0; i < kInputs.size(); ++i)
    {
        const std::string& sIn = kInputs[i];
        if (sIn.empty())
        {
            continue;
        }
        if (sIn[sIn.size() - 1] == '\\')
        {
            if (StartsWithNoCase(sPath, sIn))
            {
                return true;
            }
        }
        else if (sIn.find('*') != std::string::npos)
        {
            if (LikeMatch(sPath, sIn))
            {
                return true;
            }
        }
        else if (EqualNoCase(sPath, sIn))
        {
            return true;
        }
    }
    return false;
}

bool TreeHashes(const std::string& sRoot, TreeListFn pList, std::map<std::string, std::string>& kTree,
                std::string& sError)
{
    std::vector<std::string> kPaths;
    if (!pList(sRoot, kPaths, sError))
    {
        return false;
    }
    kTree.clear();
    for (size_t i = 0; i < kPaths.size(); ++i)
    {
        std::string sRel = kPaths[i];
        std::replace(sRel.begin(), sRel.end(), '/', '\\');
        const std::string sFull = JoinPath(sRoot, sRel);
        std::string sHash = "absent";
        if (FileExists(sFull) && !FileSha(sFull, sHash))
        {
            sError = "cannot hash " + sFull;
            return false;
        }
        kTree[LowerText(sRel)] = sHash;
    }
    return true;
}

void ParseReadTlog(const std::string& sText, const std::string& sRoot, ReadTlog& kTlog)
{
    const std::string sRootSlash = WithSlash(sRoot);
    kTlog.kSets.clear();
    kTlog.kStale.clear();
    std::vector<std::string> kCur;
    const std::vector<std::string> kLines = SplitLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& sLine = kLines[i];
        if (sLine.empty())
        {
            continue;
        }
        if (sLine[0] == '^')
        {
            kCur.clear();
            const std::vector<std::string> kUnits = SplitText(sLine.substr(1), '|');
            for (size_t j = 0; j < kUnits.size(); ++j)
            {
                if (!StartsWithNoCase(kUnits[j], sRootSlash))
                {
                    continue;
                }
                const std::string sRel = RelUnder(kUnits[j], sRootSlash);
                kTlog.kSets[sRel].insert(sRel);
                kCur.push_back(sRel);
            }
        }
        else if (!kCur.empty() && StartsWithNoCase(sLine, sRootSlash))
        {
            const std::string sRel = RelUnder(sLine, sRootSlash);
            for (size_t j = 0; j < kCur.size(); ++j)
            {
                kTlog.kSets[kCur[j]].insert(sRel);
            }
        }
    }
}

void MarkStaleUnits(ReadTlog& kTlog, const std::string& sRoot, unsigned long long uTlogTime, FileTimeFn pTime)
{
    std::map<std::string, bool> kFileStale;
    for (std::map<std::string, std::set<std::string> >::const_iterator it = kTlog.kSets.begin();
         it != kTlog.kSets.end(); ++it)
    {
        for (std::set<std::string>::const_iterator f = it->second.begin(); f != it->second.end(); ++f)
        {
            std::map<std::string, bool>::const_iterator kHit = kFileStale.find(*f);
            bool bStale = false;
            if (kHit != kFileStale.end())
            {
                bStale = kHit->second;
            }
            else
            {
                unsigned long long uTime = 0;
                bStale = !pTime(JoinPath(sRoot, *f), uTime) || uTime > uTlogTime;
                kFileStale[*f] = bStale;
            }
            if (bStale)
            {
                kTlog.kStale.insert(it->first);
                break;
            }
        }
    }
}

static bool RealFileTime(const std::string& sPath, unsigned long long& uTime)
{
    return FileExists(sPath) && FileWriteTime(sPath, uTime);
}

bool LoadReadTlog(const std::string& sPath, const std::string& sRoot, ReadTlog& kTlog)
{
    unsigned long long uTime = 0;
    std::string sText;
    if (sPath.empty() || !FileExists(sPath) || !FileWriteTime(sPath, uTime) || !ReadTextFile(sPath, sText))
    {
        return false;
    }
    ParseReadTlog(sText, sRoot, kTlog);
    MarkStaleUnits(kTlog, sRoot, uTime, RealFileTime);
    return true;
}

std::string CommandRootToken(const std::string& sCommand, const std::string& sRoot)
{
    std::string sTrim = sRoot;
    while (!sTrim.empty() && sTrim[sTrim.size() - 1] == '\\')
    {
        sTrim.erase(sTrim.size() - 1);
    }
    const std::string sUpper = UpperText(sCommand);
    const std::string sNeedle = UpperText(sTrim);
    if (sNeedle.empty())
    {
        return sUpper;
    }
    std::string sOut;
    size_t uStart = 0;
    for (;;)
    {
        const size_t uAt = sUpper.find(sNeedle, uStart);
        if (uAt == std::string::npos)
        {
            sOut += sUpper.substr(uStart);
            return sOut;
        }
        sOut += sUpper.substr(uStart, uAt - uStart);
        sOut += "$ROOT";
        uStart = uAt + sNeedle.size();
    }
}

void ParseCommandTlog(const std::string& sText, const std::string& sRoot, std::map<std::string, std::string>& kCommands)
{
    const std::string sRootSlash = WithSlash(sRoot);
    kCommands.clear();
    std::vector<std::string> kCur;
    std::map<std::string, std::string> kText;
    const std::vector<std::string> kLines = SplitLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& sLine = kLines[i];
        if (sLine.empty())
        {
            continue;
        }
        if (sLine[0] == '^')
        {
            kCur.clear();
            const std::vector<std::string> kUnits = SplitText(sLine.substr(1), '|');
            for (size_t j = 0; j < kUnits.size(); ++j)
            {
                if (StartsWithNoCase(kUnits[j], sRootSlash))
                {
                    const std::string sRel = RelUnder(kUnits[j], sRootSlash);
                    kText[sRel].clear();
                    kCur.push_back(sRel);
                }
            }
        }
        else
        {
            for (size_t j = 0; j < kCur.size(); ++j)
            {
                std::string& sCmd = kText[kCur[j]];
                if (!sCmd.empty())
                {
                    sCmd += "\n";
                }
                sCmd += sLine;
            }
        }
    }
    for (std::map<std::string, std::string>::const_iterator it = kText.begin(); it != kText.end(); ++it)
    {
        kCommands[it->first] = TextSha(CommandRootToken(it->second, sRoot));
    }
}

bool LoadCommandTlog(const std::string& sPath, const std::string& sRoot, std::map<std::string, std::string>& kCommands)
{
    std::string sText;
    if (sPath.empty() || !FileExists(sPath) || !ReadTextFile(sPath, sText))
    {
        return false;
    }
    ParseCommandTlog(sText, sRoot, kCommands);
    return true;
}

void KeyFiles(const KeyFilesInput& kIn, std::map<std::string, std::string>& kFiles, std::string& sTlogState)
{
    kFiles.clear();
    const std::map<std::string, std::string> kEmpty;
    const std::map<std::string, std::string>& kTree = kIn.pTree ? *kIn.pTree : kEmpty;
    for (std::map<std::string, std::string>::const_iterator it = kTree.begin(); it != kTree.end(); ++it)
    {
        if (InputMatches(it->first, kIn.kReads))
        {
            kFiles[it->first] = it->second;
        }
    }
    sTlogState = "none";
    if (!kIn.kSources.empty())
    {
        sTlogState = "ok";
        for (std::map<std::string, std::string>::const_iterator it = kTree.begin(); it != kTree.end(); ++it)
        {
            if (InputMatches(it->first, kIn.kSources))
            {
                kFiles[it->first] = it->second;
            }
        }
        std::set<std::string> kUnits;
        std::set<std::string> kClosure;
        if (!kIn.pRead)
        {
            sTlogState = "-";
        }
        else
        {
            std::vector<std::string> kQueue;
            for (std::map<std::string, std::set<std::string> >::const_iterator it = kIn.pRead->kSets.begin();
                 it != kIn.pRead->kSets.end(); ++it)
            {
                if (InputMatches(it->first, kIn.kSources))
                {
                    kQueue.push_back(it->first);
                }
            }
            if (kQueue.empty())
            {
                sTlogState = "-";
            }
            while (!kQueue.empty())
            {
                const std::string sUnit = kQueue.back();
                kQueue.pop_back();
                if (!kUnits.insert(sUnit).second)
                {
                    continue;
                }
                const std::set<std::string>& kSet = kIn.pRead->kSets.find(sUnit)->second;
                for (std::set<std::string>::const_iterator f = kSet.begin(); f != kSet.end(); ++f)
                {
                    kClosure.insert(*f);
                    if (!StartsWithNoCase(*f, "tests\\") || !IsHeaderPath(*f))
                    {
                        continue;
                    }
                    const std::string sCpp = f->substr(0, f->find_last_of('.')) + ".cpp";
                    const bool bUnit = kIn.pRead->kSets.find(sCpp) != kIn.pRead->kSets.end();
                    if (bUnit)
                    {
                        kQueue.push_back(sCpp);
                    }
                    else if (kTree.find(sCpp) != kTree.end())
                    {
                        kClosure.insert(sCpp);
                    }
                }
            }
        }
        for (std::set<std::string>::const_iterator u = kUnits.begin(); u != kUnits.end(); ++u)
        {
            if (kIn.pRead->kStale.count(*u))
            {
                sTlogState = "-";
            }
            std::map<std::string, std::string>::const_iterator kCmd;
            if (!kIn.pCommands || (kCmd = kIn.pCommands->find(*u)) == kIn.pCommands->end())
            {
                sTlogState = "-";
            }
            else
            {
                kFiles["cmd:" + *u] = kCmd->second;
            }
        }
        for (std::set<std::string>::const_iterator f = kClosure.begin(); f != kClosure.end(); ++f)
        {
            if (kFiles.count(*f))
            {
                continue;
            }
            std::map<std::string, std::string>::const_iterator kHit = kTree.find(*f);
            kFiles[*f] = kHit != kTree.end() ? kHit->second : (kIn.pHash ? kIn.pHash(*f) : std::string("absent"));
        }
    }
    kFiles["gate-step-definition"] = kIn.sDefinition;
}

std::string KeyScalar(const std::string& sValue)
{
    const std::string s = TrimText(sValue);
    return s.empty() ? std::string("-") : s;
}

std::string KeyList(const std::vector<std::string>& kValues)
{
    std::vector<std::string> kParts;
    for (size_t i = 0; i < kValues.size(); ++i)
    {
        if (!kValues[i].empty())
        {
            kParts.push_back(kValues[i]);
        }
    }
    if (kParts.empty())
    {
        return "-";
    }
    std::sort(kParts.begin(), kParts.end());
    return JoinText(kParts, ",");
}

std::string KeyMap(const KeyPairs& kValues)
{
    if (kValues.empty())
    {
        return "-";
    }
    std::vector<std::string> kParts;
    for (size_t i = 0; i < kValues.size(); ++i)
    {
        kParts.push_back(kValues[i].first + "=" + (kValues[i].second.empty() ? std::string("-") : kValues[i].second));
    }
    std::sort(kParts.begin(), kParts.end());
    return JoinText(kParts, ";");
}

const char* const kKeyHeaderNames[] = {"step",    "platform", "config", "toolset", "cl",     "ucrt",   "steam",
                                       "overlay", "gamedata", "exe",    "dlls",    "vendor", "asanrt", "tlog"};
const size_t kKeyHeaderCount = sizeof(kKeyHeaderNames) / sizeof(kKeyHeaderNames[0]);

std::set<std::string> JudgeOnlyFiles(const std::vector<std::string>& kKinds)
{
    std::set<std::string> kSet;
    bool bFirst = true;
    for (size_t i = 0; i < kKinds.size(); ++i)
    {
        if (kKinds[i].empty())
        {
            continue;
        }
        std::set<std::string> kPaths;
        if (EqualNoCase(kKinds[i], "compare"))
        {
            for (size_t j = 0; j < sizeof(kComparedTables) / sizeof(kComparedTables[0]); ++j)
            {
                kPaths.insert(LowerText(kComparedTables[j]));
            }
        }
        if (bFirst)
        {
            kSet = kPaths;
            bFirst = false;
        }
        else
        {
            std::set<std::string> kBoth;
            std::set_intersection(kSet.begin(), kSet.end(), kPaths.begin(), kPaths.end(),
                                  std::inserter(kBoth, kBoth.begin()));
            kSet = kBoth;
        }
    }
    return kSet;
}

StepKey MakeStepKey(const KeyInput& kIn)
{
    StepKey k;
    k.bValid = true;
    for (size_t i = 0; i < kIn.kHeader.size(); ++i)
    {
        const std::string& sName = kIn.kHeader[i].first;
        std::string sValue = KeyScalar(kIn.kHeader[i].second);
        if (sName == "overlay" && sValue != "on" && sValue != "off")
        {
            sValue = "-";
        }
        if (InvalidFieldValue(sValue))
        {
            k.bValid = false;
        }
        if (sName == "platform")
        {
            k.sPlatform = sValue;
        }
        k.kHeader.push_back(sName + "\t" + sValue);
    }
    const std::set<std::string> kJudgeSet = JudgeOnlyFiles(kIn.kJudgeKinds);
    std::vector<std::string> kRun;
    std::vector<std::string> kJudge;
    if (kIn.kFiles.empty())
    {
        k.bValid = false;
        k.kHeader.push_back("files\t-");
    }
    for (std::map<std::string, std::string>::const_iterator it = kIn.kFiles.begin(); it != kIn.kFiles.end(); ++it)
    {
        const std::string sLower = LowerText(it->first);
        bool bSkip = false;
        for (size_t j = 0; j < sizeof(kExcludedDocs) / sizeof(kExcludedDocs[0]); ++j)
        {
            if (sLower == LowerText(kExcludedDocs[j]))
            {
                bSkip = true;
            }
        }
        if (bSkip)
        {
            continue;
        }
        const std::string sLine = sLower + "\t" + it->second;
        if (kJudgeSet.count(sLower))
        {
            kJudge.push_back(sLine);
        }
        else
        {
            kRun.push_back(sLine);
        }
    }
    std::sort(kRun.begin(), kRun.end());
    std::sort(kJudge.begin(), kJudge.end());
    std::vector<std::string> kRunText = k.kHeader;
    kRunText.insert(kRunText.end(), kRun.begin(), kRun.end());
    k.sRunText = JoinText(kRunText, "\n");
    k.sJudgeText = JoinText(kJudge, "\n");
    k.sRunKey = TextSha(k.sRunText);
    k.sJudgeKey = TextSha(k.sJudgeText);
    for (size_t i = 0; i < kRun.size(); ++i)
    {
        k.kRunFiles.push_back(kRun[i].substr(0, kRun[i].find('\t')));
    }
    for (size_t i = 0; i < kJudge.size(); ++i)
    {
        k.kJudgeFiles.push_back(kJudge[i].substr(0, kJudge[i].find('\t')));
    }
    return k;
}

std::vector<std::string> InvalidKeyFields(const StepKey& kKey)
{
    std::vector<std::string> kOut;
    for (size_t i = 0; i < kKey.kHeader.size(); ++i)
    {
        const size_t uTab = kKey.kHeader[i].find('\t');
        if (DecisionInvalidValue(kKey.kHeader[i].substr(uTab + 1)))
        {
            kOut.push_back(kKey.kHeader[i].substr(0, uTab));
        }
    }
    return kOut;
}

const char* const kStepColumns[] = {"Step",     "RunKey", "JudgeKey", "Platform", "Config", "Evidence",
                                    "Result",   "Wall_s", "Tree",     "Commit",   "Date"};
const size_t kStepColumnCount = sizeof(kStepColumns) / sizeof(kStepColumns[0]);
const char* const kStepKeyColumns[] = {"Step", "Platform", "Config"};
const size_t kStepKeyColumnCount = sizeof(kStepKeyColumns) / sizeof(kStepKeyColumns[0]);
const char* const kLedgerColumns[] = {"Step",     "Platform", "Config",   "Tier",   "RunKey", "JudgeKey",
                                      "Decision", "Result",   "Evidence", "Wall_s", "Start",  "End"};
const size_t kLedgerColumnCount = sizeof(kLedgerColumns) / sizeof(kLedgerColumns[0]);

std::vector<std::string> ColumnList(const char* const* ppcColumns, size_t uCount)
{
    return std::vector<std::string>(ppcColumns, ppcColumns + uCount);
}

RecordMap ReadKeyedRecords(const std::string& sText, const std::vector<std::string>& kKeyColumns)
{
    RecordMap kRecords;
    std::vector<std::string> kLines;
    const std::vector<std::string> kAll = SplitLines(sText);
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (!kAll[i].empty())
        {
            kLines.push_back(kAll[i]);
        }
    }
    if (kLines.size() < 2 || kKeyColumns.empty())
    {
        return kRecords;
    }
    const std::vector<std::string> kCols = SplitText(kLines[0], '\t');
    for (size_t i = 1; i < kLines.size(); ++i)
    {
        const std::vector<std::string> kFields = SplitText(kLines[i], '\t');
        RecordRow kRow;
        for (size_t c = 0; c < kCols.size(); ++c)
        {
            kRow[kCols[c]] = c < kFields.size() ? kFields[c] : std::string();
        }
        std::vector<std::string> kKey;
        for (size_t c = 0; c < kKeyColumns.size(); ++c)
        {
            kKey.push_back(kRow[kKeyColumns[c]]);
        }
        if (!kRow[kKeyColumns[0]].empty())
        {
            kRecords[JoinText(kKey, "\t")] = kRow;
        }
    }
    return kRecords;
}

std::string FormatRecord(const RecordRow& kRow, const std::vector<std::string>& kColumns)
{
    std::vector<std::string> kParts;
    for (size_t i = 0; i < kColumns.size(); ++i)
    {
        RecordRow::const_iterator it = kRow.find(kColumns[i]);
        kParts.push_back(it != kRow.end() ? it->second : std::string());
    }
    return JoinText(kParts, "\t");
}

std::string MergeKeyedRecords(const std::string& sOldText, const std::vector<std::string>& kColumns,
                              const std::vector<std::string>& kKeyColumns, const std::vector<std::string>& kNewLines,
                              const std::vector<std::string>& kDropKeys)
{
    std::map<std::string, std::string> kByKey;
    const RecordMap kOld = ReadKeyedRecords(sOldText, kKeyColumns);
    for (RecordMap::const_iterator it = kOld.begin(); it != kOld.end(); ++it)
    {
        if (std::find(kDropKeys.begin(), kDropKeys.end(), it->first) != kDropKeys.end())
        {
            continue;
        }
        kByKey[it->first] = FormatRecord(it->second, kColumns);
    }
    std::vector<size_t> kAt;
    for (size_t i = 0; i < kKeyColumns.size(); ++i)
    {
        kAt.push_back((size_t)(std::find(kColumns.begin(), kColumns.end(), kKeyColumns[i]) - kColumns.begin()));
    }
    for (size_t i = 0; i < kNewLines.size(); ++i)
    {
        if (kNewLines[i].empty())
        {
            continue;
        }
        const std::vector<std::string> kFields = SplitText(kNewLines[i], '\t');
        std::vector<std::string> kKey;
        for (size_t j = 0; j < kAt.size(); ++j)
        {
            kKey.push_back(kAt[j] < kFields.size() ? kFields[kAt[j]] : std::string());
        }
        kByKey[JoinText(kKey, "\t")] = kNewLines[i];
    }
    std::string sOut = JoinText(kColumns, "\t");
    for (std::map<std::string, std::string>::const_iterator it = kByKey.begin(); it != kByKey.end(); ++it)
    {
        sOut += "\r\n" + it->second;
    }
    return sOut + "\r\n";
}

bool RecordUsable(const RecordRow* pRow)
{
    if (!pRow)
    {
        return false;
    }
    for (size_t i = 0; i < kStepColumnCount; ++i)
    {
        RecordRow::const_iterator it = pRow->find(kStepColumns[i]);
        if (it == pRow->end() || it->second.empty() || it->second == "-")
        {
            return false;
        }
    }
    return true;
}

bool RecordMatches(const RecordRow* pRow, const StepKey& kKey)
{
    if (!kKey.bValid || !RecordUsable(pRow))
    {
        return false;
    }
    return pRow->find("Platform")->second == kKey.sPlatform && pRow->find("RunKey")->second == kKey.sRunKey;
}

static std::string ParentDir(const std::string& sPath)
{
    const size_t uSlash = sPath.find_last_of('\\');
    return uSlash == std::string::npos ? std::string() : sPath.substr(0, uSlash);
}

RecordMap ReadStepRecordsFile(const std::string& sPath)
{
    std::string sText;
    if (!ReadTextFile(sPath, sText))
    {
        return RecordMap();
    }
    return ReadKeyedRecords(sText, ColumnList(kStepKeyColumns, kStepKeyColumnCount));
}

bool MergeStepRecordsFile(const std::string& sPath, const std::vector<std::string>& kNewLines,
                          const std::vector<std::string>& kDropKeys)
{
    std::string sOld;
    ReadTextFile(sPath, sOld);
    const std::string sNew = MergeKeyedRecords(sOld, ColumnList(kStepColumns, kStepColumnCount),
                                               ColumnList(kStepKeyColumns, kStepKeyColumnCount), kNewLines, kDropKeys);
    return MakeDirs(ParentDir(sPath)) && WriteTextFile(sPath, sNew);
}

bool AppendLedgerLine(const std::string& sPath, const std::string& sLine)
{
    if (!FileExists(sPath))
    {
        if (!WriteTextFile(sPath, JoinText(ColumnList(kLedgerColumns, kLedgerColumnCount), "\t") + "\r\n"))
        {
            return false;
        }
    }
    return AppendTextFile(sPath, sLine + "\r\n");
}

}
