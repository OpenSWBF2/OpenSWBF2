#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "unit/crt_trig.h"

#include "PebbleFL/Common/PblCrtTrig.h"

#include <windows.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <random>

#include "testutil.h"

namespace
{
const unsigned long long kSign = 0x8000000000000000ull;

unsigned long long Bits(double d)
{
    unsigned long long u;
    memcpy(&u, &d, sizeof(u));
    return u;
}

double FromBits(unsigned long long u)
{
    double d;
    memcpy(&d, &u, sizeof(d));
    return d;
}

unsigned long long FloatAsDouble(unsigned int uFloat)
{
    float f;
    memcpy(&f, &uFloat, sizeof(f));
    return Bits((double)f);
}

void AddBoth(std::vector<unsigned long long>& v, unsigned long long u)
{
    v.push_back(u & ~kSign);
    v.push_back(u | kSign);
}

void AddDoubleUlps(std::vector<unsigned long long>& v, unsigned long long uCentre, int iUlps)
{
    for (int k = -iUlps; k <= iUlps; ++k)
    {
        AddBoth(v, uCentre + (long long)k);
    }
}

void AddFloatUlps(std::vector<unsigned long long>& v, unsigned int uCentre, int iUlps)
{
    for (int k = -iUlps; k <= iUlps; ++k)
    {
        AddBoth(v, FloatAsDouble(uCentre + k));
    }
}

unsigned long long Draw64(std::mt19937& kRng)
{
    const unsigned long long uHi = kRng();
    return (uHi << 32) | kRng();
}

unsigned long long Value(unsigned int uFn, unsigned long long uIn, double dOut)
{
    if (CrtTrigPathOf(uFn, uIn) == CRT_TRIG_PATH_SSE)
    {
        const float f = (float)dOut;
        unsigned int u;
        memcpy(&u, &f, sizeof(u));
        return u;
    }
    return Bits(dOut);
}

double Helper(unsigned int uFn, double d)
{
    if (uFn == CRT_TRIG_SIN)
    {
        return PblCrtSin(d);
    }
    if (uFn == CRT_TRIG_COS)
    {
        return PblCrtCos(d);
    }
    return PblCrtTan(d);
}

const char* RouteName(unsigned int uRoute)
{
    return uRoute == CRT_TRIG_PRECISE ? "precise" : "plain";
}

const char* PathName(unsigned int uPath)
{
    return uPath == CRT_TRIG_PATH_X87 ? "x87" : (uPath == CRT_TRIG_PATH_TINY ? "tiny" : "sse");
}
}

const char* CrtTrigName(unsigned int uFn)
{
    return uFn == CRT_TRIG_SIN ? "sin" : (uFn == CRT_TRIG_COS ? "cos" : "tan");
}

bool CrtTrigHasRoute(unsigned int uFn, unsigned int uRoute)
{
    return uRoute == CRT_TRIG_PRECISE || uFn != CRT_TRIG_TAN;
}

unsigned int CrtTrigPathOf(unsigned int uFn, unsigned long long uIn)
{
    const unsigned int uBase = uFn == CRT_TRIG_TAN ? 0x3820u : 0x3030u;
    const short iLimit = uFn == CRT_TRIG_TAN ? (short)0x8A8 : (short)0x10C5;
    const unsigned short uTop = (unsigned short)((uIn >> 48) & 0x7FFF);
    const unsigned short uOff = (unsigned short)(uTop - uBase);
    if (uOff <= (unsigned short)iLimit)
    {
        return CRT_TRIG_PATH_SSE;
    }
    return (short)uOff > iLimit ? CRT_TRIG_PATH_X87 : CRT_TRIG_PATH_TINY;
}

void CrtTrigInputs(unsigned int uFn, std::vector<unsigned long long>& vIn)
{
    vIn.clear();
    const bool bTan = uFn == CRT_TRIG_TAN;
    const unsigned long long uThreshold = bTan ? 0x40C9000000000000ull : 0x40F6000000000000ull;
    const unsigned long long uTiny = bTan ? 0x3820000000000000ull : 0x3030000000000000ull;
    const unsigned int uThresholdFloat = bTan ? 0x46480000u : 0x47B00000u;
    std::mt19937 kRng(0x0C27F000u + uFn);

    AddFloatUlps(vIn, uThresholdFloat, 64);
    AddDoubleUlps(vIn, uThreshold, 64);
    AddDoubleUlps(vIn, uTiny, 64);
    AddDoubleUlps(vIn, 0x43E0000000000000ull, 64);
    AddFloatUlps(vIn, 0x5F000000u, 64);

    for (unsigned int uExp = uThresholdFloat >> 23; uExp <= 254; ++uExp)
    {
        for (unsigned int i = 0; i < 512; ++i)
        {
            AddBoth(vIn, FloatAsDouble((uExp << 23) | (kRng() & 0x7FFFFFu)));
        }
    }
    for (unsigned long long uExp = 0x43E; uExp <= 0x7FE; ++uExp)
    {
        for (unsigned int i = 0; i < 16; ++i)
        {
            AddBoth(vIn, (uExp << 52) | (Draw64(kRng) & 0x000FFFFFFFFFFFFFull));
        }
    }

    const unsigned long long auSpecial[] = {
        0x7FF0000000000000ull, 0x7FEFFFFFFFFFFFFFull, FloatAsDouble(0x7F7FFFFFu), 0x7FF8000000000000ull,
        0x7FF8000000000001ull, 0x7FFFFFFFFFFFFFFFull, 0x7FFC000012345678ull, 0x7FF8000100000000ull,
        0x7FF0000000000001ull, 0x7FF0000100000000ull, 0x7FF4000000000000ull, 0x7FF7FFFFFFFFFFFFull,
        0x7FF0123400000000ull, 0x7FF00000ABCD0000ull, 0x0000000000000000ull, 0x0000000000000001ull,
        0x000FFFFFFFFFFFFFull, 0x0010000000000000ull, FloatAsDouble(0x00000001u), FloatAsDouble(0x00800000u)};
    for (unsigned long long u : auSpecial)
    {
        AddBoth(vIn, u);
    }

    const float fStep = 3.141592654f * 2.0f / 2048.0f;
    for (int i = 0; i < 2048; ++i)
    {
        volatile float fAngle = (float)i * fStep;
        vIn.push_back(Bits((double)fAngle));
    }
    vIn.push_back(Bits(15.0));

    unsigned int uBelow = 0;
    while (uBelow < 32768)
    {
        const unsigned int uRaw = kRng();
        if ((uRaw & 0x7FFFFFFFu) < uThresholdFloat)
        {
            vIn.push_back(FloatAsDouble(uRaw));
            ++uBelow;
        }
    }
    for (unsigned int i = 0; i < 512; ++i)
    {
        AddBoth(vIn, Draw64(kRng) % uTiny);
    }
}

namespace
{
const unsigned int kSimSpecials[] = {
    0x00000000u, 0x00000001u, 0x00000002u, 0x007FFFFFu, 0x00800000u, 0x00800001u, 0x33000000u, 0x3F000000u,
    0x3F7FFFFFu, 0x3F800000u, 0x3F800001u, 0x40000000u, 0x40400000u, 0x40490FDBu, 0x3FC90FDBu, 0x4B000000u,
    0x4B800000u, 0x4F000000u, 0x5F000000u, 0x7F7FFFFFu, 0x7F800000u, 0x7F800001u, 0x7FA00000u, 0x7FBFFFFFu,
    0x7FC00000u, 0x7FC00001u, 0x7FC00002u, 0x7FC12345u, 0x7FFFFFFFu};

unsigned int XorShift(unsigned int& uState)
{
    uState ^= uState << 13;
    uState ^= uState >> 17;
    uState ^= uState << 5;
    return uState;
}

void AddSim(std::vector<CrtSimIn>& v, unsigned int uX, unsigned int uY)
{
    CrtSimIn k = {uX, uY};
    v.push_back(k);
}

void AddSimBoth(std::vector<CrtSimIn>& v, unsigned int uX, unsigned int uY)
{
    AddSim(v, uX & 0x7FFFFFFFu, uY);
    AddSim(v, uX | 0x80000000u, uY);
}

void AddSimUlps(std::vector<CrtSimIn>& v, unsigned int uCentre, int iUlps, unsigned int uY)
{
    for (int k = -iUlps; k <= iUlps; ++k)
    {
        AddSimBoth(v, uCentre + k, uY);
    }
}

unsigned int SimFnBoundaries(unsigned int uFn, const unsigned int*& puOut)
{
    static const unsigned int kSinCos[] = {0x47B00000u, 0x5F000000u, 0x3FC90FDBu, 0x40490FDBu, 0x40C90FDBu};
    static const unsigned int kTan[] = {0x46480000u, 0x5F000000u, 0x3FC90FDBu, 0x40490FDBu, 0x4016CBE4u};
    static const unsigned int kArc[] = {0x3F800000u, 0x3F000000u, 0x3F3504F3u};
    static const unsigned int kAtan[] = {0x3F800000u, 0x4C000000u, 0x33800000u};
    static const unsigned int kExp[] = {0x42B17218u, 0xC2AEAC50u, 0xC2CFF1B5u, 0x42B20000u, 0xC2B00000u, 0x33800000u};
    static const unsigned int kLog[] = {0x3F800000u, 0x00000001u, 0x00800000u, 0x7F7FFFFFu, 0x40490FDBu};
    static const unsigned int kPow[] = {0x3F800000u, 0x3F7FFFFFu, 0x3F800001u, 0x00800000u, 0x7F7FFFFFu};
    static const unsigned int kFmod[] = {0x3F800000u, 0x40490FDBu, 0x00800000u, 0x7F7FFFFFu};
    switch (uFn)
    {
    case CRT_SIM_SIN:
    case CRT_SIM_COS:
    case CRT_SIM_SIN_STRICT:
    case CRT_SIM_COS_STRICT:
        puOut = kSinCos;
        return sizeof(kSinCos) / sizeof(kSinCos[0]);
    case CRT_SIM_TAN:
        puOut = kTan;
        return sizeof(kTan) / sizeof(kTan[0]);
    case CRT_SIM_ASIN:
    case CRT_SIM_ACOS:
        puOut = kArc;
        return sizeof(kArc) / sizeof(kArc[0]);
    case CRT_SIM_ATAN:
    case CRT_SIM_ATAN2:
        puOut = kAtan;
        return sizeof(kAtan) / sizeof(kAtan[0]);
    case CRT_SIM_EXP:
        puOut = kExp;
        return sizeof(kExp) / sizeof(kExp[0]);
    case CRT_SIM_LOG:
    case CRT_SIM_LOG10:
        puOut = kLog;
        return sizeof(kLog) / sizeof(kLog[0]);
    case CRT_SIM_POW:
        puOut = kPow;
        return sizeof(kPow) / sizeof(kPow[0]);
    default:
        puOut = kFmod;
        return sizeof(kFmod) / sizeof(kFmod[0]);
    }
}

unsigned int SimTrigFn(unsigned int uFn)
{
    if (uFn == CRT_SIM_SIN || uFn == CRT_SIM_SIN_STRICT)
    {
        return CRT_TRIG_SIN;
    }
    if (uFn == CRT_SIM_COS || uFn == CRT_SIM_COS_STRICT)
    {
        return CRT_TRIG_COS;
    }
    return uFn == CRT_SIM_TAN ? CRT_TRIG_TAN : CRT_TRIG_FNS;
}
}

const char* CrtSimName(unsigned int uFn)
{
    static const char* const kNames[CRT_SIM_FNS] = {"sin",  "cos", "tan",   "sin.strict", "cos.strict",
                                                    "asin", "acos", "atan", "atan2",      "exp",
                                                    "log",  "log10", "pow", "fmod"};
    return uFn < CRT_SIM_FNS ? kNames[uFn] : "?";
}

bool CrtSimIsPair(unsigned int uFn)
{
    return uFn == CRT_SIM_ATAN2 || uFn == CRT_SIM_POW || uFn == CRT_SIM_FMOD;
}

bool CrtSimHasX87(unsigned int uFn)
{
    return SimTrigFn(uFn) != CRT_TRIG_FNS || uFn == CRT_SIM_ATAN2 || uFn == CRT_SIM_FMOD;
}

bool CrtSimIsX87(unsigned int uFn, const CrtSimIn& kIn)
{
    if (uFn == CRT_SIM_ATAN2)
    {
        return true;
    }
    if (uFn == CRT_SIM_FMOD)
    {
        const bool bNanX = (kIn.uX & 0x7FFFFFFFu) > 0x7F800000u;
        const bool bNanY = (kIn.uY & 0x7FFFFFFFu) > 0x7F800000u;
        return bNanX && bNanY && ((kIn.uX | 0x00400000u) & 0x007FFFFFu) == ((kIn.uY | 0x00400000u) & 0x007FFFFFu);
    }
    const unsigned int uTrig = SimTrigFn(uFn);
    return uTrig != CRT_TRIG_FNS && CrtTrigPathOf(uTrig, FloatAsDouble(kIn.uX)) == CRT_TRIG_PATH_X87;
}

void CrtSimInputs(unsigned int uFn, std::vector<CrtSimIn>& vIn)
{
    vIn.clear();
    unsigned int uState = 0x2545F491u + uFn * 0x9E3779B9u;
    const unsigned int uSpecials = sizeof(kSimSpecials) / sizeof(kSimSpecials[0]);
    const unsigned int* puBound = 0;
    const unsigned int uBounds = SimFnBoundaries(uFn, puBound);
    if (!CrtSimIsPair(uFn))
    {
        for (unsigned int i = 0; i < uSpecials; ++i)
        {
            AddSimBoth(vIn, kSimSpecials[i], 0);
        }
        for (unsigned int i = 0; i < uBounds; ++i)
        {
            AddSimUlps(vIn, puBound[i], 64, 0);
        }
        for (unsigned int uExp = 0; uExp <= 255; ++uExp)
        {
            for (unsigned int i = 0; i < 64; ++i)
            {
                AddSimBoth(vIn, (uExp << 23) | (XorShift(uState) & 0x7FFFFFu), 0);
            }
        }
        for (unsigned int i = 0; i < 262144; ++i)
        {
            AddSim(vIn, XorShift(uState), 0);
        }
        return;
    }
    for (unsigned int i = 0; i < uSpecials; ++i)
    {
        for (unsigned int j = 0; j < uSpecials; ++j)
        {
            for (unsigned int s = 0; s < 4; ++s)
            {
                AddSim(vIn, kSimSpecials[i] | ((s & 1u) << 31), kSimSpecials[j] | ((s & 2u) << 30));
            }
        }
    }
    for (unsigned int i = 0; i < uBounds; ++i)
    {
        for (int k = -16; k <= 16; ++k)
        {
            for (unsigned int j = 0; j < uSpecials; ++j)
            {
                AddSimBoth(vIn, puBound[i] + k, kSimSpecials[j]);
                AddSimBoth(vIn, kSimSpecials[j], puBound[i] + k);
            }
        }
    }
    const unsigned int kNans[] = {0x7FC00000u, 0x7FC00001u, 0x7FC00002u, 0x7FC12345u, 0x7FFFFFFFu,
                                  0x7F800001u, 0x7FA00000u, 0x7FBFFFFFu, 0x7F812345u};
    const unsigned int uNans = sizeof(kNans) / sizeof(kNans[0]);
    for (unsigned int i = 0; i < uNans; ++i)
    {
        for (unsigned int j = 0; j < uNans; ++j)
        {
            for (unsigned int s = 0; s < 4; ++s)
            {
                AddSim(vIn, kNans[i] | ((s & 1u) << 31), kNans[j] | ((s & 2u) << 30));
            }
        }
    }
    for (unsigned int uExpX = 0; uExpX <= 255; ++uExpX)
    {
        for (unsigned int uExpY = 0; uExpY <= 255; ++uExpY)
        {
            const unsigned int uSigns = XorShift(uState);
            AddSim(vIn, (uSigns & 0x80000000u) | (uExpX << 23) | (XorShift(uState) & 0x7FFFFFu),
                   ((uSigns << 1) & 0x80000000u) | (uExpY << 23) | (XorShift(uState) & 0x7FFFFFu));
        }
    }
    for (unsigned int i = 0; i < 262144; ++i)
    {
        const unsigned int uX = XorShift(uState);
        AddSim(vIn, uX, XorShift(uState));
    }
}

void CrtSimSplit(unsigned int uFn, std::vector<CrtSimIn>& vPinned, std::vector<CrtSimIn>& vX87)
{
    std::vector<CrtSimIn> vAll;
    CrtSimInputs(uFn, vAll);
    vPinned.clear();
    vX87.clear();
    for (const CrtSimIn& k : vAll)
    {
        (CrtSimIsX87(uFn, k) ? vX87 : vPinned).push_back(k);
    }
}

std::string CrtSimRange(const std::vector<CrtSimIn>& vIn, size_t uFirst, size_t uCount)
{
    char ac[128];
    _snprintf(ac, sizeof(ac), "inputs %u..%u first %08X,%08X last %08X,%08X", (unsigned int)uFirst,
              (unsigned int)(uFirst + uCount - 1), vIn[uFirst].uX, vIn[uFirst].uY, vIn[uFirst + uCount - 1].uX,
              vIn[uFirst + uCount - 1].uY);
    ac[sizeof(ac) - 1] = 0;
    return ac;
}

void CrtSimPinBlocks(CrtSimEval pfnEval, std::vector<CrtSimBlock>& vBlocks)
{
    vBlocks.clear();
    std::vector<CrtSimIn> vPinned;
    std::vector<CrtSimIn> vX87;
    for (unsigned int uFn = 0; uFn < CRT_SIM_FNS; ++uFn)
    {
        CrtSimSplit(uFn, vPinned, vX87);
        for (size_t uFirst = 0, uBlock = 0; uFirst < vPinned.size(); uFirst += CRT_SIM_BLOCK, ++uBlock)
        {
            const size_t uCount = vPinned.size() - uFirst < CRT_SIM_BLOCK ? vPinned.size() - uFirst : CRT_SIM_BLOCK;
            unsigned int uHash = 2166136261u;
            for (size_t i = uFirst; i < uFirst + uCount; ++i)
            {
                float fX;
                float fY;
                memcpy(&fX, &vPinned[i].uX, sizeof(fX));
                memcpy(&fY, &vPinned[i].uY, sizeof(fY));
                const float fOut = pfnEval(uFn, fX, fY);
                unsigned int uOut;
                memcpy(&uOut, &fOut, sizeof(uOut));
                for (int b = 0; b < 4; ++b)
                {
                    uHash = (uHash ^ ((uOut >> (8 * b)) & 0xFFu)) * 16777619u;
                }
            }
            char acLine[96];
            _snprintf(acLine, sizeof(acLine), "%s\t%u\t%u\t%08X", CrtSimName(uFn), (unsigned int)uBlock,
                      (unsigned int)uCount, uHash);
            acLine[sizeof(acLine) - 1] = 0;
            CrtSimBlock kBlock;
            kBlock.sLine = acLine;
            kBlock.sRange = CrtSimRange(vPinned, uFirst, uCount);
            vBlocks.push_back(kBlock);
        }
    }
}

bool CrtTrigWriteFile(const char* pcPath, const CrtTrigReference& kRef)
{
    FILE* pFile = fopen(pcPath, "wb");
    if (!pFile)
    {
        return false;
    }
    const unsigned int uCount = (unsigned int)kRef.vEntries.size();
    bool bOk = fwrite(CRT_TRIG_MAGIC, sizeof(CRT_TRIG_MAGIC), 1, pFile) == 1 &&
               fwrite(kRef.auVersion, sizeof(kRef.auVersion), 1, pFile) == 1 &&
               fwrite(&uCount, sizeof(uCount), 1, pFile) == 1;
    for (const CrtTrigEntry& kEntry : kRef.vEntries)
    {
        const unsigned int auHead[4] = {kEntry.uSet, kEntry.uFn, kEntry.uRoute, (unsigned int)kEntry.vOut.size()};
        bOk = bOk && fwrite(auHead, sizeof(auHead), 1, pFile) == 1 &&
              fwrite(kEntry.vOut.data(), sizeof(unsigned long long), kEntry.vOut.size(), pFile) == kEntry.vOut.size();
    }
    return fclose(pFile) == 0 && bOk;
}

bool CrtTrigReadFile(const std::string& sPath, CrtTrigReference& kRef, std::string& sWhy)
{
    kRef.vEntries.clear();
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        sWhy = "cannot open " + sPath;
        return false;
    }
    char acMagic[sizeof(CRT_TRIG_MAGIC)];
    unsigned int uCount = 0;
    bool bOk = fread(acMagic, sizeof(acMagic), 1, pFile) == 1 &&
               memcmp(acMagic, CRT_TRIG_MAGIC, sizeof(acMagic)) == 0 &&
               fread(kRef.auVersion, sizeof(kRef.auVersion), 1, pFile) == 1 &&
               fread(&uCount, sizeof(uCount), 1, pFile) == 1 &&
               uCount <= CRT_TRIG_FNS * CRT_TRIG_ROUTES + CRT_SIM_FNS;
    for (unsigned int e = 0; bOk && e < uCount; ++e)
    {
        unsigned int auHead[4];
        bOk = fread(auHead, sizeof(auHead), 1, pFile) == 1 && auHead[0] < CRT_SETS &&
              auHead[1] < (auHead[0] == CRT_SET_TRIG ? (unsigned int)CRT_TRIG_FNS : (unsigned int)CRT_SIM_FNS) &&
              auHead[2] < CRT_TRIG_ROUTES && auHead[3] <= 0x400000u;
        if (bOk)
        {
            CrtTrigEntry kEntry;
            kEntry.uSet = auHead[0];
            kEntry.uFn = auHead[1];
            kEntry.uRoute = auHead[2];
            kEntry.vOut.resize(auHead[3]);
            bOk = auHead[3] == 0 ||
                  fread(kEntry.vOut.data(), sizeof(unsigned long long), auHead[3], pFile) == auHead[3];
            kRef.vEntries.push_back(kEntry);
        }
    }
    bOk = bOk && fgetc(pFile) == EOF;
    fclose(pFile);
    if (!bOk)
    {
        sWhy = "bad or short file " + sPath;
    }
    return bOk;
}

TEST_CASE_NAMED(FCrtTrigTest1, "Unit::CrtTrig::crt trig fallback equals the x86 crt", "[ProductFilter]")
{
    WatchdogArm("crt trig corpus");
    const char* const kPrefix = "crt reference child";
    std::string sExe;
    std::string sFile;
    std::string sWhy;
    if (!RunWin32Child("crt-trig-ref", kPrefix, sExe, sFile, sWhy))
    {
        FAIL(sWhy);
    }
    CrtTrigReference kChild;
    const bool bRead = CrtTrigReadFile(sFile, kChild, sWhy);
    DeleteFileA(sFile.c_str());
    if (!bRead)
    {
        FAIL(std::string(kPrefix) + ": " + sExe + ": " + sWhy);
    }
    printf("crt reference child %s ucrtbase %u.%u.%u.%u\n", sExe.c_str(), kChild.auVersion[0], kChild.auVersion[1],
           kChild.auVersion[2], kChild.auVersion[3]);

    const CrtTrigReference* pRef = &kChild;
    CrtTrigReference kSelf;
    if (sizeof(void*) == 4)
    {
        WatchdogArm("crt trig in-process");
        if (!CrtTrigInProcess(kSelf, sWhy))
        {
            FAIL("crt reference: " + sWhy);
        }
        printf("crt reference in-process ucrtbase %u.%u.%u.%u\n", kSelf.auVersion[0], kSelf.auVersion[1],
               kSelf.auVersion[2], kSelf.auVersion[3]);
        CHECK(memcmp(kSelf.auVersion, kChild.auVersion, sizeof(kSelf.auVersion)) == 0);
        REQUIRE(kSelf.vEntries.size() == kChild.vEntries.size());
        for (size_t e = 0; e < kSelf.vEntries.size(); ++e)
        {
            const std::string sEntry = kSelf.vEntries[e].uSet == CRT_SET_TRIG
                                           ? std::string(CrtTrigName(kSelf.vEntries[e].uFn)) + " " +
                                                 RouteName(kSelf.vEntries[e].uRoute)
                                           : std::string("sim ") + CrtSimName(kSelf.vEntries[e].uFn);
            INFO(sEntry);
            CHECK(kSelf.vEntries[e].uSet == kChild.vEntries[e].uSet);
            CHECK(kSelf.vEntries[e].uFn == kChild.vEntries[e].uFn);
            CHECK(kSelf.vEntries[e].uRoute == kChild.vEntries[e].uRoute);
            CHECK(kSelf.vEntries[e].vOut == kChild.vEntries[e].vOut);
        }
        pRef = &kSelf;
    }

    unsigned int uCompared = 0;
    std::vector<unsigned long long> vIn;
    for (unsigned int uFn = 0; uFn < CRT_TRIG_FNS; ++uFn)
    {
        WatchdogArm(CrtTrigName(uFn));
        CrtTrigInputs(uFn, vIn);
        std::vector<unsigned long long> vHelper(vIn.size());
        unsigned int auPaths[3] = {0, 0, 0};
        for (size_t i = 0; i < vIn.size(); ++i)
        {
            vHelper[i] = Bits(Helper(uFn, FromBits(vIn[i])));
            ++auPaths[CrtTrigPathOf(uFn, vIn[i])];
        }
        const std::vector<unsigned long long>* apRoute[CRT_TRIG_ROUTES] = {0, 0};
        for (unsigned int uRoute = 0; uRoute < CRT_TRIG_ROUTES; ++uRoute)
        {
            if (!CrtTrigHasRoute(uFn, uRoute))
            {
                continue;
            }
            const std::string sEntry = std::string(CrtTrigName(uFn)) + " " + RouteName(uRoute);
            INFO(sEntry);
            const CrtTrigEntry* pEntry = 0;
            for (const CrtTrigEntry& kEntry : pRef->vEntries)
            {
                if (kEntry.uSet == CRT_SET_TRIG && kEntry.uFn == uFn && kEntry.uRoute == uRoute)
                {
                    pEntry = &kEntry;
                }
            }
            if (!pEntry || pEntry->vOut.size() != vIn.size())
            {
                FAIL_CHECK(std::string(kPrefix) + ": " + sExe + ": no entry of " + std::to_string(vIn.size()) +
                           " results");
                continue;
            }
            apRoute[uRoute] = &pEntry->vOut;
            ++uCompared;
            unsigned int uDiffs = 0;
            std::string sFirst;
            for (size_t i = 0; i < vIn.size(); ++i)
            {
                const unsigned long long uWant = Value(uFn, vIn[i], FromBits(pEntry->vOut[i]));
                const unsigned long long uGot = Value(uFn, vIn[i], FromBits(vHelper[i]));
                if (uWant != uGot)
                {
                    if (uDiffs == 0)
                    {
                        char acFirst[160];
                        _snprintf(acFirst, sizeof(acFirst), "%s %s %s in %016llX x86 %016llX helper %016llX",
                                  CrtTrigName(uFn), RouteName(uRoute), PathName(CrtTrigPathOf(uFn, vIn[i])), vIn[i],
                                  uWant, uGot);
                        acFirst[sizeof(acFirst) - 1] = 0;
                        sFirst = acFirst;
                    }
                    ++uDiffs;
                }
            }
            printf("crt trig %s %s inputs=%u sse=%u tiny=%u x87=%u diffs=%u\n", CrtTrigName(uFn), RouteName(uRoute),
                   (unsigned int)vIn.size(), auPaths[CRT_TRIG_PATH_SSE], auPaths[CRT_TRIG_PATH_TINY],
                   auPaths[CRT_TRIG_PATH_X87], uDiffs);
            INFO(sFirst);
            CHECK(uDiffs == 0);
        }
        if (apRoute[CRT_TRIG_PRECISE] && apRoute[CRT_TRIG_PLAIN])
        {
            unsigned int uRouteDiffs = 0;
            for (size_t i = 0; i < vIn.size(); ++i)
            {
                if (CrtTrigPathOf(uFn, vIn[i]) == CRT_TRIG_PATH_X87 &&
                    (*apRoute[CRT_TRIG_PRECISE])[i] != (*apRoute[CRT_TRIG_PLAIN])[i])
                {
                    ++uRouteDiffs;
                }
            }
            const std::string sName = CrtTrigName(uFn);
            INFO(sName);
            CHECK(uRouteDiffs == 0);
        }
    }
    CHECK(uCompared == 5);
}

namespace
{
float SimStrict(unsigned int uTrig, float f)
{
    if constexpr (sizeof(void*) == 8)
    {
        return uTrig == CRT_TRIG_SIN ? PblCrtSinF(f) : PblCrtCosF(f);
    }
    else
    {
        return (float)CrtTrigPlain(uTrig, (double)f);
    }
}

float SimEval(unsigned int uFn, float fX, float fY)
{
    switch (uFn)
    {
    case CRT_SIM_SIN:
        return PblCrtSinF(fX);
    case CRT_SIM_COS:
        return PblCrtCosF(fX);
    case CRT_SIM_TAN:
        return PblCrtTanF(fX);
    case CRT_SIM_SIN_STRICT:
        return SimStrict(CRT_TRIG_SIN, fX);
    case CRT_SIM_COS_STRICT:
        return SimStrict(CRT_TRIG_COS, fX);
    case CRT_SIM_ASIN:
        return PblCrtAsinF(fX);
    case CRT_SIM_ACOS:
        return PblCrtAcosF(fX);
    case CRT_SIM_ATAN:
        return PblCrtAtanF(fX);
    case CRT_SIM_ATAN2:
        return PblCrtAtan2F(fX, fY);
    case CRT_SIM_EXP:
        return PblCrtExpF(fX);
    case CRT_SIM_LOG:
        return PblCrtLogF(fX);
    case CRT_SIM_LOG10:
        return PblCrtLog10F(fX);
    case CRT_SIM_POW:
        return PblCrtPowF(fX, fY);
    default:
        return PblCrtFmodF(fX, fY);
    }
}

bool ReadPinLines(const char* pcPath, std::vector<std::string>& vLines)
{
    vLines.clear();
    FILE* pFile = OpenRepoFile(pcPath);
    if (!pFile)
    {
        return false;
    }
    char acLine[256];
    while (fgets(acLine, sizeof(acLine), pFile))
    {
        size_t uLen = strlen(acLine);
        while (uLen > 0 && (acLine[uLen - 1] == '\n' || acLine[uLen - 1] == '\r'))
        {
            acLine[--uLen] = 0;
        }
        vLines.push_back(acLine);
    }
    fclose(pFile);
    return true;
}
}

TEST_CASE_NAMED(FCrtTrigTest2, "Unit::CrtTrig::sim math corpus", "[ProductFilter]")
{
    WatchdogArm("sim math corpus pins");
    const char* const pcPins = "tests\\corpus\\simmath\\crt.txt";
    std::vector<std::string> vLines;
    const bool bRead = ReadPinLines(pcPins, vLines);
    if (!bRead)
    {
        FAIL_CHECK(std::string("sim math corpus cannot read ") + pcPins);
    }
    std::vector<CrtSimBlock> vBlocks;
    CrtSimPinBlocks(SimEval, vBlocks);
    CHECK(vLines.size() == vBlocks.size());
    unsigned int uPinDiffs = 0;
    for (size_t i = 0; i < vBlocks.size(); ++i)
    {
        const std::string sPinned = i < vLines.size() ? vLines[i] : std::string("(missing)");
        if (sPinned != vBlocks[i].sLine)
        {
            ++uPinDiffs;
            FAIL_CHECK("sim math corpus pinned " + sPinned + " computed " + vBlocks[i].sLine + " " +
                       vBlocks[i].sRange);
        }
    }
    printf("sim math corpus pinned blocks=%u diffs=%u\n", (unsigned int)vBlocks.size(), uPinDiffs);

    const char* const kPrefix = "crt reference child";
    CrtTrigReference kRef;
    std::string sWhy;
    if constexpr (sizeof(void*) == 4)
    {
        WatchdogArm("sim math corpus in-process");
        if (!CrtTrigInProcess(kRef, sWhy))
        {
            FAIL("crt reference: " + sWhy);
        }
        printf("sim math corpus reference in-process ucrtbase %u.%u.%u.%u\n", kRef.auVersion[0], kRef.auVersion[1],
               kRef.auVersion[2], kRef.auVersion[3]);
    }
    else
    {
        WatchdogArm("sim math corpus child");
        std::string sExe;
        std::string sFile;
        if (!RunWin32Child("crt-trig-ref", kPrefix, sExe, sFile, sWhy))
        {
            FAIL(sWhy);
        }
        const bool bChild = CrtTrigReadFile(sFile, kRef, sWhy);
        DeleteFileA(sFile.c_str());
        if (!bChild)
        {
            FAIL(std::string(kPrefix) + ": " + sExe + ": " + sWhy);
        }
        printf("sim math corpus reference child %s ucrtbase %u.%u.%u.%u\n", sExe.c_str(), kRef.auVersion[0],
               kRef.auVersion[1], kRef.auVersion[2], kRef.auVersion[3]);
    }

    unsigned int uCompared = 0;
    std::vector<CrtSimIn> vPinned;
    std::vector<CrtSimIn> vX87;
    for (unsigned int uFn = 0; uFn < CRT_SIM_FNS; ++uFn)
    {
        if (!CrtSimHasX87(uFn))
        {
            continue;
        }
        WatchdogArm(CrtSimName(uFn));
        CrtSimSplit(uFn, vPinned, vX87);
        const CrtTrigEntry* pEntry = 0;
        for (const CrtTrigEntry& kEntry : kRef.vEntries)
        {
            if (kEntry.uSet == CRT_SET_SIM && kEntry.uFn == uFn)
            {
                pEntry = &kEntry;
            }
        }
        if (!pEntry || pEntry->vOut.size() != vX87.size())
        {
            FAIL_CHECK(std::string(kPrefix) + ": no " + CrtSimName(uFn) + " x87 entry of " +
                       std::to_string(vX87.size()) + " results");
            continue;
        }
        ++uCompared;
        unsigned int uDiffs = 0;
        for (size_t uFirst = 0, uBlock = 0; uFirst < vX87.size(); uFirst += CRT_SIM_BLOCK, ++uBlock)
        {
            const size_t uCount = vX87.size() - uFirst < CRT_SIM_BLOCK ? vX87.size() - uFirst : CRT_SIM_BLOCK;
            unsigned int uBlockDiffs = 0;
            std::string sFirst;
            for (size_t i = uFirst; i < uFirst + uCount; ++i)
            {
                float fX;
                float fY;
                memcpy(&fX, &vX87[i].uX, sizeof(fX));
                memcpy(&fY, &vX87[i].uY, sizeof(fY));
                const float fOut = SimEval(uFn, fX, fY);
                unsigned int uGot;
                memcpy(&uGot, &fOut, sizeof(uGot));
                const unsigned int uWant = (unsigned int)pEntry->vOut[i];
                if (uGot != uWant)
                {
                    if (uBlockDiffs == 0)
                    {
                        char acFirst[96];
                        _snprintf(acFirst, sizeof(acFirst), "first in %08X,%08X x86 %08X got %08X", vX87[i].uX,
                                  vX87[i].uY, uWant, uGot);
                        acFirst[sizeof(acFirst) - 1] = 0;
                        sFirst = acFirst;
                    }
                    ++uBlockDiffs;
                }
            }
            if (uBlockDiffs)
            {
                FAIL_CHECK(std::string("sim math corpus x87 ") + CrtSimName(uFn) + " block " + std::to_string(uBlock) +
                           " diffs " + std::to_string(uBlockDiffs) + " " + CrtSimRange(vX87, uFirst, uCount) + " " +
                           sFirst);
            }
            uDiffs += uBlockDiffs;
        }
        printf("sim math corpus %s pinned=%u x87=%u diffs=%u\n", CrtSimName(uFn), (unsigned int)vPinned.size(),
               (unsigned int)vX87.size(), uDiffs);
    }
    CHECK(uCompared == 7);
}

TEST_CASE_NAMED(FCrtTrigTest3, "Unit::CrtTrig::crt trig sets errno like the x86 crt", "[EngineFilter]")
{
    struct ErrnoCase
    {
        unsigned long long uIn;
        int iErrno;
    };
    const ErrnoCase kCases[] = {
        {0x7FF0000000000000ull, EDOM},  {0xFFF0000000000000ull, EDOM},  {0x7FF8000000000000ull, 0},
        {0xFFF8000000000001ull, 0},     {0x7FF0000000000001ull, 0},     {0x412E848000000000ull, 0},
        {0x43E158E460913D00ull, 0},     {0x40F6000000000000ull, 0},     {0x3FE0000000000000ull, 0},
        {0x0000000000000001ull, 0},     {0x0000000000000000ull, 0},     {0x7FEFFFFFFFFFFFFFull, 0},
    };
    double (*const kFns[])(double) = {PblCrtSin, PblCrtCos, PblCrtTan};
    const char* const kNames[] = {"sin", "cos", "tan"};
    for (unsigned int f = 0; f < 3; ++f)
    {
        for (const ErrnoCase& k : kCases)
        {
            errno = 0;
            const double dOut = kFns[f](FromBits(k.uIn));
            const int iErrno = errno;
            char ac[96];
            _snprintf(ac, sizeof(ac), "%s(%016llX) errno %d want %d out %016llX", kNames[f], k.uIn, iErrno, k.iErrno,
                      Bits(dOut));
            ac[sizeof(ac) - 1] = 0;
            CHECK_MESSAGE(iErrno == k.iErrno, ac);
        }
    }
    const unsigned int kFloats[] = {0x7F800000u, 0xFF800000u, 0x7FC00000u, 0x47B00000u, 0x3F000000u};
    const int kFloatErrno[] = {EDOM, EDOM, 0, 0, 0};
    float (*const kFloatFns[])(float) = {PblCrtSinF, PblCrtCosF, PblCrtTanF};
    for (unsigned int f = 0; f < 3; ++f)
    {
        for (unsigned int i = 0; i < 5; ++i)
        {
            errno = 0;
            kFloatFns[f](PblCrtFloatOfBits(kFloats[i]));
            const int iErrno = errno;
            char ac[96];
            _snprintf(ac, sizeof(ac), "%sf(%08X) errno %d want %d", kNames[f], kFloats[i], iErrno, kFloatErrno[i]);
            ac[sizeof(ac) - 1] = 0;
            CHECK_MESSAGE(iErrno == kFloatErrno[i], ac);
        }
    }
}
