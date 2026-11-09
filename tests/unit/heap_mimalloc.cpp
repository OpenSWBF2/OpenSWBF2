#include "third_party/doctest/doctest.h"

#include <algorithm>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include <windows.h>
#include <psapi.h>

#include "testutil.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryBackend.h"
#include "PebbleFL/Common/PblHandle.h"

extern int gCheckCalls;

namespace
{

alignas(16) char sacMiRange[0x10000];

int CreateMiHeap(unsigned int uAlign, const char* pcName)
{
    const RedHeapBackend* pSaved = gRedHeapBackend;
    gRedHeapBackend = &kRedMiBackend;
    const int iHeap = RedCreateHeap(sacMiRange, sacMiRange + sizeof(sacMiRange), uAlign, pcName);
    gRedHeapBackend = pSaved;
    return iHeap;
}

class MiHeap
{
public:
    MiHeap(unsigned int uAlign, const char* pcName) : miHeap(CreateMiHeap(uAlign, pcName))
    {
    }

    ~MiHeap()
    {
        Destroy();
    }

    void Destroy()
    {
        if (miHeap >= 0)
        {
            RedDestroyHeap(miHeap);
            miHeap = -1;
        }
    }

    int miHeap;
};

unsigned int MiNeed(unsigned int uSize, unsigned int uAlign)
{
    const unsigned int uHdr = (uAlign + 12 - 1) & ~(uAlign - 1);
    return ((uSize ? uSize : 1) + uAlign - 1 + uHdr) & ~(uAlign - 1);
}

PBL_ASAN_NO_SANITIZE unsigned int ReadSlotGen(const void* pUser)
{
    const volatile unsigned int* puWords = (const volatile unsigned int*)pUser;
    return puWords[-2];
}

PBL_ASAN_NO_SANITIZE void WriteSlotGen(void* pUser, unsigned int uSlotGen)
{
    volatile unsigned int* puWords = (volatile unsigned int*)pUser;
    puWords[-2] = uSlotGen;
}

PBL_ASAN_NO_SANITIZE unsigned int ReadPrefixWord(const void* pUser, int iWord)
{
    return ((const volatile unsigned int*)pUser)[iWord];
}

PBL_ASAN_NO_SANITIZE void WritePrefixWord(void* pUser, int iWord, unsigned int uValue)
{
    ((volatile unsigned int*)pUser)[iWord] = uValue;
}

void CollectLine(const char* pcLine, int iLen, void* pArg)
{
    ((std::vector<std::string>*)pArg)->push_back(std::string(pcLine, iLen));
}

struct StaleProbe : PblHandled
{
};

int siT1SoundHeap;

void* T1SndAlloc(size_t uiSize)
{
    return RedAllocFromHeap(siT1SoundHeap, uiSize, 0);
}

void T1SndFree(void* pBlock)
{
    RedFreeToHeap(siT1SoundHeap, pBlock, 0);
}

class ScopedMiBackend
{
public:
    explicit ScopedMiBackend(long lPurgeMs) : mpSaved(gRedHeapBackend)
    {
        RedMiConfigure(lPurgeMs);
        gRedHeapBackend = &kRedMiBackend;
    }

    ~ScopedMiBackend()
    {
        gRedHeapBackend = mpSaved;
        RedMiConfigure(1000);
    }

private:
    const RedHeapBackend* mpSaved;
};

int siT1TrimCalls;

void T1CountTrim()
{
    ++siT1TrimCalls;
}

unsigned int T1CommitBytes()
{
    PROCESS_MEMORY_COUNTERS kCounters = {};
    K32GetProcessMemoryInfo(GetCurrentProcess(), &kCounters, sizeof(kCounters));
    return (unsigned int)kCounters.PagefileUsage;
}

}

TEST_CASE("mimalloc heap blocks are zero even when reused" * doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    MiHeap kHeap(0x10, "T1MiZero");
    const int iH = kHeap.miHeap;
    REQUIRE(iH > 0);
    REQUIRE(gRedHeapSlots[iH].pBackend == &kRedMiBackend);
    unsigned int auSize[64];
    unsigned char* apBlock[64];
    for (int i = 0; i < 64; ++i)
    {
        auSize[i] = 13 + (unsigned int)i * 187 / 63;
        apBlock[i] = (unsigned char*)RedAllocFromHeap(iH, auSize[i], false);
        REQUIRE(apBlock[i]);
        memset(apBlock[i], 0xFF, auSize[i]);
    }
    for (int i = 0; i < 64; ++i)
    {
        RedFreeToHeap(iH, apBlock[i], 0);
    }
    CHECK(gRedHeapSlots[iH].used == 0);
    int iDirty = 0;
    for (int i = 0; i < 64; ++i)
    {
        apBlock[i] = (unsigned char*)RedAllocFromHeap(iH, auSize[i], false);
        REQUIRE(apBlock[i]);
        for (unsigned int b = 0; b < auSize[i]; ++b)
        {
            if (apBlock[i][b] != 0)
            {
                ++iDirty;
            }
        }
    }
    CHECK(iDirty == 0);
    for (int i = 0; i < 64; ++i)
    {
        RedFreeToHeap(iH, apBlock[i], 0);
    }
    CHECK(gRedHeapSlots[iH].used == 0);
    CHECK(gCheckCalls == iChecks);
}

TEST_CASE("mimalloc heap frees a crt pointer and a mimalloc pointer each to their owner" *
          doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    MiHeap kHeap(0x10, "T1MiOwner");
    const int iH = kHeap.miHeap;
    REQUIRE(iH > 0);
    void* pCrt = malloc(32);
    REQUIRE(pCrt);
    CHECK(RedHeapOwner(pCrt) == -1);
    long alBefore[kRedHeapSlots];
    long alAfter[kRedHeapSlots];
    for (int i = 0; i < kRedHeapSlots; ++i)
    {
        alBefore[i] = gRedHeapSlots[i].used;
    }
    RedFreeToHeap(0, pCrt, 0);
    for (int i = 0; i < kRedHeapSlots; ++i)
    {
        alAfter[i] = gRedHeapSlots[i].used;
    }
    for (int i = 0; i < kRedHeapSlots; ++i)
    {
        CAPTURE(i);
        CHECK(alAfter[i] == alBefore[i]);
    }
    void* p = RedAllocFromHeap(iH, 100, false);
    REQUIRE(p);
    const long lMiUsed = gRedHeapSlots[iH].used;
    const long lZeroBefore = gRedHeapSlots[0].used;
    RedFreeToHeap(0, p, 0);
    const long lZeroAfter = gRedHeapSlots[0].used;
    const long lMiAfter = gRedHeapSlots[iH].used;
    CHECK(lMiUsed == (long)MiNeed(100, 0x10));
    CHECK(lMiAfter == 0);
    CHECK(lZeroAfter == lZeroBefore);
    CHECK(gCheckCalls == iChecks);
}

TEST_CASE("mimalloc heap owner maps interior pointers to the slot" * doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    MiHeap kHeap(0x10, "T1MiInterior");
    const int iH = kHeap.miHeap;
    REQUIRE(iH > 0);
    char* p = (char*)RedAllocFromHeap(iH, 100, false);
    REQUIRE(p);
    CHECK(RedHeapOwner(p) == iH);
    CHECK(RedHeapOwner(p + 99) == iH);
    CHECK(RedHeapOwner(p + 8) == iH);
    RedFreeToHeap(iH, p, 0);
    CHECK(gCheckCalls == iChecks);
}

TEST_CASE("mimalloc heap free classifier sorts valid stale foreign and bad prefix blocks" *
          doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    {
        MiHeap kLive(0x10, "T1MiClassLive");
        const int iH = kLive.miHeap;
        REQUIRE(iH > 0);
        void* p = RedAllocFromHeap(iH, 48, false);
        REQUIRE(p);
        CHECK(RedMiClassify(p) == kRedMiValid);
        void* pCrt = malloc(48);
        REQUIRE(pCrt);
        CHECK(RedHeapOwner(pCrt) == -1);
        CHECK(RedMiClassify(pCrt) == kRedMiBadPrefix);
        free(pCrt);
        const unsigned int uSlotGen = ReadSlotGen(p);
        CHECK((uSlotGen & 0xFFu) == (unsigned int)iH);
        const unsigned int uPrevGen = ((uSlotGen >> 8) - 1) & 0xFFFFFFu;
        WriteSlotGen(p, (uSlotGen & 0xFFu) | uPrevGen << 8);
        const RedMiBlockKind eBad = RedMiClassify(p);
        WriteSlotGen(p, uSlotGen);
        CHECK(eBad == kRedMiBadPrefix);
        CHECK(RedMiClassify(p) == kRedMiValid);
        RedFreeToHeap(iH, p, 0);
        CHECK(gRedHeapSlots[iH].used == 0);
    }
    MiHeap kGone(0x10, "T1MiClassGone");
    const int iGone = kGone.miHeap;
    REQUIRE(iGone > 0);
    void* pStale = RedAllocFromHeap(iGone, 48, false);
    REQUIRE(pStale);
    kGone.Destroy();
    CHECK(RedHeapOwner(pStale) == kRedOwnerStale);
    CHECK(RedMiClassify(pStale) == kRedMiBadPrefix);
    CHECK(gCheckCalls == iChecks);
}

TEST_CASE("mimalloc free of an unreadable block reports a bad prefix without touching it" *
          doctest::test_suite("regression"))
{
    MiHeap kHeap(0x10, "T1MiNoRead");
    const int iH = kHeap.miHeap;
    REQUIRE(iH > 0);
    char* pRegion = (char*)VirtualAlloc(0, 0x10000, MEM_RESERVE, PAGE_NOACCESS);
    REQUIRE(pRegion);
    CHECK(RedMiClassify(pRegion + 64) == kRedMiBadPrefix);
    const int iChecks = gCheckCalls;
    {
        ExpectLog kLog("OPENSWBF2 MIERROR kind=badprefix");
        CHECK(kRedMiBackend.free(iH, pRegion + 64) == 0);
    }
    CHECK(gCheckCalls == iChecks + 1);
    VirtualFree(pRegion, 0, MEM_RELEASE);
}

TEST_CASE("mimalloc heap cookie covers the size and the charge words" * doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    MiHeap kHeap(0x10, "T1MiCookie");
    const int iH = kHeap.miHeap;
    REQUIRE(iH > 0);
    void* p = RedAllocFromHeap(iH, 48, false);
    REQUIRE(p);
    CHECK(RedMiClassify(p) == kRedMiValid);
    for (int iWord = -4; iWord <= -3; ++iWord)
    {
        CAPTURE(iWord);
        const unsigned int uSaved = ReadPrefixWord(p, iWord);
        WritePrefixWord(p, iWord, uSaved ^ 0x10u);
        const RedMiBlockKind eKind = RedMiClassify(p);
        WritePrefixWord(p, iWord, uSaved);
        CHECK(eKind == kRedMiBadPrefix);
        CHECK(RedMiClassify(p) == kRedMiValid);
    }
    RedFreeToHeap(iH, p, 0);
    CHECK(gRedHeapSlots[iH].used == 0);
    CHECK(gCheckCalls == iChecks);
}

TEST_CASE("heap purge drops a handle whose block was freed on a released page" * doctest::test_suite("regression"))
{
    const unsigned int uBaseStale = RedCountStaleHandles();
    MiHeap kHeap(0x10, "T1MiPurgeStale");
    const int iH = kHeap.miHeap;
    REQUIRE(iH > 0);
    char* pRegion = (char*)VirtualAlloc(0, 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    REQUIRE(pRegion);
    RedMiRecordReservation(pRegion);
    StaleProbe* pKey = new (pRegion + 64) StaleProbe;
    const unsigned int uId = pKey->GetHandleId();
    REQUIRE(PblHandleRegistry::IsLive(pKey, uId));
    REQUIRE(RedHeapOwner(pKey) == kRedOwnerStale);
    CHECK(RedCountStaleHandles() == uBaseStale + 1);
    RedPurgeHeapHandles(iH, "T1MiPurgeStale");
    CHECK(!PblHandleRegistry::IsLive(pKey, uId));
    CHECK(RedCountStaleHandles() == uBaseStale);
    VirtualFree(pRegion, 0, MEM_RELEASE);
}

TEST_CASE("the handle audits count stale keys in build heaps and every purge (source pin)" * doctest::test_suite("regression"))
{
    std::string sGame;
    if (!ReadRepoText("src\\Battlefront2\\Source\\GameMemory.cpp", sGame))
    {
        SkipCase("GameMemory.cpp not found");
    }
    const size_t uTrim = sGame.find("RedTrimHeaps();");
    REQUIRE(uTrim != std::string::npos);
    CHECK(sGame.find("RedCountStaleHandles()", uTrim) != std::string::npos);
    std::string sMem;
    REQUIRE(ReadRepoText("src\\RedEngineFL\\Memory\\RedMemory.cpp", sMem));
    const size_t uPurge = sMem.find("void RedPurgeHeapHandles(");
    REQUIRE(uPurge != std::string::npos);
    CHECK(sMem.find("RedCountStaleHandles()", uPurge) != std::string::npos);
}

TEST_CASE("mimalloc heap slots are re-created with a new heap after destroy" * doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    const int iFirst = CreateMiHeap(0x10, "T1MiRecreate");
    REQUIRE(iFirst > 0);
    void* pOld = RedAllocFromHeap(iFirst, 64, false);
    REQUIRE(pOld);
    RedDestroyHeap(iFirst);
    const int iSecond = CreateMiHeap(0x10, "T1MiRecreate");
    CHECK(iSecond == iFirst);
    REQUIRE(iSecond > 0);
    RedHeapStats kStats;
    memset(&kStats, 0, sizeof(kStats));
    REQUIRE(RedGetHeapStats(iSecond, &kStats));
    CHECK(kStats.uUsed == 0);
    CHECK(kStats.uPeak == 0);
    CHECK(RedHeapOwner(pOld) == kRedOwnerStale);
    void* pNew = RedAllocFromHeap(iSecond, 64, false);
    REQUIRE(pNew);
    CHECK(RedMiClassify(pNew) == kRedMiValid);
    CHECK(RedHeapOwner(pNew) == iSecond);
    RedFreeToHeap(iSecond, pNew, 0);
    RedDestroyHeap(iSecond);
    CHECK(gCheckCalls == iChecks);
}

TEST_CASE("mimalloc options line lists the effective values" * doctest::test_suite("regression"))
{
    char acLine[256];
    RedMiConfigure(0);
    REQUIRE(RedMiFormatOptions(acLine, sizeof(acLine)));
    CHECK(std::string(acLine) == "OPENSWBF2 MIOPTIONS purge_delay=0 retry_on_oom=0 arena_reserve=67108864 "
                                 "arena_eager_commit=0 reserve_os_memory=0 page_full_retain=-1");
    RedMiConfigure(1000);
    REQUIRE(RedMiFormatOptions(acLine, sizeof(acLine)));
    CHECK(std::string(acLine) == "OPENSWBF2 MIOPTIONS purge_delay=1000 retry_on_oom=0 arena_reserve=67108864 "
                                 "arena_eager_commit=0 reserve_os_memory=0 page_full_retain=-1");
    char acShort[8];
    CHECK(!RedMiFormatOptions(acShort, sizeof(acShort)));
    CHECK(strlen(acShort) < sizeof(acShort));
}

bool T1CountBlock(void*, size_t, void* pArg)
{
    ++*(int*)pArg;
    return true;
}

TEST_CASE("mimalloc heap destroy visits full pages after frees on them" * doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    RedMiConfigure(1000);
    const RedHeapBackend* pSaved = gRedHeapBackend;
    gRedHeapBackend = &kRedMiBackend;
    const int iH = RedCreateHeap((char*)0x10, (char*)0x10 + 0x4000000, 0x10, "T1MiFull");
    gRedHeapBackend = pSaved;
    REQUIRE(iH > 0);
    std::vector<char*> vpBlock;
    for (int i = 0; i < 128; ++i)
    {
        char* p = (char*)RedAllocFromHeap(iH, 70000, false);
        REQUIRE(p);
        vpBlock.push_back(p);
    }
    for (size_t i = 0; i < vpBlock.size(); i += 3)
    {
        RedFreeToHeap(iH, vpBlock[i], 0);
        RedFreeToHeap(iH, vpBlock[i + 1], 0);
    }
    int iBlocks = 0;
    gRedHeapSlots[iH].pBackend->visit(iH, T1CountBlock, &iBlocks);
    CHECK(iBlocks > 0);
    RedDestroyHeap(iH);
    CHECK(gCheckCalls == iChecks);
}

TEST_CASE("mimalloc heap records every address region without a fatal check" * doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    MiHeap kHeap(0x10, "T1MiRegions");
    REQUIRE(kHeap.miHeap > 0);
    std::vector<char*> vpRegion;
    for (int i = 0; i < 12; ++i)
    {
        char* p = (char*)VirtualAlloc(0, 0x30000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        REQUIRE(p);
        vpRegion.push_back(p);
        RedMiRecordReservation(p);
    }
    CHECK(gCheckCalls == iChecks);
    for (size_t i = 0; i < vpRegion.size(); ++i)
    {
        CAPTURE(i);
        CHECK(RedHeapOwner(vpRegion[i] + 32) == kRedOwnerStale);
        CHECK(RedHeapOwner(vpRegion[i] + 0x10020) == kRedOwnerStale);
        CHECK(RedHeapOwner(vpRegion[i] + 0x20020) == kRedOwnerStale);
    }
    for (size_t i = 0; i < vpRegion.size(); ++i)
    {
        VirtualFree(vpRegion[i], 0, MEM_RELEASE);
    }
    CHECK(gCheckCalls == iChecks);
}

#if !defined(__SANITIZE_ADDRESS__)
TEST_CASE("a crt block on a released mimalloc region goes to the crt" * doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    MiHeap kHeap(0x10, "T1MiCrtReuse");
    REQUIRE(kHeap.miHeap > 0);
    char* pCrt = (char*)malloc(0x400000);
    REQUIRE(pCrt);
    RedMiRecordReservation(pCrt);
    CHECK(RedHeapOwner(pCrt) == -1);
    CHECK(RedHeapOwner(pCrt + 8) == kRedOwnerStale);
    CHECK(gCheckCalls == iChecks);
    RedFreeToHeap(0, pCrt, 0);
    CHECK(gCheckCalls == iChecks);
}
#endif

TEST_CASE("mimalloc treats exhaustion as non-fatal and corruption as fatal" * doctest::test_suite("regression"))
{
    static const char* const kapcExhaustion[] = {
        "unable to allocate memory for a thread local heap\n",
        "unable to dynamically create a thread local for a heap\n",
        "unable to allocate thread local variables\n",
        "aligned allocation request is too large (size 1, alignment 16)\n",
    };
    static const char* const kapcCorrupt[] = {
        "corrupted free list entry of size 16b at 0x1: value 0x2\n",
        "corrupted thread-free list (possibly due to a cross-thread double free)\n",
        "buffer overflow in heap block 0x1 of size 16: write after 8 bytes\n",
        "double free detected of block 0x1 with size 16\n",
        "free: invalid pointer: 0x1\n",
        "secure level 4, but failed to set guard page (at 0x1 of size 4096)\n",
    };
    for (int i = 0; i < 4; ++i)
    {
        CAPTURE(i);
        RedMiNoteOutput(kapcExhaustion[i]);
        CHECK(!RedMiTakeErrorFatal(i == 3 ? EINVAL : EFAULT));
        CHECK(RedMiTakeErrorFatal(EFAULT));
    }
    for (int i = 0; i < 6; ++i)
    {
        CAPTURE(i);
        RedMiNoteOutput(kapcCorrupt[i]);
        CHECK(RedMiTakeErrorFatal(EFAULT));
        RedMiNoteOutput(kapcCorrupt[i]);
        CHECK(RedMiTakeErrorFatal(EINVAL));
    }
    CHECK(!RedMiTakeErrorFatal(ENOMEM));
    CHECK(!RedMiTakeErrorFatal(EOVERFLOW));
    CHECK(RedMiTakeErrorFatal(EAGAIN));
    RedMiNoteOutput(kapcExhaustion[0]);
    RedMiNoteOutput("mimalloc: warning: something else\n");
    CHECK(RedMiTakeErrorFatal(EFAULT));
    RedMiNoteOutput(kapcExhaustion[1]);
    CHECK(RedMiTakeErrorFatal(EAGAIN));
}

TEST_CASE("mimalloc error hook sees the message text of each error (source pin)" * doctest::test_suite("regression"))
{
    std::string sMi;
    if (!ReadRepoText("src\\RedEngineFL\\Memory\\RedMemoryMi.cpp", sMi))
    {
        SkipCase("RedMemoryMi.cpp not found");
    }
    CHECK(sMi.find("mi_option_set(mi_option_show_errors, 1);") != std::string::npos);
    CHECK(sMi.find("mi_option_set(mi_option_max_errors, -1);") != std::string::npos);
    CHECK(sMi.find("RedMiNoteOutput(msg);") != std::string::npos);
    CHECK(sMi.find("if (!RedMiTakeErrorFatal(err))") != std::string::npos);
    struct Site
    {
        const char* pcFile;
        const char* pcText;
    };
    static const Site kaSites[] = {
        { "src\\mimalloc\\src\\heap\\heap.c", "_mi_error_message(EFAULT, \"unable to allocate memory for a thread local heap" },
        { "src\\mimalloc\\src\\heap\\heap.c", "_mi_error_message(EFAULT, \"unable to dynamically create a thread local for a heap" },
        { "src\\mimalloc\\src\\util\\threadlocal.c", "_mi_error_message(EFAULT,\"unable to allocate thread local variables" },
        { "src\\mimalloc\\src\\alloc\\alloc-aligned.c", "_mi_error_message(EINVAL, \"aligned allocation request is too large" },
    };
    for (int i = 0; i < 4; ++i)
    {
        CAPTURE(i);
        std::string sFile;
        REQUIRE(ReadRepoText(kaSites[i].pcFile, sFile));
        CHECK(sFile.find(kaSites[i].pcText) != std::string::npos);
    }
}

TEST_CASE("mimalloc output hook logs one line per message line" * doctest::test_suite("regression"))
{
    std::vector<std::string> vLines;
    RedMiOutputLines("a\nbc\n\nd", CollectLine, &vLines);
    REQUIRE(vLines.size() == 3);
    CHECK(vLines[0] == "a");
    CHECK(vLines[1] == "bc");
    CHECK(vLines[2] == "d");
}

TEST_CASE("sound allocations are charged to the heap that owns them across the sound heap switch" *
          doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    MiHeap kHeap(0x10, "T1MiSound");
    const int iH = kHeap.miHeap;
    REQUIRE(iH > 0);
    siT1SoundHeap = 0;
    const long lZeroStart = gRedHeapSlots[0].used;
    void* pA = T1SndAlloc(64);
    siT1SoundHeap = iH;
    void* pB = T1SndAlloc(64);
    const long lMiLive = gRedHeapSlots[iH].used;
    T1SndFree(pA);
    T1SndFree(pB);
    const long lZeroEnd = gRedHeapSlots[0].used;
    const long lMiEnd = gRedHeapSlots[iH].used;
    siT1SoundHeap = 0;
    CHECK(pA);
    CHECK(pB);
    CHECK(lMiLive == (long)MiNeed(64, 0x10));
    CHECK(lZeroEnd == lZeroStart);
    CHECK(lMiEnd == 0);
    CHECK(gCheckCalls == iChecks);
    std::string sSound;
    if (!ReadRepoText("src\\Battlefront2\\Source\\GameSoundEngine.cpp", sSound))
    {
        SkipCase("GameSoundEngine.cpp not found");
    }
    CHECK(sSound.find("return RedAllocFromHeap(GameSoundEngine::gSoundHeap, uiSize, 0);") != std::string::npos);
    CHECK(sSound.find("RedFreeToHeap(GameSoundEngine::gSoundHeap, pBlock, 0);") != std::string::npos);
}

TEST_CASE("mimalloc heap alloc checks the owner heap in test builds (source pin)" * doctest::test_suite("regression"))
{
    std::string sMi;
    if (!ReadRepoText("src\\RedEngineFL\\Memory\\RedMemoryMi.cpp", sMi))
    {
        SkipCase("RedMemoryMi.cpp not found");
    }
    const char* pcCheck = "check(mi_heap_of"
                          "(pBase) == sMiHeap[i]);";
    const size_t uCheck = sMi.find(pcCheck);
    REQUIRE(uCheck != std::string::npos);
    CHECK(sMi.find(pcCheck, uCheck + 1) == std::string::npos);
    const size_t uIfdef = sMi.rfind("#ifdef OPENSWBF2_TEST", uCheck);
    REQUIRE(uIfdef != std::string::npos);
    const size_t uEndif = sMi.find("#endif", uIfdef);
    REQUIRE(uEndif != std::string::npos);
    CHECK(uEndif > uCheck);
    const size_t uLineAfterIfdef = sMi.find('\n', uIfdef);
    REQUIRE(uLineAfterIfdef != std::string::npos);
    CHECK(sMi.find_first_not_of(" \t\r\n", uLineAfterIfdef) == uCheck);
}

TEST_CASE("mimalloc heap destroy visits areas only under asan (source pin)" * doctest::test_suite("regression"))
{
    std::string sMi;
    if (!ReadRepoText("src\\RedEngineFL\\Memory\\RedMemoryMi.cpp", sMi))
    {
        SkipCase("RedMemoryMi.cpp not found");
    }
    const size_t uVisit = sMi.find("mi_heap_visit_blocks(sMiHeap[i], false, MiPoisonArea, 0);");
    REQUIRE(uVisit != std::string::npos);
    const size_t uIf = sMi.rfind("#if defined(__SANITIZE_ADDRESS__)", uVisit);
    REQUIRE(uIf != std::string::npos);
    CHECK(sMi.find("#endif", uIf) > uVisit);
}

TEST_CASE("mimalloc allocator and purge switches parse their values" * doctest::test_suite("regression"))
{
    bool bRejected = false;
    CHECK(RedParseAllocatorArg(0, 0, &bRejected) == &kRedMiBackend);
    CHECK(!bRejected);
    CHECK(RedParseAllocatorArg("arena", 5, &bRejected) == &kRedArenaBackend);
    CHECK(RedParseAllocatorArg("ARENA", 5, &bRejected) == &kRedArenaBackend);
    CHECK(RedParseAllocatorArg("mimalloc", 8, &bRejected) == &kRedMiBackend);
    CHECK(!bRejected);
    const char* apcBad[] = { "arenas", "", "mi" };
    for (int i = 0; i < 3; ++i)
    {
        bRejected = false;
        CHECK(RedParseAllocatorArg(apcBad[i], (int)strlen(apcBad[i]), &bRejected) == &kRedMiBackend);
        CHECK(bRejected);
    }
    bRejected = false;
    CHECK(RedParseMiPurgeArg(0, 0, &bRejected) == 1000);
    CHECK(RedParseMiPurgeArg("0", 1, &bRejected) == 0);
    CHECK(RedParseMiPurgeArg("250", 3, &bRejected) == 250);
    CHECK(RedParseMiPurgeArg("250 -x", 3, &bRejected) == 250);
    CHECK(!bRejected);
    const char* apcBadMs[] = { "-1", "", "12x", "1234567890" };
    for (int i = 0; i < 4; ++i)
    {
        bRejected = false;
        CHECK(RedParseMiPurgeArg(apcBadMs[i], (int)strlen(apcBadMs[i]), &bRejected) == 1000);
        CHECK(bRejected);
    }
}

TEST_CASE("mimalloc heap destroy poisons every area past the first area table" * doctest::test_suite("regression"))
{
    const int iChecks = gCheckCalls;
    const RedHeapBackend* pSaved = gRedHeapBackend;
    gRedHeapBackend = &kRedMiBackend;
    const int iH = RedCreateHeap((char*)0x10, (char*)0x10 + 0x4000000, 0x10, "T1MiAreas");
    gRedHeapBackend = pSaved;
    REQUIRE(iH > 0);
    std::vector<char*> vpBlock;
    std::vector<unsigned int> vuPage;
    while (vuPage.size() <= 600)
    {
        char* p = (char*)RedAllocFromHeap(iH, 1000, false);
        REQUIRE(p);
        vpBlock.push_back(p);
        const unsigned int uPage = (unsigned int)p >> 15;
        if (vuPage.empty() || vuPage.back() != uPage)
        {
            vuPage.push_back(uPage);
        }
    }
    std::sort(vuPage.begin(), vuPage.end());
    CHECK(std::unique(vuPage.begin(), vuPage.end()) - vuPage.begin() > 512);
    RedDestroyHeap(iH);
    int iUnpoisoned = 0;
    for (size_t i = 0; i < vpBlock.size(); ++i)
    {
#if defined(__SANITIZE_ADDRESS__)
        if (!__asan_address_is_poisoned(vpBlock[i]))
        {
            ++iUnpoisoned;
        }
#endif
    }
    CHECK(iUnpoisoned == 0);
    CHECK(gCheckCalls == iChecks);
}

TEST_CASE("heap trim calls the process backend trim once and tolerates none" * doctest::test_suite("regression"))
{
    const RedHeapBackend* pSaved = gRedHeapBackend;
    RedHeapBackend kCounting = kRedArenaBackend;
    kCounting.trim = T1CountTrim;
    siT1TrimCalls = 0;
    gRedHeapBackend = &kCounting;
    RedTrimHeaps();
    CHECK(siT1TrimCalls == 1);
    kCounting.trim = 0;
    RedTrimHeaps();
    CHECK(siT1TrimCalls == 1);
    gRedHeapBackend = pSaved;
    CHECK(kRedArenaBackend.trim != nullptr);
    CHECK(kRedMiBackend.trim != nullptr);
}

TEST_CASE("mimalloc trim returns a destroyed heap's pages before the purge delay" * doctest::test_suite("regression"))
{
    ScopedMiBackend kScope(600000);
    const int iHeap = RedCreateHeap((void*)0x20, (void*)(0x20 + 0x4000000), 0x10, "T1MiTrim");
    REQUIRE(iHeap > 0);
    for (int i = 0; i < 256; ++i)
    {
        void* p = RedAllocFromHeap(iHeap, 0x10000, false);
        REQUIRE(p);
        memset(p, 0x5A, 0x10000);
    }
    RedDestroyHeap(iHeap);
    const unsigned int uBefore = T1CommitBytes();
    RedTrimHeaps();
    const unsigned int uAfter = T1CommitBytes();
    INFO("commit before trim " << uBefore << " after " << uAfter);
    CHECK(uAfter + 0x800000 <= uBefore);
}
