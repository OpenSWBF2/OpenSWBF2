#include <windows.h>
#include <string.h>
#include <io.h>
#include <sys/stat.h>

#include "testutil.h"
#include "RedEngineFL/Util/RedWarning.h"

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
