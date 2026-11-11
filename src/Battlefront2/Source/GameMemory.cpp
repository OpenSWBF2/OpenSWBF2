#include "GameMemory.h"

#include <windows.h>
#include <psapi.h>

#include "LuaMemory.h"
#include "LuaMemoryArgs.h"
#include "MemFailLog.h"
#include "pcCrash.h"
#include "PebbleFL/Common/PblCrtHeap.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"
#include "RedEngineFL/Util/RedLog.h"

namespace GameMemory
{
// GLOBAL: SWBF2 0x01e5614c
int ApplicationHeap;
// GLOBAL: SWBF2 0x01e56168
// GamespyHeap
int GamespyHeap;
// GLOBAL: SWBF2 0x01e56160
// RunTimeHeap
int RunTimeHeap;
// GLOBAL: SWBF2 0x01e56164
// TempLoadHeap
int TempLoadHeap;

// GLOBAL: SWBF2 0x01e56144
int LuaHeap;
// GLOBAL: SWBF2 0x01e5613c
int ArtistHeap;

// GLOBAL: SWBF2 0x01e56138
int TEMP_GamespyHeap;
}

// GLOBAL: SWBF2 0x01e56154
char* gArenaStart;
// GLOBAL: SWBF2 0x01e56140
char* gArenaEnd;
// GLOBAL: SWBF2 0x01f9c2b0
char* GamespyHeapstart;
// GLOBAL: SWBF2 0x01f9c2b4
char* GamespyHeapend;
// GLOBAL: SWBF2 0x01e56148
char* g_Heap3Start;
// GLOBAL: SWBF2 0x01e56150
void* gTempHeapBlock;
namespace GameMemory
{

// GLOBAL: SWBF2 0x01e56158
int gTempGamespyBlock;
}
// GLOBAL: SWBF2 0x01e5615c

char* gArtistHeapStart;

static inline char* AlignHeapStart(const void* pAddress)
{
    return (char*)(((unsigned int)pAddress + (GameMemory::kHeapStartAlignment - 1)) &
                   ~(GameMemory::kHeapStartAlignment - 1));
}

// FUNCTION: SWBF2 0x00533b20
void GameMemory::InitApplicationMemory(char* pStart, char* pEnd)
{
    gArenaStart = pStart;
    gArenaEnd = pEnd;
    gArenaStart = (char*)RedInitHeap(pStart, pEnd, 9);

    char* pAppEnd = gArenaStart + kApplicationHeapSize;
    ApplicationHeap = RedCreateHeap(gArenaStart, pAppEnd, kHeapAlignment, "Application");
    if (ApplicationHeap < 0)
    {
        return;
    }

    GamespyHeapstart = pAppEnd;
    GamespyHeapend = pAppEnd + kGamespyHeapSize;
    GamespyHeap = RedCreateHeap(pAppEnd, pAppEnd + kGamespyHeapSize, kHeapAlignment, "Gamespy");
    if (GamespyHeap < 0)
    {
        pcCrash::FatalInit("Gamespy heap", kGamespyHeapSize);
    }

    g_Heap3Start = GamespyHeapend;
    RedSetCurrentHeap(ApplicationHeap);
    OnFailure = MemFail::OnAllocFailure;
}

// FUNCTION: SWBF2 0x00533bc0
void GameMemory::BuildHeaps(bool bMission)
{
    const bool bStateEnded = RunTimeHeap != 0;
    LuaMemoryArgs::ForgetTempHeap();
    if (TempLoadHeap)
    {
        RedMarkHeapPermanent(TempLoadHeap, false);
        RedPurgeHeapHandles(TempLoadHeap, "Temp");
        RedDestroyHeap(TempLoadHeap);
        TempLoadHeap = 0;
    }

    if (RunTimeHeap)
    {
        RedMarkHeapPermanent(RunTimeHeap, false);
        if (TEMP_GamespyHeap)
        {
            RedPurgeHeapHandles(TEMP_GamespyHeap, "TEMP_Gamespy");
            RedDestroyHeap(TEMP_GamespyHeap);
            TEMP_GamespyHeap = 0;
            GameMemory::gTempGamespyBlock = 0;
        }
        RedPurgeHeapHandles(RunTimeHeap, "Runtime");
        RedDestroyHeap(RunTimeHeap);
        RunTimeHeap = 0;
    }

    if (LuaHeap)
    {
        RedMarkHeapPermanent(LuaHeap, false);
        RedPurgeHeapHandles(LuaHeap, "Lua");
        RedDestroyHeap(LuaHeap);
        LuaHeap = 0;
    }

    if (ArtistHeap)
    {
        RedMarkHeapPermanent(ArtistHeap, false);
        RedPurgeHeapHandles(ArtistHeap, "Artist");
        RedDestroyHeap(ArtistHeap);
        ArtistHeap = 0;
    }

    RedTrimHeaps();
    if (bStateEnded)
    {
        PROCESS_MEMORY_COUNTERS kCounters = {};
        K32GetProcessMemoryInfo(GetCurrentProcess(), &kCounters, sizeof(kCounters));
#ifdef OPENSWBF2_TEST
        unsigned int uCrtBusy = 0;
        unsigned int uCrtCommitted = 0;
        unsigned int uCrtLarge = 0;
        const bool bCrt = RedProcessHeapUsage(GetProcessHeap(), &uCrtBusy, &uCrtCommitted, &uCrtLarge);
        uintptr_t uArena = 0;
        size_t uArenaSize = 0;
        unsigned int uArenaCommitted = 0;
        const bool bArena = RedMiFirstArena(&uArena, &uArenaSize) &&
                            RedRangeCommitted((const void*)uArena, (unsigned int)uArenaSize, &uArenaCommitted);
        unsigned int uHeapsCommitted = 0;
        const bool bHeaps = RedHeapsCommitted(&uHeapsCommitted);
        unsigned int uThreadsCommitted = 0;
        unsigned int uThreads = 0;
        const bool bThreads = RedThreadsCommitted(&uThreadsCommitted, &uThreads);
        unsigned int uPrivate = 0;
        const bool bPrivate = RedPrivateCommitted(&uPrivate);
        const unsigned int uGameCrt = PblCrtLiveBytes();
#endif
        LOG(LogMemory, Inform, "OPENSWBF2 COMMIT state=%s bytes=%u", bMission ? "mission" : "shell",
            (unsigned int)kCounters.PagefileUsage);
#ifdef OPENSWBF2_TEST
        if (bCrt && bArena && bHeaps && bThreads && bPrivate)
        {
            LOG(LogMemory, Inform,
                "OPENSWBF2 COMMITPARTS state=%s private=%u gamecrt=%u heaps=%u threads=%u threadcount=%u arena=%u "
                "crtbusy=%u crtcommitted=%u crtlarge=%u",
                bMission ? "mission" : "shell", uPrivate, uGameCrt, uHeapsCommitted, uThreadsCommitted, uThreads,
                uArenaCommitted, uCrtBusy, uCrtCommitted, uCrtLarge);
        }
        else
        {
            LOG(LogMemory, Error, "OPENSWBF2 COMMITPARTS state=%s crt=%d arena=%d heaps=%d threads=%d private=%d failed",
                bMission ? "mission" : "shell", bCrt ? 1 : 0, bArena ? 1 : 0, bHeaps ? 1 : 0, bThreads ? 1 : 0,
                bPrivate ? 1 : 0);
        }
#endif
    }

#ifdef OPENSWBF2_TEST
    const unsigned int uStaleLeft = RedCountStaleHandles();
    if (uStaleLeft != 0)
    {
        LOG(LogMemory, Error, "OPENSWBF2 HANDLEREG audit stale left=%u", uStaleLeft);
        check(uStaleLeft == 0);
    }
#endif

    RedHeapCapOpenWindow();
    char* pLuaStart = AlignHeapStart(g_Heap3Start);
    char* pLuaEnd = pLuaStart + kLuaHeapSize;
    RedMiFailHeapCreate(pcCrash::SimulateAllocFailure("buildheapfail"));
    LuaHeap = RedCreateHeap(pLuaStart, pLuaEnd, kLuaHeapAlignment, "Lua");
    RedMiFailHeapCreate(false);
    if (LuaHeap < 0)
    {
        pcCrash::FatalInit("Lua heap", kLuaHeapSize);
    }

    char* pArtistStart = AlignHeapStart(pLuaEnd);
    gArtistHeapStart = pArtistStart;
    ArtistHeap = RedCreateHeap(pArtistStart, pArtistStart + kArtistHeapSize, kHeapAlignment, "Artist");
    if (ArtistHeap < 0)
    {
        pcCrash::FatalInit("Artist heap", kArtistHeapSize);
    }

    char* pRunTimeStart = AlignHeapStart(pArtistStart + kArtistHeapSize);
    RunTimeHeap = RedCreateHeap(pRunTimeStart, gArenaEnd, kHeapAlignment, "Runtime");
    if (RunTimeHeap < 0)
    {
        pcCrash::FatalInit("Runtime heap", (unsigned int)(gArenaEnd - pRunTimeStart));
    }
    RedSetCurrentHeap(RunTimeHeap);

    LuaMemory::CreatePools(bMission);

    gTempHeapBlock = RedAllocFromHeap(RunTimeHeap, kTempLoadBlockSize, 1);
    TempLoadHeap = RedCreateHeap(gTempHeapBlock, (char*)gTempHeapBlock + kTempLoadHeapSize,
                                 kHeapAlignment, "Temp");
    if (TempLoadHeap < 0)
    {
        pcCrash::FatalInit("Temp heap", kTempLoadHeapSize);
    }
    RedSetTempHeap(TempLoadHeap);
}

// FUNCTION: SWBF2 0x00533d50
void GameMemory::ReleaseTempHeap()
{
    if (!gTempHeapBlock)
    {
        return;
    }

    if (TempLoadHeap)
    {
        RedPurgeHeapHandles(TempLoadHeap, "Temp");
        RedDestroyHeap(TempLoadHeap);
        RedSetTempHeap(-1);
        TempLoadHeap = 0;
    }
    RedFreeToHeap(RunTimeHeap, gTempHeapBlock, 0);
    gTempHeapBlock = 0;
}
