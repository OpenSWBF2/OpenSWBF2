#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <dbghelp.h>
#include <intrin.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "pcCrash.h"
#include "GameMemory.h"
#include "LuaHelper.h"
#include "RedEngineFL/Memory/RedMemory.h"

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

static void TerminateNow(int iCode)
{
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
    BOOL bOk = gpfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, eFull, &gDumpInfo,
        0, 0);
    if (!bOk)
    {
        SetFilePointer(hFile, 0, 0, FILE_BEGIN);
        SetEndOfFile(hFile);
        bOk = gpfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal,
            &gDumpInfo, 0, 0);
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

static void WriteFatalLine(EXCEPTION_POINTERS* pInfo)
{
    char acModule[MAX_PATH];
    char acPath[MAX_PATH * 3];
    wchar_t awcModule[MAX_PATH];
    unsigned int uiCode = 0;
    unsigned int uiAddr = 0;
    unsigned int uiOffset = 0;
    HMODULE hModule = 0;
    lstrcpyA(acModule, "unknown");
    lstrcpyA(acPath, "none");
    if (pInfo != 0 && pInfo->ExceptionRecord != 0)
    {
        uiCode = (unsigned int)pInfo->ExceptionRecord->ExceptionCode;
        uiAddr = (unsigned int)(UINT_PTR)pInfo->ExceptionRecord->ExceptionAddress;
    }
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)(UINT_PTR)uiAddr, &hModule) && hModule != 0)
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
            uiOffset = uiAddr - (unsigned int)(UINT_PTR)hModule;
        }
    }
    if (gDumpPath[0] != 0)
    {
        if (WideCharToMultiByte(CP_ACP, 0, gDumpPath, -1, acPath, sizeof(acPath), 0, 0) == 0)
        {
            lstrcpyA(acPath, "none");
        }
    }
    wsprintfA(gFatalLine, "FATAL: exception 0x%08X at 0x%08X (%s+0x%X) build %s dump %s\r\n", uiCode, uiAddr,
        acModule, uiOffset, gBuildId, acPath);
    AppendLog(gFatalLine);
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
    gDumpPath[0] = 0;
    if (ghFaultEvent != 0 && ghDoneEvent != 0)
    {
        gDumpInfo.ThreadId = GetCurrentThreadId();
        gDumpInfo.ExceptionPointers = pInfo;
        gDumpInfo.ClientPointers = FALSE;
        SetEvent(ghFaultEvent);
        WaitForSingleObject(ghDoneEvent, 30000);
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
    }
    else if (iCode == 4)
    {
        AppendLog("FATAL: lua panic, exiting with code 4\r\n");
    }
    _exit(iCode);
}

static char const* FindArg(char const* pcCmd, char const* pcName, int* piLen)
{
    int iNameLen = lstrlenA(pcName);
    for (char const* pc = pcCmd; *pc != 0; ++pc)
    {
        bool bStart = (pc == pcCmd) || pc[-1] == ' ' || pc[-1] == '"';
        if (bStart && _strnicmp(pc, pcName, iNameLen) == 0 && (pc[iNameLen] == ' ' || pc[iNameLen] == 0))
        {
            char const* pcVal = pc + iNameLen;
            while (*pcVal == ' ')
            {
                ++pcVal;
            }
            char cEnd = ' ';
            if (*pcVal == '"')
            {
                cEnd = '"';
                ++pcVal;
            }
            int iLen = 0;
            while (pcVal[iLen] != 0 && pcVal[iLen] != cEnd)
            {
                ++iLen;
            }
            *piLen = iLen;
            return pcVal;
        }
    }
    return 0;
}

bool pcCrash::SimulateAllocFailure(char const* pcKind)
{
    int iLen = 0;
    char const* pcVal = FindArg(GetCommandLineA(), "-crashtest", &iLen);
    return pcVal != 0 && iLen == lstrlenA(pcKind) && _strnicmp(pcVal, pcKind, iLen) == 0;
}

void pcCrash::FatalInit(char const* pcWhat, unsigned int uiSize)
{
    if (gLogPath[0] == 0)
    {
        int iLen = 0;
        char const* pcVal = FindArg(GetCommandLineA(), "-log", &iLen);
        if (pcVal != 0 && iLen > 0 && iLen < MAX_PATH)
        {
            memcpy(gLogPath, pcVal, iLen);
            gLogPath[iLen] = 0;
        }
    }
    char acLine[160];
    wsprintfA(acLine, "FATAL: out of memory allocating %s (%u bytes)\r\n", pcWhat, uiSize);
    AppendLog(acLine);
    pcCrash::Exit(1);
}

static int LuaAllocHuge(lua_State* L)
{
    lua_newuserdata(L, 0x04000000);
    return 0;
}

void pcCrash::RunMissionCrashTest()
{
    static bool sbRan;
    if (sbRan || !gbReinstalled)
    {
        return;
    }
    if (strcmp(gCrashKind, "heapfail") == 0)
    {
        sbRan = true;
        RedAllocFromHeap(GameMemory::RunTimeHeap, 0x10000000, 0);
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
}

void pcCrash::SetCrashTest(char const* pcKind)
{
    static char const* const apcKinds[] = { "nullderef", "divzero", "stackoverflow", "debugbreak", "abort",
        "invalidparam", "fastfail", "arenaoom", "debugheapoom", "heapfail",
        "luaoom", "luapanic" };
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

void pcCrash::RunCrashTest()
{
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
}
