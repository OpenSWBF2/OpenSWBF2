#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stdint.h>
#include <string.h>
#include <string>
#include <windows.h>

#include "testutil.h"
#include "SoundFL/source/sndallocbitmap.h"
#include "SoundFL/source/sndfreelist.h"
#include "SoundFL/source/sndstreambase.h"

namespace
{

const SIZE_T kBlockBytes = 0x10000;
const SIZE_T kBlockSkew = 8;

char* spcBlock;
size_t suRequested;
void* spFreed;

void* __cdecl BlockAlloc(size_t uSize)
{
    if (spcBlock == 0 || suRequested != 0 || uSize > kBlockBytes - kBlockSkew)
    {
        return 0;
    }
    suRequested = uSize;
    return spcBlock + kBlockSkew;
}

void __cdecl BlockFree(void* p)
{
    spFreed = p;
}

struct TopDownBlock
{
    TopDownBlock()
    {
        suRequested = 0;
        spFreed = 0;
        spcBlock = (char*)VirtualAlloc(nullptr, kBlockBytes, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
    }

    ~TopDownBlock()
    {
        if (spcBlock)
        {
            VirtualFree(spcBlock, 0, MEM_RELEASE);
            spcBlock = 0;
        }
    }
};

class SegmentProbe : public Snd::StreamBase::Segment
{
public:
    SegmentProbe() : Snd::StreamBase::Segment(0, 0, 0, 0, 1, 0) {}
};

}

TEST_CASE_NAMED(FSoundWidthTest1, "Unit::SoundWidth::snd memory bitmap keeps a high start address", "[EngineFilter]")
{
    const uintptr_t uHigh = (uintptr_t)1 << (sizeof(void*) * 8 - 4);
    const uintptr_t uStart = uHigh + 0x80;
    const unsigned int uiMemory = 0x2000000;
    const unsigned int uiEntry = 0x80;
    const unsigned int uiSize = 0x100;
    static unsigned char sacBitmap[0x2000000 / 0x80 / 8];
    memset(sacBitmap, 0, sizeof(sacBitmap));

    Snd::MemoryBitmap kBitmap;
    kBitmap.Initialize(uStart, uiMemory, uiEntry, sacBitmap);

    Snd::MemoryBitmap::MemoryHandle kFirst;
    Snd::MemoryBitmap::MemoryHandle kSecond;
    REQUIRE(kBitmap.Alloc(uiSize, kFirst));
    REQUIRE(kBitmap.Alloc(uiSize, kSecond));

    const uintptr_t uFirst = kFirst.GetAddress(kBitmap);
    const uintptr_t uSecond = kSecond.GetAddress(kBitmap);
    CHECK(uFirst == uStart + (uintptr_t)kFirst.GetAllocatedEntryIndex() * uiEntry);
    CHECK(uSecond == uStart + (uintptr_t)kSecond.GetAllocatedEntryIndex() * uiEntry);
    CHECK((uFirst & ~(uintptr_t)0x3FFFFFF) == uHigh);
    CHECK((uSecond & ~(uintptr_t)0x3FFFFFF) == uHigh);
    CHECK(uFirst >= uStart);
    CHECK(uSecond >= uStart);
    CHECK(uFirst + uiSize <= uStart + uiMemory);
    CHECK(uSecond + uiSize <= uStart + uiMemory);
    CHECK(kFirst.GetSize(kBitmap) == uiSize);
    CHECK(kSecond.GetSize(kBitmap) == uiSize);
    CHECK((uSecond >= uFirst + uiSize || uFirst >= uSecond + uiSize));
}

TEST_CASE_NAMED(FSoundWidthTest2, "Unit::SoundWidth::snd free list carves aligned blocks above 4 GB", "[EngineFilter]")
{
    const unsigned int uiEntry = 96;
    const unsigned int uiAlign = 32;
    const unsigned int uiCount = 8;

    TopDownBlock kBlock;
    REQUIRE(spcBlock != (char*)0);
    CHECK((sizeof(void*) == 4 || (uint64_t)(uintptr_t)spcBlock > 0xFFFFFFFFull));

    char* const pcAlloc = spcBlock + kBlockSkew;
    Snd::FreeList kList(uiEntry, uiAlign, true, uiCount, BlockAlloc, BlockFree);
    char* apcEntry[uiCount] = {};
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        apcEntry[i] = (char*)kList.Alloc();
        REQUIRE(apcEntry[i] != (char*)0);
        CHECK((uintptr_t)apcEntry[i] % uiAlign == 0);
        CHECK(apcEntry[i] >= spcBlock);
        CHECK(apcEntry[i] + uiEntry <= spcBlock + kBlockBytes);
        CHECK(apcEntry[i] >= pcAlloc);
        CHECK(apcEntry[i] + uiEntry <= pcAlloc + suRequested);
        memset(apcEntry[i], (int)(i + 1), uiEntry);
    }
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        for (unsigned int j = 0; j < uiEntry; ++j)
        {
            if (apcEntry[i][j] != (char)(i + 1))
            {
                FAIL_CHECK("entry " << i << " byte " << j << " changed");
                break;
            }
        }
    }
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        kList.Free(apcEntry[i]);
    }
    CHECK(kList.mBlockList.IsEmpty());
    CHECK(spFreed == (void*)pcAlloc);
}

TEST_CASE_NAMED(FSoundWidthTest3, "Unit::SoundWidth::snd segment offset round-trips at full width", "[EngineFilter]")
{
    const unsigned int auiOffsets[] = { 0u, 0x7FFFFFFFu, 0x80000000u, 0xFFFFFFF0u };
    SegmentProbe kSegment;
    for (unsigned int i = 0; i < sizeof(auiOffsets) / sizeof(auiOffsets[0]); ++i)
    {
        const unsigned int uiOffset = auiOffsets[i];
        kSegment.SetOffset(uiOffset);
        CHECK(kSegment.GetOffset() == uiOffset);
        CHECK((uint64_t)(uintptr_t)kSegment.mAddress == (uint64_t)uiOffset);
    }
}

TEST_CASE_NAMED(FSoundWidthTest4, "Unit::SoundWidth::snd engine hands the sample RAM to the allocator at full width (source pin)", "[EngineFilter]")
{
    std::string sRaw;
    REQUIRE(ReadRepoText("src\\SoundFL\\source\\win\\sndengine.cpp", sRaw));
    std::string sSource;
    for (size_t i = 0; i < sRaw.size(); ++i)
    {
        if (sRaw[i] != '\r')
        {
            sSource += sRaw[i];
        }
    }
    const char* const pcCall = "EngineBase::smSoundRAMAllocator.Initialize(";
    const size_t uiStart = sSource.find(pcCall);
    REQUIRE(uiStart != std::string::npos);
    CHECK(sSource.find(pcCall, uiStart + 1) == std::string::npos);
    const size_t uiEnd = sSource.find(");", uiStart);
    REQUIRE(uiEnd != std::string::npos);
    const std::string sArgs = sSource.substr(uiStart + strlen(pcCall), uiEnd - uiStart - strlen(pcCall));
    CHECK(sArgs.find("(uintptr_t)smSampleRAM,") == 0);
    CHECK(sArgs.find("int)") == std::string::npos);
    CHECK(sArgs.find("DWORD") == std::string::npos);
    CHECK(sArgs.find("long") == std::string::npos);
    std::string sHeader;
    REQUIRE(ReadRepoText("src\\SoundFL\\source\\win\\sndengine.h", sHeader));
    CHECK(sHeader.find("static unsigned char* smSampleRAM;") != std::string::npos);
}
