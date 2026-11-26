#include <windows.h>
#include <ctype.h>
#include <netfw.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <winevt.h>

#include <algorithm>
#include <filesystem>
#include <system_error>

#include "gate/gate_machine.h"

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "wevtapi.lib")

namespace Gate
{

namespace
{

const char* const kWorkerKinds[] = {"smoke"};
const char* const kWin32WorkerFiles[] = {"BattlefrontII.exe",    "COPYING.LGPLv2.1", "d3dx9_43.dll",
                                         "Galaxy.dll",           "GalaxyPeer.dll",   "NOTICE.txt",
                                         "openswbf2-ffmpeg.dll", "steam_api.dll",    "steam_appid.txt"};
const char* const kX64WorkerFiles[] = {"BattlefrontII.exe",    "COPYING.LGPLv2.1", "d3dx9_43.dll",   "NOTICE.txt",
                                       "openswbf2-ffmpeg.dll", "steam_api64.dll",  "steam_appid.txt"};
const char* const kGalaxyOffGroups[] = {"Vendor", "None"};

const wchar_t* const kWerKeyPath = L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps\\BattlefrontII.exe";
const wchar_t* const kWerBasePath = L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting";

std::vector<std::string> Array(const char* const* ppc, size_t uCount)
{
    return std::vector<std::string>(ppc, ppc + uCount);
}

bool Contains(const std::vector<std::string>& kList, const std::string& sValue)
{
    return std::find(kList.begin(), kList.end(), sValue) != kList.end();
}

std::string EnvText(const wchar_t* pcName)
{
    const DWORD uLen = GetEnvironmentVariableW(pcName, NULL, 0);
    if (uLen == 0)
    {
        return std::string();
    }
    std::wstring sOut(uLen, L'\0');
    const DWORD uGot = GetEnvironmentVariableW(pcName, &sOut[0], uLen);
    sOut.resize(uGot);
    return WideToUtf8(sOut);
}

bool RegText(HKEY hRoot, const wchar_t* pcKey, const wchar_t* pcValue, DWORD uView, std::string& sOut)
{
    DWORD uBytes = 0;
    if (RegGetValueW(hRoot, pcKey, pcValue, RRF_RT_REG_SZ | uView, NULL, NULL, &uBytes) != ERROR_SUCCESS || uBytes < 2)
    {
        return false;
    }
    std::wstring sWide(uBytes / 2, L'\0');
    if (RegGetValueW(hRoot, pcKey, pcValue, RRF_RT_REG_SZ | uView, NULL, &sWide[0], &uBytes) != ERROR_SUCCESS)
    {
        return false;
    }
    sWide.resize(wcslen(sWide.c_str()));
    sOut = WideToUtf8(sWide);
    return true;
}

bool RegDword(HKEY hRoot, const wchar_t* pcKey, const wchar_t* pcValue, DWORD uView, DWORD& uOut)
{
    DWORD uBytes = sizeof(uOut);
    return RegGetValueW(hRoot, pcKey, pcValue, RRF_RT_REG_DWORD | uView, NULL, &uOut, &uBytes) == ERROR_SUCCESS;
}

std::string NumberText(unsigned long long uValue)
{
    char acText[32];
    _snprintf(acText, sizeof(acText), "%llu", uValue);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

std::string SignedText(long long iValue)
{
    char acText[32];
    _snprintf(acText, sizeof(acText), "%lld", iValue);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

std::wstring EnvironmentBlock(const KeyPairs& kAdd)
{
    std::vector<std::wstring> kEntries;
    wchar_t* pcEnv = GetEnvironmentStringsW();
    for (const wchar_t* p = pcEnv; p && *p; p += wcslen(p) + 1)
    {
        kEntries.push_back(p);
    }
    if (pcEnv)
    {
        FreeEnvironmentStringsW(pcEnv);
    }
    for (size_t i = 0; i < kAdd.size(); ++i)
    {
        const std::wstring sName = Utf8ToWide(kAdd[i].first);
        const std::wstring sEntry = sName + L"=" + Utf8ToWide(kAdd[i].second);
        bool bReplaced = false;
        for (size_t j = 0; j < kEntries.size(); ++j)
        {
            const size_t uEq = kEntries[j].find(L'=', 1);
            if (uEq != std::wstring::npos && uEq == sName.size() &&
                _wcsnicmp(kEntries[j].c_str(), sName.c_str(), sName.size()) == 0)
            {
                kEntries[j] = sEntry;
                bReplaced = true;
            }
        }
        if (!bReplaced)
        {
            kEntries.push_back(sEntry);
        }
    }
    std::wstring sBlock;
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        sBlock += kEntries[i];
        sBlock += L'\0';
    }
    sBlock += L'\0';
    return sBlock;
}

HANDLE NulHandle(DWORD uAccess)
{
    SECURITY_ATTRIBUTES kSa = {sizeof(kSa), NULL, TRUE};
    return CreateFileW(L"NUL", uAccess, FILE_SHARE_READ | FILE_SHARE_WRITE, &kSa, OPEN_EXISTING, 0, NULL);
}

struct DirEntry
{
    std::string sName;
    DWORD uAttr;
    unsigned long long uSize;
    unsigned long long uTime;
};

bool ListDir(const std::string& sDir, std::vector<DirEntry>& kOut)
{
    kOut.clear();
    WIN32_FIND_DATAW kData;
    HANDLE hFind = FindFirstFileExW(Utf8ToWide(JoinPath(sDir, "*")).c_str(), FindExInfoBasic, &kData,
                                    FindExSearchNameMatch, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return GetLastError() == ERROR_FILE_NOT_FOUND;
    }
    do
    {
        if (wcscmp(kData.cFileName, L".") == 0 || wcscmp(kData.cFileName, L"..") == 0)
        {
            continue;
        }
        DirEntry k;
        k.sName = WideToUtf8(kData.cFileName);
        k.uAttr = kData.dwFileAttributes;
        k.uSize = ((unsigned long long)kData.nFileSizeHigh << 32) | kData.nFileSizeLow;
        k.uTime = ((unsigned long long)kData.ftLastWriteTime.dwHighDateTime << 32) | kData.ftLastWriteTime.dwLowDateTime;
        kOut.push_back(k);
    } while (FindNextFileW(hFind, &kData));
    const bool bOk = GetLastError() == ERROR_NO_MORE_FILES;
    FindClose(hFind);
    return bOk;
}

void ManifestLines(const std::string& sDir, const std::string& sRel, std::vector<std::string>& kLines)
{
    std::vector<DirEntry> kEntries;
    ListDir(sDir, kEntries);
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        const std::string sPath = JoinPath(sDir, kEntries[i].sName);
        const std::string sChild = sRel.empty() ? kEntries[i].sName : sRel + "\\" + kEntries[i].sName;
        if (kEntries[i].uAttr & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!(kEntries[i].uAttr & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                ManifestLines(sPath, sChild, kLines);
            }
            continue;
        }
        kLines.push_back(LowerText(sChild) + "\t" + NumberText(kEntries[i].uSize) + "\t" +
                         NumberText(kEntries[i].uTime + 504911232000000000ull));
    }
}

bool DeleteContents(const std::string& sDir, std::string& sError)
{
    std::vector<DirEntry> kEntries;
    if (!ListDir(sDir, kEntries))
    {
        sError = "cannot list " + sDir;
        return false;
    }
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        const std::string sPath = JoinPath(sDir, kEntries[i].sName);
        const std::wstring sWide = Utf8ToWide(sPath);
        if (kEntries[i].uAttr & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            sError = "REFUSING to remove " + sDir + ": reparse point " + sPath + " appeared";
            return false;
        }
        if (kEntries[i].uAttr & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!DeleteContents(sPath, sError))
            {
                return false;
            }
            SetFileAttributesW(sWide.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (!RemoveDirectoryW(sWide.c_str()))
            {
                sError = "cannot remove folder " + sPath;
                return false;
            }
        }
        else
        {
            SetFileAttributesW(sWide.c_str(), FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFileW(sWide.c_str()))
            {
                sError = "cannot remove file " + sPath;
                return false;
            }
        }
    }
    return true;
}

bool CopyContents(const std::string& sFrom, const std::string& sTo, std::string& sError)
{
    if (!MakeDirs(sTo))
    {
        sError = "cannot create " + sTo;
        return false;
    }
    std::vector<DirEntry> kEntries;
    if (!ListDir(sFrom, kEntries))
    {
        sError = "cannot list " + sFrom;
        return false;
    }
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        const std::string sSrc = JoinPath(sFrom, kEntries[i].sName);
        const std::string sDst = JoinPath(sTo, kEntries[i].sName);
        if (kEntries[i].uAttr & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            sError = "folder " + sFrom + " holds a reparse point";
            return false;
        }
        if (kEntries[i].uAttr & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!CopyContents(sSrc, sDst, sError))
            {
                return false;
            }
        }
        else if (!CopyFileW(Utf8ToWide(sSrc).c_str(), Utf8ToWide(sDst).c_str(), FALSE))
        {
            sError = "cannot copy " + sSrc;
            return false;
        }
    }
    return true;
}

std::string HarnessDataLink(const std::string& sDir)
{
    const std::string sLink = JoinPath(sDir, "data");
    const std::wstring sWide = Utf8ToWide(sLink);
    const DWORD uAttr = GetFileAttributesW(sWide.c_str());
    if (uAttr == INVALID_FILE_ATTRIBUTES || !(uAttr & FILE_ATTRIBUTE_REPARSE_POINT))
    {
        return std::string();
    }
    HANDLE hLink = CreateFileW(sWide.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                               OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hLink == INVALID_HANDLE_VALUE)
    {
        return std::string();
    }
    FILE_ATTRIBUTE_TAG_INFO kTag;
    const BOOL bTag = GetFileInformationByHandleEx(hLink, FileAttributeTagInfo, &kTag, sizeof(kTag));
    CloseHandle(hLink);
    if (!bTag || kTag.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
    {
        return std::string();
    }
    std::error_code kError;
    std::wstring sTarget = std::filesystem::read_symlink(std::filesystem::path(sWide), kError).native();
    if (kError || sTarget.empty())
    {
        return std::string();
    }
    while (!sTarget.empty() && sTarget[sTarget.size() - 1] == L'\\')
    {
        sTarget.erase(sTarget.size() - 1);
    }
    const size_t uSlash = sTarget.find_last_of(L'\\');
    const std::wstring sLeaf = uSlash == std::wstring::npos ? sTarget : sTarget.substr(uSlash + 1);
    return _wcsicmp(sLeaf.c_str(), L"data") == 0 ? sLink : std::string();
}

bool RunGit(const std::string& sDir, const std::vector<std::string>& kArgs, std::string& sOut)
{
    std::vector<std::string> kAll;
    kAll.push_back("-C");
    kAll.push_back(sDir);
    kAll.insert(kAll.end(), kArgs.begin(), kArgs.end());
    ProcessSpec kSpec = NewProcessSpec("git", kAll);
    kSpec.bMergeErr = true;
    const ProcessResult k = RunProcess(kSpec);
    sOut = TrimText(k.sOutput);
    return k.bStarted && k.uCode == 0;
}

int CountEvents(const wchar_t* pcQuery, long long& iCount)
{
    EVT_HANDLE hQuery = EvtQuery(NULL, L"System", pcQuery, EvtQueryChannelPath | EvtQueryForwardDirection);
    if (!hQuery)
    {
        return 0;
    }
    EVT_HANDLE ahEvents[64];
    bool bOk = true;
    for (;;)
    {
        DWORD uGot = 0;
        if (!EvtNext(hQuery, 64, ahEvents, INFINITE, 0, &uGot))
        {
            bOk = GetLastError() == ERROR_NO_MORE_ITEMS;
            break;
        }
        for (DWORD i = 0; i < uGot; ++i)
        {
            EvtClose(ahEvents[i]);
        }
        iCount += uGot;
    }
    EvtClose(hQuery);
    return bOk ? 1 : 0;
}

}

std::string QuoteArg(const std::string& sArg)
{
    std::string sOut = "\"";
    size_t uSlashes = 0;
    for (size_t i = 0; i < sArg.size(); ++i)
    {
        if (sArg[i] == '\\')
        {
            ++uSlashes;
            continue;
        }
        if (sArg[i] == '"')
        {
            sOut.append(uSlashes * 2 + 1, '\\');
        }
        else
        {
            sOut.append(uSlashes, '\\');
        }
        uSlashes = 0;
        sOut += sArg[i];
    }
    sOut.append(uSlashes * 2, '\\');
    sOut += "\"";
    return sOut;
}

std::string CommandLine(const std::string& sExe, const std::vector<std::string>& kArgs)
{
    std::string sOut = QuoteArg(sExe);
    for (size_t i = 0; i < kArgs.size(); ++i)
    {
        sOut += " " + QuoteArg(kArgs[i]);
    }
    return sOut;
}

ProcessSpec NewProcessSpec(const std::string& sExe, const std::vector<std::string>& kArgs)
{
    ProcessSpec k;
    k.sExe = sExe;
    k.kArgs = kArgs;
    k.bEcho = false;
    k.bMergeErr = true;
    return k;
}

ProcessResult RunProcess(const ProcessSpec& kSpec)
{
    ProcessResult kOut;
    kOut.bStarted = false;
    kOut.uCode = (unsigned long)-1;
    SECURITY_ATTRIBUTES kSa = {sizeof(kSa), NULL, TRUE};
    HANDLE hRead = NULL;
    HANDLE hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &kSa, 0))
    {
        kOut.sError = "cannot create a pipe";
        return kOut;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
    HANDLE hIn = NulHandle(GENERIC_READ);
    HANDLE hErr = kSpec.bMergeErr ? hWrite : NulHandle(GENERIC_WRITE);
    STARTUPINFOW kStart;
    ZeroMemory(&kStart, sizeof(kStart));
    kStart.cb = sizeof(kStart);
    kStart.dwFlags = STARTF_USESTDHANDLES;
    kStart.hStdInput = hIn;
    kStart.hStdOutput = hWrite;
    kStart.hStdError = hErr;
    PROCESS_INFORMATION kProc;
    ZeroMemory(&kProc, sizeof(kProc));
    std::wstring sCmd = Utf8ToWide(CommandLine(kSpec.sExe, kSpec.kArgs));
    std::wstring sApp;
    const std::string sLowerExe = LowerText(kSpec.sExe);
    if (sLowerExe.size() > 4 && (sLowerExe.compare(sLowerExe.size() - 4, 4, ".bat") == 0 ||
                                 sLowerExe.compare(sLowerExe.size() - 4, 4, ".cmd") == 0))
    {
        wchar_t acSystem[MAX_PATH];
        const UINT uLen = GetSystemDirectoryW(acSystem, MAX_PATH);
        sApp = std::wstring(acSystem, uLen < MAX_PATH ? uLen : 0) + L"\\cmd.exe";
        sCmd = L"cmd.exe /s /c \"" + sCmd + L"\"";
    }
    const std::wstring sEnv = kSpec.kEnv.empty() ? std::wstring() : EnvironmentBlock(kSpec.kEnv);
    const std::wstring sWork = kSpec.sWorkDir.empty() ? std::wstring() : Utf8ToWide(FullPath(kSpec.sWorkDir));
    const BOOL bMade = CreateProcessW(sApp.empty() ? NULL : sApp.c_str(), &sCmd[0], NULL, NULL, TRUE,
                                      CREATE_UNICODE_ENVIRONMENT,
                                      sEnv.empty() ? NULL : (LPVOID)sEnv.c_str(), sWork.empty() ? NULL : sWork.c_str(),
                                      &kStart, &kProc);
    const DWORD uMadeError = GetLastError();
    CloseHandle(hWrite);
    if (hIn != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hIn);
    }
    if (!kSpec.bMergeErr && hErr != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hErr);
    }
    FILE* pOut = kSpec.sOutFile.empty() ? NULL : _wfopen(Utf8ToWide(kSpec.sOutFile).c_str(), L"wb");
    if (!bMade)
    {
        CloseHandle(hRead);
        char acMsg[64];
        _snprintf(acMsg, sizeof(acMsg), "command failed to start: error %lu", uMadeError);
        acMsg[sizeof(acMsg) - 1] = 0;
        kOut.sError = acMsg;
        if (pOut)
        {
            fprintf(pOut, "gate: %s\r\n", acMsg);
            fclose(pOut);
        }
        return kOut;
    }
    kOut.bStarted = true;
    char acBuffer[4096];
    DWORD uRead = 0;
    while (ReadFile(hRead, acBuffer, sizeof(acBuffer), &uRead, NULL) && uRead > 0)
    {
        kOut.sOutput.append(acBuffer, uRead);
        if (pOut)
        {
            fwrite(acBuffer, 1, uRead, pOut);
        }
        if (kSpec.bEcho)
        {
            fwrite(acBuffer, 1, uRead, stdout);
            fflush(stdout);
        }
    }
    CloseHandle(hRead);
    if (pOut)
    {
        fclose(pOut);
    }
    WaitForSingleObject(kProc.hProcess, INFINITE);
    DWORD uCode = 0;
    GetExitCodeProcess(kProc.hProcess, &uCode);
    kOut.uCode = uCode;
    CloseHandle(kProc.hThread);
    CloseHandle(kProc.hProcess);
    return kOut;
}

bool GitTreeList(const std::string& sRoot, std::vector<std::string>& kPaths, std::string& sError)
{
    std::vector<std::string> kArgs;
    kArgs.push_back("-C");
    kArgs.push_back(sRoot);
    kArgs.push_back("ls-files");
    kArgs.push_back("--cached");
    kArgs.push_back("--others");
    kArgs.push_back("--exclude-standard");
    kArgs.push_back("-z");
    ProcessSpec kSpec = NewProcessSpec("git", kArgs);
    kSpec.bMergeErr = false;
    const ProcessResult k = RunProcess(kSpec);
    if (!k.bStarted || k.uCode != 0)
    {
        sError = "git ls-files --cached --others --exclude-standard -z failed: " +
                 (k.bStarted ? "exit " + NumberText(k.uCode) : k.sError);
        return false;
    }
    kPaths.clear();
    const std::vector<std::string> kParts = SplitText(k.sOutput, '\0');
    for (size_t i = 0; i < kParts.size(); ++i)
    {
        if (!kParts[i].empty())
        {
            kPaths.push_back(kParts[i]);
        }
    }
    return true;
}

const char* const kWorkerPlatforms[] = {"Win32", "x64"};
const char* const kWorkerConfigurations[] = {"Debug", "Release", "Test", "Asan"};

int ProcessorCount()
{
    SYSTEM_INFO kInfo;
    GetSystemInfo(&kInfo);
    return (int)kInfo.dwNumberOfProcessors;
}

int DefaultJobs(int iProcessors)
{
    const int iHalf = iProcessors / 2;
    return (std::max)(1, (std::min)(8, iHalf));
}

bool WorkerPath(const std::string& sRoot, const std::string& sPlatform, const std::string& sConfig,
                const std::string& sKind, int iIndex, std::string& sPath, std::string& sError)
{
    if (sRoot.empty())
    {
        sError = "worker path needs a root";
        return false;
    }
    if (!Contains(Array(kWorkerPlatforms, 2), sPlatform))
    {
        sError = "worker platform '" + sPlatform + "' is not one of Win32, x64";
        return false;
    }
    if (!Contains(Array(kWorkerConfigurations, 4), sConfig))
    {
        sError = "worker configuration '" + sConfig + "' is not one of Debug, Release, Test, Asan";
        return false;
    }
    if (!Contains(Array(kWorkerKinds, 1), sKind))
    {
        sError = "worker kind '" + sKind + "' is not one of smoke";
        return false;
    }
    if (iIndex < 1 || iIndex > ProcessorCount())
    {
        sError = "worker index " + SignedText(iIndex) + " is outside 1.." + SignedText(ProcessorCount());
        return false;
    }
    sPath = JoinPath(sRoot, "artifacts\\smoke-workers\\" + sPlatform + "\\" + sConfig + "\\" + sKind + "\\w" +
                                SignedText(iIndex));
    return true;
}

std::vector<std::string> SmokeWorkerFiles(const std::string& sPlatform)
{
    if (sPlatform == "x64")
    {
        return Array(kX64WorkerFiles, sizeof(kX64WorkerFiles) / sizeof(kX64WorkerFiles[0]));
    }
    return Array(kWin32WorkerFiles, sizeof(kWin32WorkerFiles) / sizeof(kWin32WorkerFiles[0]));
}

std::string AsanRuntimeName(const std::string& sPlatform)
{
    return sPlatform == "x64" ? "clang_rt.asan_dynamic-x86_64.dll" : "clang_rt.asan_dynamic-i386.dll";
}

std::vector<std::string> FirewallRulePaths(const std::string& sRoot, const std::vector<std::string>& kPlatforms,
                                           const std::vector<std::string>& kConfigs, int iWorkers)
{
    std::vector<std::string> kPaths;
    for (size_t p = 0; p < kPlatforms.size(); ++p)
    {
        for (size_t c = 0; c < kConfigs.size(); ++c)
        {
            kPaths.push_back(JoinPath(sRoot, "bin\\" + kPlatforms[p] + "\\" + kConfigs[c] + "\\BattlefrontII.exe"));
            for (int w = 1; w <= iWorkers; ++w)
            {
                kPaths.push_back(JoinPath(sRoot, "artifacts\\smoke-workers\\" + kPlatforms[p] + "\\" + kConfigs[c] +
                                                     "\\smoke\\w" + SignedText(w) + "\\BattlefrontII.exe"));
            }
        }
    }
    return kPaths;
}

std::vector<std::string> StepConfigs(const std::vector<const Step*>& kSteps)
{
    std::vector<std::string> kSet;
    const std::string sHead = "bin\\$platform\\";
    const std::string sTail = "\\battlefrontii.exe";
    for (size_t i = 0; i < kSteps.size(); ++i)
    {
        kSet.push_back(kSteps[i]->sConfig);
        for (size_t h = 0; h < kSteps[i]->kHashes.size(); ++h)
        {
            const std::string& sHash = kSteps[i]->kHashes[h];
            const std::string sLower = LowerText(sHash);
            if (sLower.size() > sHead.size() + sTail.size() && sLower.compare(0, sHead.size(), sHead) == 0 &&
                sLower.compare(sLower.size() - sTail.size(), sTail.size(), sTail) == 0)
            {
                const std::string sMid = sHash.substr(sHead.size(), sHash.size() - sHead.size() - sTail.size());
                if (sMid.find('\\') == std::string::npos)
                {
                    kSet.push_back(sMid);
                }
            }
        }
    }
    std::vector<std::string> kOut;
    for (size_t i = 0; i < 4; ++i)
    {
        if (Contains(kSet, kWorkerConfigurations[i]))
        {
            kOut.push_back(kWorkerConfigurations[i]);
        }
    }
    return kOut;
}

std::vector<std::string> CrossRulePaths(const std::string& sRoot, const std::vector<const Step*>& kSteps,
                                        const std::string& sPlatform, int iWorkers)
{
    std::vector<std::string> kOut;
    const std::string sTail = "\\battlefrontii.exe";
    for (size_t i = 0; i < kSteps.size(); ++i)
    {
        for (size_t h = 0; h < kSteps[i]->kHashes.size(); ++h)
        {
            const std::string& sHash = kSteps[i]->kHashes[h];
            const std::string sLower = LowerText(sHash);
            if (sLower.compare(0, 4, "bin\\") != 0 || sLower.size() <= sTail.size() ||
                sLower.compare(sLower.size() - sTail.size(), sTail.size(), sTail) != 0)
            {
                continue;
            }
            const std::vector<std::string> kParts = SplitText(sHash.substr(4, sHash.size() - 4 - sTail.size()), '\\');
            if (kParts.size() != 2 || kParts[1].empty() ||
                (!EqualNoCase(kParts[0], "Win32") && !EqualNoCase(kParts[0], "x64")))
            {
                continue;
            }
            if (kParts[0] != sPlatform)
            {
                const std::vector<std::string> kMore = FirewallRulePaths(
                    sRoot, std::vector<std::string>(1, kParts[0]), std::vector<std::string>(1, kParts[1]), iWorkers);
                kOut.insert(kOut.end(), kMore.begin(), kMore.end());
            }
        }
    }
    return kOut;
}

std::vector<std::string> PreflightRulePaths(const std::string& sRoot, const std::vector<const Step*>& kSteps,
                                            const std::string& sPlatform, int iWorkers)
{
    std::vector<const Step*> kGame;
    for (size_t i = 0; i < kSteps.size(); ++i)
    {
        if (kSteps[i]->bStartsGame)
        {
            kGame.push_back(kSteps[i]);
        }
    }
    std::vector<std::string> kPaths;
    if (kGame.empty())
    {
        return kPaths;
    }
    kPaths = FirewallRulePaths(sRoot, std::vector<std::string>(1, sPlatform), StepConfigs(kSteps), iWorkers);
    const std::vector<std::string> kCross = CrossRulePaths(sRoot, kGame, sPlatform, iWorkers);
    std::vector<std::string> kAdd;
    for (size_t i = 0; i < kCross.size(); ++i)
    {
        if (!Contains(kAdd, kCross[i]) && !Contains(kPaths, kCross[i]))
        {
            kAdd.push_back(kCross[i]);
        }
    }
    kPaths.insert(kPaths.end(), kAdd.begin(), kAdd.end());
    return kPaths;
}

VsTools FindVsTools(const std::string& sPlatform)
{
    VsTools k;
    k.bFound = false;
    const std::string sVswhere =
        JoinPath(EnvText(L"ProgramFiles(x86)"), "Microsoft Visual Studio\\Installer\\vswhere.exe");
    if (!FileExists(sVswhere))
    {
        return k;
    }
    std::vector<std::string> kArgs;
    kArgs.push_back("-latest");
    kArgs.push_back("-version");
    kArgs.push_back("[17.0,18.0)");
    kArgs.push_back("-requires");
    kArgs.push_back("Microsoft.VisualStudio.Component.VC.Tools.x86.x64");
    kArgs.push_back("-property");
    kArgs.push_back("installationPath");
    ProcessSpec kSpec = NewProcessSpec(sVswhere, kArgs);
    kSpec.bMergeErr = false;
    const ProcessResult kRun = RunProcess(kSpec);
    const std::vector<std::string> kLines = SplitLines(kRun.sOutput);
    if (!kRun.bStarted || kLines.empty() || TrimText(kLines[0]).empty())
    {
        return k;
    }
    k.sInstall = TrimText(kLines[0]);
    std::string sVer;
    if (!ReadTextFile(JoinPath(k.sInstall, "VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt"), sVer))
    {
        return k;
    }
    const std::vector<std::string> kVer = SplitLines(sVer);
    k.sToolsVersion = kVer.empty() ? std::string() : TrimText(kVer[0]);
    k.sBin = JoinPath(k.sInstall, "VC\\Tools\\MSVC\\" + k.sToolsVersion + "\\bin\\HostX86\\" +
                                      (sPlatform == "x64" ? "x64" : "x86"));
    k.sMSBuild = JoinPath(k.sInstall, "MSBuild\\Current\\Bin\\MSBuild.exe");
    k.bFound = DirExists(k.sBin) && FileExists(k.sMSBuild);
    return k;
}

std::string FileVersionText(const std::string& sPath)
{
    const std::wstring sWide = Utf8ToWide(sPath);
    DWORD uHandle = 0;
    const DWORD uSize = GetFileVersionInfoSizeW(sWide.c_str(), &uHandle);
    if (uSize == 0)
    {
        return std::string();
    }
    std::vector<unsigned char> kBlock(uSize);
    if (!GetFileVersionInfoW(sWide.c_str(), 0, uSize, &kBlock[0]))
    {
        return std::string();
    }
    WORD* pTrans = NULL;
    UINT uLen = 0;
    wchar_t acQuery[64] = L"\\StringFileInfo\\040904B0\\FileVersion";
    if (VerQueryValueW(&kBlock[0], L"\\VarFileInfo\\Translation", (LPVOID*)&pTrans, &uLen) && uLen >= 4)
    {
        _snwprintf(acQuery, 64, L"\\StringFileInfo\\%04x%04x\\FileVersion", pTrans[0], pTrans[1]);
        acQuery[63] = 0;
    }
    wchar_t* pcValue = NULL;
    if (!VerQueryValueW(&kBlock[0], acQuery, (LPVOID*)&pcValue, &uLen) || !pcValue)
    {
        return std::string();
    }
    return WideToUtf8(std::wstring(pcValue, wcsnlen(pcValue, uLen)));
}

ToolchainInfo GetToolchainInfo(const std::string& sPlatform)
{
    ToolchainInfo k;
    k.bOk = false;
    const VsTools kTools = FindVsTools(sPlatform);
    if (!kTools.bFound)
    {
        return k;
    }
    k.sToolsVersion = kTools.sToolsVersion;
    const ProcessResult kCl = RunProcess(NewProcessSpec(JoinPath(kTools.sBin, "cl.exe"), std::vector<std::string>()));
    const std::vector<std::string> kLines = SplitLines(kCl.sOutput);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (!TrimText(kLines[i]).empty())
        {
            k.sBanner = kLines[i];
            break;
        }
    }
    const std::string sLower = LowerText(k.sBanner);
    const size_t uAt = sLower.find("version ");
    if (uAt != std::string::npos)
    {
        size_t uEnd = uAt + 8;
        while (uEnd < k.sBanner.size() && !isspace((unsigned char)k.sBanner[uEnd]))
        {
            ++uEnd;
        }
        k.sCl = k.sBanner.substr(uAt + 8, uEnd - uAt - 8);
    }
    const std::string sUcrt = JoinPath(SystemDllDir(sPlatform), "ucrtbase.dll");
    if (FileExists(sUcrt))
    {
        k.sUcrt = FileVersionText(sUcrt);
    }
    std::string sBuild;
    DWORD uUbr = 0;
    RegText(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuild", 0, sBuild);
    RegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"UBR", 0, uUbr);
    k.sWindows = sBuild + "." + NumberText(uUbr);
    k.bOk = true;
    return k;
}

std::string SystemDllDir(const std::string& sPlatform)
{
    const std::string sWindir = EnvText(L"windir");
    if (sPlatform == "Win32")
    {
        return JoinPath(sWindir, "SysWOW64");
    }
    return JoinPath(sWindir, sizeof(void*) == 8 ? "System32" : "Sysnative");
}

std::vector<std::string> T1Runtimes()
{
    std::vector<std::string> k;
    k.push_back("d3dx9_43.dll");
    k.push_back("vcruntime140.dll");
    k.push_back("msvcp140.dll");
    return k;
}

std::vector<std::string> GameRuntimes(const std::string& sPlatform, const std::vector<std::string>& kGroups)
{
    std::vector<std::string> k;
    k.push_back("vcruntime140.dll");
    k.push_back("msvcp140.dll");
    bool bGalaxy = false;
    for (size_t i = 0; i < kGroups.size(); ++i)
    {
        if (!kGroups[i].empty() && !EqualNoCase(kGroups[i], kGalaxyOffGroups[0]) &&
            !EqualNoCase(kGroups[i], kGalaxyOffGroups[1]))
        {
            bGalaxy = true;
        }
    }
    if (EqualNoCase(sPlatform, "Win32") && bGalaxy)
    {
        k.push_back("msvcr120.dll");
        k.push_back("msvcp120.dll");
    }
    return k;
}

std::vector<std::string> PlatformRuntimes(const std::vector<std::string>& kDlls, const std::string& sPlatform)
{
    std::vector<std::string> k = kDlls;
    if (EqualNoCase(sPlatform, "x64"))
    {
        k.push_back("vcruntime140_1.dll");
    }
    return k;
}

std::vector<std::string> MissingRuntimes(const std::string& sDir, const std::vector<std::string>& kDlls)
{
    std::vector<std::string> k;
    for (size_t i = 0; i < kDlls.size(); ++i)
    {
        const std::string sPath = JoinPath(sDir, kDlls[i]);
        if (!FileExists(sPath) && !DirExists(sPath))
        {
            k.push_back(kDlls[i]);
        }
    }
    return k;
}

unsigned long SteamActiveUser()
{
    DWORD uUser = 0;
    if (!RegDword(HKEY_CURRENT_USER, L"Software\\Valve\\Steam\\ActiveProcess", L"ActiveUser", 0, uUser))
    {
        return 0;
    }
    return uUser;
}

std::string SteamRunning()
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    bool bFound = false;
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W kEntry;
        kEntry.dwSize = sizeof(kEntry);
        for (BOOL bMore = Process32FirstW(hSnap, &kEntry); bMore; bMore = Process32NextW(hSnap, &kEntry))
        {
            if (_wcsicmp(kEntry.szExeFile, L"steam.exe") == 0)
            {
                bFound = true;
            }
        }
        CloseHandle(hSnap);
    }
    if (!bFound || SteamActiveUser() == 0)
    {
        return "no";
    }
    return "yes";
}

std::string OverlayFromVdf(const std::string& sText, const std::string& sAppId)
{
    std::vector<std::string> kStack;
    bool bKey = false;
    std::string sKey;
    bool bValue = false;
    std::string sValue;
    size_t i = 0;
    while (i < sText.size())
    {
        const char c = sText[i];
        if (c == '{')
        {
            if (!bKey)
            {
                return "-";
            }
            kStack.push_back(sKey);
            bKey = false;
            ++i;
            continue;
        }
        if (c == '}')
        {
            if (kStack.empty() || bKey)
            {
                return "-";
            }
            kStack.pop_back();
            ++i;
            continue;
        }
        if (c != '"')
        {
            ++i;
            continue;
        }
        size_t j = i + 1;
        bool bClosed = false;
        while (j < sText.size())
        {
            if (sText[j] == '"')
            {
                bClosed = true;
                break;
            }
            if (sText[j] == '\\')
            {
                if (j + 1 >= sText.size() || sText[j + 1] == '\n')
                {
                    break;
                }
                j += 2;
                continue;
            }
            ++j;
        }
        if (!bClosed)
        {
            ++i;
            continue;
        }
        const std::string sToken = sText.substr(i + 1, j - i - 1);
        i = j + 1;
        if (!bKey)
        {
            sKey = sToken;
            bKey = true;
        }
        else
        {
            if (kStack.size() == 3 && EqualNoCase(kStack[1], "apps") && kStack[2] == sAppId &&
                EqualNoCase(sKey, "OverlayAppEnable"))
            {
                sValue = sToken;
                bValue = true;
            }
            bKey = false;
        }
    }
    if (!kStack.empty() || bKey)
    {
        return "-";
    }
    if (!bValue || sValue == "1")
    {
        return "on";
    }
    if (sValue == "0")
    {
        return "off";
    }
    return "-";
}

std::string SteamOverlayState()
{
    std::string sSteam;
    if (!RegText(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", 0, sSteam) || sSteam.empty())
    {
        return "-";
    }
    const unsigned long uUser = SteamActiveUser();
    if (uUser == 0)
    {
        return "-";
    }
    std::replace(sSteam.begin(), sSteam.end(), '/', '\\');
    const std::string sVdf = JoinPath(sSteam, "userdata\\" + NumberText(uUser) + "\\config\\localconfig.vdf");
    std::string sText;
    if (!FileExists(sVdf) || !ReadTextFile(sVdf, sText))
    {
        return "-";
    }
    return OverlayFromVdf(sText, "6060");
}

bool WerKeyPresent()
{
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kWerKeyPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
    {
        return false;
    }
    RegCloseKey(hKey);
    return true;
}

std::vector<std::string> WerWarnings()
{
    std::vector<std::string> k;
    if (!WerKeyPresent())
    {
        k.push_back("WER LocalDumps key for BattlefrontII.exe is missing; import tools\\wer-localdumps.reg as "
                    "administrator");
    }
    bool bDontShow = false;
    const HKEY ahRoots[] = {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER};
    for (size_t i = 0; i < 2; ++i)
    {
        DWORD uValue = 0;
        std::string sValue;
        if (RegDword(ahRoots[i], kWerBasePath, L"DontShowUI", RRF_SUBKEY_WOW6464KEY, uValue) && uValue == 1)
        {
            bDontShow = true;
        }
        else if (RegText(ahRoots[i], kWerBasePath, L"DontShowUI", RRF_SUBKEY_WOW6464KEY, sValue) &&
                 TrimText(sValue) == "1")
        {
            bDontShow = true;
        }
    }
    if (!bDontShow)
    {
        k.push_back("WER DontShowUI is not set; a crash can leave a dialog open (see tools\\wer-localdumps.reg)");
    }
    return k;
}

bool InboundAllowPrograms(std::set<std::string>& kPrograms, std::string& sError)
{
    kPrograms.clear();
    const HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    INetFwPolicy2* pPolicy = NULL;
    INetFwRules* pRules = NULL;
    IUnknown* pEnumUnk = NULL;
    IEnumVARIANT* pEnum = NULL;
    bool bOk = false;
    if (SUCCEEDED(CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2),
                                   (void**)&pPolicy)) &&
        SUCCEEDED(pPolicy->get_Rules(&pRules)) && SUCCEEDED(pRules->get__NewEnum(&pEnumUnk)) &&
        SUCCEEDED(pEnumUnk->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pEnum)))
    {
        bOk = true;
        VARIANT kVar;
        VariantInit(&kVar);
        ULONG uGot = 0;
        while (pEnum->Next(1, &kVar, &uGot) == S_OK && uGot == 1)
        {
            INetFwRule* pRule = NULL;
            if (kVar.vt == VT_DISPATCH && kVar.pdispVal &&
                SUCCEEDED(kVar.pdispVal->QueryInterface(__uuidof(INetFwRule), (void**)&pRule)))
            {
                NET_FW_RULE_DIRECTION eDir = NET_FW_RULE_DIR_OUT;
                NET_FW_ACTION eAction = NET_FW_ACTION_BLOCK;
                VARIANT_BOOL bEnabled = VARIANT_FALSE;
                BSTR sApp = NULL;
                if (SUCCEEDED(pRule->get_Direction(&eDir)) && eDir == NET_FW_RULE_DIR_IN &&
                    SUCCEEDED(pRule->get_Action(&eAction)) && eAction == NET_FW_ACTION_ALLOW &&
                    SUCCEEDED(pRule->get_Enabled(&bEnabled)) && bEnabled == VARIANT_TRUE &&
                    SUCCEEDED(pRule->get_ApplicationName(&sApp)) && sApp)
                {
                    const std::string sProgram = WideToUtf8(std::wstring(sApp, SysStringLen(sApp)));
                    if (LikeMatch(sProgram, "*BattlefrontII*.exe"))
                    {
                        wchar_t acExpanded[MAX_PATH * 4];
                        const DWORD uLen = ExpandEnvironmentStringsW(sApp, acExpanded, MAX_PATH * 4);
                        kPrograms.insert(LowerText(uLen > 0 && uLen <= MAX_PATH * 4 ? WideToUtf8(acExpanded)
                                                                                     : sProgram));
                    }
                }
                if (sApp)
                {
                    SysFreeString(sApp);
                }
                pRule->Release();
            }
            VariantClear(&kVar);
        }
    }
    else
    {
        sError = "cannot read the firewall rules";
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pEnumUnk)
    {
        pEnumUnk->Release();
    }
    if (pRules)
    {
        pRules->Release();
    }
    if (pPolicy)
    {
        pPolicy->Release();
    }
    if (SUCCEEDED(hrInit))
    {
        CoUninitialize();
    }
    return bOk;
}

bool HostFaultCount(bool bSince, unsigned long long uSince, long long& iCount)
{
    std::wstring sTime;
    if (bSince)
    {
        FILETIME kFile;
        kFile.dwLowDateTime = (DWORD)uSince;
        kFile.dwHighDateTime = (DWORD)(uSince >> 32);
        SYSTEMTIME kSys;
        FileTimeToSystemTime(&kFile, &kSys);
        wchar_t acTime[64];
        _snwprintf(acTime, 64, L" and TimeCreated[@SystemTime>='%04u-%02u-%02uT%02u:%02u:%02u.%03uZ']", kSys.wYear,
                   kSys.wMonth, kSys.wDay, kSys.wHour, kSys.wMinute, kSys.wSecond, kSys.wMilliseconds);
        acTime[63] = 0;
        sTime = acTime;
    }
    const std::wstring akQueries[] = {
        L"*[System[Provider[@Name='Microsoft-Windows-WHEA-Logger']" + sTime + L"]]",
        L"*[System[Provider[@Name='Microsoft-Windows-Kernel-Power'] and (EventID=41)" + sTime + L"]]",
        L"*[System[Provider[@Name='EventLog'] and (EventID=6008)" + sTime + L"]]"};
    iCount = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        if (!CountEvents(akQueries[i].c_str(), iCount))
        {
            return false;
        }
    }
    return true;
}

long long FreeRamKB()
{
    MEMORYSTATUSEX kStatus;
    kStatus.dwLength = sizeof(kStatus);
    if (!GlobalMemoryStatusEx(&kStatus))
    {
        return 0;
    }
    return (long long)(kStatus.ullAvailPhys / 1024);
}

void DriveFree(const std::string& sPath, std::string& sDrive, unsigned long long& uFree)
{
    sDrive = sPath.size() >= 2 && sPath[1] == ':' ? sPath.substr(0, 2) : std::string();
    uFree = 0;
    ULARGE_INTEGER kFree;
    if (!sDrive.empty() && GetDiskFreeSpaceExW(Utf8ToWide(sDrive + "\\").c_str(), &kFree, NULL, NULL))
    {
        uFree = kFree.QuadPart;
    }
}

std::string GameDataRoot()
{
    std::string sCandidate = EnvText(L"SWBF2_GAMEDATA");
    if (sCandidate.empty())
    {
        const DWORD auViews[] = {RRF_SUBKEY_WOW6464KEY, RRF_SUBKEY_WOW6432KEY};
        for (size_t i = 0; i < 2 && sCandidate.empty(); ++i)
        {
            std::string sLoc;
            if (RegText(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 6060", L"InstallLocation",
                        auViews[i], sLoc) &&
                !sLoc.empty())
            {
                sCandidate = sLoc + "\\GameData";
            }
        }
    }
    if (!sCandidate.empty() && DirExists(JoinPath(sCandidate, "data\\_lvl_pc")))
    {
        return sCandidate;
    }
    return std::string();
}

std::string GameDataManifestHash(const std::string& sRoot)
{
    if (sRoot.empty() || !DirExists(sRoot))
    {
        return "-";
    }
    std::string sFull = FullPath(sRoot);
    while (!sFull.empty() && sFull[sFull.size() - 1] == '\\')
    {
        sFull.erase(sFull.size() - 1);
    }
    std::vector<std::string> kLines;
    ManifestLines(sFull, std::string(), kLines);
    if (kLines.empty())
    {
        return "-";
    }
    std::sort(kLines.begin(), kLines.end());
    return TextSha(JoinText(kLines, "\n"));
}

KeyPairs VendorDllHashes(const std::string& sWorkerDir, const std::string& sSystemDir, const std::string& sPlatform)
{
    KeyPairs k;
    const std::vector<std::string> kFiles = SmokeWorkerFiles(sPlatform);
    for (size_t i = 0; i < kFiles.size(); ++i)
    {
        if (!LikeMatch(kFiles[i], "*.dll"))
        {
            continue;
        }
        std::string sPath = JoinPath(sWorkerDir, kFiles[i]);
        if (!(!sWorkerDir.empty() && FileExists(sPath)) && EqualNoCase(kFiles[i], "d3dx9_43.dll") &&
            !sSystemDir.empty())
        {
            sPath = JoinPath(sSystemDir, kFiles[i]);
        }
        std::string sHash;
        if (!FileExists(sPath) || !FileSha(sPath, sHash))
        {
            sHash = "-";
        }
        k.push_back(std::make_pair(kFiles[i], sHash));
    }
    return k;
}

const long long kRamFloorKB = 5098436;
const long long kDiskFloorBytes = 15483357102ll;

std::vector<PreflightResult> PreflightVerdicts(const PreflightFacts& kFacts)
{
    std::vector<PreflightResult> kOut;
    PreflightResult k;
    k.sCheck = "steam";
    k.bPass = kFacts.sSteam == "yes";
    k.sText = k.bPass ? "Steam running with a logged-in user"
                      : "Steam is not running with a logged-in user (state '" + kFacts.sSteam + "')";
    kOut.push_back(k);

    k.sCheck = "overlay";
    k.bPass = kFacts.sOverlay == "off";
    k.sText = k.bPass ? "app 6060 overlay off"
                      : "app 6060 overlay is '" + kFacts.sOverlay +
                            "', not off: set OverlayAppEnable to 0 under apps\\6060 in localconfig.vdf with Steam "
                            "shut down";
    kOut.push_back(k);

    std::vector<std::string> kMiss;
    for (size_t i = 0; i < kFacts.kRulePaths.size(); ++i)
    {
        if (!kFacts.kAllowRules.count(LowerText(kFacts.kRulePaths[i])))
        {
            kMiss.push_back(kFacts.kRulePaths[i]);
        }
    }
    const std::string sPaths = NumberText(kFacts.kRulePaths.size());
    k.sCheck = "firewall";
    if (kFacts.kGamePlatforms.empty())
    {
        k.bPass = true;
        k.sText = "no selected step starts the game exe on " + JoinText(kFacts.kPlatforms, ", ") +
                  ", so no rule is needed";
    }
    else if (!kFacts.kRulePaths.empty() && kMiss.empty())
    {
        k.bPass = true;
        k.sText = sPaths + " rule paths have an inbound allow rule";
    }
    else if (kFacts.kRulePaths.empty())
    {
        k.bPass = false;
        k.sText = "no rule paths for this tier";
    }
    else
    {
        k.bPass = false;
        k.sText = NumberText(kMiss.size()) + " of " + sPaths + " rule paths have no enabled inbound allow rule, first " +
                  kMiss[0] + ": run tests\\gate.cmd firewall elevated";
    }
    kOut.push_back(k);

    k.sCheck = "wer";
    k.bPass = kFacts.kWer.empty();
    k.sText = k.bPass ? "LocalDumps key and DontShowUI set" : JoinText(kFacts.kWer, "; ");
    kOut.push_back(k);

    const std::string sFloorKB = SignedText(kRamFloorKB);
    k.sCheck = "ram";
    k.bPass = kFacts.iFreeRamKB >= kRamFloorKB;
    k.sText = k.bPass ? "free " + SignedText(kFacts.iFreeRamKB) + " KB, floor " + sFloorKB + " KB"
                      : "free physical memory " + SignedText(kFacts.iFreeRamKB) + " KB is below the floor " +
                            sFloorKB + " KB";
    kOut.push_back(k);

    const std::string sDiskFloor = SignedText(kDiskFloorBytes);
    std::vector<std::string> kLow;
    if ((long long)kFacts.uRepoFree < kDiskFloorBytes)
    {
        kLow.push_back(kFacts.sRepoDrive + " free " + NumberText(kFacts.uRepoFree) + " bytes is below the floor " +
                       sDiskFloor + " bytes");
    }
    if (kFacts.sGameData.empty())
    {
        kLow.push_back("retail GameData not found (set SWBF2_GAMEDATA or install Steam App 6060; is its drive "
                       "attached?)");
    }
    k.sCheck = "disk";
    k.bPass = kLow.empty();
    k.sText = k.bPass ? kFacts.sRepoDrive + " free " + NumberText(kFacts.uRepoFree) + ", floor " + sDiskFloor +
                            "; GameData " + kFacts.sGameData
                      : JoinText(kLow, "; ");
    kOut.push_back(k);

    std::vector<std::string> kTc;
    std::vector<std::string> kPlats;
    for (size_t i = 0; i < kFacts.kToolchain.size(); ++i)
    {
        const ToolchainFacts& t = kFacts.kToolchain[i];
        kPlats.push_back(t.sPlatform);
        if (!t.bMSBuild)
        {
            kTc.push_back(t.sPlatform + ": MSBuild not found");
        }
        if (!t.bDumpbin)
        {
            kTc.push_back(t.sPlatform + ": dumpbin not found");
        }
        if (!t.kMissingRuntimes.empty())
        {
            kTc.push_back(t.sPlatform + ": missing from " + t.sRuntimeDir + ": " + JoinText(t.kMissingRuntimes, ", "));
        }
    }
    if (kFacts.kToolchain.empty())
    {
        kTc.push_back("no platform toolchain checked");
    }
    k.sCheck = "toolchain";
    k.bPass = kTc.empty();
    k.sText = k.bPass ? "MSBuild, dumpbin and runtimes present for " + JoinText(kPlats, ", ") : JoinText(kTc, "; ");
    kOut.push_back(k);
    return kOut;
}

std::string PreflightLine(const PreflightResult& kResult)
{
    return std::string(kResult.bPass ? "[PASS]" : "[FAIL]") + " preflight:" + kResult.sCheck + " " + kResult.sText;
}

bool FindReparsePoints(const std::string& sDir, std::vector<std::string>& kFound, std::string& sError)
{
    const DWORD uTop = GetFileAttributesW(Utf8ToWide(sDir).c_str());
    if (uTop == INVALID_FILE_ATTRIBUTES || !(uTop & FILE_ATTRIBUTE_DIRECTORY))
    {
        return true;
    }
    if (uTop & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        kFound.push_back(FullPath(sDir));
        return true;
    }
    std::vector<std::string> kPending(1, FullPath(sDir));
    while (!kPending.empty())
    {
        const std::string sCur = kPending.back();
        kPending.pop_back();
        std::vector<DirEntry> kEntries;
        if (!ListDir(sCur, kEntries))
        {
            sError = "cannot list " + sCur;
            return false;
        }
        for (size_t i = 0; i < kEntries.size(); ++i)
        {
            const std::string sPath = JoinPath(sCur, kEntries[i].sName);
            if (kEntries[i].uAttr & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                kFound.push_back(sPath);
            }
            else if (kEntries[i].uAttr & FILE_ATTRIBUTE_DIRECTORY)
            {
                kPending.push_back(sPath);
            }
        }
    }
    return true;
}

bool CopyTree(const std::string& sFrom, const std::string& sTo, std::string& sError)
{
    std::vector<std::string> kFound;
    if (!FindReparsePoints(sFrom, kFound, sError))
    {
        return false;
    }
    if (!kFound.empty())
    {
        sError = "folder " + sFrom + " holds a reparse point";
        return false;
    }
    return CopyContents(sFrom, sTo, sError);
}

bool RemoveTree(const std::string& sDirIn, std::string& sError)
{
    if (sDirIn.empty())
    {
        sError = "Remove-HarnessTree needs a folder";
        return false;
    }
    std::string sDir = FullPath(sDirIn);
    while (sDir.size() > 3 && sDir[sDir.size() - 1] == '\\')
    {
        sDir.erase(sDir.size() - 1);
    }
    const std::wstring sWide = Utf8ToWide(sDir);
    const DWORD uAttr = GetFileAttributesW(sWide.c_str());
    if (uAttr == INVALID_FILE_ATTRIBUTES)
    {
        const DWORD uErr = GetLastError();
        if (uErr == ERROR_FILE_NOT_FOUND || uErr == ERROR_PATH_NOT_FOUND)
        {
            return true;
        }
        sError = "REFUSING to remove " + sDir + ": cannot read its attributes";
        return false;
    }
    if (uAttr & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        sError = "REFUSING to remove " + sDir + ": it is a reparse point";
        return false;
    }
    if (!(uAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        sError = "REFUSING to remove " + sDir + ": not a folder";
        return false;
    }
    const std::string sGit = JoinPath(sDir, ".git");
    if (DirExists(sGit))
    {
        sError = "REFUSING to remove " + sDir + ": it holds a .git folder";
        return false;
    }
    const bool bWorktree = FileExists(sGit);
    const std::string sLink = bWorktree ? std::string() : HarnessDataLink(sDir);
    std::vector<std::string> kFound;
    std::string sScan;
    if (!FindReparsePoints(sDir, kFound, sScan))
    {
        sError = "REFUSING to remove " + sDir + ": reparse point scan failed: " + sScan;
        return false;
    }
    std::vector<std::string> kOther;
    for (size_t i = 0; i < kFound.size(); ++i)
    {
        if (!EqualNoCase(kFound[i], sLink))
        {
            kOther.push_back(kFound[i]);
        }
    }
    if (!kOther.empty())
    {
        sError = "REFUSING to remove " + sDir + ": reparse points: " + JoinText(kOther, ", ");
        return false;
    }
    if (!sLink.empty())
    {
        RemoveDirectoryW(Utf8ToWide(sLink).c_str());
        if (GetFileAttributesW(Utf8ToWide(sLink).c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            sError = "worker data junction not removed: " + sLink;
            return false;
        }
        std::vector<std::string> kLeft;
        if (!FindReparsePoints(sDir, kLeft, sScan))
        {
            sError = "REFUSING to remove " + sDir + ": reparse point scan failed: " + sScan;
            return false;
        }
        if (!kLeft.empty())
        {
            sError = "REFUSING to remove " + sDir + ": reparse points: " + JoinText(kLeft, ", ");
            return false;
        }
    }
    if (bWorktree)
    {
        std::string sOut;
        std::vector<std::string> kArgs;
        kArgs.push_back("rev-parse");
        kArgs.push_back("--path-format=absolute");
        kArgs.push_back("--git-common-dir");
        if (!RunGit(sDir, kArgs, sOut))
        {
            sError = "git rev-parse failed in " + sDir + ": " + sOut;
            return false;
        }
        std::string sMain = FullPath(sOut);
        sMain = sMain.substr(0, sMain.find_last_of('\\'));
        kArgs.clear();
        kArgs.push_back("reset");
        kArgs.push_back("--hard");
        kArgs.push_back("-q");
        if (!RunGit(sDir, kArgs, sOut))
        {
            sError = "git reset --hard failed in " + sDir + ": " + sOut;
            return false;
        }
        kArgs.clear();
        kArgs.push_back("clean");
        kArgs.push_back("-fdq");
        if (!RunGit(sDir, kArgs, sOut))
        {
            sError = "git clean -fdq failed in " + sDir + ": " + sOut;
            return false;
        }
        kArgs.clear();
        kArgs.push_back("worktree");
        kArgs.push_back("remove");
        kArgs.push_back(sDir);
        if (!RunGit(sMain, kArgs, sOut))
        {
            sError = "git worktree remove failed for " + sDir + ": " + sOut;
            return false;
        }
    }
    else
    {
        if (!DeleteContents(sDir, sError))
        {
            return false;
        }
        SetFileAttributesW(sWide.c_str(), FILE_ATTRIBUTE_NORMAL);
        RemoveDirectoryW(sWide.c_str());
    }
    if (DirExists(sDir))
    {
        sError = "Cleanup failed: " + sDir + " is still in place";
        return false;
    }
    return true;
}

bool RemoveJunctions(const std::string& sDir, std::string& sError)
{
    if (GetFileAttributesW(Utf8ToWide(sDir).c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        return true;
    }
    std::vector<std::string> kFound;
    if (!FindReparsePoints(sDir, kFound, sError))
    {
        return false;
    }
    for (size_t i = 0; i < kFound.size(); ++i)
    {
        RemoveDirectoryW(Utf8ToWide(kFound[i]).c_str());
    }
    std::vector<std::string> kLeft;
    if (!FindReparsePoints(sDir, kLeft, sError))
    {
        return false;
    }
    if (!kLeft.empty())
    {
        sError = "junctions not removed under " + sDir + ": " + JoinText(kLeft, ", ");
        return false;
    }
    return true;
}

std::vector<std::string> CleanFolders(const std::string& sRoot, const std::vector<std::string>& kPlatforms,
                                      const std::vector<std::string>& kConfigs)
{
    std::vector<std::string> kOut;
    for (size_t p = 0; p < kPlatforms.size(); ++p)
    {
        if (kConfigs.empty())
        {
            kOut.push_back(JoinPath(sRoot, "bin\\" + kPlatforms[p] + "\\Release"));
            kOut.push_back(JoinPath(sRoot, "bin\\Tests\\" + kPlatforms[p] + "\\Debug"));
            continue;
        }
        for (size_t c = 0; c < kConfigs.size(); ++c)
        {
            kOut.push_back(JoinPath(sRoot, "bin\\" + kPlatforms[p] + "\\" + kConfigs[c]));
            kOut.push_back(JoinPath(sRoot, "bin\\Tests\\" + kPlatforms[p] + "\\" + kConfigs[c]));
        }
    }
    return kOut;
}

bool CleanStaged(const std::string& sRoot, const std::vector<std::string>& kPlatforms,
                 const std::vector<std::string>& kConfigs, std::vector<std::string>& kLines, std::string& sError)
{
    const std::vector<std::string> kDirs = CleanFolders(sRoot, kPlatforms, kConfigs);
    for (size_t i = 0; i < kDirs.size(); ++i)
    {
        const DWORD uAttr = GetFileAttributesW(Utf8ToWide(kDirs[i]).c_str());
        if (uAttr == INVALID_FILE_ATTRIBUTES)
        {
            continue;
        }
        if (uAttr & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            sError = "REFUSING to remove " + kDirs[i] + ": it is a reparse point";
            return false;
        }
        if (!RemoveJunctions(kDirs[i], sError) || !RemoveTree(kDirs[i], sError))
        {
            return false;
        }
        kLines.push_back("Removed " + kDirs[i]);
    }
    const std::string sArtifacts = JoinPath(sRoot, "artifacts");
    const DWORD uArt = GetFileAttributesW(Utf8ToWide(sArtifacts).c_str());
    if (uArt == INVALID_FILE_ATTRIBUTES)
    {
        return true;
    }
    if (uArt & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        sError = "REFUSING to clean " + sArtifacts + ": it is a reparse point";
        return false;
    }
    std::vector<std::string> kLinks;
    if (!FindReparsePoints(sArtifacts, kLinks, sError))
    {
        return false;
    }
    if (!kLinks.empty())
    {
        if (!RemoveJunctions(sArtifacts, sError))
        {
            return false;
        }
        kLines.push_back("Removed " + NumberText(kLinks.size()) + " junction(s) under " + sArtifacts);
    }
    return true;
}

namespace
{

bool EnumRules(bool bGameOnly, std::vector<FirewallRule>& kRules, std::string& sError)
{
    kRules.clear();
    const HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    INetFwPolicy2* pPolicy = NULL;
    INetFwRules* pRules = NULL;
    IUnknown* pEnumUnk = NULL;
    IEnumVARIANT* pEnum = NULL;
    bool bOk = false;
    if (SUCCEEDED(CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2),
                                   (void**)&pPolicy)) &&
        SUCCEEDED(pPolicy->get_Rules(&pRules)) && SUCCEEDED(pRules->get__NewEnum(&pEnumUnk)) &&
        SUCCEEDED(pEnumUnk->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pEnum)))
    {
        bOk = true;
        VARIANT kVar;
        VariantInit(&kVar);
        ULONG uGot = 0;
        while (pEnum->Next(1, &kVar, &uGot) == S_OK && uGot == 1)
        {
            INetFwRule* pRule = NULL;
            if (kVar.vt == VT_DISPATCH && kVar.pdispVal &&
                SUCCEEDED(kVar.pdispVal->QueryInterface(__uuidof(INetFwRule), (void**)&pRule)))
            {
                BSTR sName = NULL;
                BSTR sApp = NULL;
                pRule->get_Name(&sName);
                pRule->get_ApplicationName(&sApp);
                FirewallRule k;
                k.sName = sName ? WideToUtf8(std::wstring(sName, SysStringLen(sName))) : std::string();
                if (sApp)
                {
                    wchar_t acExpanded[MAX_PATH * 4];
                    const DWORD uLen = ExpandEnvironmentStringsW(sApp, acExpanded, MAX_PATH * 4);
                    k.sProgram = uLen > 0 && uLen <= MAX_PATH * 4 ? WideToUtf8(acExpanded)
                                                                  : WideToUtf8(std::wstring(sApp, SysStringLen(sApp)));
                }
                if (!bGameOnly || (sApp && LikeMatch(WideToUtf8(std::wstring(sApp, SysStringLen(sApp))),
                                                     "*BattlefrontII*.exe")))
                {
                    kRules.push_back(k);
                }
                if (sName)
                {
                    SysFreeString(sName);
                }
                if (sApp)
                {
                    SysFreeString(sApp);
                }
                pRule->Release();
            }
            VariantClear(&kVar);
        }
    }
    else
    {
        sError = "cannot read the firewall rules";
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pEnumUnk)
    {
        pEnumUnk->Release();
    }
    if (pRules)
    {
        pRules->Release();
    }
    if (pPolicy)
    {
        pPolicy->Release();
    }
    if (SUCCEEDED(hrInit))
    {
        CoUninitialize();
    }
    return bOk;
}

bool SameRule(const FirewallRule& kA, const FirewallRule& kB)
{
    return kA.sName == kB.sName && EqualNoCase(kA.sProgram, kB.sProgram);
}

bool HasRule(const std::vector<FirewallRule>& kList, const FirewallRule& kRule)
{
    for (size_t i = 0; i < kList.size(); ++i)
    {
        if (SameRule(kList[i], kRule))
        {
            return true;
        }
    }
    return false;
}

bool LowerLess(const std::string& sA, const std::string& sB)
{
    return LowerText(sA) < LowerText(sB);
}

bool AddRule(INetFwRules* pRules, const std::string& sPath, std::string& sError)
{
    INetFwRule* pRule = NULL;
    if (FAILED(CoCreateInstance(__uuidof(NetFwRule), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pRule)))
    {
        sError = "cannot create a firewall rule for " + sPath;
        return false;
    }
    BSTR sName = SysAllocString(Utf8ToWide("OpenSWBF2 test " + sPath).c_str());
    BSTR sApp = SysAllocString(Utf8ToWide(sPath).c_str());
    BSTR sRemote = SysAllocString(L"LocalSubnet");
    const bool bOk = sName && sApp && sRemote && SUCCEEDED(pRule->put_Name(sName)) &&
                     SUCCEEDED(pRule->put_ApplicationName(sApp)) &&
                     SUCCEEDED(pRule->put_Direction(NET_FW_RULE_DIR_IN)) &&
                     SUCCEEDED(pRule->put_Action(NET_FW_ACTION_ALLOW)) && SUCCEEDED(pRule->put_RemoteAddresses(sRemote)) &&
                     SUCCEEDED(pRule->put_Profiles(NET_FW_PROFILE2_ALL)) && SUCCEEDED(pRule->put_Enabled(VARIANT_TRUE)) &&
                     SUCCEEDED(pRules->Add(pRule));
    SysFreeString(sName);
    SysFreeString(sApp);
    SysFreeString(sRemote);
    pRule->Release();
    if (!bOk)
    {
        sError = "cannot add the firewall rule for " + sPath;
    }
    return bOk;
}

size_t CountNamed(const std::vector<FirewallRule>& kAll, const std::string& sName)
{
    size_t u = 0;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        u += kAll[i].sName == sName ? 1 : 0;
    }
    return u;
}

bool RemoveNamed(INetFwRules* pRules, const std::string& sName, const std::vector<FirewallRule>& kStale,
                 std::string& sError)
{
    std::vector<FirewallRule> kAll;
    if (!EnumRules(false, kAll, sError))
    {
        return false;
    }
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (kAll[i].sName == sName && !HasRule(kStale, kAll[i]))
        {
            sError = "REFUSING to remove rule '" + sName + "': it shares its name with the kept rule for " +
                     kAll[i].sProgram;
            return false;
        }
    }
    BSTR sWide = SysAllocString(Utf8ToWide(sName).c_str());
    size_t uLeft = CountNamed(kAll, sName);
    while (uLeft > 0)
    {
        if (FAILED(pRules->Remove(sWide)) || !EnumRules(false, kAll, sError) || CountNamed(kAll, sName) >= uLeft)
        {
            SysFreeString(sWide);
            sError = "cannot remove the firewall rule '" + sName + "'";
            return false;
        }
        uLeft = CountNamed(kAll, sName);
    }
    SysFreeString(sWide);
    return true;
}

}

FirewallPlan PlanFirewall(const std::vector<std::string>& kRoots, int iWorkers, const std::vector<FirewallRule>& kRules,
                          PathExistsFn pExists)
{
    FirewallPlan kPlan;
    std::set<std::string> kExisting;
    for (size_t i = 0; i < kRules.size(); ++i)
    {
        kExisting.insert(LowerText(kRules[i].sProgram));
    }
    std::vector<std::string> kPlatforms(kWorkerPlatforms, kWorkerPlatforms + 2);
    std::vector<std::string> kConfigs(kWorkerConfigurations, kWorkerConfigurations + 4);
    for (size_t r = 0; r < kRoots.size(); ++r)
    {
        FirewallRootPlan kRoot;
        kRoot.sRoot = kRoots[r];
        const std::vector<std::string> kPaths = FirewallRulePaths(kRoots[r], kPlatforms, kConfigs, iWorkers);
        std::set<std::string> kWanted;
        for (size_t i = 0; i < kPaths.size(); ++i)
        {
            if (kWanted.insert(LowerText(kPaths[i])).second && !kExisting.count(LowerText(kPaths[i])))
            {
                kRoot.kAdd.push_back(kPaths[i]);
            }
        }
        kRoot.uWanted = kWanted.size();
        std::sort(kRoot.kAdd.begin(), kRoot.kAdd.end(), LowerLess);
        const std::string sWorker = JoinPath(kRoots[r], "artifacts\\smoke-workers\\");
        const char* const apcStale[] = {"artifacts\\tests\\", "artifacts\\laa-compare\\", "artifacts\\spike32\\work\\"};
        for (size_t i = 0; i < kRules.size(); ++i)
        {
            const std::string& sPath = kRules[i].sProgram;
            if (kWanted.count(LowerText(sPath)))
            {
                continue;
            }
            bool bStale = StartsWithNoCase(sPath, sWorker);
            if (!bStale && !pExists(sPath))
            {
                for (size_t s = 0; s < 3; ++s)
                {
                    bStale = bStale || StartsWithNoCase(sPath, JoinPath(kRoots[r], apcStale[s]));
                }
            }
            if (bStale)
            {
                kRoot.kRemove.push_back(kRules[i]);
            }
        }
        kPlan.kRoots.push_back(kRoot);
    }
    for (size_t i = 0; i < kRules.size(); ++i)
    {
        const std::string& sPath = kRules[i].sProgram;
        if (pExists(sPath) || !(LikeMatch(sPath, "*\\.claude\\*") || LikeMatch(sPath, "*\\temp\\claude\\*")))
        {
            continue;
        }
        bool bPlanned = false;
        for (size_t r = 0; r < kPlan.kRoots.size(); ++r)
        {
            bPlanned = bPlanned || HasRule(kPlan.kRoots[r].kRemove, kRules[i]);
        }
        if (!bPlanned)
        {
            kPlan.kStrays.push_back(kRules[i]);
        }
    }
    return kPlan;
}

std::vector<std::string> FirewallPlanLines(const FirewallPlan& kPlan, bool bList)
{
    const std::string sAdd = bList ? "would add" : "added";
    const std::string sRemove = bList ? "would remove" : "removed";
    std::vector<std::string> kLines;
    size_t uAdd = 0;
    size_t uRemove = kPlan.kStrays.size();
    for (size_t r = 0; r < kPlan.kRoots.size(); ++r)
    {
        const FirewallRootPlan& k = kPlan.kRoots[r];
        for (size_t i = 0; i < k.kAdd.size(); ++i)
        {
            kLines.push_back("ADD " + k.kAdd[i]);
        }
        for (size_t i = 0; i < k.kRemove.size(); ++i)
        {
            kLines.push_back("REMOVE " + k.kRemove[i].sName + " " + k.kRemove[i].sProgram);
        }
        uAdd += k.kAdd.size();
        uRemove += k.kRemove.size();
    }
    for (size_t i = 0; i < kPlan.kStrays.size(); ++i)
    {
        kLines.push_back("REMOVE " + kPlan.kStrays[i].sName + " " + kPlan.kStrays[i].sProgram);
    }
    for (size_t r = 0; r < kPlan.kRoots.size(); ++r)
    {
        const FirewallRootPlan& k = kPlan.kRoots[r];
        kLines.push_back("root " + k.sRoot + ": " + NumberText(k.uWanted) + " rule paths, missing " +
                         NumberText(k.kAdd.size()) + ", stale " + NumberText(k.kRemove.size()) + "; " + sAdd + " " +
                         NumberText(k.kAdd.size()) + " rules, " + sRemove + " " + NumberText(k.kRemove.size()) +
                         " rules");
    }
    kLines.push_back(sAdd + " " + NumberText(uAdd) + " rules, " + sRemove + " " + NumberText(uRemove) + " rules");
    return kLines;
}

bool FirewallRules(std::vector<FirewallRule>& kRules, std::string& sError)
{
    return EnumRules(true, kRules, sError);
}

bool ApplyFirewall(const FirewallPlan& kPlan, std::string& sError)
{
    std::vector<FirewallRule> kStale = kPlan.kStrays;
    for (size_t r = 0; r < kPlan.kRoots.size(); ++r)
    {
        kStale.insert(kStale.end(), kPlan.kRoots[r].kRemove.begin(), kPlan.kRoots[r].kRemove.end());
    }
    const HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    INetFwPolicy2* pPolicy = NULL;
    INetFwRules* pRules = NULL;
    bool bOk = SUCCEEDED(CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2),
                                          (void**)&pPolicy)) &&
               SUCCEEDED(pPolicy->get_Rules(&pRules));
    if (!bOk)
    {
        sError = "cannot open the firewall rules";
    }
    for (size_t r = 0; bOk && r < kPlan.kRoots.size(); ++r)
    {
        for (size_t i = 0; bOk && i < kPlan.kRoots[r].kAdd.size(); ++i)
        {
            bOk = AddRule(pRules, kPlan.kRoots[r].kAdd[i], sError);
        }
    }
    std::set<std::string> kNames;
    for (size_t i = 0; i < kStale.size(); ++i)
    {
        kNames.insert(kStale[i].sName);
    }
    for (std::set<std::string>::const_iterator it = kNames.begin(); bOk && it != kNames.end(); ++it)
    {
        bOk = RemoveNamed(pRules, *it, kStale, sError);
    }
    if (pRules)
    {
        pRules->Release();
    }
    if (pPolicy)
    {
        pPolicy->Release();
    }
    if (SUCCEEDED(hrInit))
    {
        CoUninitialize();
    }
    return bOk;
}

bool IsElevated()
{
    BYTE acSid[SECURITY_MAX_SID_SIZE];
    DWORD uSize = sizeof(acSid);
    BOOL bMember = FALSE;
    return CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, acSid, &uSize) &&
           CheckTokenMembership(NULL, acSid, &bMember) && bMember;
}

}
