#include <windows.h>
#include <dbghelp.h>
#include <intrin.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "pcCrash.h"
#include "pcAddrSpace.h"
#include "Battlefront2/Platform/Pc/pcProgress.h"
#include "CommandLine.h"
#include "GameMemory.h"
#include "LuaHelper.h"
#include "GameState.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Util/RedLog.h"
#include "RedEngineFL/Application/pc/pcFrameLimiter.h"
#include "RedEngineFL/Graphics/PC/pcDisplayLogic.h"
#include "RedEngineFL/Graphics/PC/pcDisplaySettings.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/input/pc/pcRedInputManager.h"
#include "PlayerProfile.h"
#include "CollisionModel.h"
#include "EntityMine.h"
#include "EntityProp.h"
#include "Factory.h"
#include "LuaCallbacks_Mission.h"
#include "StickInfo.h"
#include "FLEffect.h"
#include "ParticleEmitter.h"
#include "SoundFL/source/sndvoicevirtual.h"
#include "Steam/gfVendor.h"
#include "Steam/steamworks.h"

typedef BOOL (WINAPI* MiniDumpWriteDumpFn)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
    PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
typedef BOOL (WINAPI* SetThreadStackGuaranteeFn)(PULONG);

static LONG gFaultGuard;
static volatile DWORD gFaultThreadId;
static HANDLE ghFaultEvent;
static HANDLE ghDoneEvent;
static MINIDUMP_EXCEPTION_INFORMATION gDumpInfo;
static MiniDumpWriteDumpFn gpfnMiniDumpWriteDump;
static wchar_t gDumpDir[MAX_PATH];
static wchar_t gDefaultDumpDir[MAX_PATH];
static wchar_t gDumpPath[MAX_PATH + 64];
static char gLogPath[MAX_PATH];
static char gBuildId[32];
static char gCrashKind[32];
static char gFatalLine[1024];
static LPTOP_LEVEL_EXCEPTION_FILTER gpfnUef;
static bool gbInstalled;
static bool gbReinstalled;
static bool gbCrashTestRan;
static char gCheckText[384];
static char gCheckFile[128];
static int giCheckLine;
static volatile LONG gTicks;
extern bool gIsQuitting;
static unsigned int guiHangTimeout = 60;
static bool gbWatchdog;
static bool gbSupervised;

static DWORD DumpWaitMs()
{
    return (gbSupervised || gCrashKind[0] != 0) ? INFINITE : 30000;
}

static void TerminateNow(int iCode)
{
    HwCursor::ReleaseClip();
    FrameLimiter::EndPeriod();
    TerminateProcess(GetCurrentProcess(), (UINT)iCode);
    _exit(iCode);
}

static void AppendLog(char const* pcText)
{
    if (gLogPath[0] != 0)
    {
        HANDLE hFile = CreateFileA(gLogPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD uiWritten = 0;
            WriteFile(hFile, pcText, (DWORD)lstrlenA(pcText), &uiWritten, 0);
            CloseHandle(hFile);
        }
    }
    OutputDebugStringA(pcText);
}

static void MakeDirs(wchar_t* pwcPath)
{
    for (wchar_t* pwc = pwcPath + 3; *pwc != 0; ++pwc)
    {
        if (*pwc == L'\\')
        {
            *pwc = 0;
            CreateDirectoryW(pwcPath, 0);
            *pwc = L'\\';
        }
    }
    CreateDirectoryW(pwcPath, 0);
}

static void BuildDumpPath()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t const* pwcDir = gDumpDir;
    if (GetFileAttributesW(pwcDir) == INVALID_FILE_ATTRIBUTES)
    {
        pwcDir = gDefaultDumpDir;
    }
    wsprintfW(gDumpPath, L"%s\\BattlefrontII-%04d%02d%02d-%02d%02d%02d-%u.dmp", pwcDir, (int)st.wYear,
        (int)st.wMonth, (int)st.wDay, (int)st.wHour, (int)st.wMinute, (int)st.wSecond,
        (unsigned int)GetCurrentProcessId());
}

static BOOL CALLBACK DumpProgress(PVOID, PMINIDUMP_CALLBACK_INPUT pInput, PMINIDUMP_CALLBACK_OUTPUT pOutput)
{
    pcProgress::Step();
    if (pInput == 0 || pOutput == 0)
    {
        return FALSE;
    }
    switch (pInput->CallbackType)
    {
    case ModuleCallback:
    case ThreadCallback:
    case ThreadExCallback:
    case IncludeThreadCallback:
    case IncludeModuleCallback:
        return TRUE;
    case IncludeVmRegionCallback:
        pOutput->Continue = TRUE;
        return TRUE;
    default:
        return FALSE;
    }
}

static void WriteDump()
{
    gDumpPath[0] = 0;
    if (gpfnMiniDumpWriteDump == 0)
    {
        return;
    }
    BuildDumpPath();
    HANDLE hFile = CreateFileW(gDumpPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        gDumpPath[0] = 0;
        return;
    }
    MINIDUMP_TYPE eFull = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithThreadInfo);
    PMINIDUMP_EXCEPTION_INFORMATION pExc = gDumpInfo.ExceptionPointers ? &gDumpInfo : 0;
    MINIDUMP_CALLBACK_INFORMATION kCallback;
    kCallback.CallbackRoutine = DumpProgress;
    kCallback.CallbackParam = 0;
    BOOL bOk = gpfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, eFull, pExc,
        0, &kCallback);
    if (!bOk)
    {
        SetFilePointer(hFile, 0, 0, FILE_BEGIN);
        SetEndOfFile(hFile);
        bOk = gpfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal,
            pExc, 0, &kCallback);
    }
    CloseHandle(hFile);
    if (!bOk)
    {
        DeleteFileW(gDumpPath);
        gDumpPath[0] = 0;
    }
}

static DWORD WINAPI DumpThread(LPVOID)
{
    for (;;)
    {
        WaitForSingleObject(ghFaultEvent, INFINITE);
        WriteDump();
        SetEvent(ghDoneEvent);
    }
}

#ifdef OPENSWBF2_TEST
static void WriteOffThreadTotal()
{
    unsigned int uiCount = 0;
    unsigned int uiSources = 0;
    unsigned int uiDropped = 0;
    RedGetOffThreadTotals(&uiCount, &uiSources, &uiDropped);
    char acOffLine[96];
    wsprintfA(acOffLine, "OPENSWBF2 MEMREPORT offthread count=%u sources=%u dropped=%u\r\n", uiCount, uiSources,
              uiDropped);
    AppendLog(acOffLine);
}
#endif

static void WriteEnsureSummary()
{
    unsigned int uiFailures = RedLog::EnsureFailureCount();
    if (uiFailures != 0)
    {
        char acLine[96];
        wsprintfA(acLine, "ensure summary: %u failures at %u sites\r\n", uiFailures, RedLog::EnsureSiteCount());
        AppendLog(acLine);
    }
#ifdef OPENSWBF2_TEST
    WriteOffThreadTotal();
#endif
}

static void WriteFatalLine(EXCEPTION_POINTERS* pInfo)
{
    char acModule[MAX_PATH];
    char acPath[MAX_PATH * 3];
    wchar_t awcModule[MAX_PATH];
    unsigned int uiCode = 0;
    uintptr_t uAddr = 0;
    uintptr_t uOffset = 0;
    HMODULE hModule = 0;
    lstrcpyA(acModule, "unknown");
    lstrcpyA(acPath, "none");
    if (pInfo != 0 && pInfo->ExceptionRecord != 0)
    {
        uiCode = (unsigned int)pInfo->ExceptionRecord->ExceptionCode;
        uAddr = (uintptr_t)pInfo->ExceptionRecord->ExceptionAddress;
    }
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)uAddr, &hModule) && hModule != 0)
    {
        DWORD uiLen = GetModuleFileNameW(hModule, awcModule, MAX_PATH);
        if (uiLen > 0 && uiLen < MAX_PATH)
        {
            wchar_t* pwcBase = awcModule;
            for (wchar_t* pwc = awcModule; *pwc != 0; ++pwc)
            {
                if (*pwc == L'\\' || *pwc == L'/')
                {
                    pwcBase = pwc + 1;
                }
            }
            if (WideCharToMultiByte(CP_ACP, 0, pwcBase, -1, acModule, MAX_PATH, 0, 0) == 0)
            {
                lstrcpyA(acModule, "unknown");
            }
            uOffset = uAddr - (uintptr_t)hModule;
        }
    }
    if (gDumpPath[0] != 0)
    {
        if (WideCharToMultiByte(CP_ACP, 0, gDumpPath, -1, acPath, sizeof(acPath), 0, 0) == 0)
        {
            lstrcpyA(acPath, "none");
        }
    }
    if (uiCode == 0xE0000004)
    {
        wsprintfA(gFatalLine, "FATAL: check failed: %.200s at %.120s:%d build %s dump %.400s\r\n", gCheckText, gCheckFile,
            giCheckLine, gBuildId, acPath);
    }
    else
    {
        char acDll[80];
        acDll[0] = 0;
        const char* pcDll = gf::vendor::LastDelayLoadFailure();
        if ((uiCode == 0xC06D007E || uiCode == 0xC06D007F) && pcDll[0] != 0)
        {
            wsprintfA(acDll, " dll=%.64s", pcDll);
        }
        wsprintfA(gFatalLine, "FATAL: exception 0x%08X at 0x%08IX (%.64s+0x%IX)%s build %s dump %.400s\r\n", uiCode,
            uAddr, acModule, uOffset, acDll, gBuildId, acPath);
    }
    AppendLog(gFatalLine);
    WriteEnsureSummary();
}

long pcCrash::Filter(struct _EXCEPTION_POINTERS* pInfo)
{
    if (IsDebuggerPresent())
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (pInfo != 0 && pInfo->ExceptionRecord != 0)
    {
        DWORD uiCode = pInfo->ExceptionRecord->ExceptionCode;
        if (uiCode == 0x406D1388 || uiCode == 0x40010006 || uiCode == 0x4001000A)
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }
    if (InterlockedExchange(&gFaultGuard, 1) == 1)
    {
        if (gFaultThreadId != GetCurrentThreadId())
        {
            Sleep(60000);
        }
        TerminateNow(3);
    }
    gFaultThreadId = GetCurrentThreadId();
    HwCursor::ReleaseClip();
    gDumpPath[0] = 0;
    if (ghFaultEvent != 0 && ghDoneEvent != 0)
    {
        gDumpInfo.ThreadId = GetCurrentThreadId();
        gDumpInfo.ExceptionPointers = pInfo;
        gDumpInfo.ClientPointers = FALSE;
        SetEvent(ghFaultEvent);
        WaitForSingleObject(ghDoneEvent, DumpWaitMs());
    }
    WriteFatalLine(pInfo);
    TerminateNow(3);
    return EXCEPTION_EXECUTE_HANDLER;
}

static LONG WINAPI UefStub(EXCEPTION_POINTERS* pInfo)
{
    return pcCrash::Filter(pInfo);
}

static void RaiseCrash(DWORD uiCode)
{
    __try
    {
        RaiseException(uiCode, EXCEPTION_NONCONTINUABLE, 0, 0);
    }
    __except (pcCrash::Filter(GetExceptionInformation()))
    {
        TerminateNow(3);
    }
    TerminateNow(3);
}

static void __cdecl InvalidParamHandler(wchar_t const*, wchar_t const*, wchar_t const*, unsigned int,
    uintptr_t)
{
    RaiseCrash(0xE0000001);
}

static void __cdecl PureCallHandler()
{
    RaiseCrash(0xE0000002);
}

static void __cdecl AbortHandler(int)
{
    RaiseCrash(0xE0000003);
}

static char const* FindArg(char const* pcCmd, char const* pcName, int* piLen);

static void ReadLogArg()
{
    int iLen = 0;
    char const* pcVal = FindArg(GetCommandLineA(), "-log", &iLen);
    if (pcVal != 0 && iLen > 0 && iLen < MAX_PATH)
    {
        char acPath[MAX_PATH];
        memcpy(acPath, pcVal, iLen);
        acPath[iLen] = 0;
        pcCrash::SetLogPath(acPath);
    }
}

static void InstallHandlers()
{
    gpfnUef = UefStub;
    SetUnhandledExceptionFilter(UefStub);
    _set_invalid_parameter_handler(InvalidParamHandler);
    _set_purecall_handler(PureCallHandler);
    signal(SIGABRT, AbortHandler);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
}

void pcCrash::Install()
{
    if (gbInstalled)
    {
        return;
    }
    gbInstalled = true;
    ReadLogArg();
    pcProgress::Open();

    wchar_t awcSys[MAX_PATH];
    UINT uiSysLen = GetSystemDirectoryW(awcSys, MAX_PATH - 16);
    if (uiSysLen > 0 && uiSysLen < MAX_PATH - 16)
    {
        lstrcatW(awcSys, L"\\dbghelp.dll");
        HMODULE hDbgHelp = LoadLibraryW(awcSys);
        if (hDbgHelp != 0)
        {
            gpfnMiniDumpWriteDump = (MiniDumpWriteDumpFn)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
        }
    }

    wchar_t awcLocal[MAX_PATH];
    DWORD uiLocalLen = GetEnvironmentVariableW(L"LOCALAPPDATA", awcLocal, MAX_PATH - 32);
    if (uiLocalLen > 0 && uiLocalLen < MAX_PATH - 32)
    {
        wsprintfW(gDefaultDumpDir, L"%s\\OpenSWBF2\\crashes", awcLocal);
        MakeDirs(gDefaultDumpDir);
    }
    else
    {
        lstrcpyW(gDefaultDumpDir, L".");
    }
    if (gDumpDir[0] == 0)
    {
        lstrcpyW(gDumpDir, gDefaultDumpDir);
    }

    HMODULE hExe = GetModuleHandleW(0);
    IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)hExe;
    IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)((char*)hExe + pDos->e_lfanew);
    wsprintfA(gBuildId, "%08X%X", (unsigned int)pNt->FileHeader.TimeDateStamp,
        (unsigned int)pNt->OptionalHeader.SizeOfImage);

    ghFaultEvent = CreateEventW(0, FALSE, FALSE, 0);
    ghDoneEvent = CreateEventW(0, FALSE, FALSE, 0);
    if (ghFaultEvent != 0 && ghDoneEvent != 0)
    {
        HANDLE hThread = CreateThread(0, 64 * 1024, DumpThread, 0, 0, 0);
        if (hThread != 0)
        {
            CloseHandle(hThread);
        }
    }

    HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
    SetThreadStackGuaranteeFn pfnGuarantee = 0;
    if (hKernel != 0)
    {
        pfnGuarantee = (SetThreadStackGuaranteeFn)GetProcAddress(hKernel, "SetThreadStackGuarantee");
    }
    if (pfnGuarantee != 0)
    {
        ULONG uiStack = 65536;
        pfnGuarantee(&uiStack);
    }

    InstallHandlers();
}

void __cdecl pcCrash::Check(char const* pcExpr, char const* pcFile, int iLine)
{
    lstrcpynA(gCheckText, pcExpr ? pcExpr : "?", sizeof(gCheckText));
    lstrcpynA(gCheckFile, pcFile ? pcFile : "?", sizeof(gCheckFile));
    giCheckLine = iLine;
    RaiseCrash(0xE0000004);
}

void pcCrash::Tick()
{
    InterlockedIncrement(&gTicks);
}

bool pcCrash::SetHangTimeout(int iSeconds)
{
    if (iSeconds == 0)
    {
        gbSupervised = true;
        return true;
    }
    if (iSeconds >= 1 && iSeconds <= 3600)
    {
        guiHangTimeout = (unsigned int)iSeconds;
        return true;
    }
    return false;
}

char const* pcCrash::GetBuildId()
{
    return gBuildId;
}

static void HangExit()
{
    if (InterlockedExchange(&gFaultGuard, 1) == 1)
    {
        return;
    }
    gFaultThreadId = GetCurrentThreadId();
    HwCursor::ReleaseClip();
    gDumpPath[0] = 0;
    gDumpInfo.ExceptionPointers = 0;
    if (ghFaultEvent != 0 && ghDoneEvent != 0)
    {
        SetEvent(ghFaultEvent);
        WaitForSingleObject(ghDoneEvent, DumpWaitMs());
    }
    char acPath[MAX_PATH * 3];
    lstrcpyA(acPath, "none");
    if (gDumpPath[0] != 0 && WideCharToMultiByte(CP_ACP, 0, gDumpPath, -1, acPath, sizeof(acPath), 0, 0) == 0)
    {
        lstrcpyA(acPath, "none");
    }
    if (gbSupervised)
    {
        wsprintfA(gFatalLine, "FATAL: hang (supervisor saw no progress, last frame %d) build %s dump %.400s\r\n",
            RedLog::gFrame, gBuildId, acPath);
    }
    else
    {
        wsprintfA(gFatalLine, "FATAL: hang (no frame update for %u s, last frame %d) build %s dump %.400s\r\n",
            guiHangTimeout, RedLog::gFrame, gBuildId, acPath);
    }
    AppendLog(gFatalLine);
    WriteEnsureSummary();
    TerminateNow(5);
}

static DWORD WINAPI WatchdogThread(LPVOID)
{
    LONG iLast = gTicks;
    DWORD uiChanged = GetTickCount();
    for (;;)
    {
        Sleep(500);
        if (gbSupervised)
        {
            if (pcProgress::StallRequested())
            {
                HangExit();
                return 0;
            }
            continue;
        }
        LONG iNow = gTicks;
        DWORD uiNow = GetTickCount();
        if (iNow != iLast || gIsQuitting)
        {
            iLast = iNow;
            uiChanged = uiNow;
        }
        else if ((DWORD)(uiNow - uiChanged) >= guiHangTimeout * 1000)
        {
            HangExit();
            return 0;
        }
    }
}

bool pcCrash::Supervised()
{
    return gbSupervised;
}

void pcCrash::StartWatchdog()
{
    if (gbWatchdog || IsDebuggerPresent())
    {
        return;
    }
    gbWatchdog = true;
    HANDLE hThread = CreateThread(0, 64 * 1024, WatchdogThread, 0, 0, 0);
    if (hThread != 0)
    {
        CloseHandle(hThread);
    }
}

char const* pcCrash::GetCrashTest()
{
    return gCrashKind;
}

void pcCrash::FatalReset(long hr, unsigned int uiFailures, unsigned int uiElapsedMs)
{
    if (InterlockedExchange(&gFaultGuard, 1) == 1)
    {
        TerminateNow(1);
    }
    gFaultThreadId = GetCurrentThreadId();
    HwCursor::ReleaseClip();
    gDumpPath[0] = 0;
    gDumpInfo.ExceptionPointers = 0;
    if (ghFaultEvent != 0 && ghDoneEvent != 0)
    {
        SetEvent(ghFaultEvent);
        WaitForSingleObject(ghDoneEvent, DumpWaitMs());
    }
    char acPath[MAX_PATH * 3];
    lstrcpyA(acPath, "none");
    if (gDumpPath[0] != 0 && WideCharToMultiByte(CP_ACP, 0, gDumpPath, -1, acPath, sizeof(acPath), 0, 0) == 0)
    {
        lstrcpyA(acPath, "none");
    }
    wsprintfA(gFatalLine, "FATAL: device reset failed hr=0x%08X failures=%u after=%u ms build %s dump %.400s\r\n",
        (unsigned int)hr, uiFailures, uiElapsedMs, gBuildId, acPath);
    AppendLog(gFatalLine);
    WriteEnsureSummary();
    FrameLimiter::Shutdown();
    TerminateNow(1);
}

void pcCrash::Reinstall()
{
    InstallHandlers();
    gbReinstalled = true;
}

void pcCrash::SetLogPath(char const* pcPath)
{
    if (pcPath == 0 || strlen(pcPath) >= MAX_PATH)
    {
        return;
    }
    strcpy(gLogPath, pcPath);
    int iSep = -1;
    for (int i = 0; gLogPath[i] != 0; ++i)
    {
        if (gLogPath[i] == '\\' || gLogPath[i] == '/')
        {
            iSep = i;
        }
    }
    if (iSep < 0)
    {
        lstrcpyW(gDumpDir, L".");
        return;
    }
    if (iSep == 0)
    {
        iSep = 1;
    }
    int iLen = MultiByteToWideChar(CP_ACP, 0, gLogPath, iSep, gDumpDir, MAX_PATH - 1);
    gDumpDir[iLen] = 0;
}

void __cdecl pcCrash::Exit(int iCode)
{
    HwCursor::ReleaseClip();
    FrameLimiter::Shutdown();

    if (iCode == 0)
    {
        LPTOP_LEVEL_EXCEPTION_FILTER pfnPrev = SetUnhandledExceptionFilter(gpfnUef);
        if (pfnPrev != gpfnUef)
        {
            AppendLog("WARNING: unhandled exception filter was replaced\r\n");
        }
    }
    else if (iCode == 1)
    {
        AppendLog("FATAL: initialization failed, exiting with code 1\r\n");
#ifdef OPENSWBF2_TEST
        WriteOffThreadTotal();
#endif
    }
    else if (iCode == 4)
    {
        AppendLog("FATAL: lua panic, exiting with code 4\r\n");
#ifdef OPENSWBF2_TEST
        WriteOffThreadTotal();
#endif
    }
    _exit(iCode);
}

static char const* FindArg(char const* pcCmd, char const* pcName, int* piLen)
{
    char const* pcVal = 0;
    FindCommandLineArg(pcCmd, pcName, &pcVal, piLen);
    return pcVal;
}

bool pcCrash::FindCommandArg(char const* pcName, char const** ppcVal, int* piLen)
{
    int iLen = 0;
    char const* pcVal = FindArg(GetCommandLineA(), pcName, &iLen);
    *ppcVal = pcVal;
    *piLen = iLen;
    return pcVal != 0;
}

static bool CrashTestKindIs(char const* pcKind)
{
    int iLen = 0;
    char const* pcVal = FindArg(GetCommandLineA(), "-crashtest", &iLen);
    return pcVal != 0 && iLen == lstrlenA(pcKind) && _strnicmp(pcVal, pcKind, iLen) == 0;
}

bool pcCrash::SimulateAllocFailure(char const* pcKind)
{
    return CrashTestKindIs(pcKind);
}

void pcCrash::EarlyCrashTest(char const* pcKind)
{
    if (CrashTestKindIs(pcKind))
    {
        volatile int* piNull = 0;
        *piNull = 1;
    }
}

void pcCrash::ReserveLowAddressSpace()
{
    int iLen = 0;
    if (FindArg(GetCommandLineA(), "-hogaddrspace", &iLen) == 0)
    {
        return;
    }
    pcAddrSpace::ReserveBelow(pcAddrSpace::HogLimit(), pcAddrSpace::MinRegion());
}

#ifdef OPENSWBF2_TEST
static bool sbPerturbPresent;
static bool sbPerturbValid;
static char sacPerturbSpec[64];

void pcCrash::InitPerturbFromCommandLine()
{
    int iLen = 0;
    char const* pcVal = FindArg(GetCommandLineA(), "-perturbheap", &iLen);
    if (pcVal == 0)
    {
        return;
    }
    sbPerturbPresent = true;
    const int iCopy = iLen < (int)sizeof(sacPerturbSpec) ? iLen : (int)sizeof(sacPerturbSpec) - 1;
    lstrcpynA(sacPerturbSpec, pcVal, iCopy + 1);
    unsigned int uSeed = 0;
    unsigned int uModes = 0;
    if (iLen < (int)sizeof(sacPerturbSpec) && RedPerturbParse(sacPerturbSpec, &uSeed, &uModes))
    {
        RedPerturbSet(uSeed, uModes);
        sbPerturbValid = true;
    }
}

void pcCrash::LogPerturbState()
{
    if (sbPerturbValid)
    {
        char acModes[32];
        RedPerturbFormatModes(RedPerturbModes(), acModes, sizeof(acModes));
        LOG(LogCore, Inform, "OPENSWBF2 PERTURB seed=%u modes=%s", RedPerturbSeed(), acModes);
    }
    else if (sbPerturbPresent)
    {
        LOG(LogCore, Warning, "-perturbheap spec not understood, perturbation off: %s", sacPerturbSpec);
    }
}

static int RepeatMissionArg()
{
    static int siRepeat = -1;
    if (siRepeat < 0)
    {
        int iLen = 0;
        char const* pcVal = FindArg(GetCommandLineA(), "-repeatmission", &iLen);
        siRepeat = 0;
        if (pcVal != 0)
        {
            const int iCount = CommandLineRepeatCount(pcVal, iLen);
            siRepeat = iCount > 0 ? iCount : 1;
        }
    }
    return siRepeat;
}

int pcCrash::RepeatMissionCount()
{
    return gExitOnGameOver ? RepeatMissionArg() : 0;
}

bool pcCrash::ShellFirstRequested()
{
    static int siShellFirst = -1;
    if (siShellFirst < 0)
    {
        int iLen = 0;
        siShellFirst = FindArg(GetCommandLineA(), "-shellfirst", &iLen) != 0 ? 1 : 0;
    }
    return siShellFirst == 1;
}

bool pcCrash::TakeShellFirst()
{
    static bool sbTaken;
    if (sbTaken || !ShellFirstRequested())
    {
        return false;
    }
    sbTaken = true;
    return true;
}

bool pcCrash::ExitOnMovieCloseRequested()
{
    static int siExit = -1;
    if (siExit < 0)
    {
        int iLen = 0;
        siExit = FindArg(GetCommandLineA(), "-exitonmovieclose", &iLen) != 0 ? 1 : 0;
    }
    return siExit == 1 && gExitOnGameOver;
}

void pcCrash::LogRepeatState()
{
    if (RepeatMissionArg() != 0 && !gExitOnGameOver)
    {
        LOG(LogCore, Warning, "-repeatmission needs -exitongameover, ignored");
    }
}
#endif

bool pcCrash::IsLargeAddressAware()
{
    char* pBase = (char*)GetModuleHandleA(0);
    IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pBase;
    IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)(pBase + pDos->e_lfanew);
    return (pNt->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;
}

void pcCrash::FatalInit(char const* pcWhat, unsigned int uiSize)
{
    if (gLogPath[0] == 0)
    {
        ReadLogArg();
    }
    char acLine[256];
    int iLen = wsprintfA(acLine, "FATAL: out of memory allocating %.64s (%u bytes)", pcWhat, uiSize);
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms))
    {
        iLen += wsprintfA(acLine + iLen, " load %u%% phys %u MB virt %u MB", (unsigned int)ms.dwMemoryLoad,
            (unsigned int)(ms.ullAvailPhys >> 20), (unsigned int)(ms.ullAvailVirtual >> 20));
    }
    lstrcpyA(acLine + iLen, "\r\n");
    AppendLog(acLine);
    WriteEnsureSummary();
    pcCrash::Exit(1);
}

void pcCrash::FatalExit(char const* pcReason)
{
    char acLine[256];
    wsprintfA(acLine, "FATAL: %.200s\r\n", pcReason);
    AppendLog(acLine);
    WriteEnsureSummary();
    pcCrash::Exit(1);
}

static int LuaAllocHuge(lua_State* L)
{
    lua_newuserdata(L, 0x04000000);
    return 0;
}

extern bool pcWindowed;

static unsigned int MenuTestVsync(const PlayerProfile::tPCOpt& rPC)
{
    const DisplayCfg::DisplaySettings& rDisplay = DisplayCfg::gDisplay;
    return DisplayLogic::EffectiveVsync((rDisplay.uiSet & DisplayCfg::kVsync) != 0, rDisplay.uiVsync,
                                        rPC.mVSync);
}

enum MenuTest
{
    kMenuRevert,
    kMenuKeep,
    kMenuSaveFail,
    kMenuAdapterAccept,
    kMenuOrder,
    kMenuOrderRev,
    kMenuRevertSave,
    kMenuApplyFail
};

static void MenuConfirmFrame(MenuTest eTest, bool& rbRan)
{
    const bool bOrder = eTest == kMenuOrder || eTest == kMenuOrderRev;
    const bool bArmKeep = eTest != kMenuRevert && !bOrder;
    const bool bKeep = eTest == kMenuKeep || eTest == kMenuSaveFail;
    static unsigned int suiFrame;
    static unsigned int suiArmedAt;
    static unsigned int suiCountedAt;
    static unsigned int suiAfter;
    static unsigned int suiMode;
    static bool sbArmed;
    static bool sbCounted;
    static bool sbSkewed;

    ++suiFrame;
    if (!sbArmed)
    {
        if (suiFrame < 30)
        {
            return;
        }
        PlayerProfile::tProfile* pProfile = PlayerProfile::GetCurProfile();
        if (!pProfile)
        {
            LOG(LogRender, Warning, "OPENSWBF2 DISPLAY menutest no profile");
            rbRan = true;
            return;
        }
        PlayerProfile::MarkDisplay();
        PlayerProfile::tPCOpt& rPC = pProfile->mPCOptions;
        const unsigned int uiBaseW = rPC.mResolutionX;
        const unsigned int uiBaseH = rPC.mResolutionY;
        LOG(LogRender, Inform, "OPENSWBF2 DISPLAY menutest base %ux%u fps=%u waitquery=%d", uiBaseW,
            uiBaseH, DisplayCfg::gDisplay.uiFps, DisplayCfg::gDisplay.bNoWaitQuery ? 0 : 1);
        const DisplayLogic::DisplayMode eBase =
            DisplayLogic::ResolveMode(DisplayCfg::gDisplay.bBorderless, pcWindowed, rPC.mWindowed);
        const unsigned int uiAcceptFps = DisplayCfg::gDisplay.uiFps == 30 ? 60 : 30;
        if (eTest == kMenuRevertSave || eTest == kMenuApplyFail)
        {
            LOG(LogRender, Inform, "OPENSWBF2 DISPLAY menutest prior vsync=%u fps=%u accept=%u",
                MenuTestVsync(rPC), DisplayCfg::gDisplay.uiFps, uiAcceptFps);
            DisplayCfg::MenuFields kPlain;
            kPlain.uiMode = eBase;
            kPlain.uiRefresh = DisplayCfg::gDisplay.uiRefresh;
            kPlain.uiVsync = MenuTestVsync(rPC);
            kPlain.uiFps = uiAcceptFps;
            kPlain.bWaitQuery = !DisplayCfg::gDisplay.bNoWaitQuery;
            PlayerProfile::ApplyMenuDisplay(kPlain);
        }
        DisplayCfg::MenuFields kFields;
        kFields.uiMode = eBase == DisplayLogic::kModeFullscreen ? DisplayLogic::kModeWindowed
                                                                 : DisplayLogic::kModeFullscreen;
        kFields.uiRefresh = DisplayCfg::gDisplay.uiRefresh;
        kFields.uiVsync = bArmKeep ? 2 : MenuTestVsync(rPC);
        kFields.uiFps = DisplayCfg::gDisplay.uiFps;
        kFields.bWaitQuery = !DisplayCfg::gDisplay.bNoWaitQuery;
        if (eTest == kMenuRevert)
        {
            kFields.uiFps = uiAcceptFps;
            kFields.bWaitQuery = DisplayCfg::gDisplay.bNoWaitQuery;
        }
        else if (eTest == kMenuApplyFail)
        {
            kFields.uiFps = uiAcceptFps + 15;
            kFields.bWaitQuery = DisplayCfg::gDisplay.bNoWaitQuery;
        }
        if (uiBaseW == 1280 && uiBaseH == 720)
        {
            rPC.mResolutionX = 1024;
            rPC.mResolutionY = 768;
        }
        else
        {
            rPC.mResolutionX = 1280;
            rPC.mResolutionY = 720;
        }
        pProfile->UpdateProfCRC();
        suiMode = kFields.uiMode;
        if (eTest == kMenuOrder)
        {
            LOG(LogRender, Inform, "OPENSWBF2 DISPLAY menutest order=legacy-menu");
            PlayerProfile::ApplyLiveDisplay();
            PlayerProfile::ApplyMenuDisplay(kFields);
        }
        else if (eTest == kMenuOrderRev)
        {
            LOG(LogRender, Inform, "OPENSWBF2 DISPLAY menutest order=menu-legacy");
            PlayerProfile::ApplyMenuDisplay(kFields);
            PlayerProfile::ApplyLiveDisplay();
        }
        else
        {
            PlayerProfile::ApplyMenuDisplay(kFields);
        }
        if (bArmKeep)
        {
            if (eTest == kMenuAdapterAccept)
            {
                RedRenderer::pcRequestAdapter(0);
            }
            LOG(LogRender, Inform, "OPENSWBF2 DISPLAY menutest save=%u", PlayerProfile::SaveDisplay());
            PlayerProfile::MarkDisplay();
        }
        sbArmed = true;
        suiArmedAt = suiFrame;
        return;
    }

    unsigned int uiState = 0;
    unsigned int uiLeft = 0;
    unsigned int uiResult = 0;
    unsigned int uiKeepSave = 0;
    PlayerProfile::PollDisplayConfirm(uiState, uiLeft, uiResult, uiKeepSave);
    if (suiFrame - suiArmedAt >= 600)
    {
        LOG(LogRender, Warning, "OPENSWBF2 DISPLAY menutest no confirm state=%u", uiState);
        rbRan = true;
        return;
    }
    if (eTest == kMenuApplyFail)
    {
        sbCounted = true;
        sbSkewed = true;
    }
    if (!sbCounted)
    {
        if (uiState != DisplayLogic::kConfirmCounting)
        {
            return;
        }
        sbCounted = true;
        suiCountedAt = suiFrame;
        if (bKeep)
        {
            if (eTest == kMenuSaveFail)
            {
                char acPath[MAX_PATH];
                if (DisplayCfg::DisplayIniPath(acPath, sizeof(acPath)))
                {
                    WritePrivateProfileStringA("Display", "Marker", "1", acPath);
                    SetFileAttributesA(acPath, FILE_ATTRIBUTE_READONLY);
                }
            }
            PlayerProfile::EndDisplayConfirm(true);
            PlayerProfile::SkewDisplayConfirmClock(DisplayLogic::kConfirmMs + 1000);
            sbSkewed = true;
        }
        else if (eTest == kMenuRevert)
        {
            PlayerProfile::tProfile* pProfile = PlayerProfile::GetCurProfile();
            DisplayCfg::MenuFields kFields;
            kFields.uiMode = suiMode;
            kFields.uiRefresh = DisplayCfg::gDisplay.uiRefresh;
            kFields.uiVsync = (pProfile && MenuTestVsync(pProfile->mPCOptions) == 0) ? 1 : 0;
            kFields.uiFps = DisplayCfg::gDisplay.uiFps;
            kFields.bWaitQuery = !DisplayCfg::gDisplay.bNoWaitQuery;
            LOG(LogRender, Inform, "OPENSWBF2 DISPLAY menutest plain vsync=%u", kFields.uiVsync);
            PlayerProfile::ApplyMenuDisplay(kFields);
        }
        return;
    }
    if (!sbSkewed)
    {
        if (suiFrame - suiCountedAt >= 10)
        {
            PlayerProfile::SkewDisplayConfirmClock(DisplayLogic::kConfirmMs);
            sbSkewed = true;
        }
        return;
    }
    if (uiState != DisplayLogic::kConfirmNone || ++suiAfter < 30)
    {
        return;
    }
    if (eTest == kMenuRevertSave)
    {
        PlayerProfile::RestoreDisplay();
        PlayerProfile::tProfile* pProfile = PlayerProfile::GetCurProfile();
        LOG(LogRender, Inform, "OPENSWBF2 DISPLAY menutest restored vsync=%u fps=%u",
            pProfile ? MenuTestVsync(pProfile->mPCOptions) : 0u, DisplayCfg::gDisplay.uiFps);
    }
    if (eTest == kMenuSaveFail)
    {
        LOG(LogRender, Inform, "OPENSWBF2 DISPLAY menutest save=%u", PlayerProfile::SaveDisplay());
        char acPath[MAX_PATH];
        if (DisplayCfg::DisplayIniPath(acPath, sizeof(acPath)))
        {
            SetFileAttributesA(acPath, FILE_ATTRIBUTE_NORMAL);
        }
    }
    LOG(LogRender, Inform,
        "OPENSWBF2 DISPLAY menutest done result=%u fps=%u waitquery=%d keepsave=%u", uiResult,
        DisplayCfg::gDisplay.uiFps, DisplayCfg::gDisplay.bNoWaitQuery ? 0 : 1, uiKeepSave);
    rbRan = true;
}

static void AdapterCancelFrame(bool& rbRan)
{
    static unsigned int suiFrame;
    if (++suiFrame < 30)
    {
        return;
    }
    PlayerProfile::MarkDisplay();
    RedRenderer::pcRequestAdapter(0);
    PlayerProfile::RestoreDisplay();
    LOG(LogRender, Inform, "OPENSWBF2 DISPLAY adaptertest save=%u", PlayerProfile::SaveDisplay());
    rbRan = true;
}

static void PoolCountTest()
{
    MemoryPool kPool("crashtestpool", 16);
    lua_State* L = LuaHelper::mState;
    lua_pushstring(L, "SetMemoryPoolSize");
    lua_gettable(L, LUA_GLOBALSINDEX);
    lua_pushstring(L, "crashtestpool");
    lua_pushnumber(L, 2147483647.0);
    if (lua_pcall(L, 2, 0, 0) != 0)
    {
        lua_pop(L, 1);
    }
    MemoryPool kHuge("crashtesthuge", 16);
    kHuge.Create(0x40000001, 16, 0);
    MemoryPool kSmall("crashtestsmall", 16);
    kSmall.Create(4, 16, 0);
    void* pFirst = kSmall.Allocate(16);
    void* pSecond = kSmall.Allocate(16);
    kSmall.Free(pFirst);
    kSmall.Free(pSecond);
}

static void PoolFullTest()
{
    MemoryPool kPool("crashtestpool", 16);
    kPool.Create(2, 16, 0);
    void* apBlocks[8];
    for (int i = 0; i < 8; ++i)
    {
        apBlocks[i] = kPool.Allocate(16);
    }
    for (int i = 0; i < 8; ++i)
    {
        kPool.Free(apBlocks[i]);
    }
}

static EntityClass* FindEntityClassOfRtti(unsigned int uRtti, unsigned int uExclude)
{
    typedef Factory<Entity, EntityClass, EntityDesc> EntityFactory;
    for (PblList<EntityFactory>::Itor it(EntityFactory::sList.GetFirst()); it; ++it)
    {
        EntityClass* pClass = (EntityClass*)(&*it);
        if (it->mParent && pClass->IsRtti(uRtti) && !pClass->IsRtti(uExclude))
        {
            return pClass;
        }
    }
    return 0;
}

static Entity* BuildEntity(EntityClass* pClass)
{
    EntityDesc desc;
    return pClass->Build(desc);
}

static void LuaDeleteEntity(Entity* pEntity)
{
    lua_State* L = LuaHelper::mState;
    lua_pushcfunction(L, Lua_Callbacks::DeleteEntity);
    lua_pushlightuserdata(L, pEntity);
    if (lua_pcall(L, 1, 0, 0) != 0)
    {
        lua_pop(L, 1);
    }
}

static void StickDeleteTest()
{
    EntityClass* pPropClass = FindEntityClassOfRtti(EntityPropClass::rttiHashEntityPropClass,
                                                    GameObjectClass::rttiHashGameObjectClass);
    EntityClass* pMineClass = FindEntityClassOfRtti(EntityMineClass::rttiHashEntityMineClass, 0);
    if (!pPropClass || !pMineClass)
    {
        LOG(LogCore, Warning, "OPENSWBF2 STICKDELETE no class prop=%d mine=%d", pPropClass != 0, pMineClass != 0);
        return;
    }
    Entity* pPropEntity = BuildEntity(pPropClass);
    Entity* pMineEntity = BuildEntity(pMineClass);
    if (!pPropEntity || !pMineEntity)
    {
        LOG(LogCore, Warning, "OPENSWBF2 STICKDELETE build failed prop=%d mine=%d", pPropEntity != 0, pMineEntity != 0);
        return;
    }
    EntityGeometry* pProp = static_cast<EntityGeometry*>(pPropEntity);
    EntityMine* pMine = static_cast<EntityMine*>(pMineEntity);
    CollisionObject* pTarget = pProp;
    const bool bPropClass = pPropClass->IsRtti(EntityPropClass::rttiHashEntityPropClass);
    const bool bGameObject = pTarget->GetGameObject() != 0;

    CollisionResult kResult;
    memset(&kResult, 0, sizeof(kResult));
    kResult.mSeparationNormal = PblVector3(0.0f, 1.0f, 0.0f);
    kResult.mReceiverId = 0;
    pMine->CollisionCallback(&kResult, pTarget, 0);
    const bool bStuck = pMine->mStickInfo != 0 && pMine->mStickInfo->mCollObject == pTarget &&
                        pMine->mParent == pTarget;
    pMine->Update(1.0f / 30.0f);

    StickInfo* pStick = pMine->mStickInfo;
    LuaDeleteEntity(pPropEntity);
    const bool bCleared = pStick != 0 && pMine->mStickInfo == pStick && pStick->mCollObject == 0 &&
                          pMine->mParent == 0;
    pMine->Update(1.0f / 30.0f);
    const bool bFell = pMine->mStickInfo == 0;
    LuaDeleteEntity(pMineEntity);
    LOG(LogCore, Inform, "OPENSWBF2 STICKDELETE prop=%d gameobject=%d stuck=%d cleared=%d fell=%d", bPropClass,
        bGameObject, bStuck, bCleared, bFell);
}

static ParticleEmitter* StartParticleSound(const ParticleEmitterClass* pClass)
{
    ParticleEmitter* pEmitter = ParticleEmitter::Create(pClass);
    if (!pEmitter)
    {
        return 0;
    }
    ParticleEmitterInfoData* pInfo = ParticleEmitterInfoData::Create();
    if (!pInfo)
    {
        delete pEmitter;
        return 0;
    }
    pInfo->mPrev = 0;
    pInfo->mNext = 0;
    pInfo->mStartTime = 0.0f;
    pInfo->mLifeTime = 0.0f;
    pInfo->mVelocity = PblVector3(0.0f, 0.0f, 0.0f);
    pInfo->mInitialVelocity = PblVector3(0.0f, 0.0f, 0.0f);
    pInfo->mMatrix = gMatrixIdentity;
    pEmitter->Register(pInfo);
    pInfo->Release();
    pEmitter->Update(0.0f);
    pEmitter->Update(0.0f);
    if (!pEmitter->mAmbientSound.GetVoiceVirtual())
    {
        delete pEmitter;
        return 0;
    }
    return pEmitter;
}

static void ParticleSoundTest()
{
    const ParticleEmitterClass* pClass = 0;
    ParticleEmitter* pEmitter = 0;
    for (PblHashTable<FLEffectClass, 256>::Itor it(FLEffect::s_EffectClasses); it && !pEmitter; ++it)
    {
        if (it->mType != 1)
        {
            continue;
        }
        pClass = static_cast<const ParticleEmitterClass*>((FLEffectClass*)it);
        pEmitter = StartParticleSound(pClass);
    }
    if (!pEmitter)
    {
        LOG(LogCore, Warning, "OPENSWBF2 PARTICLESOUND no emitter class plays an ambient sound");
        return;
    }
    delete pEmitter;
    ParticleEmitter* pReset = StartParticleSound(pClass);
    if (!pReset)
    {
        LOG(LogCore, Warning, "OPENSWBF2 PARTICLESOUND restart failed");
        return;
    }
    pReset->Reset(pClass);
    const int iVoiceAfterReset = pReset->mAmbientSound.GetVoiceVirtual() != 0;
    delete pReset;
    LOG(LogCore, Inform, "OPENSWBF2 PARTICLESOUND class=0x%08X deleted=1 reset=1 voice_after_reset=%d",
        pClass->m_uiClassName, iVoiceAfterReset);
}

static ParticleEmitter* StartParticleBurst(const ParticleEmitterClass* pClass, bool& bPlaying, bool& bReleased,
                                           bool& bVoiceOnNext)
{
    ParticleEmitter* pEmitter = StartParticleSound(pClass);
    if (!pEmitter)
    {
        return 0;
    }
    ParticleEmitterInfoData* pOld = pEmitter->mFirstInfo;
    const float fBurstMin = pEmitter->mSpawnDelay + pClass->mBurstDelay.GetMin();
    ParticleEmitterInfoData* pNew = fBurstMin > 0.0f ? ParticleEmitterInfoData::Create() : 0;
    if (!pNew)
    {
        delete pEmitter;
        return 0;
    }
    const float fTime = pOld->mStartTime + pEmitter->mSpawnDelay + pClass->mTransformer.mLifeTime + 0.1f;
    pNew->mStartTime = fTime;
    pNew->mVelocity = PblVector3(0.0f, 0.0f, 0.0f);
    pNew->mInitialVelocity = PblVector3(0.0f, 0.0f, 0.0f);
    pNew->mMatrix = gMatrixIdentity;
    pNew->mLifeTime = 0.0f;
    pOld->LinkNext(pNew);
    pOld->mLifeTime = fBurstMin * 0.5f;
    pEmitter->Register(pNew);
    pNew->Release();
    pEmitter->Update(fTime);
    Snd::VoiceVirtual* pVoice = pEmitter->mAmbientSound.GetVoiceVirtual();
    bPlaying = pVoice != 0 && pEmitter->mAmbientSound.GetPlaying();
    bReleased = pEmitter->mFirstInfo == pNew;
    bVoiceOnNext = pVoice != 0 && pVoice->mPos3D == &pNew->mMatrix.GetTranslation();
    return pEmitter;
}

static void ParticleBurstTest()
{
    for (PblHashTable<FLEffectClass, 256>::Itor it(FLEffect::s_EffectClasses); it; ++it)
    {
        if (it->mType != 1)
        {
            continue;
        }
        const ParticleEmitterClass* pClass = static_cast<const ParticleEmitterClass*>((FLEffectClass*)it);
        bool bPlaying = false;
        bool bReleased = false;
        bool bVoiceOnNext = false;
        ParticleEmitter* pEmitter = StartParticleBurst(pClass, bPlaying, bReleased, bVoiceOnNext);
        if (!pEmitter)
        {
            continue;
        }
        if (!bPlaying || !bReleased)
        {
            delete pEmitter;
            continue;
        }
        LOG(LogCore, Inform, "OPENSWBF2 PARTICLEBURST class=0x%08X playing=%d released=%d voice_on_next=%d",
            pClass->m_uiClassName, bPlaying, bReleased, bVoiceOnNext);
        delete pEmitter;
        return;
    }
    LOG(LogCore, Warning, "OPENSWBF2 PARTICLEBURST no emitter class bursts with a playing ambient sound");
}

void pcCrash::RunMissionCrashTest()
{
    static bool sbRan;
    if (sbRan || !gbReinstalled || gCrashKind[0] == 0)
    {
        return;
    }
    if (strcmp(gCrashKind, "heapfail") == 0)
    {
        sbRan = true;
        RedAllocFromHeap(GameMemory::RunTimeHeap, 0x10000000, 0);
    }
    else if (strcmp(gCrashKind, "capexhaust") == 0)
    {
        sbRan = true;
        void* p = 0;
        do
        {
            p = RedAllocFromHeap(GameMemory::RunTimeHeap, 0x100000, 0);
        } while (p != 0);
        check(p != 0);
    }
    else if (strcmp(gCrashKind, "midoublefree") == 0)
    {
        sbRan = true;
        void* p = RedAllocFromHeap(GameMemory::RunTimeHeap, 64, 0);
        RedFreeToHeap(GameMemory::RunTimeHeap, p, 0);
        RedFreeToHeap(GameMemory::RunTimeHeap, p, 0);
    }
    else if (strcmp(gCrashKind, "luaoom") == 0)
    {
        sbRan = true;
        lua_cpcall(LuaHelper::mState, LuaAllocHuge, 0);
    }
    else if (strcmp(gCrashKind, "luapanic") == 0)
    {
        sbRan = true;
        lua_pushstring(LuaHelper::mState, "crashtest");
        lua_error(LuaHelper::mState);
    }
    else if (strcmp(gCrashKind, "hang") == 0)
    {
        sbRan = true;
        for (;;)
        {
            Sleep(10);
        }
    }
    else if (strcmp(gCrashKind, "check") == 0)
    {
        sbRan = true;
        (void)ensure(false);
        check(false);
    }
    else if (strcmp(gCrashKind, "resetlaterinit") == 0)
    {
        sbRan = true;
        GameState::SetState(0x8FF60339);
    }
    else if (strcmp(gCrashKind, "menuconfirm") == 0)
    {
        MenuConfirmFrame(kMenuRevert, sbRan);
    }
    else if (strcmp(gCrashKind, "menukeep") == 0)
    {
        MenuConfirmFrame(kMenuKeep, sbRan);
    }
    else if (strcmp(gCrashKind, "menusavefail") == 0)
    {
        MenuConfirmFrame(kMenuSaveFail, sbRan);
    }
    else if (strcmp(gCrashKind, "adaptercancel") == 0)
    {
        AdapterCancelFrame(sbRan);
    }
    else if (strcmp(gCrashKind, "adapteraccept") == 0)
    {
        MenuConfirmFrame(kMenuAdapterAccept, sbRan);
    }
    else if (strcmp(gCrashKind, "menuorder") == 0)
    {
        MenuConfirmFrame(kMenuOrder, sbRan);
    }
    else if (strcmp(gCrashKind, "menuorderrev") == 0)
    {
        MenuConfirmFrame(kMenuOrderRev, sbRan);
    }
    else if (strcmp(gCrashKind, "menurevertsave") == 0)
    {
        MenuConfirmFrame(kMenuRevertSave, sbRan);
    }
    else if (strcmp(gCrashKind, "menuapplyfail") == 0)
    {
        MenuConfirmFrame(kMenuApplyFail, sbRan);
    }
    else if (strcmp(gCrashKind, "poolcount") == 0)
    {
        sbRan = true;
        PoolCountTest();
    }
    else if (strcmp(gCrashKind, "poolfull") == 0)
    {
        sbRan = true;
        PoolFullTest();
    }
    else if (strcmp(gCrashKind, "stickdelete") == 0)
    {
        sbRan = true;
        StickDeleteTest();
    }
    else if (strcmp(gCrashKind, "particlesound") == 0)
    {
        sbRan = true;
        ParticleSoundTest();
    }
    else if (strcmp(gCrashKind, "particleburst") == 0)
    {
        sbRan = true;
        ParticleBurstTest();
    }
    else
    {
        sbRan = true;
    }
}

void pcCrash::SetCrashTest(char const* pcKind)
{
    static char const* const apcKinds[] = { "nullderef", "divzero", "stackoverflow", "debugbreak", "abort",
        "invalidparam", "fastfail", "delayload", "arenaoom", "debugheapoom", "heapfail", "capexhaust", "midoublefree",
        "luaoom", "luapanic", "hang", "check", "resetrecover", "resetfatal", "resetinitlost",
        "resetflap", "resetlaterinit", "applymode", "applymodesmall", "applyfail", "applypresentfail", "applyadapter",
        "applytargetfail", "gammapass", "gammafallback", "gammadrawfail",
        "gammalost", "menuconfirm", "menukeep", "menusavefail", "adaptercancel",
        "adapteraccept", "shellstretch", "menuorder", "menuorderrev",
        "menurevertsave", "menuapplyfail", "poolcount", "poolfull", "stickdelete",
        "particlesound", "particleburst" };
    if (pcKind == 0)
    {
        return;
    }
    for (int i = 0; i < (int)(sizeof(apcKinds) / sizeof(apcKinds[0])); ++i)
    {
        if (_stricmp(pcKind, apcKinds[i]) == 0)
        {
            strcpy(gCrashKind, apcKinds[i]);
            return;
        }
    }
}

static __declspec(noinline) int Recurse(int iDepth)
{
    volatile char acBuf[4096];
    acBuf[0] = (char)iDepth;
    acBuf[4095] = (char)(iDepth >> 8);
    return Recurse(iDepth + 1) + acBuf[0] + acBuf[4095];
}

#ifdef OPENSWBF2_TEST
static void SaveTestFrame();
#endif

void pcCrash::RunCrashTest()
{
#ifdef OPENSWBF2_TEST
    SaveTestFrame();
#endif
    if (gCrashKind[0] == 0 || !gbReinstalled || gbCrashTestRan)
    {
        return;
    }
    gbCrashTestRan = true;
    if (strcmp(gCrashKind, "nullderef") == 0)
    {
        volatile int* piNull = 0;
        *piNull = 1;
    }
    else if (strcmp(gCrashKind, "divzero") == 0)
    {
        volatile int iZero = 0;
        volatile int iResult = 1 / iZero;
        (void)iResult;
    }
    else if (strcmp(gCrashKind, "stackoverflow") == 0)
    {
        Recurse(0);
    }
    else if (strcmp(gCrashKind, "debugbreak") == 0)
    {
        __debugbreak();
    }
    else if (strcmp(gCrashKind, "abort") == 0)
    {
        abort();
    }
    else if (strcmp(gCrashKind, "invalidparam") == 0)
    {
        strcpy_s(0, 1, "x");
    }
    else if (strcmp(gCrashKind, "fastfail") == 0)
    {
        __fastfail(7);
    }
    else if (strcmp(gCrashKind, "delayload") == 0)
    {
        SteamAPI_RunCallbacks();
    }
}

#ifdef OPENSWBF2_TEST
#include "SaveFsm.h"
#include "GameLoop.h"
#include "PebbleFL/Common/PblUnicode.h"

namespace SaveGameManager
{
extern SaveFsm mSaveFsm;
extern void (*mOnDone)();
bool GetSuccess();
void ResetFileList();
unsigned short const* GetNextFileName(int iType, int* piArg);
}

extern unsigned int CalcCRC(void const* pData, unsigned int uiSize, unsigned int uiSeed);

static const char kSaveTestCanon[] =
    "local function canon(v) local t = type(v) if t == 'table' then local p = {} "
    "for k, x in pairs(v) do table.insert(p, canon(k) .. '=' .. canon(x)) end table.sort(p) "
    "return '{' .. table.concat(p, ',') .. '}' elseif t == 'string' then return string.format('%q', v) end "
    "return t .. ':' .. tostring(v) end ";

static const char kSaveTestWriteCampaign[] =
    "local a = { true, 2.5, 'planet', { res = 40, [3] = 'three', nested = { deep = 7 } } } "
    "ScriptCB_SaveCampaignState(a[1], a[2], a[3], a[4]) "
    "ScriptCB_StartSaveCampaign(nil, ScriptCB_tounicode('SaveTest')) return canon(a), table.getn(a)";

static const char kSaveTestLoadCampaign[] = "local a = { ScriptCB_LoadCampaignState() } return canon(a), table.getn(a)";

enum SaveTestStep
{
    kSaveTestIdle,
    kSaveTestPreOp,
    kSaveTestProfile,
    kSaveTestCampaign,
    kSaveTestLoad,
    kSaveTestDone
};

static int siSaveTestStep;
static bool sbSaveTestPending;
static bool sbSaveTestSuccess;
static unsigned int suSaveTestFields;
static char sacSaveTestFile[128];

static bool SaveTestRun(const char* pcPrefix, const char* pcChunk, unsigned int* puDigest, int* piCount)
{
    char acChunk[1024];
    _snprintf(acChunk, sizeof(acChunk) - 1, "%s%s", pcPrefix, pcChunk);
    acChunk[sizeof(acChunk) - 1] = 0;
    lua_State* L = LuaHelper::mState;
    const int iTop = lua_gettop(L);
    const bool bOk = luaL_loadbuffer(L, acChunk, strlen(acChunk), "=savetest") == 0 && lua_pcall(L, 0, 2, 0) == 0;
    if (!bOk)
    {
        const char* pcErr = lua_tostring(L, -1);
        LOG(LogCore, Warning, "OPENSWBF2 SAVETEST lua error: %s", pcErr ? pcErr : "?");
    }
    else if (puDigest)
    {
        const char* pcText = lua_tostring(L, iTop + 1);
        *puDigest = pcText ? CalcCRC(pcText, (unsigned int)lua_strlen(L, iTop + 1), 0) : 0;
        *piCount = (int)lua_tonumber(L, iTop + 2);
    }
    lua_settop(L, iTop);
    return bOk;
}

static void SaveTestCall(const char* pcFunc, const unsigned short* pusArg)
{
    lua_State* L = LuaHelper::mState;
    const int iTop = lua_gettop(L);
    lua_pushstring(L, pcFunc);
    lua_gettable(L, LUA_GLOBALSINDEX);
    LuaHelper::PushString(pusArg);
    if (lua_pcall(L, 1, 0, 0) != 0)
    {
        const char* pcErr = lua_tostring(L, -1);
        LOG(LogCore, Warning, "OPENSWBF2 SAVETEST lua error: %s", pcErr ? pcErr : "?");
    }
    lua_settop(L, iTop);
}

static void SaveTestQueued()
{
    sbSaveTestPending = SaveGameManager::mOnDone != 0;
    sbSaveTestSuccess = false;
}

static unsigned int SaveTestProfileFields()
{
    PlayerProfile::tProfile kCopy;
    memcpy(&kCopy, PlayerProfile::GetProfile(0), sizeof(kCopy));
    kCopy.mIsDirty = false;
    kCopy.mCRC = 0;
    return CalcCRC(&kCopy, sizeof(kCopy), 0);
}

static bool SaveTestFindCampaign(const char* pcName)
{
    unsigned short ausWant[16];
    PblUnicode::ustrncnv(ausWant, pcName, 16);
    int iFound = 0;
    int iArg = 0;
    SaveGameManager::ResetFileList();
    for (unsigned short const* pusFile = SaveGameManager::GetNextFileName(3, &iArg); pusFile;
         pusFile = SaveGameManager::GetNextFileName(3, &iArg))
    {
        unsigned short ausName[128];
        SaveFile::SplitMetagameFilename(pusFile, ausName, 0, 0, 0, 0);
        if (PblUnicode::ustrcmp(ausName, ausWant) == 0)
        {
            ++iFound;
            PblUnicode::ustrncnv(sacSaveTestFile, pusFile, sizeof(sacSaveTestFile));
        }
    }
    if (iFound != 1)
    {
        LOG(LogCore, Warning, "OPENSWBF2 SAVETEST found %d campaign saves named %s", iFound, pcName);
        strcpy(sacSaveTestFile, pcName);
    }
    return iFound == 1;
}

static void SaveTestLogWrite(int iType, const char* pcName, const char* pcExt)
{
    char acPath[160];
    _snprintf(acPath, sizeof(acPath) - 1, "SaveGames\\%s.%s", pcName, pcExt);
    acPath[sizeof(acPath) - 1] = 0;
    long lBytes = 0;
    unsigned char aucCrc[4] = {};
    FILE* pFile = fopen(acPath, "rb");
    const bool bFile = pFile != 0;
    if (bFile)
    {
        if (fread(aucCrc, 1, sizeof(aucCrc), pFile) != sizeof(aucCrc))
        {
            memset(aucCrc, 0, sizeof(aucCrc));
        }
        fseek(pFile, 0, SEEK_END);
        lBytes = ftell(pFile);
        fclose(pFile);
    }
    const unsigned int uCrc = aucCrc[0] | (aucCrc[1] << 8) | (aucCrc[2] << 16) | ((unsigned int)aucCrc[3] << 24);
    LOG(LogCore, Inform, "OPENSWBF2 SAVETEST write type=%d name=%s bytes=%ld crc=0x%08X fields=0x%08X result=%s", iType,
        pcName, lBytes, uCrc, suSaveTestFields, sbSaveTestSuccess && bFile ? "ok" : "fail");
}

static void SaveTestStartLoad()
{
    if (gSaveTestType == 1)
    {
        unsigned short ausName[16];
        PblUnicode::ustrncnv(ausName, gSaveTestName, 16);
        strcpy(sacSaveTestFile, gSaveTestName);
        SaveTestCall("ScriptCB_StartLoadProfile", ausName);
    }
    else
    {
        SaveTestFindCampaign(gSaveTestName);
        unsigned short ausFile[128];
        PblUnicode::ustrncnv(ausFile, sacSaveTestFile, 128);
        SaveTestCall("ScriptCB_StartLoadCampaign", ausFile);
    }
    SaveTestQueued();
}

static void SaveTestLogLoad()
{
    bool bOk = sbSaveTestSuccess;
    unsigned int uFields = 0;
    if (gSaveTestType == 1)
    {
        uFields = SaveTestProfileFields();
    }
    else
    {
        int iItems = 0;
        SaveTestRun(kSaveTestCanon, kSaveTestLoadCampaign, &uFields, &iItems);
        bOk = bOk && iItems > 0;
    }
    if (bOk)
    {
        LOG(LogCore, Inform, "OPENSWBF2 SAVETEST load type=%d name=%s result=ok fields=0x%08X", gSaveTestType,
            sacSaveTestFile, uFields);
    }
    else
    {
        LOG(LogCore, Inform, "OPENSWBF2 SAVETEST load type=%d name=%s result=reject fields=none", gSaveTestType,
            sacSaveTestFile);
    }
}

static void SaveTestFrame()
{
    if (gSaveTestMode == 0 || siSaveTestStep == kSaveTestDone || GameState::GetState() != 0x11E1FC01)
    {
        return;
    }
    SaveFsm& rFsm = SaveGameManager::mSaveFsm;
    if (sbSaveTestPending)
    {
        if (SaveGameManager::mOnDone != 0)
        {
            if (!rFsm.IsDone() && rFsm.mLoadSavePopupAnswer == -1 && rFsm.mBtns != 0)
            {
                char acAnswer[64];
                _snprintf(acAnswer, sizeof(acAnswer) - 1, "ScriptCB_LoadSavePopupResult(%u)",
                          (unsigned int)rFsm.mBtnDefault);
                acAnswer[sizeof(acAnswer) - 1] = 0;
                LOG(LogCore, Inform, "OPENSWBF2 SAVETEST popup answer=%u", (unsigned int)rFsm.mBtnDefault);
                SaveTestRun("", acAnswer, 0, 0);
            }
            return;
        }
        sbSaveTestPending = false;
        sbSaveTestSuccess = SaveGameManager::GetSuccess();
    }
    switch (siSaveTestStep)
    {
    case kSaveTestIdle:
        if (!rFsm.IsDone() || SaveGameManager::mOnDone != 0)
        {
            return;
        }
        siSaveTestStep = kSaveTestPreOp;
        SaveTestRun("", gSaveTestMode == 1 ? "ScriptCB_StartPreOp(false, ScriptCB_tounicode('savetest'), true)"
                                           : "ScriptCB_StartPreOp(true, ScriptCB_tounicode('savetest'), true)",
                    0, 0);
        SaveTestQueued();
        return;
    case kSaveTestPreOp:
        if (!sbSaveTestSuccess)
        {
            LOG(LogCore, Warning, "OPENSWBF2 SAVETEST preop failed");
        }
        if (gSaveTestMode == 2)
        {
            siSaveTestStep = kSaveTestLoad;
            SaveTestStartLoad();
            return;
        }
        siSaveTestStep = kSaveTestProfile;
        SaveTestRun("", "ScriptCB_AddProfile(ScriptCB_tounicode('SaveTest'))", 0, 0);
        suSaveTestFields = SaveTestProfileFields();
        SaveTestRun("", "ScriptCB_StartSaveProfile(1)", 0, 0);
        SaveTestQueued();
        return;
    case kSaveTestProfile:
    {
        SaveTestLogWrite(1, "SaveTest", "profile");
        siSaveTestStep = kSaveTestCampaign;
        suSaveTestFields = 0;
        int iItems = 0;
        SaveTestRun(kSaveTestCanon, kSaveTestWriteCampaign, &suSaveTestFields, &iItems);
        SaveTestQueued();
        return;
    }
    case kSaveTestCampaign:
        SaveTestFindCampaign("SaveTest");
        SaveTestLogWrite(3, sacSaveTestFile, "rote");
        break;
    case kSaveTestLoad:
        SaveTestLogLoad();
        break;
    default:
        return;
    }
    siSaveTestStep = kSaveTestDone;
    GameLoop::PostRunComplete();
}

bool pcCrash::HandsOffRequested()
{
    static int siHandsOff = -1;
    if (siHandsOff < 0)
    {
        int iLen = 0;
        siHandsOff = FindArg(GetCommandLineA(), "-handsoff", &iLen) != 0 ? 1 : 0;
    }
    return siHandsOff == 1;
}
#endif
