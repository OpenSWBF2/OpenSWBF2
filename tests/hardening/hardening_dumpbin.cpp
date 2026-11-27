#include "hardening/hardening_dumpbin.h"

#include <windows.h>

#include <map>
#include <thread>

#include "gate/gate_keys.h"
#include "gate/gate_machine.h"
#include "hardening/hardening_binary.h"
#include "testutil.h"

namespace Hardening
{

bool Dumpbin(const std::string& sPlatform, const std::vector<std::string>& kArgs, DumpbinRun& kRun,
             std::string& sError, const DumpbinChunkFn& fnChunk)
{
    kRun.bStarted = false;
    kRun.uCode = (unsigned long)-1;
    kRun.sText.clear();
    kRun.sErr.clear();
    static std::map<std::string, Gate::VsTools> sTools;
    if (!sTools.count(sPlatform))
    {
        sTools[sPlatform] = Gate::FindVsTools(sPlatform);
    }
    const Gate::VsTools& kTools = sTools[sPlatform];
    if (!kTools.bFound)
    {
        sError = "Visual Studio 2022 with the C++ x86/x64 build tools not found.";
        return false;
    }
    const std::string sLabel = "dumpbin " + Gate::JoinText(kArgs, " ");
    WatchdogArm(sLabel.c_str());
    SECURITY_ATTRIBUTES kSa = {sizeof(kSa), NULL, TRUE};
    HANDLE hOutRead = NULL;
    HANDLE hOutWrite = NULL;
    HANDLE hErrRead = NULL;
    HANDLE hErrWrite = NULL;
    if (!CreatePipe(&hOutRead, &hOutWrite, &kSa, 0))
    {
        sError = "cannot create a pipe for dumpbin";
        return false;
    }
    if (!CreatePipe(&hErrRead, &hErrWrite, &kSa, 0))
    {
        CloseHandle(hOutRead);
        CloseHandle(hOutWrite);
        sError = "cannot create a pipe for dumpbin";
        return false;
    }
    SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hErrRead, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW kStart;
    ZeroMemory(&kStart, sizeof(kStart));
    kStart.cb = sizeof(kStart);
    kStart.dwFlags = STARTF_USESTDHANDLES;
    kStart.hStdInput = NULL;
    kStart.hStdOutput = hOutWrite;
    kStart.hStdError = hErrWrite;
    PROCESS_INFORMATION kProc;
    ZeroMemory(&kProc, sizeof(kProc));
    std::wstring sCmd = Gate::Utf8ToWide(Gate::CommandLine(Gate::JoinPath(kTools.sBin, "dumpbin.exe"), kArgs));
    const std::wstring sWork = Gate::Utf8ToWide(Gate::FullPath(RepoRoot()));
    const BOOL bMade =
        CreateProcessW(NULL, &sCmd[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, sWork.c_str(), &kStart, &kProc);
    const DWORD uMadeError = GetLastError();
    CloseHandle(hOutWrite);
    CloseHandle(hErrWrite);
    if (!bMade)
    {
        CloseHandle(hOutRead);
        CloseHandle(hErrRead);
        sError = "dumpbin did not start: error " + std::to_string((unsigned long)uMadeError);
        return false;
    }
    kRun.bStarted = true;
    std::string& sErrText = kRun.sErr;
    std::thread kErrThread([hErrRead, &sErrText]() {
        char acBuf[4096];
        DWORD uGot = 0;
        while (ReadFile(hErrRead, acBuf, sizeof(acBuf), &uGot, NULL) && uGot > 0)
        {
            sErrText.append(acBuf, uGot);
        }
    });
    std::string sOut;
    char acBuffer[65536];
    DWORD uRead = 0;
    while (ReadFile(hOutRead, acBuffer, sizeof(acBuffer), &uRead, NULL) && uRead > 0)
    {
        WatchdogArm(sLabel.c_str());
        if (fnChunk)
        {
            fnChunk(acBuffer, uRead);
        }
        else
        {
            sOut.append(acBuffer, uRead);
        }
    }
    CloseHandle(hOutRead);
    kErrThread.join();
    CloseHandle(hErrRead);
    WaitForSingleObject(kProc.hProcess, INFINITE);
    DWORD uCode = 0;
    GetExitCodeProcess(kProc.hProcess, &uCode);
    CloseHandle(kProc.hThread);
    CloseHandle(kProc.hProcess);
    kRun.uCode = uCode;
    if (!fnChunk)
    {
        kRun.sText = JoinedLines(sOut);
    }
    return true;
}

}
