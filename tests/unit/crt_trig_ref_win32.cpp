#include "unit/crt_trig.h"

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace
{
__declspec(noinline) double RefSin(double d)
{
    return sin(d);
}

__declspec(noinline) double RefCos(double d)
{
    return cos(d);
}

__declspec(noinline) double RefTan(double d)
{
    return tan(d);
}

__declspec(noinline) float RawAsin(float f)
{
    return (float)asin((double)f);
}

__declspec(noinline) float RawAcos(float f)
{
    return (float)acos((double)f);
}

__declspec(noinline) float RawAtan(float f)
{
    return (float)atan((double)f);
}

__declspec(noinline) float RawAtan2(float fY, float fX)
{
    return (float)atan2((double)fY, (double)fX);
}

__declspec(noinline) float RawExp(float f)
{
    return (float)exp((double)f);
}

__declspec(noinline) float RawLog(float f)
{
    return (float)log((double)f);
}

__declspec(noinline) float RawLog10(float f)
{
    return (float)log10((double)f);
}

__declspec(noinline) float RawPow(float fX, float fY)
{
    return (float)pow((double)fX, (double)fY);
}

__declspec(noinline) float RawFmod(float fX, float fY)
{
    return (float)fmod((double)fX, (double)fY);
}

double RefPrecise(unsigned int uFn, double d)
{
    if (uFn == CRT_TRIG_SIN)
    {
        return RefSin(d);
    }
    if (uFn == CRT_TRIG_COS)
    {
        return RefCos(d);
    }
    return RefTan(d);
}

bool UcrtVersion(unsigned int* puVersion, std::string& sWhy)
{
    HMODULE hUcrt = GetModuleHandleA("ucrtbase.dll");
    HRSRC hInfo = hUcrt ? FindResourceA(hUcrt, MAKEINTRESOURCEA(1), MAKEINTRESOURCEA(16)) : 0;
    HGLOBAL hData = hInfo ? LoadResource(hUcrt, hInfo) : 0;
    const unsigned char* pData = hData ? (const unsigned char*)LockResource(hData) : 0;
    const DWORD uSize = hInfo ? SizeofResource(hUcrt, hInfo) : 0;
    for (DWORD i = 0; pData && i + sizeof(VS_FIXEDFILEINFO) <= uSize; i += 4)
    {
        VS_FIXEDFILEINFO kInfo;
        memcpy(&kInfo, pData + i, sizeof(kInfo));
        if (kInfo.dwSignature == 0xFEEF04BD)
        {
            puVersion[0] = HIWORD(kInfo.dwFileVersionMS);
            puVersion[1] = LOWORD(kInfo.dwFileVersionMS);
            puVersion[2] = HIWORD(kInfo.dwFileVersionLS);
            puVersion[3] = LOWORD(kInfo.dwFileVersionLS);
            return true;
        }
    }
    sWhy = "cannot read the ucrtbase.dll file version";
    return false;
}
}

bool CrtTrigInProcess(CrtTrigReference& kRef, std::string& sWhy)
{
    kRef.vEntries.clear();
    if (!UcrtVersion(kRef.auVersion, sWhy))
    {
        return false;
    }
    std::vector<unsigned long long> vIn;
    for (unsigned int uFn = 0; uFn < CRT_TRIG_FNS; ++uFn)
    {
        CrtTrigInputs(uFn, vIn);
        for (unsigned int uRoute = 0; uRoute < CRT_TRIG_ROUTES; ++uRoute)
        {
            if (!CrtTrigHasRoute(uFn, uRoute))
            {
                continue;
            }
            CrtTrigEntry kEntry;
            kEntry.uSet = CRT_SET_TRIG;
            kEntry.uFn = uFn;
            kEntry.uRoute = uRoute;
            kEntry.vOut.resize(vIn.size());
            for (size_t i = 0; i < vIn.size(); ++i)
            {
                double d;
                memcpy(&d, &vIn[i], sizeof(d));
                const double dOut = uRoute == CRT_TRIG_PRECISE ? RefPrecise(uFn, d) : CrtTrigPlain(uFn, d);
                memcpy(&kEntry.vOut[i], &dOut, sizeof(dOut));
            }
            kRef.vEntries.push_back(kEntry);
        }
    }
    std::vector<CrtSimIn> vPinned;
    std::vector<CrtSimIn> vX87;
    for (unsigned int uFn = 0; uFn < CRT_SIM_FNS; ++uFn)
    {
        if (!CrtSimHasX87(uFn))
        {
            continue;
        }
        CrtSimSplit(uFn, vPinned, vX87);
        CrtTrigEntry kEntry;
        kEntry.uSet = CRT_SET_SIM;
        kEntry.uFn = uFn;
        kEntry.uRoute = 0;
        kEntry.vOut.resize(vX87.size());
        for (size_t i = 0; i < vX87.size(); ++i)
        {
            float fX;
            float fY;
            memcpy(&fX, &vX87[i].uX, sizeof(fX));
            memcpy(&fY, &vX87[i].uY, sizeof(fY));
            const float fOut = CrtSimRaw(uFn, fX, fY);
            unsigned int uOut;
            memcpy(&uOut, &fOut, sizeof(uOut));
            kEntry.vOut[i] = uOut;
        }
        kRef.vEntries.push_back(kEntry);
    }
    return true;
}

float CrtSimRaw(unsigned int uFn, float fX, float fY)
{
    switch (uFn)
    {
    case CRT_SIM_SIN:
        return (float)RefSin((double)fX);
    case CRT_SIM_COS:
        return (float)RefCos((double)fX);
    case CRT_SIM_TAN:
        return (float)RefTan((double)fX);
    case CRT_SIM_SIN_STRICT:
        return (float)CrtTrigPlain(CRT_TRIG_SIN, (double)fX);
    case CRT_SIM_COS_STRICT:
        return (float)CrtTrigPlain(CRT_TRIG_COS, (double)fX);
    case CRT_SIM_ASIN:
        return RawAsin(fX);
    case CRT_SIM_ACOS:
        return RawAcos(fX);
    case CRT_SIM_ATAN:
        return RawAtan(fX);
    case CRT_SIM_ATAN2:
        return RawAtan2(fX, fY);
    case CRT_SIM_EXP:
        return RawExp(fX);
    case CRT_SIM_LOG:
        return RawLog(fX);
    case CRT_SIM_LOG10:
        return RawLog10(fX);
    case CRT_SIM_POW:
        return RawPow(fX, fY);
    default:
        return RawFmod(fX, fY);
    }
}

int CrtSimWritePins(const std::string& sOutDir)
{
    std::vector<CrtSimBlock> vBlocks;
    CrtSimPinBlocks(CrtSimRaw, vBlocks);
    std::string sText;
    for (const CrtSimBlock& kBlock : vBlocks)
    {
        sText += kBlock.sLine + "\r\n";
    }
    const std::string sDir = sOutDir + "\\simmath";
    CreateDirectoryA(sDir.c_str(), 0);
    FILE* pFile = fopen((sDir + "\\crt.txt").c_str(), "wb");
    if (!pFile)
    {
        printf("gen-corpus: cannot write %s\\crt.txt\n", sDir.c_str());
        return 1;
    }
    const bool bOk = fwrite(sText.data(), 1, sText.size(), pFile) == sText.size();
    fclose(pFile);
    printf("gen-corpus: simmath blocks=%u\n", (unsigned int)vBlocks.size());
    return bOk ? 0 : 1;
}

int CrtTrigRefWriteFile(const char* pcPath)
{
    CrtTrigReference kRef;
    std::string sWhy;
    if (!CrtTrigInProcess(kRef, sWhy))
    {
        printf("crt-trig-ref: %s\n", sWhy.c_str());
        return 3;
    }
    if (!CrtTrigWriteFile(pcPath, kRef))
    {
        printf("crt-trig-ref: write failed %s\n", pcPath);
        return 4;
    }
    printf("crt-trig-ref: %u entries\n", (unsigned int)kRef.vEntries.size());
    return 0;
}
