#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_CONFIG_NO_WINDOWS_SEH
#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "testutil.h"
#include "harness/gamerun.h"
#include "harness/runner.h"
#include "RedEngineFL/Util/RedWarning.h"
#include "RedEngineFL/Util/RedLog.h"
#include "Battlefront2/Platform/Pc/pcMathStartup.h"
#include "Battlefront2/Platform/Pc/pcProgress.h"
#include "Battlefront2/Source/pcAddrSpace.h"

char gCurrentCase[256];
unsigned int gFuzzSeed = 1;
unsigned int gFuzzIter = 0;
unsigned int gFuzzCount = 0;
bool gFuzzDump = false;
char gFuzzOutPath[260];

static char sLabel[256];
static CRITICAL_SECTION sWatchdogLock;

void WatchdogArm(const char* pcLabel)
{
    strncpy(sLabel, pcLabel, sizeof(sLabel) - 1);
    pcProgress::Step();
}

void WatchdogHold(bool bHold)
{
    if (bHold)
    {
        EnterCriticalSection(&sWatchdogLock);
    }
    else
    {
        LeaveCriticalSection(&sWatchdogLock);
    }
}

static DWORD WINAPI WatchdogProc(LPVOID)
{
    for (;;)
    {
        Sleep(50);
        EnterCriticalSection(&sWatchdogLock);
        if (pcProgress::StallRequested())
        {
            printf("[STALL] %s %s\n", gCurrentCase, sLabel);
            fflush(stdout);
            TerminateProcess(GetCurrentProcess(), 124);
        }
        LeaveCriticalSection(&sWatchdogLock);
    }
}

static void CrashLine(unsigned int uiCode)
{
    printf("[CRASH] %s code=0x%08X\n", gCurrentCase, uiCode);
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), uiCode);
}

static LONG WINAPI CrashFilter(EXCEPTION_POINTERS* pInfo)
{
    CrashLine((unsigned int)pInfo->ExceptionRecord->ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void __cdecl InvalidParameterCrash(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
    CrashLine(0xE0000001);
}

static void __cdecl PureCallCrash()
{
    CrashLine(0xE0000002);
}

static void __cdecl AbortCrash(int)
{
    CrashLine(0xE0000003);
}

static int RunCrashChild(const char* pcKind)
{
    if (strcmp(pcKind, "invalid-parameter") == 0)
    {
        _invalid_parameter_noinfo_noreturn();
    }
    if (strcmp(pcKind, "purecall") == 0)
    {
        _purecall();
    }
    if (strcmp(pcKind, "abort") == 0)
    {
        abort();
    }
    if (strcmp(pcKind, "access-violation") == 0)
    {
        *(volatile int*)0 = 1;
    }
    return 2;
}

static char* HogPut(char* p, char* pEnd, const char* pc)
{
    while (*pc && p < pEnd)
    {
        *p++ = *pc++;
    }
    return p;
}

static char* HogHex(char* p, char* pEnd, uintptr_t u)
{
    p = HogPut(p, pEnd, "0x");
    for (int i = (int)sizeof(u) * 8 - 4; i >= 0 && p < pEnd; i -= 4)
    {
        *p++ = "0123456789ABCDEF"[(u >> i) & 15];
    }
    return p;
}

static void RunHogChild()
{
    char acText[512];
    char* const pEnd = acText + sizeof(acText) - 1;
    char* p = acText;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    const uintptr_t uLimit = sizeof(void*) == 8 ? (uintptr_t)0x100000000ull : (uintptr_t)0x80000000u;
    const uintptr_t uHog = pcAddrSpace::HogLimit();
    const uintptr_t uMin = pcAddrSpace::MinRegion();
    const unsigned int uReserved = pcAddrSpace::ReserveBelow(uHog, uMin);
    const unsigned int uFree = pcAddrSpace::FreeRegionsBelow(uLimit, uMin, false);
    unsigned int uCode = 0;
    if (uFree != 0)
    {
        uCode = 10;
    }
    p = HogPut(p, pEnd, "hog limit=");
    p = HogHex(p, pEnd, uLimit);
    p = HogPut(p, pEnd, " hoglimit=");
    p = HogHex(p, pEnd, uHog);
    p = HogPut(p, pEnd, " reserved=");
    p = HogHex(p, pEnd, uReserved);
    p = HogPut(p, pEnd, " free=");
    p = HogHex(p, pEnd, uFree);
    if (sizeof(void*) == 8)
    {
        const unsigned int uGranules = pcAddrSpace::FreeRegionsBelow(uLimit, 0x10000, true);
        void* pVa = VirtualAlloc(0, 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        HANDLE hHeap = HeapCreate(0, 0, 0);
        void* pHeap = hHeap ? HeapAlloc(hHeap, 0, 64) : 0;
        void* pMalloc = malloc(0x400000);
        if (uCode == 0 && uGranules != 0)
        {
            uCode = 11;
        }
        if (uCode == 0 && (uintptr_t)pVa < uLimit)
        {
            uCode = 12;
        }
        if (uCode == 0 && (uintptr_t)pHeap < uLimit)
        {
            uCode = 13;
        }
        if (uCode == 0 && (uintptr_t)pMalloc < uLimit)
        {
            uCode = 14;
        }
        p = HogPut(p, pEnd, " granules=");
        p = HogHex(p, pEnd, uGranules);
        p = HogPut(p, pEnd, " va=");
        p = HogHex(p, pEnd, (uintptr_t)pVa);
        p = HogPut(p, pEnd, " heap=");
        p = HogHex(p, pEnd, (uintptr_t)pHeap);
        p = HogPut(p, pEnd, " malloc=");
        p = HogHex(p, pEnd, (uintptr_t)pMalloc);
    }
    p = HogPut(p, pEnd, " code=");
    p = HogHex(p, pEnd, uCode);
    p = HogPut(p, pEnd, "\n");
    DWORD uWritten = 0;
    WriteFile(hOut, acText, (DWORD)(p - acText), &uWritten, 0);
    TerminateProcess(GetCurrentProcess(), uCode);
}

extern void __cdecl PblTraceP(const char* Format, ...);
int FloatCorpusGenerate(const char* pcOutDir);
int D3dxRefWriteFile(const char* pcPath);
int CrtTrigRefWriteFile(const char* pcPath);
int RunExpectProbe(const char* pcKind);

int main(int argc, char** argv)
{
    PcMathStartup();
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--hog-child") == 0)
        {
            RunHogChild();
        }
        if (strcmp(argv[i], "--debug-output-child") == 0)
        {
            RedWarning::Init();
            RedLog::ParseCmds("all Verbose, LogNet Warning");
            RedLog::Apply("test");
            RedLog::Write(RedLog::LogCore, RedLog::Inform, "x.cpp", 1, "debug-output-marker %d", 7);
            RedLog::Write(RedLog::LogNet, RedLog::Inform, "x.cpp", 2, "net-filtered-marker");
            PblTraceP("pbltrace-marker\n");
            RedWarning::LogMessage("legacy-marker\n");
            return 0;
        }
        if (strncmp(argv[i], "--gen-corpus=", 13) == 0)
        {
            return FloatCorpusGenerate(argv[i] + 13);
        }
    }

    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_error_mode(_OUT_TO_STDERR);
    SetUnhandledExceptionFilter(CrashFilter);
    _set_invalid_parameter_handler(InvalidParameterCrash);
    _set_purecall_handler(PureCallCrash);
    signal(SIGABRT, AbortCrash);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], "--crash-child=", 14) == 0)
        {
            strncpy(gCurrentCase, "crash child", sizeof(gCurrentCase) - 1);
            return RunCrashChild(argv[i] + 14);
        }
        if (strncmp(argv[i], "--d3dx-ref=", 11) == 0)
        {
            strncpy(gCurrentCase, "d3dx-ref child", sizeof(gCurrentCase) - 1);
            return D3dxRefWriteFile(argv[i] + 11);
        }
        if (strncmp(argv[i], "--crt-trig-ref=", 15) == 0)
        {
            strncpy(gCurrentCase, "crt-trig-ref child", sizeof(gCurrentCase) - 1);
            return CrtTrigRefWriteFile(argv[i] + 15);
        }
        if (strcmp(argv[i], "--runner") == 0)
        {
            return RunnerMain(argc, argv);
        }
        if (strncmp(argv[i], "--expect-probe=", 15) == 0)
        {
            strncpy(gCurrentCase, "expect probe", sizeof(gCurrentCase) - 1);
            RedWarning::Init();
            RedLog::gObserver = TestLogObserver;
            RunExpectProbe(argv[i] + 15);
            return TestStrictnessReport(argv[i] + 15) != 0 ? 1 : 0;
        }
    }
    InitializeCriticalSection(&sWatchdogLock);
    pcProgress::Open();

    bool bListing = false;
    for (int i = 1; i < argc; ++i)
    {
        const char* pcArg = argv[i];
        if (strncmp(pcArg, "-tc=", 4) == 0 || strncmp(pcArg, "--test-case=", 12) == 0)
        {
            strncpy(gCurrentCase, strchr(pcArg, '=') + 1, sizeof(gCurrentCase) - 1);
        }
        else if (strncmp(pcArg, "--fuzz-seed=", 12) == 0)
        {
            gFuzzSeed = (unsigned int)strtoul(pcArg + 12, 0, 0);
        }
        else if (strncmp(pcArg, "--fuzz-iter=", 12) == 0)
        {
            gFuzzIter = (unsigned int)strtoul(pcArg + 12, 0, 0);
        }
        else if (strncmp(pcArg, "--fuzz-count=", 13) == 0)
        {
            gFuzzCount = (unsigned int)strtoul(pcArg + 13, 0, 0);
        }
        else if (strcmp(pcArg, "--fuzz-dump") == 0)
        {
            gFuzzDump = true;
        }
        else if (strncmp(pcArg, "--fuzz-out=", 11) == 0)
        {
            strncpy(gFuzzOutPath, pcArg + 11, sizeof(gFuzzOutPath) - 1);
        }
        else if (GameRunArg(pcArg))
        {
        }
        else if (strstr(pcArg, "list-test") || strcmp(pcArg, "-ltc") == 0 || strcmp(pcArg, "-lts") == 0
                 || strcmp(pcArg, "-c") == 0 || strcmp(pcArg, "--count") == 0)
        {
            bListing = true;
        }
    }

    doctest::Context context;
    context.setOption("abort-after", 100);
    context.applyCommandLine(argc, argv);

    if (!bListing)
    {
        RedWarning::Init();
        RedLog::gObserver = TestLogObserver;
        printf("[RUN] %s\n", gCurrentCase);
        fflush(stdout);
        pcProgress::Step();
        CreateThread(0, 0, WatchdogProc, 0, 0, 0);
    }
    int iRc = context.run();
    if (!bListing && TestStrictnessReport(gCurrentCase) != 0)
    {
        return 1;
    }
    return iRc;
}
