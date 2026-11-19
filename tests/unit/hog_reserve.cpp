#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>

namespace
{

struct HogChildRun
{
    std::string sOut;
    DWORD uCode;
};

HogChildRun RunHogChild()
{
    HogChildRun kRun;
    kRun.uCode = 0xFFFFFFFF;

    char acExe[MAX_PATH];
    DWORD uLen = GetModuleFileNameA(0, acExe, sizeof(acExe));
    REQUIRE(uLen > 0);
    REQUIRE(uLen < sizeof(acExe));

    char acCmd[MAX_PATH + 64];
    _snprintf(acCmd, sizeof(acCmd) - 1, "\"%s\" --hog-child", acExe);
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
    const BOOL bStarted = CreateProcessA(0, acCmd, 0, 0, TRUE, CREATE_NO_WINDOW, 0, 0, &si, &pi);
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

}

TEST_CASE("hog reserves every region below the platform limit" * doctest::test_suite("regression"))
{
    const HogChildRun kRun = RunHogChild();
    INFO("hog child exit=" << kRun.uCode << " output: " << kRun.sOut);
    REQUIRE(kRun.uCode == 0);
}
