#include <windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "gate/gate_keys.h"
#include "gate/gate_machine.h"
#include "tables/tables_commit.h"
#include "tables/tables_readers.h"
#include "tables/tables_repin.h"
#include "testutil.h"

namespace
{

const char* const kSteamExe = "C:\\Program Files (x86)\\Steam\\steam.exe";

Tables::FileText ReadPathText(const std::string& sPath)
{
    const std::string sRoot = RepoRoot() + "\\";
    if (Gate::StartsWithNoCase(sPath, sRoot))
    {
        std::string sText;
        return ReadRepoBytes(sPath.substr(sRoot.size()), sText) ? Tables::Found(sText) : Tables::Absent();
    }
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return Tables::Absent();
    }
    std::string sText;
    char acBuf[65536];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    return Tables::Found(sText);
}

bool WritePathText(const std::string& sPath, const std::string& sText)
{
    FILE* pFile = fopen(sPath.c_str(), "wb");
    if (!pFile)
    {
        return false;
    }
    const bool bOk = fwrite(sText.data(), 1, sText.size(), pFile) == sText.size();
    return fclose(pFile) == 0 && bOk;
}

void FindCommitLogs(const std::string& sDir, std::vector<std::string>& kTexts)
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + "\\*").c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    const std::string sLeaf = sDir.substr(sDir.find_last_of('\\') + 1);
    do
    {
        const std::string sName = kFind.cFileName;
        if (sName == "." || sName == "..")
        {
            continue;
        }
        if (kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!(kFind.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                FindCommitLogs(sDir + "\\" + sName, kTexts);
            }
        }
        else if (_stricmp(sName.c_str(), "run.log") == 0 && sLeaf == "commit-flat")
        {
            kTexts.push_back(ReadPathText(sDir + "\\" + sName).sText);
        }
    } while (FindNextFileA(hFind, &kFind));
    FindClose(hFind);
}

std::string ArgValue(int argc, char** argv, const char* pcName, bool& bSeen)
{
    const size_t uLen = strlen(pcName);
    std::string sValue;
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], pcName, uLen) == 0 && argv[i][uLen] == '=')
        {
            sValue = argv[i] + uLen + 1;
            bSeen = true;
        }
    }
    return sValue;
}

std::string ArgValue(int argc, char** argv, const char* pcName)
{
    bool bSeen = false;
    return ArgValue(argc, argv, pcName, bSeen);
}

bool HasArg(int argc, char** argv, const char* pcName)
{
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], pcName) == 0)
        {
            return true;
        }
    }
    return false;
}

void PrintLines(const std::vector<std::string>& kLines)
{
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        printf("%s\n", kLines[i].c_str());
    }
    fflush(stdout);
}

std::string FullPathText(const std::string& sPath)
{
    char acFull[MAX_PATH * 4];
    const DWORD uLen = GetFullPathNameA(sPath.c_str(), sizeof(acFull), acFull, 0);
    std::string s = uLen > 0 && uLen < sizeof(acFull) ? std::string(acFull, uLen) : sPath;
    while (!s.empty() && s[s.size() - 1] == '\\')
    {
        s.erase(s.size() - 1);
    }
    return s;
}

std::vector<Tables::ProcessEntry> ProcessList()
{
    std::vector<Tables::ProcessEntry> kOut;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        return kOut;
    }
    PROCESSENTRY32W kEntry;
    kEntry.dwSize = sizeof(kEntry);
    for (BOOL bMore = Process32FirstW(hSnap, &kEntry); bMore; bMore = Process32NextW(hSnap, &kEntry))
    {
        Tables::ProcessEntry k;
        k.uPid = kEntry.th32ProcessID;
        k.sExe = Gate::WideToUtf8(kEntry.szExeFile);
        kOut.push_back(k);
    }
    CloseHandle(hSnap);
    return kOut;
}

bool SteamProcessRunning()
{
    const std::vector<Tables::ProcessEntry> kList = ProcessList();
    for (size_t i = 0; i < kList.size(); ++i)
    {
        if (_stricmp(kList[i].sExe.c_str(), "steam.exe") == 0)
        {
            return true;
        }
    }
    return false;
}

void StartSteamSilent()
{
    std::string sCmd = std::string("\"") + kSteamExe + "\" -silent";
    std::vector<char> kCmd(sCmd.begin(), sCmd.end());
    kCmd.push_back(0);
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    if (CreateProcessA(0, &kCmd[0], 0, 0, FALSE, 0, 0, 0, &si, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

std::string NewStampedDir(const std::string& sParent)
{
    Gate::MakeDirs(sParent);
    HANDLE hMutex = CreateMutexA(0, FALSE, "Local\\OpenSWBF2-stamped-dir");
    if (hMutex)
    {
        WaitForSingleObject(hMutex, INFINITE);
    }
    SYSTEMTIME k;
    GetLocalTime(&k);
    char acBase[32];
    _snprintf(acBase, sizeof(acBase), "%04u%02u%02u-%02u%02u%02u", k.wYear, k.wMonth, k.wDay, k.wHour, k.wMinute,
              k.wSecond);
    acBase[sizeof(acBase) - 1] = 0;
    std::string sName = acBase;
    for (int i = 2; Gate::DirExists(sParent + "\\" + sName); ++i)
    {
        sName = std::string(acBase) + "-" + std::to_string(i);
    }
    const std::string sDir = sParent + "\\" + sName;
    Gate::MakeDirs(sDir);
    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return sDir;
}

}

int RepinMain(int argc, char** argv)
{
    Tables::RepinInput k;
    k.sRoot = RepoRoot();
    k.sRunDir = ArgValue(argc, argv, "--run");
    k.sTable = ArgValue(argc, argv, "--table");
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], "--key=", 6) == 0)
        {
            k.kKeyArgs.push_back(argv[i] + 6);
        }
    }
    k.sBaselineDir = ArgValue(argc, argv, "--baseline");
    k.sExpectedFile = ArgValue(argc, argv, "--expected");
    k.sExe = ArgValue(argc, argv, "--exe");
    if (k.sRunDir.empty() || k.sTable.empty())
    {
        printf("[FAIL] repin - --run=<run folder> and --table=<name> are required\n");
        fflush(stdout);
        return 2;
    }
    k.sRunDir = FullPathText(k.sRunDir);
    k.fnRead = ReadPathText;
    k.fnExeSha = [](const std::string& sPath, std::string& sSha) {
        std::string sError;
        return Gate::FileExists(sPath) && Gate::PeNormalisedSha(sPath, sSha, sError);
    };
    const Tables::RepinResult r = Tables::Repin(k);
    if (r.iExit == 0 && !WritePathText(r.sWritePath, r.sWriteText))
    {
        printf("[FAIL] repin - cannot write %s\n", r.sWritePath.c_str());
        fflush(stdout);
        return 1;
    }
    PrintLines(r.kLines);
    return r.iExit;
}

int CaptureCommitMain(int argc, char** argv)
{
    Tables::CapturePreflight kPre;
    kPre.sPlatform = ArgValue(argc, argv, "--platform");
    kPre.sSteamState = ArgValue(argc, argv, "--steam");
    kPre.bHog = HasArg(argc, argv, "--hog");
    const std::string sOut = ArgValue(argc, argv, "--out");
    if (kPre.sPlatform.empty() || kPre.sSteamState.empty() || sOut.empty())
    {
        printf("[FAIL] capture-commit - --platform, --steam and --out are required\n");
        fflush(stdout);
        return 2;
    }
    const std::string sRoot = RepoRoot();
    kPre.sOutFull = FullPathText(sOut);
    kPre.sBaselineFull = FullPathText(sRoot + "\\tests\\baseline");
    kPre.kProcesses = ProcessList();
    kPre.uSelfPid = GetCurrentProcessId();
    const std::string sWas = Gate::SteamRunning();
    kPre.sSteamRunning = sWas;
    const Tables::CaptureVerdict kVerdict = Tables::CaptureCommitPreflight(kPre);
    if (kVerdict.iExit != 0)
    {
        PrintLines(kVerdict.kLines);
        return kVerdict.iExit;
    }
    Tables::CaptureRun kRun;
    kRun.bStarted = false;
    kRun.iRunCode = -1;
    kRun.sSteamState = kPre.sSteamState;
    kRun.bHog = kPre.bHog;
    kRun.sOutFull = kPre.sOutFull;
    std::string sThrown;
    if (kPre.sSteamState == "Closed" && SteamProcessRunning())
    {
        std::vector<std::string> kArgs(1, "-shutdown");
        Gate::RunProcess(Gate::NewProcessSpec(kSteamExe, kArgs));
        const unsigned long long uDeadline = GetTickCount64() + 3ull * 60ull * 1000ull;
        while (SteamProcessRunning() && GetTickCount64() < uDeadline)
        {
            Sleep(2000);
        }
        if (SteamProcessRunning())
        {
            sThrown = "Steam did not exit within 3 minutes of -shutdown";
        }
    }
    if (sThrown.empty())
    {
        kRun.sSeen = Gate::SteamRunning();
        kRun.sRunDir = NewStampedDir(sRoot + "\\artifacts\\tests");
        std::vector<std::string> kArgs;
        kArgs.push_back("--runner");
        kArgs.push_back("--out=" + kRun.sRunDir);
        kArgs.push_back("--platform=" + kPre.sPlatform);
        kArgs.push_back("--config=Test");
        kArgs.push_back("--exclude=" + sRoot + "\\tests\\excluded.tsv");
        kArgs.push_back("--tags=*[Standard]*");
        kArgs.push_back("--name=Game::Standard::commit-flat");
        kArgs.push_back("--game-dir=" + sRoot + "\\bin\\" + kPre.sPlatform + "\\Test");
        Gate::ProcessSpec kSpec =
            Gate::NewProcessSpec(sRoot + "\\bin\\Tests\\" + kPre.sPlatform + "\\Test\\OpenSWBF2Tests.exe", kArgs);
        kSpec.sWorkDir = sRoot + "\\tests";
        kSpec.bEcho = true;
        const Gate::ProcessResult kRes = Gate::RunProcess(kSpec);
        kRun.bStarted = true;
        kRun.iRunCode = kRes.bStarted ? (int)kRes.uCode : -1;
    }
    if (sWas == "yes" && Gate::SteamRunning() != "yes")
    {
        StartSteamSilent();
        const unsigned long long uDeadline = GetTickCount64() + 5ull * 60ull * 1000ull;
        while (Gate::SteamRunning() != "yes" && GetTickCount64() < uDeadline)
        {
            Sleep(3000);
        }
        printf("capture-commit: Steam restarted, Get-SteamRunning is %s\n", Gate::SteamRunning().c_str());
        fflush(stdout);
    }
    if (!sThrown.empty())
    {
        printf("[FAIL] capture-commit - %s\n", sThrown.c_str());
        fflush(stdout);
        return 1;
    }
    if (kRun.bStarted)
    {
        FindCommitLogs(kRun.sRunDir, kRun.kCommitFlatLogs);
    }
    std::vector<std::string> kGit;
    kGit.push_back("-C");
    kGit.push_back(sRoot);
    kGit.push_back("rev-parse");
    kGit.push_back("--short=8");
    kGit.push_back("HEAD");
    Gate::ProcessSpec kGitSpec = Gate::NewProcessSpec("git", kGit);
    kGitSpec.bMergeErr = false;
    kRun.sShortSha = Gate::TrimText(Gate::RunProcess(kGitSpec).sOutput);
    const Tables::CaptureResult r = Tables::CaptureCommitRows(kRun);
    if (r.bWrite)
    {
        Gate::MakeDirs(kRun.sOutFull);
        if (!WritePathText(r.sCommitPath, r.sCommitText) || !WritePathText(r.sRunPath, r.sRunText))
        {
            printf("[FAIL] capture-commit - cannot write %s\n", r.sCommitPath.c_str());
            fflush(stdout);
            return 1;
        }
    }
    PrintLines(r.kLines);
    return r.iExit;
}
