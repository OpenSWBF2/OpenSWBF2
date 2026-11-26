#include "tables/tables_commit.h"

#include <algorithm>
#include <regex>
#include <stdlib.h>

#include "tables/tables_readers.h"

namespace Tables
{

namespace
{

std::string TrimSlash(std::string s)
{
    while (!s.empty() && s[s.size() - 1] == '\\')
    {
        s.erase(s.size() - 1);
    }
    return s;
}

bool InSet(const std::string& s, const char* pcA, const char* pcB)
{
    return EqualNoCase(s, pcA) || EqualNoCase(s, pcB);
}

std::string Stem(const std::string& sExe)
{
    if (sExe.size() > 4 && EqualNoCase(sExe.substr(sExe.size() - 4), ".exe"))
    {
        return sExe.substr(0, sExe.size() - 4);
    }
    return sExe;
}

std::string JoinNumbers(const std::vector<long long>& k)
{
    std::vector<std::string> kText;
    for (size_t i = 0; i < k.size(); ++i)
    {
        kText.push_back(std::to_string(k[i]));
    }
    return JoinText(kText, ", ");
}

std::vector<long long> SortedUnique(std::vector<long long> k)
{
    std::sort(k.begin(), k.end());
    k.erase(std::unique(k.begin(), k.end()), k.end());
    return k;
}

}

std::string CaptureSteamWant(const std::string& sSteamState)
{
    return sSteamState == "Running" ? "yes" : "no";
}

CaptureVerdict CaptureCommitPreflight(const CapturePreflight& kIn)
{
    CaptureVerdict k;
    k.iExit = 0;
    if (!InSet(kIn.sPlatform, "Win32", "x64"))
    {
        k.iExit = 1;
        k.kLines.push_back("[FAIL] capture-commit - -Platform " + kIn.sPlatform + " does not belong to the set Win32,x64");
        return k;
    }
    if (!InSet(kIn.sSteamState, "Running", "Closed"))
    {
        k.iExit = 1;
        k.kLines.push_back("[FAIL] capture-commit - -SteamState " + kIn.sSteamState +
                           " does not belong to the set Running,Closed");
        return k;
    }
    const std::string sOut = TrimSlash(kIn.sOutFull);
    const std::string sBase = TrimSlash(kIn.sBaselineFull);
    if (EqualNoCase(sOut, sBase) || StartsWith(LowerAscii(sOut), LowerAscii(sBase + "\\")))
    {
        k.iExit = 2;
        k.kLines.push_back("[FAIL] capture-commit - -OutDir " + sOut +
                           " is inside tests\\baseline; capture into a scratch folder");
        return k;
    }
    std::vector<std::string> kBlockers;
    for (size_t i = 0; i < kIn.kProcesses.size(); ++i)
    {
        const ProcessEntry& p = kIn.kProcesses[i];
        if (StartsWith(LowerAscii(Stem(p.sExe)), "battlefrontii"))
        {
            kBlockers.push_back("game process " + Stem(p.sExe) + " (" + std::to_string(p.uPid) + ")");
        }
    }
    for (size_t i = 0; i < kIn.kProcesses.size(); ++i)
    {
        const ProcessEntry& p = kIn.kProcesses[i];
        if ((EqualNoCase(p.sExe, "OpenSWBF2Tests.exe") && p.uPid != kIn.uSelfPid) ||
            EqualNoCase(p.sExe, "OpenSWBF2Gate.exe"))
        {
            kBlockers.push_back("harness process " + std::to_string(p.uPid) + ": " + p.sExe);
        }
    }
    if (!kBlockers.empty())
    {
        k.iExit = 2;
        k.kLines.push_back("[FAIL] capture-commit - a commit capture runs alone; refusing beside:");
        for (size_t i = 0; i < kBlockers.size(); ++i)
        {
            k.kLines.push_back("    " + kBlockers[i]);
        }
        return k;
    }
    if (kIn.sSteamState == "Running" && kIn.sSteamRunning != "yes")
    {
        k.iExit = 2;
        k.kLines.push_back("[FAIL] capture-commit - -SteamState Running needs Steam running and logged in; "
                           "Get-SteamRunning is " +
                           kIn.sSteamRunning);
        return k;
    }
    if (kIn.bHog != (kIn.sPlatform == "x64"))
    {
        k.iExit = 2;
        k.kLines.push_back("[FAIL] capture-commit - Game::Standard::commit-flat runs under -hogaddrspace on x64 only; "
                           "pass -HogAddrSpace exactly with -Platform x64");
        return k;
    }
    return k;
}

CaptureResult CaptureCommitRows(const CaptureRun& kRun)
{
    CaptureResult k;
    k.iExit = 1;
    k.bWrite = false;
    if (!kRun.bStarted)
    {
        k.kLines.push_back("[FAIL] capture-commit - the test runner did not start");
        return k;
    }
    const std::string sWant = CaptureSteamWant(kRun.sSteamState);
    if (kRun.sSeen != sWant)
    {
        k.kLines.push_back("[FAIL] capture-commit - Steam was steam=" + kRun.sSeen + " at the run, -SteamState " +
                           kRun.sSteamState + " needs steam=" + sWant);
        return k;
    }
    if (kRun.kCommitFlatLogs.size() != 1)
    {
        k.kLines.push_back("[FAIL] capture-commit - " + std::to_string(kRun.kCommitFlatLogs.size()) +
                           " commit-flat run logs under " + kRun.sRunDir);
        return k;
    }
    const int iSettled = 8;
    const std::string sHog = kRun.bHog ? "1" : "0";
    const std::regex kRx("Commit parts state=(\\w+) private=\\d+ gamecrt=(\\d+) heaps=\\d+ threads=\\d+ "
                         "threadcount=\\d+ arena=(\\d+) crtbusy=\\d+ crtcommitted=\\d+ crtlarge=\\d+ gamevm=(\\d+)");
    struct Part
    {
        std::string sState;
        long long llArena;
        long long llRest;
    };
    std::vector<Part> kParts;
    const std::string& sLog = kRun.kCommitFlatLogs[0];
    for (std::sregex_iterator it(sLog.begin(), sLog.end(), kRx), e; it != e; ++it)
    {
        Part p;
        p.sState = (*it)[1].str();
        p.llArena = _strtoi64((*it)[3].str().c_str(), 0, 10);
        p.llRest = _strtoi64((*it)[4].str().c_str(), 0, 10) + _strtoi64((*it)[2].str().c_str(), 0, 10);
        kParts.push_back(p);
    }
    std::vector<std::string> kRows;
    std::vector<std::string> kBad;
    static const char* const sapcStates[] = {"mission", "shell"};
    for (size_t s = 0; s < 2; ++s)
    {
        std::vector<long long> kArena;
        std::vector<long long> kRest;
        int iSeen = 0;
        for (size_t i = 0; i < kParts.size(); ++i)
        {
            if (kParts[i].sState != sapcStates[s])
            {
                continue;
            }
            if (++iSeen > iSettled)
            {
                kArena.push_back(kParts[i].llArena);
                kRest.push_back(kParts[i].llRest);
            }
        }
        if (kArena.empty())
        {
            kBad.push_back(std::string(sapcStates[s]) + " has no COMMITPARTS rows after cycle " + std::to_string(iSettled));
            continue;
        }
        const std::vector<long long> kA = SortedUnique(kArena);
        const std::vector<long long> kR = SortedUnique(kRest);
        if (kA.size() != 1)
        {
            kBad.push_back(std::string(sapcStates[s]) + " arena after cycle " + std::to_string(iSettled) +
                           " is not one value: " + JoinNumbers(kA));
        }
        if (kR.size() != 1)
        {
            kBad.push_back(std::string(sapcStates[s]) + " rest after cycle " + std::to_string(iSettled) +
                           " is not one value: " + JoinNumbers(kR));
        }
        kRows.push_back(sWant + "\t" + sHog + "\t" + sapcStates[s] + "\t" + std::to_string(kA[0]) + "\t" +
                        std::to_string(kR[0]));
    }
    if (!kBad.empty())
    {
        k.kLines.push_back("[FAIL] capture-commit - " + kRun.sRunDir);
        for (size_t i = 0; i < kBad.size(); ++i)
        {
            k.kLines.push_back("    " + kBad[i]);
        }
        return k;
    }
    std::vector<std::string> kText;
    kText.push_back("# commit " + kRun.sShortSha);
    kText.push_back("steam\thog\tstate\tarena\trest");
    kText.insert(kText.end(), kRows.begin(), kRows.end());
    k.bWrite = true;
    k.sCommitPath = JoinPath(kRun.sOutFull, "commit.txt");
    k.sCommitText = JoinText(kText, "\r\n") + "\r\n";
    k.sRunPath = JoinPath(kRun.sOutFull, "run.txt");
    k.sRunText = "run=" + kRun.sRunDir + "\r\nexit=" + std::to_string(kRun.iRunCode) + "\r\n";
    k.kLines.push_back("capture-commit: wrote " + k.sCommitPath + " from " + kRun.sRunDir + " (test runner exit " +
                       std::to_string(kRun.iRunCode) + ")");
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        k.kLines.push_back("    " + kRows[i]);
    }
    if (kRun.iRunCode != 0)
    {
        k.kLines.push_back("[FAIL] capture-commit - the test runner exited " + std::to_string(kRun.iRunCode));
        return k;
    }
    k.iExit = 0;
    return k;
}

}
