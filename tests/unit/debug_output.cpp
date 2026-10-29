#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <string.h>
#include <string>

namespace
{

int CountOf(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    const char* pc = sText.c_str();
    while ((pc = strstr(pc, pcNeedle)) != 0)
    {
        ++iCount;
        pc += strlen(pcNeedle);
    }
    return iCount;
}

}

TEST_CASE("debugger output mirrors the log" * doctest::test_suite("regression"))
{
    char acExe[MAX_PATH];
    DWORD uLen = GetModuleFileNameA(0, acExe, sizeof(acExe));
    REQUIRE(uLen > 0);
    REQUIRE(uLen < sizeof(acExe));

    char acCmd[MAX_PATH + 64];
    _snprintf(acCmd, sizeof(acCmd) - 1, "\"%s\" --debug-output-child", acExe);
    acCmd[sizeof(acCmd) - 1] = 0;

    char acTemp[MAX_PATH];
    REQUIRE(GetTempPathA(MAX_PATH, acTemp) > 0);
    std::string sDir = std::string(acTemp) + "owb2_dbgout";
    CreateDirectoryA(sDir.c_str(), 0);

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
    REQUIRE(CreateProcessA(0, acCmd, 0, 0, TRUE, DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, 0, sDir.c_str(), &si, &pi));
    CloseHandle(hWrite);

    std::string sOut;
    bool bExited = false;
    bool bTimedOut = false;
    bool bSawLoaderBreak = false;
    DWORD uExitCode = 0xFFFFFFFF;
    while (!bExited)
    {
        DEBUG_EVENT ev;
        if (!WaitForDebugEvent(&ev, 10000))
        {
            bTimedOut = true;
            break;
        }

        DWORD uContinue = DBG_CONTINUE;
        switch (ev.dwDebugEventCode)
        {
        case OUTPUT_DEBUG_STRING_EVENT:
            if (ev.u.DebugString.fUnicode == 0)
            {
                char acBuf[2048];
                SIZE_T uWant = ev.u.DebugString.nDebugStringLength;
                if (uWant > sizeof(acBuf) - 1)
                {
                    uWant = sizeof(acBuf) - 1;
                }
                SIZE_T uRead = 0;
                if (ReadProcessMemory(pi.hProcess, ev.u.DebugString.lpDebugStringData, acBuf, uWant, &uRead))
                {
                    acBuf[uRead] = 0;
                    sOut += acBuf;
                }
            }
            break;
        case CREATE_PROCESS_DEBUG_EVENT:
            if (ev.u.CreateProcessInfo.hFile)
            {
                CloseHandle(ev.u.CreateProcessInfo.hFile);
            }
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (ev.u.LoadDll.hFile)
            {
                CloseHandle(ev.u.LoadDll.hFile);
            }
            break;
        case EXCEPTION_DEBUG_EVENT:
            if (!bSawLoaderBreak && ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
            {
                bSawLoaderBreak = true;
            }
            else
            {
                uContinue = DBG_EXCEPTION_NOT_HANDLED;
            }
            break;
        case EXIT_PROCESS_DEBUG_EVENT:
            uExitCode = ev.u.ExitProcess.dwExitCode;
            bExited = true;
            break;
        }
        ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, uContinue);
    }

    if (bTimedOut)
    {
        TerminateProcess(pi.hProcess, 1);
        DebugActiveProcessStop(pi.dwProcessId);
    }
    WaitForSingleObject(pi.hProcess, 5000);

    std::string sStdOut;
    char acPipe[4096];
    DWORD uGot = 0;
    while (ReadFile(hRead, acPipe, sizeof(acPipe), &uGot, 0) && uGot > 0)
    {
        sStdOut.append(acPipe, uGot);
    }
    CloseHandle(hRead);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DeleteFileA((sDir + "\\BFront2.log").c_str());
    RemoveDirectoryA(sDir.c_str());

    REQUIRE_FALSE(bTimedOut);
    CHECK(uExitCode == 0);
    CHECK(CountOf(sOut, "LogCore Inform x.cpp:1: debug-output-marker 7") == 1);
    CHECK(CountOf(sOut, "pbltrace-marker") == 1);
    CHECK(CountOf(sOut, "LogCore Verbose PblTrace.cpp:0: pbltrace-marker") == 1);
    CHECK(CountOf(sOut, "legacy-marker") == 1);
    CHECK(CountOf(sOut, "net-filtered-marker") == 0);
    CHECK(CountOf(sStdOut, "LogCore Inform x.cpp:1: debug-output-marker 7") == 1);
    CHECK(CountOf(sStdOut, "LogCore Verbose PblTrace.cpp:0: pbltrace-marker") == 1);
    CHECK(CountOf(sStdOut, "legacy-marker") == 1);
    CHECK(CountOf(sStdOut, "net-filtered-marker") == 0);
}
