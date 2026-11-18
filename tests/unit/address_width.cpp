#include "third_party/doctest/doctest.h"

#include <stdint.h>
#include <string.h>
#include <string>

#include "testutil.h"
#include "Battlefront2/Source/SoldierAnimator.h"
#include "RedEngineFL/Memory/RedAlign.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"

namespace
{

uintptr_t AlignedAt(uintptr_t uAddress, uintptr_t uAlign)
{
    return (uintptr_t)RedAlignUp((const void*)uAddress, uAlign);
}

std::string SourceSpan(const char* pcPath, const char* pcStart, const char* pcEnd)
{
    std::string sSource;
    REQUIRE(ReadRepoText(pcPath, sSource));
    const size_t uStart = sSource.find(pcStart);
    REQUIRE(uStart != std::string::npos);
    CHECK(sSource.find(pcStart, uStart + 1) == std::string::npos);
    const size_t uEnd = sSource.find(pcEnd, uStart);
    REQUIRE(uEnd != std::string::npos);
    return sSource.substr(uStart, uEnd - uStart);
}

}

TEST_CASE("align helper keeps the high address bits" * doctest::test_suite("unit"))
{
    const uintptr_t uHigh = (uintptr_t)1 << (sizeof(void*) * 8 - 4);
    CHECK(AlignedAt(uHigh, 16) == uHigh);
    CHECK(AlignedAt(uHigh + 1, 16) == uHigh + 16);
    CHECK(AlignedAt(uHigh + 15, 16) == uHigh + 16);
    CHECK(AlignedAt(uHigh + 16, 16) == uHigh + 16);
    CHECK(AlignedAt(uHigh + 0x41, 0x40) == uHigh + 0x80);
    CHECK(AlignedAt(uHigh + 0xFFF1, 0x10000) == uHigh + 0x10000);
    CHECK(AlignedAt(uHigh - 1, 0x40) == uHigh);
    CHECK(AlignedAt(uHigh - 0x40, 0x40) == uHigh - 0x40);
}

TEST_CASE("game memory aligns heap starts through the helper" * doctest::test_suite("regression"))
{
    const std::string sBody = SourceSpan("src\\Battlefront2\\Source\\GameMemory.cpp",
                                         "static inline char* AlignHeapStart(const void* pAddress)", "}");
    CHECK(sBody.find("return RedAlignUp(pAddress, GameMemory::kHeapStartAlignment);") != std::string::npos);
    CHECK(sBody.find("(unsigned int)") == std::string::npos);
}

TEST_CASE("connectivity graph aligns patches through the helper" * doctest::test_suite("regression"))
{
    const std::string sLine = SourceSpan("src\\Battlefront2\\Source\\ConnectivityGraph.cpp",
                                         "ConnectivityGraphPatch* pPatch = (ConnectivityGraphPatch*)", ";");
    CHECK(sLine.find("RedAlignUp(pData, 16)") != std::string::npos);
    CHECK(sLine.find("(unsigned int)") == std::string::npos);
}

TEST_CASE("soldier animator stores its owner as a pointer" * doctest::test_suite("regression"))
{
    std::string sHeader;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\SoldierAnimator.h", sHeader));
    CHECK(sHeader.find("    const EntitySoldier* mOwnerId;") != std::string::npos);
    CHECK(sHeader.find("unsigned int mOwnerId") == std::string::npos);

    const std::string sGetOwner = SourceSpan("src\\Battlefront2\\Source\\SoldierAnimator.h",
                                             "const EntitySoldier* GetOwner() const", "}");
    CHECK(sGetOwner.find("return mOwner ? mOwner : mOwnerId;") != std::string::npos);
    CHECK(sGetOwner.find("(const EntitySoldier*)") == std::string::npos);
    CHECK(sGetOwner.find("(unsigned int)") == std::string::npos);
}

TEST_CASE("soldier animator owner keeps its full address" * doctest::test_suite("regression"))
{
    alignas(SoldierAnimator) unsigned char acRaw[sizeof(SoldierAnimator)];
    memset(acRaw, 0, sizeof(acRaw));
    SoldierAnimator* pAnimator = reinterpret_cast<SoldierAnimator*>(acRaw);
    const EntitySoldier* pFar = (const EntitySoldier*)(((uintptr_t)1 << (sizeof(void*) * 8 - 4)) + 0x40);
    pAnimator->mOwnerId = pFar;
    CHECK(pAnimator->GetOwner() == pFar);
    const EntitySoldier* pNear = (const EntitySoldier*)(uintptr_t)0x80;
    pAnimator->mOwner = pNear;
    CHECK(pAnimator->GetOwner() == pNear);
}

TEST_CASE("heap ranges keep their full width" * doctest::test_suite("regression"))
{
    const size_t uSpan = ((size_t)1 << (sizeof(size_t) * 8 - 4)) + 0x1000;
    char* pcStart = (char*)(uintptr_t)0x10;
    char* pcEnd = (char*)((uintptr_t)0x10 + uSpan);
    int iFree = -1;
    for (int i = 0; i < kRedHeapSlots && iFree < 0; ++i)
    {
        if (!gRedHeapSlots[i].bLive)
        {
            iFree = i;
        }
    }
    REQUIRE(iFree >= 0);
    CHECK(RedMiCreate(iFree, pcStart, pcEnd, 0x10) == uSpan);
    RedMiDestroy(iFree);

    const int iH = RedCreateHeap(pcStart, pcEnd, 0x10, "T1Width");
    CHECK((iH >= 0) == (uSpan <= 0xFFFFFFFFu));
    if (iH >= 0)
    {
        CHECK(gRedHeapSlots[iH].size == uSpan);
        RedDestroyHeap(iH);
    }
}
