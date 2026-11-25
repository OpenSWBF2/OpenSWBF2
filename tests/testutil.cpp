#include <windows.h>
#include <string.h>
#include <io.h>
#include <sys/stat.h>

#include "testutil.h"
#include "RedEngineFL/Util/RedWarning.h"
#include "RedEngineFL/Util/RedLog.h"

std::vector<std::string> LogCapture::sLines;

LogCapture::LogCapture()
    : mpOldCallback(RedWarning::g_fnLogCallback)
    , meOldSeverity((int)RedWarning::GetSeverityToDestination(RED_LOG_DESTINATION_INDEX_CALLBACK))
{
    sLines.clear();
    RedWarning::g_fnLogCallback = &LogCapture::Sink;
    RedWarning::SetSeverityToDestination(RED_LOG_DESTINATION_INDEX_CALLBACK, RedWarning::RED_SEVERITY_VERBOSE);
}

LogCapture::~LogCapture()
{
    RedWarning::SetSeverityToDestination(RED_LOG_DESTINATION_INDEX_CALLBACK,
                                         (RedWarning::RED_ERROR_SEVERITY)meOldSeverity);
    RedWarning::g_fnLogCallback = mpOldCallback;
}

bool LogCapture::Contains(const char* pcText) const
{
    for (size_t i = 0; i < sLines.size(); ++i)
    {
        if (sLines[i].find(pcText) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

int LogCapture::Count(const char* pcText) const
{
    int iCount = 0;
    for (size_t i = 0; i < sLines.size(); ++i)
    {
        if (sLines[i].find(pcText) != std::string::npos)
        {
            ++iCount;
        }
    }
    return iCount;
}

void __cdecl LogCapture::Sink(char const* pcLine, ...)
{
    sLines.push_back(pcLine);
}

static bool DirExists(const std::string& sPath)
{
    DWORD dwAttr = GetFileAttributesA(sPath.c_str());
    return dwAttr != INVALID_FILE_ATTRIBUTES && (dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static bool ReadInstallLocation(REGSAM samView, std::string& sOut)
{
    HKEY hKey = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 6060",
                      0, KEY_READ | samView, &hKey) != ERROR_SUCCESS)
    {
        return false;
    }
    char acBuf[MAX_PATH] = {0};
    DWORD dwSize = sizeof(acBuf) - 1;
    DWORD dwType = 0;
    LONG lResult = RegQueryValueExA(hKey, "InstallLocation", 0, &dwType, (LPBYTE)acBuf, &dwSize);
    RegCloseKey(hKey);
    if (lResult != ERROR_SUCCESS || dwType != REG_SZ)
    {
        return false;
    }
    acBuf[dwSize < sizeof(acBuf) ? dwSize : sizeof(acBuf) - 1] = 0;
    sOut = acBuf;
    return !sOut.empty();
}

const char* FindGameDataRoot()
{
    static std::string sRoot;
    static int iState = 0;
    if (iState == 0)
    {
        iState = 2;
        const char* pcEnv = getenv("SWBF2_GAMEDATA");
        std::string sCandidate;
        if (pcEnv && *pcEnv)
        {
            sCandidate = pcEnv;
        }
        else if (ReadInstallLocation(KEY_WOW64_64KEY, sCandidate) || ReadInstallLocation(KEY_WOW64_32KEY, sCandidate))
        {
            sCandidate += "\\GameData";
        }
        if (!sCandidate.empty() && DirExists(sCandidate + "\\data\\_lvl_pc"))
        {
            sRoot = sCandidate;
            iState = 1;
        }
    }
    return iState == 1 ? sRoot.c_str() : 0;
}

void SkipCase(const char* pcReason)
{
    printf("[SKIP] %s: %s\n", gCurrentCase, pcReason);
    fflush(stdout);
    _exit(77);
}

void HarnessError(const char* pcReason)
{
    printf("[HARNESS] %s: %s\n", gCurrentCase, pcReason);
    fflush(stdout);
    _exit(3);
}

std::string RepoRoot()
{
    char acExe[MAX_PATH];
    DWORD uLen = GetModuleFileNameA(0, acExe, sizeof(acExe));
    if (uLen == 0 || uLen >= sizeof(acExe))
    {
        return std::string();
    }
    std::string sPath(acExe, uLen);
    for (int i = 0; i < 5; ++i)
    {
        size_t uiSlash = sPath.find_last_of("\\/");
        if (uiSlash == std::string::npos)
        {
            return std::string();
        }
        sPath.erase(uiSlash);
    }
    return sPath;
}

bool ReadRepoText(const char* pcRelPath, std::string& sOut)
{
    std::string sPath = RepoRoot();
    if (sPath.empty())
    {
        return false;
    }
    sPath += "\\";
    sPath += pcRelPath;
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return false;
    }
    sOut.clear();
    char acBuf[4096];
    size_t uiRead;
    while ((uiRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        for (size_t i = 0; i < uiRead; ++i)
        {
            if (acBuf[i] != '\r')
            {
                sOut += acBuf[i];
            }
        }
    }
    fclose(pFile);
    return true;
}

static std::string ChildFail(const char* pcPrefix, const std::string& sExe, const std::string& sReason)
{
    return std::string(pcPrefix) + ": " + sExe + ": " + sReason;
}

static std::string ChildLogText(const std::string& sPath)
{
    std::string sText;
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return sText;
    }
    char acBuf[4096];
    size_t uRead;
    while (sText.size() < 4096 && (uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    while (!sText.empty() && (sText[sText.size() - 1] == '\n' || sText[sText.size() - 1] == '\r'))
    {
        sText.erase(sText.size() - 1);
    }
    return sText;
}

static unsigned long long FileBytes(const std::string& sPath)
{
    WIN32_FILE_ATTRIBUTE_DATA kData;
    if (!GetFileAttributesExA(sPath.c_str(), GetFileExInfoStandard, &kData))
    {
        return 0;
    }
    return ((unsigned long long)kData.nFileSizeHigh << 32) | kData.nFileSizeLow;
}

bool RunWin32Child(const char* pcMode, const char* pcPrefix, std::string& sExe, std::string& sOutPath,
                   std::string& sWhy)
{
    sExe.clear();
    sOutPath.clear();
    sWhy.clear();
    char acSelf[MAX_PATH];
    const DWORD uSelf = GetModuleFileNameA(0, acSelf, sizeof(acSelf));
    const std::string sRoot = RepoRoot();
    if (uSelf == 0 || uSelf >= sizeof(acSelf) || sRoot.empty())
    {
        sWhy = ChildFail(pcPrefix, "OpenSWBF2Tests.exe", "cannot resolve the tests exe path");
        return false;
    }
    std::string sDir(acSelf, uSelf);
    sDir.erase(sDir.find_last_of("\\/"));
    const std::string sConfig = sDir.substr(sDir.find_last_of("\\/") + 1);
    sExe = sRoot + "\\bin\\Tests\\Win32\\" + sConfig + "\\OpenSWBF2Tests.exe";
    if (GetFileAttributesA(sExe.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        sWhy = ChildFail(pcPrefix, sExe, "missing");
        return false;
    }
    char acTemp[MAX_PATH];
    const DWORD uTemp = GetTempPathA(sizeof(acTemp), acTemp);
    if (uTemp == 0 || uTemp >= sizeof(acTemp))
    {
        sWhy = ChildFail(pcPrefix, sExe, "no temp directory");
        return false;
    }
    char acName[96];
    _snprintf(acName, sizeof(acName), "owb2_%s_%lu", pcMode, GetCurrentProcessId());
    acName[sizeof(acName) - 1] = 0;
    const std::string sOut = std::string(acTemp) + acName + ".bin";
    const std::string sLog = std::string(acTemp) + acName + ".log";
    DeleteFileA(sOut.c_str());
    DeleteFileA(sLog.c_str());

    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hLog = CreateFileA(sLog.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, &sa, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_TEMPORARY, 0);
    if (hLog == INVALID_HANDLE_VALUE)
    {
        sWhy = ChildFail(pcPrefix, sExe, "cannot create " + sLog);
        return false;
    }
    HANDLE hJob = CreateJobObjectA(0, 0);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION kLimit;
    memset(&kLimit, 0, sizeof(kLimit));
    kLimit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!hJob || !SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &kLimit, sizeof(kLimit)))
    {
        CloseHandle(hLog);
        if (hJob)
        {
            CloseHandle(hJob);
        }
        sWhy = ChildFail(pcPrefix, sExe, "cannot create a job object");
        return false;
    }

    std::string sCmd = "\"" + sExe + "\" \"--" + pcMode + "=" + sOut + "\"";
    std::vector<char> cmd(sCmd.begin(), sCmd.end());
    cmd.push_back(0);
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hLog;
    si.hStdError = hLog;
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    const BOOL bStarted =
        CreateProcessA(0, &cmd[0], 0, 0, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, 0, 0, &si, &pi);
    CloseHandle(hLog);
    if (!bStarted)
    {
        CloseHandle(hJob);
        DeleteFileA(sLog.c_str());
        char acErr[32];
        _snprintf(acErr, sizeof(acErr), "cannot start (%lu)", GetLastError());
        acErr[sizeof(acErr) - 1] = 0;
        sWhy = ChildFail(pcPrefix, sExe, acErr);
        return false;
    }
    if (!AssignProcessToJobObject(hJob, pi.hProcess))
    {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(hJob);
        DeleteFileA(sLog.c_str());
        sWhy = ChildFail(pcPrefix, sExe, "cannot assign the job object");
        return false;
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    WatchdogArm(pcPrefix);
    unsigned long long uSeen = 0;
    while (WaitForSingleObject(pi.hProcess, 250) == WAIT_TIMEOUT)
    {
        const unsigned long long uNow = FileBytes(sOut);
        if (uNow != uSeen)
        {
            uSeen = uNow;
            WatchdogArm(pcPrefix);
        }
    }
    DWORD uCode = 0xFFFFFFFF;
    GetExitCodeProcess(pi.hProcess, &uCode);
    CloseHandle(pi.hProcess);
    CloseHandle(hJob);
    const std::string sLogText = ChildLogText(sLog);
    DeleteFileA(sLog.c_str());
    if (uCode != 0)
    {
        DeleteFileA(sOut.c_str());
        char acCode[32];
        _snprintf(acCode, sizeof(acCode), "exit 0x%08lX", uCode);
        acCode[sizeof(acCode) - 1] = 0;
        sWhy = ChildFail(pcPrefix, sExe, std::string(acCode) + (sLogText.empty() ? "" : ": " + sLogText));
        return false;
    }
    if (GetFileAttributesA(sOut.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        sWhy = ChildFail(pcPrefix, sExe, "no output file " + sOut);
        return false;
    }
    sOutPath = sOut;
    return true;
}
