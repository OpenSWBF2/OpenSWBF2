#include "third_party/doctest/doctest.h"

#include "unit/d3dx_corpus.h"

#include "testutil.h"

TEST_CASE("d3dx port matches pinned x86 bits" * doctest::test_suite("corpus"))
{
    WatchdogArm("d3dx pinned corpus");
    unsigned int uCount = 0;
    const D3dxCorpusFn* pFns = D3dxCorpusFns(uCount);
    unsigned int uPinned = 0;
    for (unsigned int f = 0; f < uCount; ++f)
    {
        const D3dxCorpusFn& kFn = pFns[f];
        if (!kFn.bPinned || !D3dxCorpusHasPort(kFn))
        {
            continue;
        }
        INFO(kFn.pcName);
        ++uPinned;
        unsigned int uCases = 0;
        unsigned int uPin = 0;
        REQUIRE(D3dxReadPin(kFn.pcName, uCases, uPin));
        CHECK(uCases == D3DX_CORPUS_CASES);
        const unsigned int uHash = D3dxCorpusHash(kFn, kFn.pfnPort);
        printf("d3dx pin %s port=%08X pin=%08X\n", kFn.pcName, uHash, uPin);
        CHECK(uHash == uPin);
    }
    CHECK(uPinned == 5);
}

namespace
{
std::string Hex(const unsigned int* puWords, unsigned int uCount)
{
    std::string s;
    char acWord[12];
    for (unsigned int i = 0; i < uCount; ++i)
    {
        _snprintf(acWord, sizeof(acWord), i ? " %08X" : "%08X", puWords[i]);
        acWord[sizeof(acWord) - 1] = 0;
        s += acWord;
    }
    return s;
}

struct ReferenceScope
{
    ~ReferenceScope()
    {
        D3dxReferenceClose();
    }
};
}

TEST_CASE("d3dx port equals the x86 d3dx9 dll" * doctest::test_suite("corpus"))
{
    WatchdogArm("d3dx dll corpus");
    ReferenceScope kScope;
    std::string sWhy;
    if (!D3dxReferenceOpen(sWhy))
    {
        FAIL(sWhy);
    }
    printf("d3dx reference %s\n", D3dxReferenceSource().c_str());
    unsigned int uCount = 0;
    const D3dxCorpusFn* pFns = D3dxCorpusFns(uCount);
    unsigned int uCompared = 0;
    for (unsigned int f = 0; f < uCount; ++f)
    {
        const D3dxCorpusFn& kFn = pFns[f];
        INFO(kFn.pcName);
        if (!D3dxCorpusHasPort(kFn))
        {
            if (sizeof(void*) == 4 && kFn.bX64PortOnly && kFn.pcNoPort)
            {
                printf("[N/A] %s - %s\n", kFn.pcName, kFn.pcNoPort);
            }
            else
            {
                FAIL_CHECK(std::string("no port evaluator for ") + kFn.pcName);
            }
            continue;
        }
        WatchdogArm(kFn.pcName);
        ++uCompared;
        std::vector<unsigned int> in((size_t)D3DX_CORPUS_BLOCK * kFn.uInWords);
        std::vector<unsigned int> outPort(kFn.uOutWords);
        std::vector<unsigned int> outRef;
        unsigned int uDiffs = 0;
        unsigned int uRefHash = 2166136261u;
        std::string sFirst;
        for (unsigned int uFirst = 0; uFirst < D3DX_CORPUS_CASES; uFirst += D3DX_CORPUS_BLOCK)
        {
            if (!D3dxReferenceBlock(f, uFirst / D3DX_CORPUS_BLOCK, outRef, sWhy))
            {
                FAIL(sWhy);
            }
            D3dxCorpusInputs(kFn, uFirst, D3DX_CORPUS_BLOCK, &in[0]);
            for (unsigned int c = 0; c < D3DX_CORPUS_BLOCK; ++c)
            {
                const unsigned int* puIn = &in[(size_t)c * kFn.uInWords];
                const unsigned int* puRef = &outRef[(size_t)c * kFn.uOutWords];
                const unsigned int uVariant = D3dxCorpusVariant(kFn, uFirst + c);
                kFn.pfnPort(puIn, uVariant, &outPort[0]);
                uRefHash = D3dxFnv1a(uRefHash, puRef, kFn.uOutWords);
                if (memcmp(&outPort[0], puRef, kFn.uOutWords * sizeof(unsigned int)) != 0)
                {
                    if (uDiffs == 0)
                    {
                        char acHead[64];
                        _snprintf(acHead, sizeof(acHead), "case %u variant %u in ", uFirst + c, uVariant);
                        acHead[sizeof(acHead) - 1] = 0;
                        sFirst = std::string(acHead) + Hex(puIn, kFn.uInWords) + " dll " +
                                 Hex(puRef, kFn.uOutWords) + " port " + Hex(&outPort[0], kFn.uOutWords);
                    }
                    ++uDiffs;
                }
            }
        }
        printf("d3dx dll %s diffs=%u\n", kFn.pcName, uDiffs);
        INFO(sFirst);
        CHECK(uDiffs == 0);
        if (kFn.bPinned)
        {
            unsigned int uCases = 0;
            unsigned int uPin = 0;
            REQUIRE(D3dxReadPin(kFn.pcName, uCases, uPin));
            CHECK(uCases == D3DX_CORPUS_CASES);
            CHECK(uRefHash == uPin);
        }
    }
    CHECK(uCompared > 0);
}
