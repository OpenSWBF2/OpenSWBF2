#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>

namespace
{

struct ChildRun
{
    std::string sOut;
    DWORD uCode;
};

ChildRun RunCrashChild(const char* pcKind, const char* pcAsanOptions = 0)
{
    ChildRun kRun;
    kRun.uCode = 0xFFFFFFFF;

    char acExe[MAX_PATH];
    DWORD uLen = GetModuleFileNameA(0, acExe, sizeof(acExe));
    REQUIRE(uLen > 0);
    REQUIRE(uLen < sizeof(acExe));

    char acCmd[MAX_PATH + 64];
    _snprintf(acCmd, sizeof(acCmd) - 1, "\"%s\" --crash-child=%s", acExe, pcKind);
    acCmd[sizeof(acCmd) - 1] = 0;

    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hRead = 0;
    HANDLE hWrite = 0;
    REQUIRE(CreatePipe(&hRead, &hWrite, &sa, 65536));
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    char acOldAsan[1024];
    DWORD uOldAsan = 0;
    if (pcAsanOptions)
    {
        uOldAsan = GetEnvironmentVariableA("ASAN_OPTIONS", acOldAsan, sizeof(acOldAsan));
        REQUIRE(uOldAsan < sizeof(acOldAsan));
        REQUIRE(SetEnvironmentVariableA("ASAN_OPTIONS", pcAsanOptions));
    }
    const BOOL bStarted = CreateProcessA(0, acCmd, 0, 0, TRUE, CREATE_NO_WINDOW, 0, 0, &si, &pi);
    if (pcAsanOptions)
    {
        SetEnvironmentVariableA("ASAN_OPTIONS", uOldAsan ? acOldAsan : 0);
    }
    CloseHandle(hWrite);
    REQUIRE(bStarted);

    char acBuf[4096];
    DWORD uRead = 0;
    while (ReadFile(hRead, acBuf, sizeof(acBuf), &uRead, 0) && uRead > 0)
    {
        kRun.sOut.append(acBuf, uRead);
    }
    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &kRun.uCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return kRun;
}

void CheckCrashLine(const char* pcKind, DWORD uWant, const char* pcAsanOptions = 0)
{
    const ChildRun kRun = RunCrashChild(pcKind, pcAsanOptions);
    char acLine[64];
    _snprintf(acLine, sizeof(acLine) - 1, "[CRASH] crash child code=0x%08X", (unsigned int)uWant);
    acLine[sizeof(acLine) - 1] = 0;
    CHECK(kRun.uCode == uWant);
    CHECK(kRun.sOut.find(acLine) != std::string::npos);
}

}

TEST_CASE_NAMED(FCrashReportTest1, "Unit::CrashReport::harness names an access violation crash", "[EngineFilter]")
{
#if defined(__SANITIZE_ADDRESS__)
    const ChildRun kRun = RunCrashChild("access-violation", "exitcode=66");
    CHECK(kRun.uCode == 66);
    CHECK(kRun.sOut.find("ERROR: AddressSanitizer: access-violation on unknown address") != std::string::npos);
    CHECK(kRun.sOut.find("The signal is caused by a WRITE memory access.") != std::string::npos);
    CHECK(kRun.sOut.find("[CRASH]") == std::string::npos);
    CheckCrashLine("access-violation", 0xC0000005, "exitcode=66:handle_segv=0");
#else
    CheckCrashLine("access-violation", 0xC0000005);
#endif
}

TEST_CASE_NAMED(FCrashReportTest2, "Unit::CrashReport::harness names an invalid parameter crash", "[EngineFilter]")
{
    CheckCrashLine("invalid-parameter", 0xE0000001);
}

TEST_CASE_NAMED(FCrashReportTest3, "Unit::CrashReport::harness names a pure call crash", "[EngineFilter]")
{
    CheckCrashLine("purecall", 0xE0000002);
}

TEST_CASE_NAMED(FCrashReportTest4, "Unit::CrashReport::harness names an abort crash", "[EngineFilter]")
{
    CheckCrashLine("abort", 0xE0000003);
}
