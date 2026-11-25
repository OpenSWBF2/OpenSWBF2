#include "harness/judge.h"

#include <stdio.h>

#include "RedEngineFL/Util/RedLog.h"

static RunVerdict Fail(RunVerdict kVerdict, const std::string& sReason)
{
    kVerdict.bPass = false;
    kVerdict.sReason = sReason;
    return kVerdict;
}

RunVerdict JudgeRun(const RunFacts& kFacts, const RunExpect& kExpect, ExpectedMessages& kMessages)
{
    RunVerdict kVerdict;
    std::string sUncoveredEnsure;
    for (size_t i = 0; i < kFacts.kLog.kLines.size(); ++i)
    {
        const LogLine& kLine = kFacts.kLog.kLines[i];
        const int iSeverity = kLine.iSeverity < 0 ? RedLog::Verbose : kLine.iSeverity;
        const bool bCovered = kMessages.Observe(iSeverity, kLine.sText.c_str());
        if (iSeverity >= RedLog::Error)
        {
            kVerdict.kErrors.push_back(kLine.sText);
        }
        if (!bCovered && sUncoveredEnsure.empty() && kLine.bParsed &&
            kLine.sMessage.compare(0, 15, "ensure failed: ") == 0)
        {
            sUncoveredEnsure = kLine.sText;
        }
    }
    char acLine[160];
    if (!kExpect.bFatal && !kFacts.kLog.sFatalLine.empty())
    {
        return Fail(kVerdict, "fatal: " + kFacts.kLog.sFatalLine);
    }
    if (kExpect.bFatal && kFacts.kLog.sFatalLine.empty())
    {
        return Fail(kVerdict, "expected a FATAL line, none was logged");
    }
    if (!sUncoveredEnsure.empty())
    {
        return Fail(kVerdict, "ensure: " + sUncoveredEnsure);
    }
    if (kFacts.bKilled)
    {
        return Fail(kVerdict, "killed by the stall rule");
    }
    if ((int)kFacts.uExitCode != kExpect.iExitCode)
    {
        _snprintf(acLine, sizeof(acLine), "exit code 0x%08lX, expected %d", kFacts.uExitCode, kExpect.iExitCode);
        acLine[sizeof(acLine) - 1] = 0;
        return Fail(kVerdict, acLine);
    }
    if (kFacts.kLog.bHasEndMarker && kFacts.kLog.iMarkerExitCode != (int)kFacts.uExitCode)
    {
        _snprintf(acLine, sizeof(acLine), "exit code %d, the end marker says %d", (int)kFacts.uExitCode,
                  kFacts.kLog.iMarkerExitCode);
        acLine[sizeof(acLine) - 1] = 0;
        return Fail(kVerdict, acLine);
    }
    if (!kExpect.bFatal && !kExpect.bEndsEarly && !kFacts.kLog.bHasEndMarker)
    {
        return Fail(kVerdict, "no end marker: the process ended before its planned end");
    }
    if (kExpect.bEndsEarly && kFacts.kLog.bHasEndMarker)
    {
        return Fail(kVerdict, "an end marker on a run expected to end before its planned end");
    }
    if (kFacts.bAsanReport)
    {
        return Fail(kVerdict, "Asan report");
    }
    if (!kMessages.Undeclared().empty())
    {
        return Fail(kVerdict, "no expectation covers " + kMessages.Undeclared()[0]);
    }
    std::string sCounts;
    if (kMessages.Report("game run", sCounts) != 0)
    {
        while (!sCounts.empty() && (sCounts[sCounts.size() - 1] == '\n' || sCounts[sCounts.size() - 1] == '\r'))
        {
            sCounts.erase(sCounts.size() - 1);
        }
        return Fail(kVerdict, sCounts);
    }
    if (kFacts.iDumps != kExpect.iDumps)
    {
        _snprintf(acLine, sizeof(acLine), "%d dumps, expected %d", kFacts.iDumps, kExpect.iDumps);
        acLine[sizeof(acLine) - 1] = 0;
        return Fail(kVerdict, acLine);
    }
    return kVerdict;
}
