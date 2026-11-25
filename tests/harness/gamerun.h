#pragma once
#include <string>
#include <thread>
#include <vector>

#include "harness/expect.h"
#include "harness/judge.h"
#include "harness/logparse.h"
#include "harness/runner.h"

struct GameRunResult
{
    std::vector<std::string> Errors;
    std::vector<std::string> Ensures;
    std::string FatalLine;
    unsigned long ExitCode = 0xFFFFFFFFul;
    std::vector<std::string> Dumps;
    bool HasEndMarker = false;
    int MarkerExitCode = 0;
    bool HasProgress = false;
    unsigned int ProgressTurns = 0;
    unsigned int ProgressSteps = 0;
    bool Killed = false;
    bool Stalled = false;
    bool AsanReport = false;
    std::vector<LogLine> Lines;
    std::string DisplayIni;
    bool Passed = false;
    std::string Verdict;
};

bool GameRunArg(const char* pcArg);
std::string GameRunTail(bool bSound);

class GameRun
{
public:
    explicit GameRun(const char* pcName);
    GameRun(const char* pcName, const char* pcPlatform, const std::string& sGameDir);
    ~GameRun();

    static const std::string& Platform();
    static const std::string& Config();
    static const std::string& GameDir();
    static const std::string& Param();
    static bool LongSoak();

    void AddArg(const std::string& sArgs);
    void OmitDll(const char* pcName);
    void SwapDll(const char* pcTarget, const std::string& sSource);
    void UseExe(const std::string& sSource);
    void SetDisplayIni(const std::string& sPath);
    void WriteWorkerFile(const char* pcName, const std::string& sBytes);
    void SeedWorkerFolder(const char* pcFolder, const std::string& sFromDir);
    void KeepWorkerFolder(const char* pcFolder);
    static bool HasInGameTests();
    void AddInGameTest(const char* pcName, const std::string& sParameters = std::string());

    void AddExpectedError(const char* pcPattern,
                          EAutomationExpectedMessageFlags::MatchType eCompareType =
                              EAutomationExpectedMessageFlags::Contains,
                          int iOccurrences = 1, bool bIsRegex = true);
    void AddExpectedErrorPlain(const char* pcPattern,
                               EAutomationExpectedMessageFlags::MatchType eCompareType =
                                   EAutomationExpectedMessageFlags::Contains,
                               int iOccurrences = 1);
    void AddExpectedMessage(const char* pcPattern, RedLog::Severity eVerbosity,
                            EAutomationExpectedMessageFlags::MatchType eCompareType =
                                EAutomationExpectedMessageFlags::Contains,
                            int iOccurrences = 1, bool bIsRegex = true);
    void AddExpectedMessage(const char* pcPattern,
                            EAutomationExpectedMessageFlags::MatchType eCompareType =
                                EAutomationExpectedMessageFlags::Contains,
                            int iOccurrences = 1, bool bIsRegex = true);
    void AddExpectedMessagePlain(const char* pcPattern, RedLog::Severity eVerbosity,
                                 EAutomationExpectedMessageFlags::MatchType eCompareType =
                                     EAutomationExpectedMessageFlags::Contains,
                                 int iOccurrences = 1);
    void AddExpectedMessagePlain(const char* pcPattern,
                                 EAutomationExpectedMessageFlags::MatchType eCompareType =
                                     EAutomationExpectedMessageFlags::Contains,
                                 int iOccurrences = 1);

    void ExpectExitCode(int iCode);
    void ExpectFatal();
    void ExpectEndsEarly();
    void ExpectDumps(int iCount);
    void ExpectDumpMatchesFatal();
    void CollectWerDump();
    static bool WerLocalDumps();

    void Start();
    bool Wait();
    const GameRunResult& Result() const;
    const std::string& RunDir() const;

private:
    GameRun(const GameRun&);
    GameRun& operator=(const GameRun&);

    std::string Expand(const std::string& sText) const;
    void RunInWorker(const std::vector<std::string>& kFiles, const std::string& sArgs, const std::string& sAsan);
    void AcquireWorker();
    void ReleaseWorker();
    void Init(const char* pcName);

    std::string mName;
    std::string mPlatform;
    std::string mGameDir;
    std::vector<std::pair<std::string, std::string> > mWriteFiles;
    std::vector<std::pair<std::string, std::string> > mSeedFolders;
    std::vector<std::string> mKeepFolders;
    std::string mInGameFilter;
    std::string mRunDir;
    std::string mArgs;
    std::string mDisplayIni;
    std::vector<std::string> mOmit;
    std::vector<std::pair<std::string, std::string> > mSwap;
    std::string mExe;
    ExpectedMessages mMessages;
    RunExpect mExpect;
    std::string mWorkerDir;
    void* mpWorkerLock;
    std::thread mThread;
    bool mbStarted;
    bool mbWaited;
    bool mbStartFailed;
    bool mbDumpMatchesFatal;
    bool mbCollectWer;
    unsigned long long mullStartTime;
    std::string mStartError;
    ChildResult mChild;
    GameRunResult mResult;
};
