#include "third_party/doctest/doctest.h"

#include <string.h>

#include "RedEngineFL/Util/ext/ListAllocator.h"

namespace
{

const int kLaPages = 8;
const unsigned int kLaPageBytes = 0x2000;
alignas(16) char sacLaPool[kLaPages * kLaPageBytes];
char* sapLaGiven[kLaPages];
int siLaGiven;
bool sbLaDescending;

class LaPages : public util::IAllocator
{
public:
    virtual void Allocate(util::Buffer& rBuf)
    {
        if (rBuf.size_ > kLaPageBytes || siLaGiven >= kLaPages)
        {
            rBuf.data_ = 0;
            return;
        }
        const int iSlot = sbLaDescending ? kLaPages - 1 - siLaGiven : siLaGiven;
        rBuf.data_ = sacLaPool + iSlot * kLaPageBytes;
        sapLaGiven[siLaGiven++] = (char*)rBuf.data_;
    }

    virtual void Deallocate(util::Buffer&)
    {
    }
};

struct LaPick
{
    int iPage;
    int iOffset;
};

LaPick LaLocate(void* p)
{
    LaPick kPick = { -1, -1 };
    for (int i = 0; i < siLaGiven; ++i)
    {
        if ((char*)p >= sapLaGiven[i] && (char*)p < sapLaGiven[i] + kLaPageBytes)
        {
            kPick.iPage = i;
            kPick.iOffset = (int)((char*)p - sapLaGiven[i]);
        }
    }
    return kPick;
}

void LaRun(bool bDescending, LaPick* pSmall, LaPick* pLarge)
{
    sbLaDescending = bDescending;
    siLaGiven = 0;
    LaPages kPages;
    util::ListAllocator* pAlloc = util::ListAllocator::Create(kLaPageBytes, kLaPages * kLaPageBytes, &kPages);
    REQUIRE(pAlloc);
    util::Buffer akBuf[3];
    for (int i = 0; i < 3; ++i)
    {
        akBuf[i] = util::Buffer(0, 0x1800);
        pAlloc->Allocate(akBuf[i]);
        REQUIRE(akBuf[i].data_);
    }
    CHECK(siLaGiven == 3);
    CHECK(LaLocate(akBuf[0].data_).iPage == 0);
    CHECK(LaLocate(akBuf[1].data_).iPage == 1);
    CHECK(LaLocate(akBuf[2].data_).iPage == 2);
    for (int i = 0; i < 3; ++i)
    {
        pAlloc->Deallocate(akBuf[i]);
    }
    util::Buffer kSmall(0, 0x400);
    pAlloc->Allocate(kSmall);
    REQUIRE(kSmall.data_);
    *pSmall = LaLocate(kSmall.data_);
    util::Buffer kLarge(0, 0x3000);
    pAlloc->Allocate(kLarge);
    *pLarge = LaLocate(kLarge.data_);
    if (kLarge.data_)
    {
        pAlloc->Deallocate(kLarge);
    }
    pAlloc->Deallocate(kSmall);
    delete pAlloc;
}

}

TEST_CASE("cloth list allocator picks the same block whatever order its pages have in memory" *
          doctest::test_suite("regression"))
{
    LaPick kAscSmall;
    LaPick kAscLarge;
    LaPick kDescSmall;
    LaPick kDescLarge;
    LaRun(false, &kAscSmall, &kAscLarge);
    LaRun(true, &kDescSmall, &kDescLarge);
    CHECK(kAscSmall.iPage == 0);
    CHECK(kDescSmall.iPage == kAscSmall.iPage);
    CHECK(kDescSmall.iOffset == kAscSmall.iOffset);
    CHECK(kAscLarge.iPage == -1);
    CHECK(kDescLarge.iPage == -1);
}

namespace
{

const unsigned int kLcPoolBytes = 0x100000;
alignas(16) char sacLcPool[kLcPoolBytes];
unsigned int suLcUsed;
bool sbLcExhausted;

class LcPages : public util::IAllocator
{
public:
    virtual void Allocate(util::Buffer& rBuf)
    {
        if (rBuf.size_ > kLcPoolBytes - suLcUsed)
        {
            sbLcExhausted = true;
            rBuf.data_ = 0;
            return;
        }
        rBuf.data_ = sacLcPool + suLcUsed;
        suLcUsed += (rBuf.size_ + 15) & ~15u;
    }

    virtual void Deallocate(util::Buffer&)
    {
    }
};

struct LcPattern
{
    unsigned int uMany;
    unsigned int uFixed;
};

struct LcRepeatRow
{
    unsigned int uMany;
    unsigned int uFixed;
    unsigned int uBase;
    unsigned int uCreateMax;
    unsigned int uCeiling;
    int iX86;
};

struct LcMissionRow
{
    const char* pcName;
    unsigned int uBase;
    unsigned int uCreateMax;
    unsigned int uCeiling;
    int iX86;
    const char* pcOps;
};

struct LcCloth
{
    int iPattern;
    util::Buffer akBuf[5];
};

const LcPattern kLcPatterns[] = {
    { 288, 48 },
    { 300, 60 },
    { 564, 216 },
    { 648, 216 },
    { 660, 192 },
    { 660, 216 },
    { 864, 324 },
    { 972, 216 },
    { 1452, 696 },
    { 1716, 312 },
    { 1824, 1104 },
    { 1836, 900 },
    { 2112, 432 },
    { 2280, 432 },
    { 2652, 180 },
    { 2652, 348 },
    { 2760, 1632 },
};

const LcRepeatRow kLcRepeatRows[] = {
    { 288, 48, 131072, 262144, 262144, 1008 },
    { 288, 48, 32768, 32768, 262144, 1007 },
    { 300, 60, 131072, 262144, 262144, 916 },
    { 300, 60, 32768, 32768, 262144, 913 },
    { 564, 216, 131072, 262144, 262144, 500 },
    { 564, 216, 32768, 32768, 262144, 498 },
    { 648, 216, 131072, 262144, 262144, 433 },
    { 648, 216, 32768, 32768, 262144, 431 },
    { 660, 192, 131072, 262144, 262144, 437 },
    { 660, 192, 32768, 32768, 262144, 433 },
    { 660, 216, 131072, 262144, 262144, 433 },
    { 660, 216, 32768, 32768, 262144, 431 },
    { 864, 324, 131072, 262144, 262144, 334 },
    { 864, 324, 32768, 32768, 262144, 332 },
    { 972, 216, 131072, 262144, 262144, 303 },
    { 972, 216, 32768, 32768, 262144, 301 },
    { 1452, 696, 131072, 262144, 262144, 195 },
    { 1452, 696, 32768, 32768, 262144, 195 },
    { 1716, 312, 131072, 262144, 262144, 168 },
    { 1716, 312, 32768, 32768, 262144, 162 },
    { 1824, 1104, 131072, 262144, 262144, 147 },
    { 1824, 1104, 32768, 32768, 262144, 143 },
    { 1836, 900, 131072, 262144, 262144, 148 },
    { 1836, 900, 32768, 32768, 262144, 143 },
    { 2112, 432, 131072, 262144, 262144, 132 },
    { 2112, 432, 32768, 32768, 262144, 122 },
    { 2280, 432, 131072, 262144, 262144, 126 },
    { 2280, 432, 32768, 32768, 262144, 121 },
    { 2652, 180, 131072, 262144, 262144, 118 },
    { 2652, 180, 32768, 32768, 262144, 117 },
    { 2652, 348, 131072, 262144, 262144, 116 },
    { 2652, 348, 32768, 32768, 262144, 116 },
    { 2760, 1632, 131072, 262144, 262144, 90 },
    { 2760, 1632, 32768, 32768, 262144, 82 },
    { 4064, 4064, 32768, 32768, 262144, 64 },
    { 4064, 4064, 131072, 262144, 262144, 64 },
    { 32, 32, 32768, 32768, 262144, 5442 },
    { 32, 32, 131072, 262144, 262144, 5450 },
    { 32736, 32736, 32768, 32768, 32768, 1 },
    { 131040, 131040, 131072, 163840, 163840, 1 },
};

const LcMissionRow kLcMissionRows[] = {
    { "cor1c_con", 131072, 262144, 262144, 518,
      "cfffccfFfFfFFCFfCFcffffFfFfcFFfFfFf" },
    { "cor1c_ctf", 32768, 32768, 262144, 701,
      "ppPpPp!cfffccPoPoCFOpPofFOpOpPofPoCOpOpPoOpPofPoOpPoCOpcPoOpFPoOpOpFPoPo"
      "fOpCFPoOpOpPoPoFOpcPoOpcfPoOpPoOpcPoOpPoOpPoOpPocOpCFCPoOpPofOpCPoOpOpPo"
      "fOpfPoOpPoPoOpfFPoOpPoOpPoOpfffOpFPoPoOpOpPoCPoOpcPoFOpPoFOpOpfCPofPoFcO"
      "pfOpF" },
    { "cor1g_ctf", 32768, 32768, 262144, 536,
      "ppPpPp!PoPoOpPoOpOpPoPoOpOpPoPoOpPoOpOpPoPoOpOpPoPoOpPoOpPoOpPoOpOpPoPoO"
      "pPoOpPoOpOpPoOpPoPoOpPoOpPoOpPoOpPoOpOpPoPoOpPoOpOpPoPoOpPoOpOpPoOpPoPoO"
      "pPoOpPoOpPoOpPoOpOpPoOpPoPo" },
    { "dag1c_con", 131072, 262144, 262144, 500,
      "cffffcFFFfffcFfFFffffFFfFFFf" },
    { "dag1c_ctf", 32768, 32768, 262144, 611,
      "ppPpPp!cffffcCFcPoPofFfFCccfOpPofCOpPoFFfOpcOpPoCPoFOpfcfPoOpPoOpPoFCCOp"
      "PoOpfcPoFFOpCFPoFOpOpffPoPocCOpPocFFfcCOpOpPoPoOpcPofOpOpPoOpPoOpPoPofFO"
      "pPoCFcfFOpOpPofPoFfCCOpPofOp" },
    { "dag1g_ctf", 32768, 32768, 262144, 286,
      "ppPpPp!PoPoOpPoOpPoOpPoOpOpPoPoOpPoOpPoOpPoOpOpPoPoOpOpPoPoOpPoOpOpPoOp" },
    { "dea1c_1flag", 32768, 32768, 262144, 570,
      "p!cfffffPocFOpPoOpPoOpPocOpfPoFCOpPoOpPoFfCFOpPoFOpCPoOpfPoFOpPoOpcPoOpP"
      "oOpPofOpPoOpPoOpPoOpfPoOpcfPoFfCFOpPofFOpPofCcOpPoOpPocFOpPoOpPoOpPoOpPo"
      "OpPoFFOpPoOpPoOpPoffCFF" },
    { "dea1c_con", 131072, 262144, 262144, 508,
      "cffffffcFFFFFffCfFfFffFfcFfFFffF" },
    { "dea1g_1flag", 32768, 32768, 262144, 351,
      "p!PoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPo"
      "OpPoOpPoOpPoOpPoOpPoOpPo" },
    { "end1g_1flag", 32768, 32768, 262144, 246,
      "p!PoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOp" },
    { "fel1c_1flag", 32768, 32768, 262144, 390,
      "p!ffffPoOpPoFOpfPoOpfPofOpFPoOpPoOpPoFOpPoFfFOpPoOpPoFOpPoFOpPoFfOpPofOp"
      "fPoOpPoOpfPoFFFfOpPoffOpPo" },
    { "fel1c_con", 131072, 262144, 262144, 483,
      "fffffFFfFFFFffffffFFFffF" },
    { "fel1g_1flag", 32768, 32768, 262144, 271,
      "p!PoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPo" },
    { "geo1c_con", 131072, 262144, 262144, 469,
      "feffFeeFffEFfFEfFf" },
    { "geo1c_ctf", 32768, 32768, 262144, 368,
      "ppPpPp!feefPoPoFOpPofEeeOpPoOpPoEOpOpfEPoPoOpePoEFOpPoOpPoOpPoOpPoOpOpPo"
      "PoOpEFPoOpPoOpfPoF" },
    { "geo1c_xl", 131072, 262144, 262144, 813,
      "feefffffffffeeeeeeeeEFfFfeFFfEEEfeFeEfeEEFfFeEeEfeeEeFFfFEefFfFfefEFEeEE"
      "FFEeeeefffEFeFfEEefEFfeeEFfeEFFEffeFFEfefeEFEefEeFFFEeEefefEefFFffFfEeFE"
      "fEFeeEFfeEfFEefEeeFFfEEFfefe" },
    { "hot1g_1flag", 32768, 32768, 262144, 596,
      "pd!ddddddddddPodOpdPoOpDDPodddDddDDdOpPoDdddDDDOpddPoDOpPoDdOpDPodOpPoDO"
      "pdPoDddDDOpDPoddDdDDOpddDPoDDdOpddPoDDDdddddDdDdDddDDDOpPodDdDddOpDPoOpP"
      "oDDDdDdD" },
    { "hot1g_c", 32768, 32768, 262144, 436,
      "d!D" },
    { "hot1g_con", 32768, 32768, 262144, 510,
      "d!dddddddddddddDDdDdDdDdDdddDdDDDdDddDDddDDdD" },
    { "hot1g_uber", 32768, 32768, 262144, 436,
      "d!D" },
    { "hot1g_xl", 32768, 32768, 262144, 1364,
      "d!ddddddddddddddddddddddddddddddddddddddddddddddddddddddddDdDDDdddDdDDdd"
      "DdDDDDdddDDDDDDDDdddddddddDdDddDDdDdDDDddddDDDDddDddDDDDDDdDddddddDDDddd"
      "DdDdDDDDddddDDDDdddddDDDDddDddDDDDDDdDdDddddDddddDDDdddDDddDDDdddDDDdddD"
      "DDdDddDDDDDDDDDddddDDddDDddddddddDDDDddddDDDDDDdddDddDDdDDdddddDDDdddDDD"
      "DDDddddddDdDDDDDDDDDDdDddddddDddDDdddDddDddDDddDDDdddDDDDDdddDdDDDDDdddd"
      "ddDDDDDDDDDddddddddddDdDDDDddddDdDDDDDDdddddDDDddddDDDDDddDddDDDDD" },
    { "kam1c_1flag", 32768, 32768, 262144, 328,
      "p!cffffPoFOpcPoCFfFfFfCOpcPoOpPofCFOpPoOpcfFFPoOpfPoFOpPoF" },
    { "kam1c_con", 131072, 262144, 262144, 495,
      "cffccfCfFfcFFffFFcF" },
    { "kam1g_1flag", 32768, 32768, 262144, 301,
      "p!PoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPo"
      "OpPo" },
    { "kas2c_con", 131072, 262144, 262144, 443,
      "ffffFF" },
    { "kas2c_ctf", 32768, 32768, 262144, 292,
      "ppPpPp!fffPoFFPofffOpOpFPoPoOpFfOpPoOpPoPoFfFffOpOpPoPoOp" },
    { "kas2c_uber", 131072, 262144, 262144, 458,
      "ffffffffFFFFF" },
    { "kas2c_xl", 131072, 262144, 262144, 1088,
      "ffffffffffffffffffffffffffffffffffffffffffffffffFFfFFFFFFffffFfffFFFFfFF"
      "fffFFFFFfffFFffFffFFffFffffFFFFFFffffFFffFfffFfFFfffFFFffFFffFfFFFFffFff"
      "FfFFFffffFFfFfFfFFFFffffffFFFFfffFFFFFFFffffFfffFFFfffFfFFFfffffFFFFFFFF"
      "FfFfFfffffffFFfFfFfFfFFfffffFFffFFfFfFFFffffFfFFFFffffFFFFFFffffffFFFfFf"
      "FFFfFfFFfFfffffFFfFff" },
    { "kas2g_ctf", 32768, 32768, 262144, 236,
      "ppPpPp!PoPoOpPoOpPoOpPoOpPoOpOpPoPoOpPoOpOpPoPoOpPo" },
    { "mus1c_con", 131072, 262144, 262144, 566,
      "cfffffcFfFCcFFFfffFcfffFFFCfffFFFFcFffffCFfFCFFcFffFf" },
    { "mus1c_ctf", 32768, 32768, 262144, 403,
      "ppPpPp!fffffPoPoFfFFFOpfPoFOpPoOpPofOpFPoOpFfFOpffPoPofOpPofOpPoFOpFPoFO"
      "pOpPoOpfFPofPoOpPoOpPoOpPoFOp" },
    { "mus1g_ctf", 32768, 32768, 262144, 286,
      "ppPpPp!PoOpPoPoOpPoOpOpPoPoOpPoOpPoOpPoOpPoOpOpPoPoOpPoOpPoOpPoOpPoOpOp" },
    { "myg1c_con", 131072, 262144, 262144, 509,
      "cffffFfCccfFFffFfcFFfFCCCFfcff" },
    { "myg1c_ctf", 32768, 32768, 262144, 451,
      "ppPpPp!cffffFPocPoffFFOpPoOpOpPoPoOpfFPoOpPoCfOpFPoOpPoOpPoOpOpPofCOpPoP"
      "oOpccOpfFPoOpPoOpFPoPoCfCFOpOpFPoPoOpcPofOpPoOpf" },
    { "myg1g_ctf", 32768, 32768, 262144, 396,
      "ppPpPp!PoPoOpPoOpPoOpPoOpPoOpPoOpOpPoPoOpPoOpPoOpOpPoOpPoOpPoPoOpPoOpOpP"
      "oOpPoOpPoPoOpPoOpPoOpPoOpOpPoPoOpOpPoOpPoOp" },
    { "nab2c_con", 131072, 262144, 262144, 536,
      "cffccCfffCFFFcFcfcCfcfCCFffCCfFcCcFfF" },
    { "nab2c_ctf", 32768, 32768, 262144, 331,
      "ppPoPo!cffccfffCCFFcFcffFCffcFFFCcFf" },
    { "nab2g_ctf", 32768, 32768, 262144, 276,
      "ppPpPp!PoPoOpPoOpOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoPoOpPoOpPoOpPoOpOp" },
    { "pol1c_con", 131072, 262144, 262144, 542,
      "cffccffFFfCfCfFfFFFccFffCfFffcCfFFcFFFfF" },
    { "pol1c_ctf", 32768, 32768, 262144, 478,
      "ppPpPp!cffccPoOpPoOpPoPofOpFPoCfFOpOpCPoPocOpCOpPoPoOpPofOpPofFOpcOpfPoP"
      "oOpOpPoOpfPoPoFFfOpPoCFFffOpfOpFCPoOpPoPoOpPoOpfcPoFOpOpPofPo" },
    { "pol1g_ctf", 32768, 32768, 262144, 346,
      "ppPpPp!PoOpPoOpPoOpPoPoOpPoOpPoOpPoOpOpPoPoOpPoOpPoOpOpPoOpPoPoOpOpPoPoO"
      "pOpPoOpPoPoOpPoOpPoOpOp" },
    { "tan1c_1flag", 32768, 32768, 262144, 400,
      "p!cfffffPoOpCPoOpcPocFFFOpPoFOpPoffCfOpPoFOpPoOpPocFOpPoffOpPoCFCffcOpPo"
      "FfcFFCOpPocFfFFffOpPoC" },
    { "tan1c_con", 131072, 262144, 262144, 540,
      "cfffffFcFFffFfFfCFFcFffFfFfFfFfFfCfFfFFFccfff" },
    { "tan1g_1flag", 32768, 32768, 262144, 261,
      "p!PoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPo" },
    { "tat2c_con", 131072, 262144, 262144, 488,
      "cffffccFCCCFF" },
    { "tat2c_ctf", 32768, 32768, 262144, 362,
      "ppPpPp!cfffPofPoFcFOpPofCOpFfPocFOpfPofOpFPoOpCfPocFOpFPoOpCPofffOpcPoFO"
      "pPoccfC" },
    { "tat2g_ctf", 32768, 32768, 262144, 271,
      "ppPpPp!PoPoOpPoOpPoOpOpPoPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOp" },
    { "tat2g_eli", 131072, 262144, 262144, 1847,
      "qjmlmnagibbkhLJjlIiMLmnjlglihBBAHhabbQqMmLKIGkgilANanQHIqiHhLANManmlhJMm"
      "jNIniKBBGbbQkLJqjlgMMGmmgHhNIniLMABBalmQJqjMbbJKmjkNGGNIngignJjKGkgAaJjB"
      "BbbHQLMJJqjmlhMmjAGagIBBbbiNMABBanbbmMmIJKJkjjGiNMBBmnbbgAQqaMLKklmMmNHJ"
      "GBBHbbnjghhGgLMIQAqlmiLKlIaGBBQqgbbikHMmhMmKGNAJQqnjagkLNlJMnImjiHQBBKqh"
      "bbNLNknnlMAHmhaBBQLJqlbbjMJmjHHKhkhGLMlmgNGngBBbbGNnGgLQqlgJLMNKmnkjlMIm"
      "iINAQqaniGKkgBBLHGbblghMmNLNIlnniKAkMJGQmjqgaIiGgMAmaGKkgIiNMQBBmqbbnKJM"
      "mjkQGMJNmgqjnLGIIQgqliiMLNmlnHKkhGgBBNMJmnbbIGgjiLAalHIGihgBBIIHMmibbihN"
      "AanKQqkHMGmghLLllIMmiGJIijgGgMKIkimABBQqabbMHmhIHAiahLJLMmljlKIkiMmMJGGm"
      "ggjIBBHNibbhnHLAahlMBBmbbJAQqajNIiHnGKNMLkhmgnlMLmlJIAiajMGGKkgmgNnQJjqL"
      "JHMMHKmhkhjmlBBQqbbAKGagkHhMILImiil" },
    { "tat3c_1flag", 32768, 32768, 262144, 406,
      "p!cfffffPocFcfOpfFPoCFOpFPoOpPocOpPoOpCfCFfPofccFOpPofOpPoFCfOpPocfFFCFc"
      "fOpFPoOpCPoFFOpCPo" },
    { "tat3c_con", 131072, 262144, 262144, 561,
      "cfffffcFFFfFffFffFffCcFFFcFfFffCfCFFffFCcfFFcFffFFf" },
    { "tat3g_1flag", 32768, 32768, 262144, 266,
      "p!PoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOp" },
    { "uta1c_1flag", 32768, 32768, 262144, 541,
      "p!cffffPocOpPocCOpPoOpPoCfOpPoFcOpfFCFPoOpcPoFfCcOpFPoOpffPoFOpcPoOpPoCf"
      "OpFPoOpPoOpCPoOpPoOpPofCcOpPocFOpFcPoCffOpcffPoCOpPoFOpCPoOpPoFFCfcOpPoF"
      "fFc" },
    { "uta1c_con", 131072, 262144, 262144, 547,
      "cffccfCcCcCffccFFCfCcffCfcFFFFfFffFFfFFC" },
    { "uta1g_1flag", 32768, 32768, 262144, 261,
      "p!PoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPo" },
    { "yav1c_1flag", 32768, 32768, 262144, 472,
      "p!cffffPocOpfPoFOpPoFOpPoFCCOpPofOpFPofcOpPocOpPofcFOpfPoCcfFFOpPoCCfFOp"
      "PofOpFPofOpPoFOpCPoFfOpFcPoFffCOpPoOpPofcOpPofFcF" },
    { "yav1c_con", 131072, 262144, 262144, 521,
      "cffccfCCfffcFCcFFcCfFffFCffCcFFffFff" },
    { "yav1g_1flag", 32768, 32768, 262144, 361,
      "p!PoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPoOpPo"
      "OpPoOpPoOpPoOpPoOpPoOpPoOpPo" },
};

util::ListAllocator* LcCreate(LcPages& rPages, unsigned int uBase, unsigned int uCreateMax)
{
    suLcUsed = 0;
    sbLcExhausted = false;
    return util::ListAllocator::Create(uBase, uCreateMax, &rPages);
}

bool LcInit(util::ListAllocator* pAlloc, int iPattern, LcCloth* pCloth, int* piCount)
{
    pCloth->iPattern = iPattern;
    for (int i = 0; i < 5; ++i)
    {
        pCloth->akBuf[i] = util::Buffer(0, i < 4 ? kLcPatterns[iPattern].uMany : kLcPatterns[iPattern].uFixed);
        pAlloc->Allocate(pCloth->akBuf[i]);
        if (!pCloth->akBuf[i].data_)
        {
            return false;
        }
        ++*piCount;
    }
    return true;
}

int LcRepeatCount(const LcRepeatRow& rRow)
{
    LcPages kPages;
    util::ListAllocator* pAlloc = LcCreate(kPages, rRow.uBase, rRow.uCreateMax);
    pAlloc->SetMaxMem((int)rRow.uCeiling);
    int iCount = 0;
    for (int i = 0; iCount < 100000; ++i)
    {
        util::Buffer kBuf(0, i % 5 < 4 ? rRow.uMany : rRow.uFixed);
        pAlloc->Allocate(kBuf);
        if (!kBuf.data_)
        {
            break;
        }
        ++iCount;
    }
    delete pAlloc;
    return iCount;
}

int LcMissionCount(const LcMissionRow& rRow)
{
    static LcCloth sakLive[512];
    static int saiInits[1024];
    int iLive = 0;
    int iInits = 0;
    int iCount = 0;
    bool bRefused = false;
    LcPages kPages;
    util::ListAllocator* pAlloc = LcCreate(kPages, rRow.uBase, rRow.uCreateMax);
    for (const char* pc = rRow.pcOps; *pc && !bRefused; ++pc)
    {
        if (*pc == '!')
        {
            pAlloc->SetMaxMem((int)rRow.uCeiling);
        }
        else if (*pc >= 'a' && *pc <= 'z')
        {
            const int iPattern = *pc - 'a';
            REQUIRE(iInits < 1024);
            REQUIRE(iLive < 512);
            saiInits[iInits++] = iPattern;
            bRefused = !LcInit(pAlloc, iPattern, &sakLive[iLive], &iCount);
            if (!bRefused)
            {
                ++iLive;
            }
        }
        else
        {
            const int iPattern = *pc - 'A';
            for (int i = 0; i < iLive; ++i)
            {
                if (sakLive[i].iPattern == iPattern)
                {
                    for (int k = 0; k < 5; ++k)
                    {
                        pAlloc->Deallocate(sakLive[i].akBuf[k]);
                    }
                    for (int j = i + 1; j < iLive; ++j)
                    {
                        sakLive[j - 1] = sakLive[j];
                    }
                    --iLive;
                    break;
                }
            }
        }
    }
    for (int i = 0; !bRefused && iCount < 100000; ++i)
    {
        LcCloth kCloth;
        bRefused = !LcInit(pAlloc, saiInits[i % iInits], &kCloth, &iCount);
    }
    delete pAlloc;
    return iCount;
}

}

TEST_CASE("cloth list allocator holds as many allocations at its ceiling on x64 as on x86" *
          doctest::test_suite("regression"))
{
    for (unsigned int i = 0; i < sizeof(kLcRepeatRows) / sizeof(kLcRepeatRows[0]); ++i)
    {
        const LcRepeatRow& rRow = kLcRepeatRows[i];
        const int iCount = LcRepeatCount(rRow);
        INFO("pattern " << rRow.uMany << "," << rRow.uFixed << " base " << rRow.uBase << " ceiling " << rRow.uCeiling
                        << " count " << iCount << " x86 " << rRow.iX86);
        CHECK_FALSE(sbLcExhausted);
        CHECK(iCount >= rRow.iX86);
        if (sizeof(void*) == 4)
        {
            CHECK(iCount == rRow.iX86);
        }
    }
    for (unsigned int i = 0; i < sizeof(kLcMissionRows) / sizeof(kLcMissionRows[0]); ++i)
    {
        const LcMissionRow& rRow = kLcMissionRows[i];
        const int iCount = LcMissionCount(rRow);
        INFO("mission " << rRow.pcName << " count " << iCount << " x86 " << rRow.iX86);
        CHECK_FALSE(sbLcExhausted);
        CHECK(iCount >= rRow.iX86);
        if (sizeof(void*) == 4)
        {
            CHECK(iCount == rRow.iX86);
        }
    }
}
