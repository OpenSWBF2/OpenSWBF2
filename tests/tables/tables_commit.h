#pragma once
#include <string>
#include <vector>

namespace Tables
{

struct ProcessEntry
{
    unsigned long uPid;
    std::string sExe;
};

struct CapturePreflight
{
    std::string sPlatform;
    std::string sSteamState;
    bool bHog;
    std::string sOutFull;
    std::string sBaselineFull;
    std::vector<ProcessEntry> kProcesses;
    unsigned long uSelfPid;
    std::string sSteamRunning;
};

struct CaptureVerdict
{
    int iExit;
    std::vector<std::string> kLines;
};

CaptureVerdict CaptureCommitPreflight(const CapturePreflight& kIn);
std::string CaptureSteamWant(const std::string& sSteamState);

struct CaptureRun
{
    bool bStarted;
    std::string sRunDir;
    int iRunCode;
    std::string sSteamState;
    std::string sSeen;
    bool bHog;
    std::vector<std::string> kCommitFlatLogs;
    std::string sShortSha;
    std::string sOutFull;
};

struct CaptureResult
{
    int iExit;
    std::vector<std::string> kLines;
    bool bWrite;
    std::string sCommitPath;
    std::string sCommitText;
    std::string sRunPath;
    std::string sRunText;
};

CaptureResult CaptureCommitRows(const CaptureRun& kRun);

}
