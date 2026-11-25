#include "harness/gamerun.h"

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "third_party/doctest/doctest.h"
#include "harness/minidump.h"
#include "testutil.h"
#include "Battlefront2/Platform/Pc/pcProgress.h"

namespace
{

struct GameRunContext
{
    std::string sPlatform;
    std::string sConfig;
    std::string sGameDir;
    std::string sCaseDir;
    std::string sSoak;
    std::string sParam;
    std::string sSlots;
};

GameRunContext& Context()
{
    static GameRunContext sContext;
    return sContext;
}

const char* const sapcWorkerFilesWin32[] = {"BattlefrontII.exe", "COPYING.LGPLv2.1",     "d3dx9_43.dll",
                                            "Galaxy.dll",        "GalaxyPeer.dll",       "NOTICE.txt",
                                            "openswbf2-ffmpeg.dll", "steam_api.dll",     "steam_appid.txt"};
const char* const sapcWorkerFilesX64[] = {"BattlefrontII.exe",    "COPYING.LGPLv2.1", "d3dx9_43.dll",   "NOTICE.txt",
                                          "openswbf2-ffmpeg.dll", "steam_api64.dll",  "steam_appid.txt"};

bool FileExists(const std::string& sPath)
{
    const DWORD uAttr = GetFileAttributesA(sPath.c_str());
    return uAttr != INVALID_FILE_ATTRIBUTES && !(uAttr & FILE_ATTRIBUTE_DIRECTORY);
}

bool DirExists(const std::string& sPath)
{
    const DWORD uAttr = GetFileAttributesA(sPath.c_str());
    return uAttr != INVALID_FILE_ATTRIBUTES && (uAttr & FILE_ATTRIBUTE_DIRECTORY);
}

bool ReadFileText(const std::string& sPath, std::string& sText)
{
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return false;
    }
    char acBuf[4096];
    size_t uRead;
    while ((uRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sText.append(acBuf, uRead);
    }
    fclose(pFile);
    return true;
}

std::vector<std::string> ListFiles(const std::string& sDir, const char* pcPattern)
{
    std::vector<std::string> kFiles;
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + "\\" + pcPattern).c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return kFiles;
    }
    do
    {
        if (!(kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            kFiles.push_back(sDir + "\\" + kFind.cFileName);
        }
    } while (FindNextFileA(hFind, &kFind));
    FindClose(hFind);
    return kFiles;
}

void RemoveTree(const std::string& sDir, bool bTop)
{
    const DWORD uAttr = GetFileAttributesA(sDir.c_str());
    if (uAttr == INVALID_FILE_ATTRIBUTES)
    {
        return;
    }
    if (uAttr & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        HarnessError(("refusing to remove a reparse point inside a worker folder: " + sDir).c_str());
    }
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + "\\*").c_str(), &kFind);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            const std::string sName = kFind.cFileName;
            if (sName == "." || sName == "..")
            {
                continue;
            }
            const std::string sPath = sDir + "\\" + sName;
            if (kFind.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                if (!bTop || _stricmp(sName.c_str(), "data") != 0 || !(kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    FindClose(hFind);
                    HarnessError(("refusing to remove a worker folder that holds the reparse point " + sPath).c_str());
                }
                if (!RemoveDirectoryA(sPath.c_str()))
                {
                    FindClose(hFind);
                    HarnessError(("cannot remove the worker data junction " + sPath).c_str());
                }
            }
            else if (kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                RemoveTree(sPath, false);
            }
            else
            {
                SetFileAttributesA(sPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFileA(sPath.c_str());
            }
        } while (FindNextFileA(hFind, &kFind));
        FindClose(hFind);
    }
    RemoveDirectoryA(sDir.c_str());
    if (DirExists(sDir))
    {
        HarnessError(("cannot remove the worker folder " + sDir).c_str());
    }
}

void MakeDirs(const std::string& sPath)
{
    for (size_t i = 3; i <= sPath.size(); ++i)
    {
        if (i == sPath.size() || sPath[i] == '\\')
        {
            CreateDirectoryA(sPath.substr(0, i).c_str(), 0);
        }
    }
}

bool MakeJunction(const std::string& sLink, const std::string& sTarget)
{
    std::string sCmd = "cmd.exe /d /c mklink /J " + QuoteArg(sLink) + " " + QuoteArg(sTarget);
    std::vector<char> kCmd(sCmd.begin(), sCmd.end());
    kCmd.push_back(0);
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    if (!CreateProcessA(0, &kCmd[0], 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &si, &pi))
    {
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD uCode = 1;
    GetExitCodeProcess(pi.hProcess, &uCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    const DWORD uAttr = GetFileAttributesA(sLink.c_str());
    return uCode == 0 && uAttr != INVALID_FILE_ATTRIBUTES && (uAttr & FILE_ATTRIBUTE_REPARSE_POINT);
}

bool HasToken(const std::string& sArgs, const char* pcToken)
{
    const size_t uLen = strlen(pcToken);
    for (size_t i = 0; i + uLen <= sArgs.size(); ++i)
    {
        if ((i == 0 || sArgs[i - 1] == ' ') && (i + uLen == sArgs.size() || sArgs[i + uLen] == ' ') &&
            _strnicmp(sArgs.c_str() + i, pcToken, uLen) == 0)
        {
            return true;
        }
    }
    return false;
}

void Tick()
{
    pcProgress::Step();
}

}

bool GameRunArg(const char* pcArg)
{
    const struct
    {
        const char* pcPrefix;
        std::string* psValue;
    } kArgs[] = {{"--game-platform=", &Context().sPlatform},
                 {"--game-config=", &Context().sConfig},
                 {"--game-dir=", &Context().sGameDir},
                 {"--case-dir=", &Context().sCaseDir},
                 {"--game-soak=", &Context().sSoak},
                 {"--game-param=", &Context().sParam},
                 {"--game-slots=", &Context().sSlots}};
    for (size_t i = 0; i < sizeof(kArgs) / sizeof(kArgs[0]); ++i)
    {
        const size_t uLen = strlen(kArgs[i].pcPrefix);
        if (strncmp(pcArg, kArgs[i].pcPrefix, uLen) == 0)
        {
            *kArgs[i].psValue = pcArg + uLen;
            return true;
        }
    }
    return false;
}

std::string GameRunTail(bool bSound)
{
    return std::string("-norender") + (bSound ? "" : " -nosound") +
           " -fixedrate 30 -nowait -logfinalscore -DecompCONMinutes 5 -seed 1 -exitongameover -nofocuspause -log "
           "\"{CASEDIR}\\run.log\" -hangtimeout 0";
}

namespace
{

bool IsWorkerName(const std::string& sName, bool bFolder)
{
    if (sName.empty() || sName.find_first_of(bFolder ? ":/" : "\\:/") != std::string::npos ||
        sName.find("..") != std::string::npos || sName[0] == '\\' || _stricmp(sName.c_str(), "BattlefrontII.exe") == 0 ||
        _stricmp(sName.c_str(), "data") == 0 || _strnicmp(sName.c_str(), "data\\", 5) == 0)
    {
        return false;
    }
    return true;
}

bool WerDumpFolder(std::string& sFolder)
{
    HKEY hKey = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps\\BattlefrontII.exe", 0,
                      KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
    {
        return false;
    }
    char acValue[MAX_PATH];
    DWORD uType = 0;
    DWORD uSize = sizeof(acValue) - 1;
    const bool bSet = RegQueryValueExA(hKey, "DumpFolder", 0, &uType, (LPBYTE)acValue, &uSize) == ERROR_SUCCESS &&
                      (uType == REG_SZ || uType == REG_EXPAND_SZ) && uSize > 1;
    RegCloseKey(hKey);
    if (bSet)
    {
        acValue[uSize < sizeof(acValue) ? uSize : sizeof(acValue) - 1] = 0;
    }
    else
    {
        strcpy(acValue, "%LOCALAPPDATA%\\CrashDumps");
    }
    char acExpanded[MAX_PATH];
    const DWORD uLen = ExpandEnvironmentStringsA(acValue, acExpanded, sizeof(acExpanded));
    if (uLen == 0 || uLen > sizeof(acExpanded))
    {
        return false;
    }
    sFolder = acExpanded;
    return true;
}

void CopyFolderFiles(const std::string& sFrom, const std::string& sTo)
{
    const std::vector<std::string> kFiles = ListFiles(sFrom, "*");
    if (kFiles.empty())
    {
        return;
    }
    MakeDirs(sTo);
    for (size_t i = 0; i < kFiles.size(); ++i)
    {
        const std::string sLeaf = kFiles[i].substr(kFiles[i].find_last_of('\\') + 1);
        if (!CopyFileA(kFiles[i].c_str(), (sTo + "\\" + sLeaf).c_str(), FALSE))
        {
            HarnessError(("cannot copy " + kFiles[i] + " into " + sTo).c_str());
        }
    }
}

}

GameRun::GameRun(const char* pcName)
    : mpWorkerLock(0), mbStarted(false), mbWaited(false), mbStartFailed(false), mbDumpMatchesFatal(false),
      mbCollectWer(false), mullStartTime(0)
{
    mPlatform = Context().sPlatform;
    mGameDir = Context().sGameDir;
    Init(pcName);
}

GameRun::GameRun(const char* pcName, const char* pcPlatform, const std::string& sGameDir)
    : mpWorkerLock(0), mbStarted(false), mbWaited(false), mbStartFailed(false), mbDumpMatchesFatal(false),
      mbCollectWer(false), mullStartTime(0)
{
    mPlatform = pcPlatform ? pcPlatform : "";
    mGameDir = sGameDir;
    Init(pcName);
}

void GameRun::Init(const char* pcName)
{
    mName = pcName ? pcName : "";
    for (size_t i = 0; i < mName.size(); ++i)
    {
        const char c = mName[i];
        if (!(isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.'))
        {
            HarnessError(("game run name '" + mName + "' holds a character other than a letter, digit, '-', '_' or '.'").c_str());
        }
    }
    if (mName.empty())
    {
        HarnessError("a game run needs a name");
    }
    mChild.uCode = 0xFFFFFFFFul;
    mChild.uPid = 0;
    mChild.bStalled = false;
    mChild.bKilled = false;
    mChild.bAsanReport = false;
    mChild.bHasProgress = false;
    mChild.uTurns = 0;
    mChild.uSteps = 0;
    mRunDir = Context().sCaseDir.empty() ? std::string() : Context().sCaseDir + "\\" + mName;
}

GameRun::~GameRun()
{
    if (mThread.joinable())
    {
        mThread.join();
    }
    ReleaseWorker();
}

const std::string& GameRun::Platform()
{
    return Context().sPlatform;
}

const std::string& GameRun::Config()
{
    return Context().sConfig;
}

const std::string& GameRun::GameDir()
{
    return Context().sGameDir;
}

const std::string& GameRun::Param()
{
    return Context().sParam;
}

bool GameRun::LongSoak()
{
    return Context().sSoak == "Long";
}

std::string GameRun::Expand(const std::string& sText) const
{
    std::string sOut = sText;
    const std::string sToken = "{CASEDIR}";
    for (size_t uAt = sOut.find(sToken); uAt != std::string::npos; uAt = sOut.find(sToken, uAt + mRunDir.size()))
    {
        sOut.replace(uAt, sToken.size(), mRunDir);
    }
    return sOut;
}

void GameRun::AddArg(const std::string& sArgs)
{
    if (!sArgs.empty())
    {
        mArgs += (mArgs.empty() ? "" : " ") + sArgs;
    }
}

void GameRun::OmitDll(const char* pcName)
{
    mOmit.push_back(pcName);
}

void GameRun::SwapDll(const char* pcTarget, const std::string& sSource)
{
    mSwap.push_back(std::make_pair(std::string(pcTarget), sSource));
}

void GameRun::UseExe(const std::string& sSource)
{
    mExe = sSource;
}

void GameRun::SetDisplayIni(const std::string& sPath)
{
    mDisplayIni = sPath;
    AddArg("-displayini " + QuoteArg(sPath));
}

void GameRun::WriteWorkerFile(const char* pcName, const std::string& sBytes)
{
    if (!pcName || !IsWorkerName(pcName, false))
    {
        HarnessError(("refusing to write the worker file '" + std::string(pcName ? pcName : "") + "'").c_str());
    }
    mWriteFiles.push_back(std::make_pair(std::string(pcName), sBytes));
}

void GameRun::SeedWorkerFolder(const char* pcFolder, const std::string& sFromDir)
{
    if (!pcFolder || !IsWorkerName(pcFolder, true))
    {
        HarnessError(("refusing to seed the worker folder '" + std::string(pcFolder ? pcFolder : "") + "'").c_str());
    }
    mSeedFolders.push_back(std::make_pair(std::string(pcFolder), sFromDir));
}

void GameRun::KeepWorkerFolder(const char* pcFolder)
{
    if (!pcFolder || !IsWorkerName(pcFolder, true))
    {
        HarnessError(("refusing to keep the worker folder '" + std::string(pcFolder ? pcFolder : "") + "'").c_str());
    }
    mKeepFolders.push_back(pcFolder);
}

bool GameRun::HasInGameTests()
{
    return Context().sConfig == "Test" || Context().sConfig == "Asan";
}

void GameRun::AddInGameTest(const char* pcName, const std::string& sParameters)
{
    if (!HasInGameTests())
    {
        return;
    }
    const std::string sName = pcName ? pcName : "";
    if (sName.empty() || sName.find_first_of("+=*\" ") != std::string::npos ||
        sParameters.find_first_of("+\"") != std::string::npos)
    {
        HarnessError(("in-game test '" + sName + "' or its parameters hold a character -autotest cannot carry").c_str());
    }
    mInGameFilter += (mInGameFilter.empty() ? "" : "+") + sName + (sParameters.empty() ? "" : "=" + sParameters);
    const std::string sLine = "Test Completed. Result={Success} Name={" + sName.substr(sName.find_last_of('.') + 1) +
                              "} Path={" + sName + "}";
    AddExpectedMessagePlain(sLine.c_str(), RedLog::Inform, EAutomationExpectedMessageFlags::Exact, 1);
}

void GameRun::AddExpectedError(const char* pcPattern, EAutomationExpectedMessageFlags::MatchType eCompareType,
                               int iOccurrences, bool bIsRegex)
{
    mMessages.Add(pcPattern, RedLog::Warning, eCompareType, iOccurrences, bIsRegex);
}

void GameRun::AddExpectedErrorPlain(const char* pcPattern, EAutomationExpectedMessageFlags::MatchType eCompareType,
                                    int iOccurrences)
{
    mMessages.Add(pcPattern, RedLog::Warning, eCompareType, iOccurrences, false);
}

void GameRun::AddExpectedMessage(const char* pcPattern, RedLog::Severity eVerbosity,
                                 EAutomationExpectedMessageFlags::MatchType eCompareType, int iOccurrences, bool bIsRegex)
{
    mMessages.Add(pcPattern, eVerbosity, eCompareType, iOccurrences, bIsRegex);
}

void GameRun::AddExpectedMessage(const char* pcPattern, EAutomationExpectedMessageFlags::MatchType eCompareType,
                                 int iOccurrences, bool bIsRegex)
{
    mMessages.Add(pcPattern, ExpectedMessages::kAnyVerbosity, eCompareType, iOccurrences, bIsRegex);
}

void GameRun::AddExpectedMessagePlain(const char* pcPattern, RedLog::Severity eVerbosity,
                                      EAutomationExpectedMessageFlags::MatchType eCompareType, int iOccurrences)
{
    mMessages.Add(pcPattern, eVerbosity, eCompareType, iOccurrences, false);
}

void GameRun::AddExpectedMessagePlain(const char* pcPattern, EAutomationExpectedMessageFlags::MatchType eCompareType,
                                      int iOccurrences)
{
    mMessages.Add(pcPattern, ExpectedMessages::kAnyVerbosity, eCompareType, iOccurrences, false);
}

void GameRun::ExpectExitCode(int iCode)
{
    mExpect.iExitCode = iCode;
}

void GameRun::ExpectFatal()
{
    mExpect.bFatal = true;
}

void GameRun::ExpectEndsEarly()
{
    mExpect.bEndsEarly = true;
}

void GameRun::ExpectDumps(int iCount)
{
    mExpect.iDumps = iCount;
}

void GameRun::ExpectDumpMatchesFatal()
{
    mbDumpMatchesFatal = true;
}

void GameRun::CollectWerDump()
{
    mbCollectWer = true;
}

bool GameRun::WerLocalDumps()
{
    std::string sFolder;
    return WerDumpFolder(sFolder);
}

void GameRun::Start()
{
    if (mbStarted)
    {
        HarnessError(("game run " + mName + " started twice").c_str());
    }
    if (!mInGameFilter.empty())
    {
        AddArg("-autotest \"" + mInGameFilter + "\"");
        mInGameFilter.clear();
    }
    const GameRunContext& kContext = Context();
    if (mGameDir.empty() || kContext.sCaseDir.empty() || mPlatform.empty() || kContext.sConfig.empty())
    {
        SkipCase("game runs start from the runner with --game-dir");
    }
    if (!FindGameDataRoot())
    {
        SkipCase("no retail GameData found (set SWBF2_GAMEDATA or install Steam App 6060)");
    }
    const std::string sExe = mGameDir + "\\BattlefrontII.exe";
    if (!FileExists(sExe))
    {
        SkipCase(("BattlefrontII.exe not found in " + mGameDir).c_str());
    }
    if (!FileExists(mGameDir + "\\data\\_lvl_pc\\shell.lvl"))
    {
        SkipCase(("staged data missing in " + mGameDir).c_str());
    }
    const bool bHandsOffConfig = kContext.sConfig == "Test" || kContext.sConfig == "Asan";
    if (!HasToken(mArgs, "-norender"))
    {
        if (!bHandsOffConfig)
        {
            HarnessError(("game run " + mName + " renders on " + kContext.sConfig +
                          ", which ignores -handsoff; rendered runs need Test or Asan")
                             .c_str());
        }
        if (!HasToken(mArgs, "-handsoff"))
        {
            AddArg("-handsoff");
        }
    }
    if (mRunDir.find('\'') != std::string::npos)
    {
        HarnessError(("the Asan log path cannot be quoted for " + mRunDir).c_str());
    }
    if (DirExists(mRunDir))
    {
        HarnessError(("game run folder " + mRunDir + " already exists: two game runs share the name " + mName).c_str());
    }
    MakeDirs(mRunDir);
    const bool bX64 = mPlatform == "x64";
    std::vector<std::string> kFiles;
    if (bX64)
    {
        kFiles.assign(sapcWorkerFilesX64, sapcWorkerFilesX64 + sizeof(sapcWorkerFilesX64) / sizeof(sapcWorkerFilesX64[0]));
    }
    else
    {
        kFiles.assign(sapcWorkerFilesWin32,
                      sapcWorkerFilesWin32 + sizeof(sapcWorkerFilesWin32) / sizeof(sapcWorkerFilesWin32[0]));
    }
    if (kContext.sConfig == "Asan")
    {
        kFiles.push_back(bX64 ? "clang_rt.asan_dynamic-x86_64.dll" : "clang_rt.asan_dynamic-i386.dll");
    }
    for (size_t i = 0; i < mOmit.size(); ++i)
    {
        if (mOmit[i].empty() || mOmit[i].find_first_of("\\/:") != std::string::npos ||
            _stricmp(mOmit[i].c_str(), "BattlefrontII.exe") == 0 || _stricmp(mOmit[i].c_str(), "data") == 0)
        {
            HarnessError(("refusing to omit '" + mOmit[i] + "'").c_str());
        }
    }
    for (size_t i = 0; i < mSwap.size(); ++i)
    {
        const std::string& sTarget = mSwap[i].first;
        if (sTarget.empty() || sTarget.find_first_of("\\/:") != std::string::npos ||
            _stricmp(sTarget.c_str(), "BattlefrontII.exe") == 0 || _stricmp(sTarget.c_str(), "data") == 0)
        {
            HarnessError(("refusing to swap '" + sTarget + "'").c_str());
        }
        if (!FileExists(mSwap[i].second))
        {
            SkipCase((mSwap[i].second + " not found").c_str());
        }
    }
    if (!mExe.empty() && !FileExists(mExe))
    {
        HarnessError(("the game exe " + mExe + " does not exist").c_str());
    }
    const std::string sArgs = Expand(mArgs);
    const std::string sAsan = kContext.sConfig == "Asan" ? "exitcode=66:log_path='" + mRunDir + "\\asan'" : "";
    mbStarted = true;
    mThread = std::thread([this, kFiles, sArgs, sAsan]() { RunInWorker(kFiles, sArgs, sAsan); });
}

void GameRun::AcquireWorker()
{
    const GameRunContext& kContext = Context();
    const std::string sRoot = RepoRoot();
    if (sRoot.empty())
    {
        HarnessError("cannot find the repository root from the tests exe");
    }
    const std::string sWorkers =
        sRoot + "\\artifacts\\smoke-workers\\" + mPlatform + "\\" + kContext.sConfig + "\\smoke";
    const int iMax = (int)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    for (int i = 1; i <= iMax && !mpWorkerLock; ++i)
    {
        char acName[128];
        _snprintf(acName, sizeof(acName), "Local\\OpenSWBF2Worker.%s.%s.%d", mPlatform.c_str(),
                  kContext.sConfig.c_str(), i);
        acName[sizeof(acName) - 1] = 0;
        HANDLE hLock = CreateMutexA(0, TRUE, acName);
        if (hLock && GetLastError() != ERROR_ALREADY_EXISTS)
        {
            char acIndex[16];
            _snprintf(acIndex, sizeof(acIndex), "%d", i);
            acIndex[sizeof(acIndex) - 1] = 0;
            mpWorkerLock = hLock;
            mWorkerDir = sWorkers + "\\w" + acIndex;
        }
        else if (hLock)
        {
            CloseHandle(hLock);
        }
    }
    if (!mpWorkerLock)
    {
        HarnessError("no free worker folder");
    }
}

void GameRun::RunInWorker(const std::vector<std::string>& kFiles, const std::string& sArgs, const std::string& sAsan)
{
    HANDLE hSlots = 0;
    if (!Context().sSlots.empty())
    {
        hSlots = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, Context().sSlots.c_str());
        if (!hSlots)
        {
            HarnessError(("cannot open the game slot semaphore " + Context().sSlots).c_str());
        }
        while (WaitForSingleObject(hSlots, 1000) == WAIT_TIMEOUT)
        {
            Tick();
        }
    }
    AcquireWorker();
    RemoveTree(mWorkerDir, true);
    MakeDirs(mWorkerDir);
    for (size_t i = 0; i < kFiles.size(); ++i)
    {
        const std::string sSource = mGameDir + "\\" + kFiles[i];
        if (FileExists(sSource) && !CopyFileA(sSource.c_str(), (mWorkerDir + "\\" + kFiles[i]).c_str(), FALSE))
        {
            HarnessError(("cannot copy " + sSource + " into " + mWorkerDir).c_str());
        }
    }
    if (!mExe.empty() && !CopyFileA(mExe.c_str(), (mWorkerDir + "\\BattlefrontII.exe").c_str(), FALSE))
    {
        HarnessError(("cannot copy the game exe " + mExe + " into " + mWorkerDir).c_str());
    }
    for (size_t i = 0; i < mOmit.size(); ++i)
    {
        const std::string sPath = mWorkerDir + "\\" + mOmit[i];
        SetFileAttributesA(sPath.c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFileA(sPath.c_str());
    }
    for (size_t i = 0; i < mSwap.size(); ++i)
    {
        const std::string& sTarget = mSwap[i].first;
        if (!CopyFileA(mSwap[i].second.c_str(), (mWorkerDir + "\\" + sTarget).c_str(), FALSE))
        {
            HarnessError(("cannot swap " + sTarget + " from " + mSwap[i].second).c_str());
        }
    }
    for (size_t i = 0; i < mWriteFiles.size(); ++i)
    {
        FILE* pFile = fopen((mWorkerDir + "\\" + mWriteFiles[i].first).c_str(), "wb");
        const bool bWritten = pFile && fwrite(mWriteFiles[i].second.data(), 1, mWriteFiles[i].second.size(), pFile) ==
                                           mWriteFiles[i].second.size();
        if (pFile)
        {
            fclose(pFile);
        }
        if (!bWritten)
        {
            HarnessError(("cannot write the worker file " + mWriteFiles[i].first).c_str());
        }
    }
    for (size_t i = 0; i < mSeedFolders.size(); ++i)
    {
        CopyFolderFiles(mSeedFolders[i].second, mWorkerDir + "\\" + mSeedFolders[i].first);
    }
    if (!FileExists(mWorkerDir + "\\steam_appid.txt"))
    {
        FILE* pFile = fopen((mWorkerDir + "\\steam_appid.txt").c_str(), "wb");
        if (pFile)
        {
            fputs("6060\r\n", pFile);
            fclose(pFile);
        }
    }
    char acFull[MAX_PATH];
    const DWORD uFull = GetFullPathNameA((mGameDir + "\\data").c_str(), sizeof(acFull), acFull, 0);
    if (uFull == 0 || uFull >= sizeof(acFull) || !MakeJunction(mWorkerDir + "\\data", std::string(acFull, uFull)))
    {
        HarnessError(("cannot link " + mWorkerDir + "\\data to the staged data").c_str());
    }
    const std::string sCmd = QuoteArg(mWorkerDir + "\\BattlefrontII.exe") + " " + sArgs;
    FILE* pArgs = fopen((mRunDir + "\\command.txt").c_str(), "wb");
    if (pArgs)
    {
        fputs(sCmd.c_str(), pArgs);
        fclose(pArgs);
    }
    FILETIME kNow;
    GetSystemTimeAsFileTime(&kNow);
    mullStartTime = ((unsigned long long)kNow.dwHighDateTime << 32) | kNow.dwLowDateTime;
    ChildOptions kOptions;
    kOptions.sWorkDir = mWorkerDir;
    kOptions.bBelowNormal = true;
    kOptions.pfnTick = Tick;
    mbStartFailed = !RunChild(sCmd, mRunDir + "\\stdout.txt", sAsan, mChild, mStartError, &kOptions);
    std::string sWerFolder;
    if (mbCollectWer && !mbStartFailed && mChild.uPid != 0 && WerDumpFolder(sWerFolder))
    {
        char acLeaf[64];
        _snprintf(acLeaf, sizeof(acLeaf), "BattlefrontII.exe.%lu.dmp", mChild.uPid);
        acLeaf[sizeof(acLeaf) - 1] = 0;
        const std::string sWer = sWerFolder + "\\" + acLeaf;
        WIN32_FILE_ATTRIBUTE_DATA kData;
        if (GetFileAttributesExA(sWer.c_str(), GetFileExInfoStandard, &kData) &&
            (((unsigned long long)kData.ftLastWriteTime.dwHighDateTime << 32) | kData.ftLastWriteTime.dwLowDateTime) >=
                mullStartTime &&
            !CopyFileA(sWer.c_str(), (mRunDir + "\\" + acLeaf).c_str(), FALSE))
        {
            HarnessError(("cannot copy the WER dump " + sWer + " into " + mRunDir).c_str());
        }
    }
    const std::string sFinalScore = mWorkerDir + "\\finalscore.log";
    if (FileExists(sFinalScore))
    {
        CopyFileA(sFinalScore.c_str(), (mRunDir + "\\finalscore.log").c_str(), FALSE);
    }
    for (size_t i = 0; i < mKeepFolders.size(); ++i)
    {
        CopyFolderFiles(mWorkerDir + "\\" + mKeepFolders[i], mRunDir + "\\" + mKeepFolders[i]);
    }
    ReleaseWorker();
    if (hSlots)
    {
        ReleaseSemaphore(hSlots, 1, 0);
        CloseHandle(hSlots);
    }
}

void GameRun::ReleaseWorker()
{
    if (!mWorkerDir.empty())
    {
        RemoveTree(mWorkerDir, true);
    }
    mWorkerDir.clear();
    if (mpWorkerLock)
    {
        ReleaseMutex((HANDLE)mpWorkerLock);
        CloseHandle((HANDLE)mpWorkerLock);
        mpWorkerLock = 0;
    }
}

bool GameRun::Wait()
{
    if (!mbStarted)
    {
        HarnessError(("game run " + mName + " waited on before Start").c_str());
    }
    if (mbWaited)
    {
        return mResult.Passed;
    }
    mThread.join();
    mbWaited = true;
    if (mbStartFailed)
    {
        HarnessError(("game run " + mName + ": " + mStartError).c_str());
    }
    std::string sText;
    ReadFileText(mRunDir + "\\run.log", sText);
    if (!sText.empty() && sText[sText.size() - 1] != '\n')
    {
        sText += "\n";
    }
    ReadFileText(mRunDir + "\\stdout.txt", sText);
    RunFacts kFacts;
    ParseLogText(sText, kFacts.kLog);
    kFacts.uExitCode = mChild.uCode;
    kFacts.bKilled = mChild.bKilled;
    const std::vector<std::string> kDumps = ListFiles(mRunDir, "*.dmp");
    kFacts.iDumps = (int)kDumps.size();
    kFacts.bAsanReport = !ListFiles(mRunDir, "asan.*").empty();
    RunVerdict kVerdict = JudgeRun(kFacts, mExpect, mMessages);
    if (kVerdict.bPass && mbDumpMatchesFatal)
    {
        std::string sDump;
        std::string sWhy;
        if (kDumps.size() != 1)
        {
            kVerdict.bPass = false;
            kVerdict.sReason = "the dump check needs one dump";
        }
        else if (!ReadFileText(kDumps[0], sDump) || !DumpMatchesFatal(sDump, kFacts.kLog.sFatalLine, sWhy))
        {
            kVerdict.bPass = false;
            kVerdict.sReason = "the dump does not match the FATAL line: " + (sWhy.empty() ? "cannot read " + kDumps[0] : sWhy);
        }
    }
    mResult.Errors = kVerdict.kErrors;
    mResult.Ensures = kFacts.kLog.kEnsures;
    mResult.FatalLine = kFacts.kLog.sFatalLine;
    mResult.ExitCode = mChild.uCode;
    mResult.Dumps = kDumps;
    mResult.HasEndMarker = kFacts.kLog.bHasEndMarker;
    mResult.MarkerExitCode = kFacts.kLog.iMarkerExitCode;
    mResult.HasProgress = mChild.bHasProgress;
    mResult.ProgressTurns = mChild.uTurns;
    mResult.ProgressSteps = mChild.uSteps;
    mResult.Killed = mChild.bKilled;
    mResult.Stalled = mChild.bStalled;
    mResult.AsanReport = kFacts.bAsanReport;
    mResult.Lines = kFacts.kLog.kLines;
    if (!mDisplayIni.empty())
    {
        ReadFileText(Expand(mDisplayIni), mResult.DisplayIni);
    }
    mResult.Passed = kVerdict.bPass;
    mResult.Verdict = kVerdict.sReason;
    char acCode[32];
    _snprintf(acCode, sizeof(acCode), "0x%08lX", mChild.uCode);
    acCode[sizeof(acCode) - 1] = 0;
    printf("[GAMERUN] %s exit=%s turns=%u steps=%u dumps=%d %s%s\n", mName.c_str(), acCode, mChild.uTurns,
           mChild.uSteps, kFacts.iDumps, kVerdict.bPass ? "PASS" : "FAIL - ", kVerdict.sReason.c_str());
    fflush(stdout);
    if (kVerdict.bPass)
    {
        for (size_t i = 0; i < kDumps.size(); ++i)
        {
            DeleteFileA(kDumps[i].c_str());
        }
    }
    else
    {
        printf("    details: %s\n", mRunDir.c_str());
        fflush(stdout);
        FAIL_CHECK("game run " << mName << ": " << kVerdict.sReason);
    }
    return kVerdict.bPass;
}

const GameRunResult& GameRun::Result() const
{
    return mResult;
}

const std::string& GameRun::RunDir() const
{
    return mRunDir;
}
