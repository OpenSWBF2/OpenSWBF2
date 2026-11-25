#ifdef OPENSWBF2_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AutomationTest.h"
#include "FLGuiManager.h"
#include "GameMovie.h"
#include "GameState.h"
#include "LuaHelper.h"
#include "LuaSaveItems.h"
#include "NetSetup.h"
#include "SimDigest.h"
#include "pcCrash.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Application/pc/pcWindow.h"
#include "RedEngineFL/Graphics/PC/pcDisplayLogic.h"
#include "RedEngineFL/Graphics/PC/pcGammaPass.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/input/pc/pcRedInputManager.h"
#include "RedEngineFL/movie/RedMovie.h"
#include "RedEngineFL/movie/pc/pcRedMoviePlayer.h"
#include "RedEngineFL/Util/RedLog.h"

namespace
{

bool FindParam(char const* pcParams, char const* pcKey, char* pcOut, size_t uSize)
{
    const size_t uKey = strlen(pcKey);
    char const* pc = pcParams;
    while (pc && *pc)
    {
        char const* pcEnd = strchr(pc, ',');
        const size_t uLen = pcEnd ? (size_t)(pcEnd - pc) : strlen(pc);
        if (uLen > uKey && strncmp(pc, pcKey, uKey) == 0 && pc[uKey] == ':')
        {
            const size_t uValue = uLen - uKey - 1;
            if (uValue >= uSize)
            {
                return false;
            }
            memcpy(pcOut, pc + uKey + 1, uValue);
            pcOut[uValue] = 0;
            return true;
        }
        pc = pcEnd ? pcEnd + 1 : 0;
    }
    return false;
}

bool KnownKeys(FAutomationTestBase& rTest, char const* pcParams, char const* const* ppcKeys, size_t uKeys)
{
    bool bOk = true;
    char const* pc = pcParams;
    while (pc && *pc)
    {
        char const* pcColon = strchr(pc, ':');
        char const* pcEnd = strchr(pc, ',');
        if (!pcColon || (pcEnd && pcColon > pcEnd))
        {
            rTest.AddError("parameter part without a key:value in \"%s\"", pcParams);
            return false;
        }
        bool bKnown = false;
        for (size_t i = 0; i < uKeys && !bKnown; ++i)
        {
            bKnown = strlen(ppcKeys[i]) == (size_t)(pcColon - pc) && strncmp(pc, ppcKeys[i], pcColon - pc) == 0;
        }
        if (!bKnown)
        {
            rTest.AddError("unknown parameter key %.*s in \"%s\"", (int)(pcColon - pc), pc, pcParams);
            bOk = false;
        }
        pc = pcEnd ? pcEnd + 1 : 0;
    }
    return bOk;
}

void CheckCount(FAutomationTestBase& rTest, char const* pcParams, char const* pcKey, unsigned int uActual)
{
    char acValue[32];
    if (!FindParam(pcParams, pcKey, acValue, sizeof(acValue)))
    {
        return;
    }
    char* pcEnd = 0;
    const unsigned long ulWant = strtoul(acValue, &pcEnd, 0);
    const bool bAtLeast = pcEnd != acValue && strcmp(pcEnd, "..") == 0;
    if (pcEnd == acValue || (!bAtLeast && *pcEnd != 0))
    {
        rTest.AddError("parameter %s:%s is not a count", pcKey, acValue);
        return;
    }
    if (bAtLeast ? uActual < ulWant : uActual != ulWant)
    {
        rTest.AddError("%s is %u, expected %s", pcKey, uActual, acValue);
    }
}

void CheckText(FAutomationTestBase& rTest, char const* pcParams, char const* pcKey, char const* pcActual)
{
    char acValue[64];
    if (FindParam(pcParams, pcKey, acValue, sizeof(acValue)))
    {
        rTest.TestEqual(pcKey, pcActual, acValue);
    }
}

bool OneOf(char const* pcValue, char const* pcChoices)
{
    const size_t uLen = strlen(pcValue);
    char const* pc = pcChoices;
    while (*pc)
    {
        char const* pcEnd = strchr(pc, '|');
        const size_t uPart = pcEnd ? (size_t)(pcEnd - pc) : strlen(pc);
        if (uPart == uLen && strncmp(pc, pcValue, uLen) == 0)
        {
            return true;
        }
        if (!pcEnd)
        {
            break;
        }
        pc = pcEnd + 1;
    }
    return false;
}

void CheckChoice(FAutomationTestBase& rTest, char const* pcParams, char const* pcKey, char const* pcActual)
{
    char acValue[64];
    if (FindParam(pcParams, pcKey, acValue, sizeof(acValue)) && !OneOf(pcActual, acValue))
    {
        rTest.AddError("%s is \"%s\", expected one of \"%s\"", pcKey, pcActual, acValue);
    }
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameMemoryMiOptionsTest, "Game.Memory.MiOptions", EAutomationTestFlags::ProductFilter)

bool FGameMemoryMiOptionsTest::RunTest(char const* pcParameters)
{
    char const* pcVal = 0;
    int iLen = 0;
    bool bRejected = false;
    pcCrash::FindCommandArg("-mipurge", &pcVal, &iLen);
    const long lPurge = RedParseMiPurgeArg(pcVal, iLen, &bRejected);
    char acWant[256];
    _snprintf(acWant, sizeof(acWant) - 1,
              "Mimalloc options purge_delay=%ld retry_on_oom=0 arena_reserve=67108864 arena_eager_commit=0 "
              "reserve_os_memory=0 page_full_retain=-1",
              lPurge);
    acWant[sizeof(acWant) - 1] = 0;
    char acHave[256];
    TestTrue("the mimalloc options fit the line", RedMiFormatOptions(acHave, sizeof(acHave)));
    TestEqual("mimalloc options", acHave, acWant);
    return TestEqual("parameters", pcParameters, "");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameMemoryPoolSizesTest, "Game.Memory.PoolSizes", EAutomationTestFlags::ProductFilter)

bool FGameMemoryPoolSizesTest::RunTest(char const* pcParameters)
{
    TestEqual("memory pool size refusals", (int)MemoryPool::RefusedSizes(), 0);
    return TestEqual("parameters", pcParameters, "");
}

namespace
{

bool HeapByName(char const* pcName, int iLen, RedHeapStats* pStats)
{
    for (int iHeap = 0; iHeap < maxheaps; ++iHeap)
    {
        char const* pcHeap = RedGetHeapName(iHeap);
        if (pcHeap && (int)strlen(pcHeap) == iLen && strncmp(pcHeap, pcName, iLen) == 0 && RedGetHeapStats(iHeap, pStats))
        {
            return true;
        }
    }
    return false;
}

bool ReportedPool(char const* pcName, int iLen)
{
    const bool bPrefix = iLen > 0 && pcName[iLen - 1] == '*';
    const int iMatch = bPrefix ? iLen - 1 : iLen;
    for (PblList<MemoryPool>::Itor itor(*GetMemoryPoolList()); itor; ++itor)
    {
        if (itor->mPool == 0 && itor->GetPeak() == 0)
        {
            continue;
        }
        if (strncmp(itor->mLabel, pcName, iMatch) == 0 && (bPrefix || itor->mLabel[iMatch] == 0))
        {
            return true;
        }
    }
    return false;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameMemoryReportTest, "Game.Memory.Report", EAutomationTestFlags::ProductFilter)

bool FGameMemoryReportTest::RunTest(char const* pcParameters)
{
    static char const* const sapcKeys[] = {"heaps", "pools", "cap"};
    if (!KnownKeys(*this, pcParameters, sapcKeys, sizeof(sapcKeys) / sizeof(sapcKeys[0])))
    {
        return false;
    }
    char acList[256];
    if (FindParam(pcParameters, "heaps", acList, sizeof(acList)))
    {
        for (char const* pc = acList; *pc;)
        {
            char const* pcEnd = strchr(pc, '|');
            const int iLen = pcEnd ? (int)(pcEnd - pc) : (int)strlen(pc);
            RedHeapStats kStats;
            if (!HeapByName(pc, iLen, &kStats))
            {
                AddError("heap %.*s is not live", iLen, pc);
            }
            pc += iLen + (pcEnd ? 1 : 0);
        }
    }
    if (FindParam(pcParameters, "pools", acList, sizeof(acList)))
    {
        for (char const* pc = acList; *pc;)
        {
            char const* pcEnd = strchr(pc, '|');
            const int iLen = pcEnd ? (int)(pcEnd - pc) : (int)strlen(pc);
            if (!ReportedPool(pc, iLen))
            {
                AddError("no reported memory pool %.*s", iLen, pc);
            }
            pc += iLen + (pcEnd ? 1 : 0);
        }
    }
    if (FindParam(pcParameters, "cap", acList, sizeof(acList)))
    {
        char const* pcSlash = strchr(acList, '/');
        RedHeapStats kStats;
        if (!pcSlash)
        {
            AddError("parameter cap:%s is not <heap>/<bytes>", acList);
        }
        else if (!HeapByName(acList, (int)(pcSlash - acList), &kStats))
        {
            AddError("heap %.*s is not live", (int)(pcSlash - acList), acList);
        }
        else if (kStats.uCap != strtoul(pcSlash + 1, 0, 10))
        {
            AddError("heap %.*s cap %u, expected %s", (int)(pcSlash - acList), acList, kStats.uCap, pcSlash + 1);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameRenderPacingTest, "Game.Render.Pacing", EAutomationTestFlags::ProductFilter)

bool FGameRenderPacingTest::RunTest(char const* pcParameters)
{
    static char const* const sapcKeys[] = {"fps", "vsync", "interval", "waitquery", "timer"};
    if (!KnownKeys(*this, pcParameters, sapcKeys, sizeof(sapcKeys) / sizeof(sapcKeys[0])))
    {
        return false;
    }
    const RedPacingState kState = RedGetPacingState();
    char acInterval[16];
    _snprintf(acInterval, sizeof(acInterval) - 1, "0x%08lX", kState.ulInterval);
    acInterval[sizeof(acInterval) - 1] = 0;
    char const* pcVsync = kState.ulInterval == D3DPRESENT_INTERVAL_IMMEDIATE
                              ? "off"
                              : (kState.ulInterval == D3DPRESENT_INTERVAL_TWO ? "half" : "on");
    CheckCount(*this, pcParameters, "fps", kState.uFps);
    CheckText(*this, pcParameters, "vsync", pcVsync);
    CheckText(*this, pcParameters, "interval", acInterval);
    CheckCount(*this, pcParameters, "waitquery", kState.bWaitQuery ? 1u : 0u);
    CheckChoice(*this, pcParameters, "timer", kState.pcTimer);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameWindowDpiTest, "Game.Window.Dpi", EAutomationTestFlags::ProductFilter)

bool FGameWindowDpiTest::RunTest(char const* pcParameters)
{
    static char const* const sapcKeys[] = {"awareness"};
    if (!KnownKeys(*this, pcParameters, sapcKeys, sizeof(sapcKeys) / sizeof(sapcKeys[0])))
    {
        return false;
    }
    CheckChoice(*this, pcParameters, "awareness", DisplayLogic::DpiLevelName(PcWindow::GetDpiLevel()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameRenderGammaTest, "Game.Render.Gamma", EAutomationTestFlags::ProductFilter)

bool FGameRenderGammaTest::RunTest(char const* pcParameters)
{
    if (g_bNoRender)
    {
        TestEqual("gamma path without a renderer", GammaPass::TestPathName(), "ramp");
        TestEqual("gamma reason without a renderer", GammaPass::TestReason(), "norender");
    }
    else
    {
        TestFalse("a rendered run took the gamma ramp for want of a renderer",
                  strcmp(GammaPass::TestReason(), "norender") == 0);
    }
    return TestEqual("parameters", pcParameters, "");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameInputCursorTest, "Game.Input.Cursor", EAutomationTestFlags::ProductFilter)

bool FGameInputCursorTest::RunTest(char const* pcParameters)
{
    static char const* const sapcKeys[] = {"mode", "image"};
    if (!KnownKeys(*this, pcParameters, sapcKeys, sizeof(sapcKeys) / sizeof(sapcKeys[0])))
    {
        return false;
    }
    CheckText(*this, pcParameters, "mode", HwCursor::Software() ? "sw" : "hw");
    CheckText(*this, pcParameters, "image", HwCursor::BuiltImage());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStateShellFirstTest, "Game.State.ShellFirst", EAutomationTestFlags::ProductFilter)

bool FGameStateShellFirstTest::RunTest(char const* pcParameters)
{
    TestTrue("-shellfirst was requested", pcCrash::ShellFirstRequested());
    TestTrue("the shell handed over to the mission for -shellfirst", pcCrash::ShellFirstTaken());
    TestTrue("the run ends in the mission", GameState::GetState() == 0x8FF60339);
    return TestEqual("parameters", pcParameters, "");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSimDigestDetailTest, "Game.Sim.DigestDetail", EAutomationTestFlags::ProductFilter)

bool FGameSimDigestDetailTest::RunTest(char const* pcParameters)
{
    char* pcEnd = 0;
    const unsigned long ulFirst = strtoul(pcParameters, &pcEnd, 10);
    if (pcEnd == pcParameters || *pcEnd != 0)
    {
        AddError("Game.Sim.DigestDetail needs the first detail turn, got \"%s\"", pcParameters);
        return false;
    }
    const SimDigest::DetailRecord& rDetail = SimDigest::GetDetailRecord();
    TestEqual("detail turns", (int)rDetail.uTurns, 3);
    TestEqual("first detail turn", (int)rDetail.uFirst, (int)ulFirst);
    TestEqual("last detail turn", (int)rDetail.uLast, (int)ulFirst + 2);
    TestEqual("detail time rows", (int)rDetail.uTimes, 3);
    TestTrue("detail character rows", rDetail.uCharacters > 0);
    TestTrue("detail object rows", rDetail.uObjects > 0);
    TestTrue("detail team rows", rDetail.uTeams > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameMovieEventsTest, "Game.Movie.Events", EAutomationTestFlags::ProductFilter)

bool FGameMovieEventsTest::RunTest(char const* pcParameters)
{
    static char const* const sapcKeys[] = {"opens",     "closes",    "rejects",  "reject",     "ends",
                                           "mismatch",  "endseg",    "endframes", "endshown",  "endheader",
                                           "endwaits",  "hashed",    "frame300", "notify",     "testframes",
                                           "waits",     "skip",      "state"};
    if (!KnownKeys(*this, pcParameters, sapcKeys, sizeof(sapcKeys) / sizeof(sapcKeys[0])))
    {
        return false;
    }
    const RedMovieTestRecord& rMovie = gRedMovieTestRecord;
    CheckCount(*this, pcParameters, "opens", rMovie.uOpens);
    CheckCount(*this, pcParameters, "closes", gRedMovieCloses);
    CheckCount(*this, pcParameters, "rejects", rMovie.uRejects);
    CheckText(*this, pcParameters, "reject", rMovie.acReject);
    CheckCount(*this, pcParameters, "ends", rMovie.uEnds);
    CheckCount(*this, pcParameters, "mismatch", rMovie.uEndMismatches);
    CheckCount(*this, pcParameters, "endseg", rMovie.uEndSegment);
    CheckCount(*this, pcParameters, "endframes", rMovie.uEndFrames);
    CheckCount(*this, pcParameters, "endshown", rMovie.uEndShown);
    CheckCount(*this, pcParameters, "endheader", rMovie.uEndHeader);
    CheckCount(*this, pcParameters, "endwaits", rMovie.uEndWaits);
    CheckCount(*this, pcParameters, "hashed", rMovie.uHashed);
    CheckCount(*this, pcParameters, "notify", gRedMovieNotifyEnds);
    CheckCount(*this, pcParameters, "testframes", gRedMovieTestFrames);
    CheckCount(*this, pcParameters, "waits", rMovie.uWaits);
    CheckCount(*this, pcParameters, "skip", GameMovie::TestSkipFrame());
    char acFrame[48];
    _snprintf(acFrame, sizeof(acFrame) - 1, "0x%08X/%016llX", rMovie.uFrame300Segment, rMovie.ullFrame300);
    acFrame[sizeof(acFrame) - 1] = 0;
    CheckText(*this, pcParameters, "frame300", acFrame);
    CheckText(*this, pcParameters, "state", GameState::GetState() == 0x11E1FC01 ? "shell" : "mission");
    return true;
}

namespace
{

bool WriteSaveSidecar(char const* pcPath, pcCrash::SaveTestRecord const& rRecord)
{
    FILE* pFile = fopen(pcPath, "wb");
    if (!pFile)
    {
        return false;
    }
    for (int i = 0; i < 2; ++i)
    {
        fprintf(pFile, "%d %s 0x%08X\r\n", i == 0 ? 1 : 3, rRecord.akWrites[i].acName, rRecord.akWrites[i].uFields);
    }
    return fclose(pFile) == 0;
}

bool ReadSaveSidecar(char const* pcPath, int iType, char* pcName, size_t uName, unsigned int* puFields)
{
    FILE* pFile = fopen(pcPath, "rb");
    if (!pFile)
    {
        return false;
    }
    bool bFound = false;
    char acLine[256];
    while (!bFound && fgets(acLine, (int)sizeof(acLine), pFile))
    {
        int iLineType = 0;
        char acName[128];
        unsigned int uFields = 0;
        if (sscanf(acLine, "%d %127s 0x%8X", &iLineType, acName, &uFields) == 3 && iLineType == iType &&
            strlen(acName) < uName)
        {
            strcpy(pcName, acName);
            *puFields = uFields;
            bFound = true;
        }
    }
    fclose(pFile);
    return bFound;
}

void CheckSaveProblems(FAutomationTestBase& rTest, pcCrash::SaveTestRecord const& rRecord)
{
    rTest.TestEqual("save test lua errors, missing campaign saves, failed preops and popups", (int)rRecord.uProblems,
                    0);
    rTest.TestEqual("rejected saves and profile CRC mismatches", (int)SaveRejects::Count(), 0);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSaveWriteTest, "Game.Save.Write", EAutomationTestFlags::ProductFilter)

bool FGameSaveWriteTest::RunTest(char const* pcParameters)
{
    if (!*pcParameters)
    {
        AddError("Game.Save.Write needs the sidecar path");
        return false;
    }
    pcCrash::SaveTestRecord const& rRecord = pcCrash::GetSaveTestRecord();
    static const long salBytes[2] = {4356, 260};
    for (int i = 0; i < 2; ++i)
    {
        pcCrash::SaveTestWrite const& rWrite = rRecord.akWrites[i];
        TestTrue(i == 0 ? "the profile was written" : "the campaign save was written", rWrite.bWritten && rWrite.bOk);
        TestEqual(i == 0 ? "profile bytes" : "campaign save bytes", (int)rWrite.lBytes, (int)salBytes[i]);
    }
    TestEqual("profile name", rRecord.akWrites[0].acName, "SaveTest");
    const char* pcCampaign = rRecord.akWrites[1].acName;
    bool bCampaignName = strlen(pcCampaign) == 16 && strncmp(pcCampaign, "SaveTest", 8) == 0;
    for (int i = 8; i < 16 && bCampaignName; ++i)
    {
        const char c = pcCampaign[i];
        bCampaignName = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    if (!bCampaignName)
    {
        AddError("campaign save name \"%s\" is not SaveTest and 8 letters or digits", pcCampaign);
    }
    CheckSaveProblems(*this, rRecord);
    TestTrue("the write fields are kept for the load runs", WriteSaveSidecar(pcParameters, rRecord));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSaveLoadTest, "Game.Save.Load", EAutomationTestFlags::ProductFilter)

bool FGameSaveLoadTest::RunTest(char const* pcParameters)
{
    char const* pcBar = strchr(pcParameters, '|');
    const int iType = atoi(pcParameters);
    if (!pcBar || (iType != 1 && iType != 3) || pcBar != pcParameters + 1 || !pcBar[1])
    {
        AddError("Game.Save.Load needs <1|3>|<sidecar path>, got \"%s\"", pcParameters);
        return false;
    }
    char acName[128];
    unsigned int uFields = 0;
    if (!ReadSaveSidecar(pcBar + 1, iType, acName, sizeof(acName), &uFields))
    {
        AddError("no type %d write in the sidecar %s", iType, pcBar + 1);
        return false;
    }
    pcCrash::SaveTestRecord const& rRecord = pcCrash::GetSaveTestRecord();
    TestTrue("the save was loaded", rRecord.bLoaded && rRecord.bLoadOk);
    TestEqual("load type", rRecord.iLoadType, iType);
    TestEqual("loaded save", rRecord.acLoadName, acName);
    if (rRecord.uLoadFields != uFields)
    {
        AddError("load fields 0x%08X differ from the write fields 0x%08X", rRecord.uLoadFields, uFields);
    }
    CheckSaveProblems(*this, rRecord);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameShellMovieDriveTest, "Game.Shell.MovieDrive", EAutomationTestFlags::ProductFilter)

bool FGameShellMovieDriveTest::RunTest(char const* pcParameters)
{
    if (!TestTrue("the run ends in the shell", GameState::GetState() == 0x11E1FC01))
    {
        return false;
    }
    GuiManager* pGui = GuiManager::Instance();
    LOG(LogCore, Inform, "movie drive: screen before the drive %s", pGui && pGui->GetCurrentScreen() ? pGui->GetCurrentScreen() : "none");
    static char const* const sapcProperties[] = {"shell_main", "shell_sub_left", "attractmovie"};
    for (size_t i = 0; i < sizeof(sapcProperties) / sizeof(sapcProperties[0]); ++i)
    {
        LOG(LogCore, Inform, "movie drive: properties %s in the open shell movie: %s", sapcProperties[i],
            RedMovie::Properties::FindByHashID(PblHash(sapcProperties[i])) ? "found" : "not found");
    }
    const int iBefore = GameMovie::TestPlayRequests();
    static char const kChunk[] =
        "ScriptCB_AddProfile(ScriptCB_tounicode('MovieDrive')) "
        "ScriptCB_PushScreen('ifs_main') "
        "return ScriptCB_IsMoviePlaying()";
    lua_State* L = LuaHelper::mState;
    const int iTop = lua_gettop(L);
    const bool bOk = luaL_loadbuffer(L, kChunk, strlen(kChunk), "=moviedrive") == 0 && lua_pcall(L, 0, 1, 0) == 0;
    if (!bOk)
    {
        const char* pcErr = lua_tostring(L, -1);
        AddError("movie drive lua error: %s", pcErr ? pcErr : "?");
    }
    const bool bPlaying = bOk && lua_toboolean(L, -1) != 0;
    lua_settop(L, iTop);
    LOG(LogCore, Inform, "movie drive: screen after the drive %s, movie playing %d",
        pGui && pGui->GetCurrentScreen() ? pGui->GetCurrentScreen() : "none", bPlaying ? 1 : 0);
    for (int i = iBefore; i < GameMovie::TestPlayRequests(); ++i)
    {
        const GameMovie::TestPlayRequest& rRequest = GameMovie::GetTestPlayRequest(i);
        LOG(LogCore, Inform, "movie drive: play request %s loop=%d blocked=%d found=%d", rRequest.acName,
            rRequest.bLoop ? 1 : 0, rRequest.bBlocked ? 1 : 0, rRequest.bFound ? 1 : 0);
    }
    TestTrue("the drive reached ifs_main",
             pGui && pGui->GetCurrentScreen() && strcmp(pGui->GetCurrentScreen(), "ifs_main") == 0);
    return TestEqual("parameters", pcParameters, "");
}

#endif
